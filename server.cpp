#include <cstdio>
#include <fstream>
#include <string>
#include <vdr/osd.h>
#include <vdr/device.h>
#include <vdr/channels.h>
#include <filesystem>

#include "server.h"
#include "osd.h"
#include "webplayer.h"
#include "fpng.h"

// message types (VDR --> Browser)
const uint32_t MESSAGE_TYPE_PNG       = 1;
const uint32_t MESSAGE_TYPE_SIZE      = 2;
const uint32_t MESSAGE_TYPE_RESET     = 3;
const uint32_t MESSAGE_TYPE_CLEAR_OSD = 4;

cWebOsdServer *webOsdServer;
cWebPlayer *webPlayer;

/**
 * AsyncFileStreamer copied from https://github.com/uNetworking/uWebSockets/discussions/1352
 */
#define AsyncFileStreamer_BLOCK_SIZE (65536)

struct AsyncStreamer {
    std::string root;

    explicit AsyncStreamer(std::string root) : root(root) {}

    template<bool SSL>
    std::pair<bool, bool>
    sendBlock(uWS::HttpResponse<SSL> *res, FILE *f, long offset, size_t len, size_t &actuallyRead) {
        int blockSize = AsyncFileStreamer_BLOCK_SIZE;
        char block[AsyncFileStreamer_BLOCK_SIZE];

        fseek(f, offset, SEEK_SET);
        actuallyRead = fread(block, 1, blockSize, f);
        actuallyRead = actuallyRead > 0 ? actuallyRead : 0; // truncate anything < 0

        return res->tryEnd(std::string_view(block, actuallyRead), len);
    }

    template<bool SSL>
    void streamFile(uWS::HttpResponse<SSL> *res, std::string_view url) {
        // NOTE: This is very unsafe, people can inject things like ../../bla in the URL and completely bypass the root folder restriction.
        // Make sure to harden this code if it's intended to use it in anything internet facing.
        std::string nview = root + "/" + std::string{url.substr(1)};
        FILE *f = fopen(nview.c_str(), "rb");

        stream(res, f);
    }

    template<bool SSL>
    void streamVideo(uWS::HttpResponse<SSL> *res, std::string_view url) {
        // NOTE: This is very unsafe, people can inject things like ../../bla in the URL and completely bypass the root folder restriction.
        // Make sure to harden this code if it's intended to use it in anything internet facing.
        std::string nview =std::string(STREAM_DIR) + "/" + std::string{url.substr(8)};

        // wait maximum x seconds until file exists
        int sleepTime = 100; // ms
        int sleepCountMax = (15 * 1000) / sleepTime;
        std::filesystem::path desiredFile{nview};
        while (!std::filesystem::exists(desiredFile) && --sleepCountMax > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }

        if (std::filesystem::exists(desiredFile)) {
            FILE *f = fopen(nview.c_str(), "rb");
            stream(res, f);
        } else {
            res->writeStatus("404 Not Found");
            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            res->end("<b>404 Not Found</b>");
        }
    }

    template<bool SSL>
    void streamBuffer(uWS::HttpResponse<SSL> *res, void *buffer, size_t size) {
        FILE *f = fmemopen(buffer, size, "rb");
        stream(res, f);
    }

    template<bool SSL>
    void stream(uWS::HttpResponse<SSL> *res, FILE *file) {
        if (file == nullptr) {
            res->writeStatus("404 Not Found");
            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            res->end("<b>404 Not Found</b>");
        } else {
            res->writeStatus(uWS::HTTP_200_OK);
            fseek(file, 0, SEEK_END);
            long len = ftell(file);
            fseek(file, 0, SEEK_SET);

            res->onAborted([file]() {
                fclose(file);
            });

            auto fillStream = [this, len, res, file]() {
                bool retry;
                bool completed;
                do {
                    size_t actuallyRead;
                    auto tryEndResult = sendBlock(res, file, res->getWriteOffset(), len, actuallyRead);
                    retry = tryEndResult.first /*ok*/ && tryEndResult.second == false /*completed == false*/ ;
                    completed = tryEndResult.second;
                } while (retry);

                if (completed)
                    fclose(file);
            };

            res->onWritable([this, fillStream](uintmax_t offset) {
                fillStream();
                return true;
            });

            fillStream();
        }
    }
};

AsyncStreamer streamer("/home/rh/idea/vdr-plugin-webout/static-html");

cWebOsdServer::cWebOsdServer(const char *Description, bool LowPriority) : cThread(Description, LowPriority) {
    osdProvider = new cWebOsdProvider(OSDPROVIDER_IDX);
    webOsdServer = this;
    webStatus = nullptr;
}

cWebOsdServer::~cWebOsdServer() {
    cOsdProvider::DeleteOsdProvider(OSDPROVIDER_IDX);
    webOsdServer = nullptr;
    delete webStatus;
}

void cWebOsdServer::Action(void) {
    uWS::App app = uWS::App()
            .ws<PerSocketData>("/command", {
                    .compression = uWS::SHARED_COMPRESSOR,
                    .maxPayloadLength = 1 * 1024 * 1024,
                    .idleTimeout = 12,
                    .maxBackpressure = 1 * 1024 * 1024,
                    .upgrade = [](auto *res, auto *req, auto *context) {
                        res->template upgrade<PerSocketData>({
                                                                     /* We initialize PerSocketData struct here */
                                                                     .something = 13
                                                             }, req->getHeader("sec-websocket-key"),
                                                             req->getHeader("sec-websocket-protocol"),
                                                             req->getHeader("sec-websocket-extensions"),
                                                             context);
                    },
                    .open = [this](auto *ws) {
                        gws = ws;
                        sendSize();

                        esyslog("Create new OSD Provider/Receiver/Status");
                        cOsdProvider::ActivateOsdProvider(OSDPROVIDER_IDX);
                        webReceiver = new cWebReceiver();
                        webStatus = new cWebStatus();
                        webPlayer = new cWebPlayer();

                        auto ctrl = new cWebControl(webPlayer);
                        cControl::Launch(ctrl);
                        cControl::Attach();
                    },
                    .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
                        std::cout << "Got message: " << message << ":" << message.length() << std::endl;

                        long unsigned int position;
                        if ((position = message.find(':')) != std::string::npos) {
                            auto token = message.substr(0, position - 1);
                            auto data = message.substr(position + 1, message.length());

                            if (token.compare("3")) {
                                // got key from browser
                                receiveKeyEvent(data);
                            }
                        }
                    },
                    .drain = [](auto */*ws*/) {
                        /* Check ws->getBufferedAmount() here */
                        printf("Drain\n");
                    },
                    .close = [this](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
                        cOsdProvider::ActivateOsdProvider(0);
                        DELETENULL(webReceiver);
                        DELETENULL(webStatus);
                        webStatus = nullptr;
                        cDevice::PrimaryDevice()->Detach(webPlayer);
                    }
            }).get("/", [](auto *res, auto *req) {
                // send index.html
                res->writeHeader("Content-Type", "text/html; charset=utf-8");
                streamer.streamFile(res, "/index.html");
            }).get("/video.js", [](auto *res, auto *req) {
                res->writeHeader("Content-Type", "application/javascript");
                streamer.streamFile(res, "/video.min.js");
            }).get("/video-js.css", [](auto *res, auto *req) {
                res->writeHeader("Content-Type", "text/css");
                streamer.streamFile(res, "/video-js.css");
            }).get("/stream/*", [](auto *res, auto *req) {
                // send video stream / m3u8
                streamer.streamVideo(res, req->getUrl());
            }).get("/*", [](auto *res, auto *req) {
                fprintf(stderr, "File %s is not configured\n", std::string(req->getUrl()).c_str());

                res->writeStatus("404 Not Found");
                res->writeHeader("Content-Type", "text/html; charset=utf-8");
                res->end("<b>404 Not Found</b>");
            }).listen(port, [this](auto *socket) {
                if (socket) {
                    std::cout << "Start OsdServer on port " << port << std::endl;
                    this->listenSocket = socket;
                }
            });

    globalAppLoop = uWS::Loop::get();
    globalApp = &app;

    app.run();

    globalApp = nullptr;
}

void cWebOsdServer::Cancel(int WaitSeconds) {
    // stop at first the websocket server and then call the super function
    globalAppLoop->defer([this]() {
        us_listen_socket_close(false, listenSocket);
    });

    cThread::Cancel(WaitSeconds);
}

int cWebOsdServer::sendPngImage(int x, int y, int w, int h, int bufferSize, uint8_t *buffer) {
    uint8_t sendBuffer[7 * sizeof(uint32_t) + bufferSize];

    // get current OSD size
    int width;
    int height;
    double pa;

    cDevice::PrimaryDevice()->GetOsdSize(width, height, pa);

    // fill buffer
    ((uint32_t *) sendBuffer)[0] = MESSAGE_TYPE_PNG; // type PNG
    ((uint32_t *) sendBuffer)[1] = width;
    ((uint32_t *) sendBuffer)[2] = height;
    ((uint32_t *) sendBuffer)[3] = x;
    ((uint32_t *) sendBuffer)[4] = y;
    ((uint32_t *) sendBuffer)[5] = w;
    ((uint32_t *) sendBuffer)[6] = h;
    memcpy(sendBuffer + 7 * sizeof(uint32_t), reinterpret_cast<const void *>(buffer), bufferSize);

    return gws->send(std::string_view((char *) &sendBuffer, 20 + bufferSize), uWS::OpCode::BINARY);
}

int cWebOsdServer::sendSize() {
    uint32_t sendBuffer[3];

    int width;
    int height;
    double pa;

    cDevice::PrimaryDevice()->GetOsdSize(width, height, pa);

    // fill buffer
    sendBuffer[0] = MESSAGE_TYPE_SIZE; // type SIZE
    sendBuffer[1] = width;
    sendBuffer[2] = height;

    return gws->send(std::string_view((char *) &sendBuffer, sizeof(sendBuffer)), uWS::OpCode::BINARY);
}

int cWebOsdServer:: sendClearOsd() {
    uint32_t sendBuffer[1];

    // clear OSD
    sendBuffer[0] = MESSAGE_TYPE_CLEAR_OSD; // type CLEAR_OSD
    return gws->send(std::string_view((char *) &sendBuffer, sizeof(sendBuffer)), uWS::OpCode::BINARY);
}

int cWebOsdServer::sendPlayerReset() {
    uint32_t sendBuffer[1];

    // reset
    sendBuffer[0] = MESSAGE_TYPE_RESET; // type RESET
    return gws->send(std::string_view((char *) &sendBuffer, sizeof(sendBuffer)), uWS::OpCode::BINARY);
}

void cWebOsdServer::receiveKeyEvent(std::string_view event) {
    std::string data = std::string(event);
    webRemote.ProcessKey(data.c_str());
}
