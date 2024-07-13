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



class CliOptions {
public:
    CliOptions()
#if defined(WIN32)
        : h264_url("D:/__develop__/1080p.25fps.4M.264")
        , h265_url("D:/__develop__/1080p.25fps.3M.265")
        , log_path("transcode.log")
#else
        : h264_url("/media/1080p.25fps.4M.264")
        , h265_url("/media/1080p.25fps.3M.265")
        , log_path("/media/transcode.log")
#endif
        , input_width(1920)
        , input_height(1080)
        , concurrency(1)
        , limit_input_frames(INT_MAX)
        , task_name("h264_to_cif_h264")
        , ffmpeg_log_level(AV_LOG_INFO)
    {
    }

    void add_options(CLI::App &app)
    {
        app.add_option("--input_h264", h264_url, fmt::format("input h264 url (default {})", h264_url));
        app.add_option("--input_h265", h265_url, fmt::format("input h265 url (default {})", h265_url));
        app.add_option("--input_width", input_width, fmt::format("input video width (default {})", input_width));
        app.add_option("--input_height", input_height, fmt::format("input video height (default {})", input_height));
        app.add_option("--limit_input_frames", limit_input_frames, "limit input frames (default INT_MAX)");
        app.add_option("--concurrency", concurrency, fmt::format("concurrent threads (default {})", concurrency));
        app.add_option("--task_name", task_name, fmt::format("transcode task name (default {}, support list: {})", task_name, TranscodeTypeCvt::support_list()));
        app.add_option("--log_path", log_path, fmt::format("log path (default {})", log_path));
        app.add_option("--ffmpeg_log_level", ffmpeg_log_level, "ffmpeg log level (default AV_LOG_INFO)");
    }

    std::string h264_url;
    std::string h265_url;
    int input_width;
    int input_height;
    int concurrency;
    int limit_input_frames;
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
    SPDLOG_INFO("========== {} threads {} frames {} begin ==========", cli.concurrency, cli.limit_input_frames, cli.task_name);
    TimeIt ti;
    TranscodeType task_type = TranscodeTypeCvt::from_string(cli.task_name);
    if (task_type != TranscodeType::Invalid) {
        std::string intput_codec;
        std::vector<std::string> output_codec;
        std::vector<int> output_width;
        std::vector<int> output_height;
        std::vector<int64_t> output_bitrate;

        if (task_type != TranscodeType::AllTasks) {
            std::vector<FFmpegPacket> frames_queue = "h264" ? demux_all_packets(cli.h264_url, cli.limit_input_frames) : demux_all_packets(cli.h265_url, cli.limit_input_frames);

            FFmpegTranscodeFactory factory;
            FFmpegTranscode *transcode = factory.create(task_type, intput_codec, output_codec, output_width, output_height, output_bitrate);
            transcode->multi_threading_test(frames_queue, cli.concurrency, intput_codec, cli.input_width, cli.input_height, output_codec, output_width, output_height, output_bitrate);
        }
        else {
            std::vector<FFmpegPacket> frames_queue_264 = demux_all_packets(cli.h264_url, cli.limit_input_frames);
            std::vector<FFmpegPacket> frames_queue_265 = demux_all_packets(cli.h265_url, cli.limit_input_frames);

            for (auto e = (int)TranscodeType::DecodeH264Only; e < (int)TranscodeType::AllTasks; e++) {
                FFmpegTranscodeFactory factory;
                FFmpegTranscode *transcode = factory.create((TranscodeType)e, intput_codec, output_codec, output_width, output_height, output_bitrate);
                transcode->multi_threading_test(frames_queue_264, cli.concurrency, intput_codec, cli.input_width, cli.input_height, output_codec, output_width, output_height, output_bitrate);
            }
        }
    }
    SPDLOG_INFO("========== {} threads {} frames {} end with {:.2f}s ==========", cli.concurrency, cli.limit_input_frames, cli.task_name, ti.elapsed_seconds());

    return 0;
}
