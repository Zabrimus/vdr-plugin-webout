#include "webencoder.h"

// Fix some compile problems
#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)

#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts, tb)

int cWebEncoder::bytes_remuxed = 0;
std::mutex cWebEncoder::buf_mutex;
std::condition_variable cWebEncoder::cond;
std::atomic<bool> cWebEncoder::receiving_finished{false};
std::atomic<bool> cWebEncoder::remuxing_thread_done{false};
RingBuffer* cWebEncoder::buff;

cWebEncoder::cWebEncoder() {
    bytes_remuxed = 0;
    buff = new RingBuffer(263200);

    remuxing_thread = std::thread(&cWebEncoder::remux_worker, std::string("test.mp4"));

    av_log_set_level(AV_LOG_INFO);
}

cWebEncoder::~cWebEncoder() {
    cond.notify_all();
    remuxing_thread.join();

    delete buff;
    //delete remuxing_thread;
}

bool cWebEncoder::create_input() {

}

void cWebEncoder::remux_worker(const std::string out_filename) {
    // create input format context
    AVIOContext* avio_input_ctx = NULL; // this is IO (input/output) context, needed for i/o customizations
    AVFormatContext* input_ctx = NULL;  // this is AV (audio/video) context
    if (!make_input_ctx(&input_ctx, &avio_input_ctx, buff)) {
        return;
    }

    // create output format context
    AVFormatContext* output_ctx = NULL; // this is AV (audio/video) context

    /*
    if (!make_output_ctx(&output_ctx, "flv", out_filename)) {
        return;
    }
    */

    // create streams map, filtering out all streams except audio/video
    int* streams_map = NULL;
    if (!make_streams_map(&input_ctx, &streams_map)) {
        return;
    }

    // init output context from input context (create output streams in output context, copying codec params from input)
    /*
    if (!ctx_init_output_from_input(&input_ctx, &output_ctx)) {
        return;
    }
    */

    // dump input and output formats/streams info
    // https://ffmpeg.org/doxygen/trunk/group__lavf__misc.html#gae2645941f2dc779c307eb6314fd39f10
    std::cout << "-------------------------------- IN ------------------------------------\n";
    std::cout << "Number of streams: " << input_ctx->nb_streams << std::endl;
    av_dump_format(input_ctx, 0, "input_file", 0);

    /*
    std::cout << "-------------------------------- OUT -----------------------------------\n";
    av_dump_format(output_ctx, 0, out_filename, 1);
    std::cout << "------------------------------------------------------------------------\n";

    // create and open output file and write file header
    if (!open_output_file(&output_ctx, out_filename)) {
        return;
    }
    */

    // read input file streams, remux them and write into output file
    if (!transcode(&input_ctx, &output_ctx, streams_map)) {
        return;
    }

    /*
    // close output file
    if (!close_output_file(&output_ctx)) {
        return;
    }

    // close input context
    avformat_close_input(&input_ctx);
    */

    // cleanup: free memory
    avformat_free_context(input_ctx);
    // avformat_free_context(output_ctx);
    av_freep(&streams_map);
    av_freep(&avio_input_ctx);
}

bool cWebEncoder::make_input_ctx(AVFormatContext** input_ctx, AVIOContext** avio_input_ctx, RingBuffer* buff) {
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
    *avio_input_ctx = avio_alloc_context(
            ctx_buffer,        // memory buffer
            buffer_size,       // memory buffer size
            0,                 // 0 for reading, 1 for writing. we're reading, so — 0.
            reader_ptr,        // pass our reader to context, it will be transparenty passed to read callback on each invocation
            &read_callback,    // out read callback
            NULL,              // write callback — we don't need one
            NULL               // seek callback - we don't need one
    );

    // allocate new AVFormatContext
    *input_ctx = avformat_alloc_context();

    // assign our new and shiny custom i/o context to AVFormatContext
    (*input_ctx)->pb = *avio_input_ctx;

    // tell our input context that we're using custom i/o and there's no backing file
    //(*input_ctx)->flags |= AVFMT_FLAG_CUSTOM_IO | AVFMT_NOFILE;

    // note "some_dummy_filename", ffmpeg requires it as some default non-empty placeholder
    int ret = avformat_open_input(input_ctx, "some_dummy_filename", NULL, NULL);
    if (ret < 0) {
        std::cout << "Could not open input stream, reason: " << av_err2str(ret) << '\n';
        return false;
    }

    ret = avformat_find_stream_info(*input_ctx, NULL);
    if (ret < 0) {
        std::cout << "Failed to retrieve input stream information, reason: " << av_err2str(ret) << '\n';
        return false;
    }

    return true;
}

bool cWebEncoder::make_streams_map(AVFormatContext** input_ctx, int** streams_map) {
    int* smap = NULL;
    int stream_index = 0;
    int input_streams_count = (*input_ctx)->nb_streams;

    smap = (int*)av_mallocz_array(input_streams_count, sizeof(int));
    if (!smap) {
        std::cout << "Could not allocate streams list.\n";
        return false;
    }

    for (int i = 0; i < input_streams_count; i++) {
        AVCodecParameters* c = (*input_ctx)->streams[i]->codecpar;
        if (c->codec_type != AVMEDIA_TYPE_AUDIO && c->codec_type != AVMEDIA_TYPE_VIDEO) {
            smap[i] = -1;
            continue;
        }

        printf("Stream %d -> %s\n", i, avcodec_get_name(c->codec_id));

        smap[i] = stream_index++;
    }

    *streams_map = smap;
    return true;
}

bool cWebEncoder::transcode(AVFormatContext** input_ctx, AVFormatContext** output_ctx, int* streams_map) {
    AVPacket packet;
    int input_streams_count = (*input_ctx)->nb_streams;

    while (1) {
        int ret = av_read_frame(*input_ctx, &packet);
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
        /*
        AVStream* in_stream  = (*input_ctx)->streams[packet.stream_index];
        AVStream* out_stream = (*output_ctx)->streams[packet.stream_index];

        AVRounding avr = (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, avr);
        packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, avr);
        packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
        */

        // https://ffmpeg.org/doxygen/trunk/structAVPacket.html#ab5793d8195cf4789dfb3913b7a693903
        packet.pos = -1;

        // printf("Packet: Streamindex %d\n", packet.stream_index);
        // printf("Packet: PTS %ld\n", packet.pts);

        //https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
        /*
        ret = av_interleaved_write_frame(*output_ctx, &packet);
        if (ret < 0) {
            std::cout << "Failed to write packet to output, reason: " << av_err2str(ret) << '\n';
            return false;
        }
        */

        av_packet_unref(&packet);
    }

    return true;
}

// this callback will be used for our custom i/o context (AVIOContext)
int cWebEncoder::read_callback(void* opaque, uint8_t* buf, int buf_size) {
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

int cWebEncoder::PlayTS(const uint8_t *Data, int Length) {
    static int nr = 0;

    if (nr++ % 1000 == 0) {
        printf("Avail: %ld, Size %ld\n", buff->avail(), buff->size());
    }

    int len = buff->write(Data, Length);
    cond.notify_one();
    return len;
}
