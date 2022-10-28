#include "webencoder.h"

// Fix some compile problems
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts, tb)

int bytes_remuxed = 0;
std::mutex buf_mutex;
std::condition_variable cond;
std::atomic<bool> receiving_finished{false};
std::atomic<bool> remuxing_thread_done{false};
RingBuffer* buff;

// input
AVIOContext* avio_input_ctx = NULL;
AVFormatContext* input_ctx = NULL;
int* streams_map = NULL;

// output
AVFormatContext* output_ctx = NULL;

// this callback will be used for our custom i/o context (AVIOContext)
int read_callback(void* opaque, uint8_t* buf, int buf_size) {
    auto& buff = *reinterpret_cast<RingBuffer*>(opaque);

    std::unique_lock<std::mutex> lk(buf_mutex);

    // wait for more data arrival
    while (buff.size() == 0) {
        if (receiving_finished.load()) {
            return AVERROR_EOF; // this is the way to tell our input context that there's no more data
        }

        cond.wait(lk);
    }

    size_t read_size = buff.read((uint8_t*)buf, buf_size);
    cond.notify_one();

    bytes_remuxed += read_size;
    return read_size;
}

bool make_streams_map() {
    int stream_index = 0;
    int input_streams_count = input_ctx->nb_streams;

    streams_map = (int*)av_mallocz_array(input_streams_count, sizeof(int));
    if (!streams_map) {
        std::cout << "Could not allocate streams list.\n";
        return false;
    }

    for (int i = 0; i < input_streams_count; i++) {
        AVCodecParameters* c = input_ctx->streams[i]->codecpar;
        //if (c->codec_type != AVMEDIA_TYPE_AUDIO && c->codec_type != AVMEDIA_TYPE_VIDEO) {
        if (c->codec_type != AVMEDIA_TYPE_VIDEO) {
            streams_map[i] = -1;
            continue;
        }

        printf("Stream %d -> %s\n", i, avcodec_get_name(c->codec_id));

        streams_map[i] = stream_index++;
    }

    return true;
}

bool make_input_ctx() {
    // now we need to allocate a memory buffer for our context to use. keep in mind, that buffer size
    // should be chosen correctly for various containers, this noticeably affectes performance
    // NOTE: this buffer is managed by AVIOContext and you should not deallocate by yourself
    const size_t buffer_size = 8192;
    unsigned char* ctx_buffer = (unsigned char*)(av_malloc(buffer_size));
    if (ctx_buffer == NULL) {
        std::cout << "Could not allocate read buffer for AVIOContext\n";
        return false;
    }

    // let's setup a custom AVIOContext for AVFormatContext

    // cast reader to convenient short variable
    void* reader_ptr = reinterpret_cast<void*>(static_cast<RingBuffer*>(buff));

    // now the important part, we need to create a custom AVIOContext, provide it buffer and
    // buffer size for reading and read callback that will do the actual reading into the buffer
    avio_input_ctx = avio_alloc_context(
            ctx_buffer,        // memory buffer
            buffer_size,       // memory buffer size
            0,                 // 0 for reading, 1 for writing. we're reading, so — 0.
            reader_ptr,        // pass our reader to context, it will be transparenty passed to read callback on each invocation
            &read_callback,    // out read callback
            NULL,              // write callback — we don't need one
            NULL               // seek callback - we don't need one
    );

    // allocate new AVFormatContext
    input_ctx = avformat_alloc_context();

    // assign our new and shiny custom i/o context to AVFormatContext
    input_ctx->pb = avio_input_ctx;

    // tell our input context that we're using custom i/o and there's no backing file
    //(*input_ctx)->flags |= AVFMT_FLAG_CUSTOM_IO | AVFMT_NOFILE;

    // note "some_dummy_filename", ffmpeg requires it as some default non-empty placeholder
    int ret = avformat_open_input(&input_ctx, "some_dummy_filename", NULL, NULL);
    if (ret < 0) {
        std::cout << "Could not open input stream, reason: " << av_err2str(ret) << '\n';
        return false;
    }

    ret = avformat_find_stream_info(input_ctx, NULL);
    if (ret < 0) {
        std::cout << "Failed to retrieve input stream information, reason: " << av_err2str(ret) << '\n';
        return false;
    }

    // create streams map, filtering out all streams except audio/video
    if (!make_streams_map()) {
        return false;
    }

    return true;
}

bool make_output_ctx(std::string format_name, std::string filename) {
    int ret = avformat_alloc_output_context2(&output_ctx, NULL, format_name.c_str(), filename.c_str());
    if (ret < 0) {
        std::cout << "Could not create output context, reason: " << av_err2str(ret) << '\n';
        return false;
    }

    if (!output_ctx) {
        std::cout << "Could not create output context, no further details.\n";
        return false;
    }

    return true;
}

bool ctx_init_output_from_input() {
    int input_streams_count = input_ctx->nb_streams;

    for (int i = 0; i < input_streams_count; i++) {
        AVStream* in_stream = input_ctx->streams[i];
        AVCodecParameters* in_codecpar = in_stream->codecpar;

        //if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO && in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        if (in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }

        AVStream* out_stream = avformat_new_stream(output_ctx, NULL);
        if (!out_stream) {
            std::cout << "Failed allocating output stream\n";
            return false;
        }

        int ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            std::cout << "Failed to copy codec parameters, reason: " << av_err2str(ret) << '\n';
            return false;
        }

        // set stream codec tag to 0, for libav to detect automatically
        out_stream->codecpar->codec_tag = 0;
        out_stream->time_base = in_stream->time_base;
    }

    return true;
}

bool open_output_file(std::string filename) {
    // unless it's a no file (we'll talk later about that) write to the disk (FLAG_WRITE)
    // but basically it's a way to save the file to a buffer so you can store it
    // wherever you want.
    int ret = avio_open(&(output_ctx->pb), filename.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        std::cout << "Could not open output file " << filename << ", reason: " << av_err2str(ret) << '\n';
        return false;
    }

    // https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga18b7b10bb5b94c4842de18166bc677cb
    /*
    ret = avformat_write_header(output_ctx, NULL);
    if (ret < 0) {
        std::cout << "Failed to write output file header to " << filename << ", reason: " << av_err2str(ret) << '\n';
        return false;
    }
    */

    return true;
}

bool close_output_file() {
    //https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13
    int ret = av_write_trailer(output_ctx);
    if (ret < 0) {
        std::cout << "Failed to write trailer to output, reason: " << av_err2str(ret) << '\n';
        return false;
    }

    /* close output */
    if (output_ctx && !(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_closep(&output_ctx->pb);
        if (ret < 0) {
            std::cout << "Failed to close AV output, reason: " << av_err2str(ret) << '\n';
            return false;
        }
    }

    return true;
}

bool transcode() {
    AVPacket packet;
    int input_streams_count = input_ctx->nb_streams;

    while (1) {
        int ret = av_read_frame(input_ctx, &packet);
        if (ret == AVERROR_EOF) { // we have reached end of input file
            break;
        }

        // handle any other error
        if (ret < 0) {
            std::cout << "Failed to read packet from input, reason: " << av_err2str(ret) << '\n';
            return false;
        }

        // ignore any packets that are present in non-mapped streams
        if (packet.stream_index >= input_streams_count || streams_map[packet.stream_index] < 0) {
            av_packet_unref(&packet);
            continue;
        }

        // set stream index, based on our map
        packet.stream_index = streams_map[packet.stream_index];

        /* copy packet */
        AVStream* in_stream  = input_ctx->streams[packet.stream_index];
        AVStream* out_stream = output_ctx->streams[packet.stream_index];

        AVRounding avr = (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, avr);
        packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, avr);
        packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);

        // https://ffmpeg.org/doxygen/trunk/structAVPacket.html#ab5793d8195cf4789dfb3913b7a693903
        packet.pos = -1;

        printf("Packet: Streamindex %d\n", packet.stream_index);
        printf("Packet: PTS %ld\n", packet.pts);

        //https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
        ret = av_interleaved_write_frame(output_ctx, &packet);
        if (ret < 0) {
            std::cout << "Failed to write packet to output, reason: " << av_err2str(ret) << '\n';
            return false;
        }

        av_packet_unref(&packet);
    }

    return true;
}

void remux_worker(const std::string out_filename) {
    // create input format context
    if (!make_input_ctx()) {
        return;
    }

    if (!make_output_ctx("mp4", out_filename)) {
        return;
    }

    if (!ctx_init_output_from_input()) {
        return;
    }

    // dump input and output formats/streams info
    // https://ffmpeg.org/doxygen/trunk/group__lavf__misc.html#gae2645941f2dc779c307eb6314fd39f10
    std::cout << "-------------------------------- IN ------------------------------------\n";
    std::cout << "Number of streams: " << input_ctx->nb_streams << std::endl;
    av_dump_format(input_ctx, 0, "input_file", 0);

    std::cout << "-------------------------------- OUT -----------------------------------\n";
    std::cout << "Number of streams: " << output_ctx->nb_streams << std::endl;
    av_dump_format(output_ctx, 0, out_filename.c_str(), 1);
    std::cout << "------------------------------------------------------------------------\n";

    // create and open output file and write file header
    if (!open_output_file(out_filename)) {
        return;
    }

    // read input file streams, remux them and write into output file
    if (!transcode()) {
        return;
    }

    // close output file
    if (!close_output_file()) {
        return;
    }

    // close input context
    avformat_close_input(&input_ctx);

    // cleanup: free memory
    avformat_free_context(input_ctx);
    avformat_free_context(output_ctx);
    av_freep(&streams_map);
    av_freep(&avio_input_ctx);
}


cWebEncoder::cWebEncoder() {
    bytes_remuxed = 0;
    buff = new RingBuffer(263200);

    remuxing_thread = std::thread(remux_worker, std::string("test.mp4"));

    av_log_set_level(AV_LOG_INFO);
}

cWebEncoder::~cWebEncoder() {
    cond.notify_all();
    remuxing_thread.join();

    delete buff;
}

int cWebEncoder::PlayTS(const uint8_t *Data, int Length) {
    static int nr = 0;

    if (nr++ % 1000 == 0) {
        printf("Avail: %ld, Size %ld\n", buff->avail(), buff->size());
    }

    int len = buff->write(Data, Length);
    cond.notify_one();
    return len;
}
