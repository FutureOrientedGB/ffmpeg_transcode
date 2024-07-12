// project
#include "ffmpeg_transcode.hpp"
#include "ffmpeg_utils.hpp"
#include "math_utils.hpp"

// ffmpeg
extern "C" {
#include <libavutil/log.h>
}

// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

// cli11
#include <CLI/CLI.hpp>



#if defined(WIN32)
std::string default_log_path("transcode.log");
std::string default_h264_path("D:/__develop__/1080p.25fps.4M.264");
std::string default_h265_path("D:/__develop__/1080p.25fps.3M.265");
#else
std::string default_log_path("/media/transcode.log");
std::string default_h264_path("/media/1080p.25fps.4M.264");
std::string default_h265_path("/media/1080p.25fps.3M.265");
#endif



class CliOptions {
public:
    CliOptions()
        : h264_path(default_h264_path)
        , h265_path(default_h265_path)
        , input_width(1920)
        , input_height(1080)
        , concurrency_threads(1)
        , limit_frames(INT_MAX)
        , task_name("h264_to_cif_h264")
        , log_path(default_log_path)
        , ffmpeg_log_level(AV_LOG_INFO)
    {
    }

    void add_options(CLI::App &app)
    {
        app.add_option("--h264", h264_path, "h264 path (default " + h264_path + ")");
        app.add_option("--h265", h265_path, "h265 path (default " + h265_path + ")");
        app.add_option("--width", input_width, "input video width (default 1920)");
        app.add_option("--height", input_height, "input video width (default 1080)");
        app.add_option("-c,--concurrency", concurrency_threads, "concurrency threads (default 1)");
        app.add_option("-f,--frames", limit_frames, "limit frames (default INT_MAX)");
        app.add_option("-t,--task", task_name, "transcode task name (" + TranscodeTypeCvt::help() + ")");
        app.add_option("-p,--path", log_path, "log path (default " + log_path + ")");
        app.add_option("-l,--level", ffmpeg_log_level, "ffmpeg log level (default AV_LOG_INFO)");
    }

    std::string h264_path;
    std::string h265_path;
    int input_width;
    int input_height;
    int concurrency_threads;
    int limit_frames;
    std::string task_name;
    std::string log_path;
    int ffmpeg_log_level;
};



int main(int argc, char **argv) {
    // parse cli
    CLI::App app ("ffmpeg transcode");
    CliOptions cli;
    cli.add_options(app);
    CLI11_PARSE(app, argc, argv);

    // setup logger
    ffmpeg_log_default(cli.ffmpeg_log_level);

    auto file_logger = spdlog::basic_logger_mt("transcode", cli.log_path);
    spdlog::set_default_logger(file_logger);
    spdlog::flush_on(spdlog::level::info);
    spdlog::flush_every(std::chrono::seconds(1));

    // transcode
    SPDLOG_INFO("========== {} threads {} frames {} begin ==========", cli.concurrency_threads, cli.limit_frames, cli.task_name);
    TimeIt time_it;
    TranscodeType task_type = TranscodeTypeCvt::from_str(cli.task_name);
    if (task_type != TranscodeType::Invalid) {
        std::string intput_codec;
        std::vector<std::string> output_codec;
        std::vector<int> output_width;
        std::vector<int> output_height;
        std::vector<int64_t> output_bitrate;

        if (task_type != TranscodeType::AllTasks) {
            std::vector<FFmpegPacket> frames_queue = "h264" ? demux_all_packets(cli.h264_path, cli.limit_frames) : demux_all_packets(cli.h265_path, cli.limit_frames);

            FFmpegTranscodeFactory factory;
            FFmpegTranscode *transcode = factory.create(task_type, intput_codec, output_codec, output_width, output_height, output_bitrate);
            transcode->multi_threading_test(frames_queue, cli.concurrency_threads, intput_codec, cli.input_width, cli.input_height, output_codec, output_width, output_height, output_bitrate);
        }
        else {
            std::vector<FFmpegPacket> frames_queue_264 = demux_all_packets(cli.h264_path, cli.limit_frames);
            std::vector<FFmpegPacket> frames_queue_265 = demux_all_packets(cli.h265_path, cli.limit_frames);

            for (auto e = (int)TranscodeType::DecodeH264Only; e < (int)TranscodeType::AllTasks; e++) {
                FFmpegTranscodeFactory factory;
                FFmpegTranscode *transcode = factory.create((TranscodeType)e, intput_codec, output_codec, output_width, output_height, output_bitrate);
                transcode->multi_threading_test(frames_queue_264, cli.concurrency_threads, intput_codec, cli.input_width, cli.input_height, output_codec, output_width, output_height, output_bitrate);
            }
        }
    }
    SPDLOG_INFO("========== {} threads {} frames {} end with {:.2f}s ==========", cli.concurrency_threads, cli.limit_frames, cli.task_name, time_it.elapsed_seconds());

    return 0;
}
