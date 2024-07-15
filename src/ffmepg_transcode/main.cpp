// project
#include "ffmpeg_demux.hpp"
#include "ffmpeg_transcode.hpp"
#include "ffmpeg_utils.hpp"
#include "math_utils.hpp"
#include "string.utils.hpp"

// c
#include <limits.h>

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
        : input_h264_url("D:/__develop__/1080p.25fps.4M.264")
        , input_h265_url("D:/__develop__/1080p.25fps.3M.265")
        , log_path("transcode.log")
#else
        : input_h264_url("/media/1080p.25fps.4M.264")
        , input_h265_url("/media/1080p.25fps.3M.265")
        , log_path("/media/transcode.log")
#endif
        , log_level((int)spdlog::level::info)
        , ffmpeg_log_level(AV_LOG_INFO)
        , threads(1)
        , limit_input_frames(INT_MAX)
        , task("h264_to_cif_h264")
    {
    }

    void add_options(CLI::App &app)
    {
        app.add_option("--input_h264", input_h264_url, fmt::format("input h264 url (default {})", input_h264_url));
        app.add_option("--input_h265", input_h265_url, fmt::format("input h265 url (default {})", input_h265_url));
        app.add_option("--limit_input_frames", limit_input_frames, "limit input frames (default INT_MAX)");
        app.add_option("--threads", threads, fmt::format("concurrent threads (default {})", threads));
        app.add_option("--task", task, fmt::format("transcode task name (default {}, support list: {})", task, TranscodeTypeCvt::support_list()));
        app.add_option("--log_path", log_path, fmt::format("log path (default {})", log_path));
        app.add_option("--log_level", log_path, "log level (default spdlog::level::info)");
        app.add_option("--ffmpeg_log_level", ffmpeg_log_level, "ffmpeg log level (default AV_LOG_INFO)");
    }

    std::string input_h264_url;
    std::string input_h265_url;
    int threads;
    int limit_input_frames;
    std::string task;
    std::string log_path;
    int log_level;
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
    spdlog::set_level((spdlog::level::level_enum)cli.log_level);
    spdlog::flush_on((spdlog::level::level_enum)cli.log_level);

    // transcode
    TimeIt ti;
    TranscodeType task_type = TranscodeTypeCvt::from_string(cli.task);
    if (task_type != TranscodeType::Invalid) {
        std::string intput_codec;
        std::vector<std::string> output_codec;
        std::vector<int> output_width;
        std::vector<int> output_height;
        std::vector<int64_t> output_bitrate;

        if (task_type != TranscodeType::AllTasks) {
            FFmpegDemux demux = startswith(TranscodeTypeCvt::to_string(task_type), "h264_") ? FFmpegDemux(cli.input_h264_url) : FFmpegDemux(cli.input_h265_url);
            if (!demux.setup()) {
                return -1;
            }

            int width = demux.width();
            int height = demux.height();
            std::vector<FFmpegPacket> frames_queue = demux.read_some_frames(cli.limit_input_frames);

            ti.reset();
            SPDLOG_INFO("========== threads: {}, frames: {}, {} begin ==========", cli.threads, frames_queue.size(), cli.task);

            FFmpegTranscodeFactory factory;
            FFmpegTranscode *transcode = factory.create(task_type, intput_codec, output_codec, output_width, output_height, output_bitrate);
            transcode->multi_threading_test(cli.threads, frames_queue, intput_codec, width, height, output_codec, output_width, output_height, output_bitrate);

            SPDLOG_INFO("========== threads: {}, frames: {}, {} end with {:.2f}s ==========", cli.threads, frames_queue.size(), cli.task, ti.elapsed_seconds());
        }
        else {
            FFmpegDemux demux_h264(cli.input_h264_url);
            if (!demux_h264.setup()) {
                return -2;
            }
            std::vector<FFmpegPacket> frames_queue_h264 = demux_h264.read_some_frames(cli.limit_input_frames);
            int width_h264 = demux_h264.width();
            int height_h264 = demux_h264.height();

            FFmpegDemux demux_h265(cli.input_h265_url);
            if (!demux_h265.setup()) {
                return -3;
            }
            std::vector<FFmpegPacket> frames_queue_h265 = demux_h265.read_some_frames(cli.limit_input_frames);
            int width_h265 = demux_h265.width();
            int height_h265 = demux_h265.height();

            ti.reset();
            SPDLOG_INFO(
                "========== threads: {}, h264 frames: {}, h265 frames: {}, {} begin ==========",
                cli.threads, frames_queue_h264.size(), frames_queue_h265.size(), cli.task
            );

            for (auto e = (int)TranscodeType::H264DecodeOnly; e < (int)TranscodeType::AllTasks; e++) {
                FFmpegTranscodeFactory factory;
                FFmpegTranscode *transcode = factory.create((TranscodeType)e, intput_codec, output_codec, output_width, output_height, output_bitrate);

                bool is_h264 = startswith(TranscodeTypeCvt::to_string((TranscodeType)e), "h264_");
                transcode->multi_threading_test(
                    cli.threads, is_h264 ? frames_queue_h264 : frames_queue_h265, intput_codec, is_h264 ? width_h264 : width_h265, is_h264 ? height_h264 : height_h265,
                    output_codec, output_width, output_height, output_bitrate
                );
            }

            SPDLOG_INFO(
                "========== threads: {}, h264 frames: {}, h265 frames: {}, {} end with {}s ==========",
                cli.threads, frames_queue_h264.size(), frames_queue_h265.size(), cli.task, ti.elapsed_seconds()
            );
        }
    }

    return 0;
}
