// project
#include "ffmpeg_demux.hpp"
#include "ffmpeg_transcode.hpp"
#include "ffmpeg_utils.hpp"
#include "math_utils.hpp"
#include "string_utils.hpp"

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



class CommandArguments {
public:
    CommandArguments()
        : input_h264_url("./media/1080p.25fps.4M.264")
        , input_h265_url("./media/1080p.25fps.3M.265")
        , log_path("transcode.log")
        , log_level((int)spdlog::level::info)
        , ffmpeg_log_level(AV_LOG_TRACE)
        , threads(1)
        , limit_input_frames(INT_MAX)
        , task("h264_to_cif_h264")
        , intel_quick_sync_video(false)
        , nvidia_video_codec(false)
        , amd_advanced_media_framework(false)
    {
    }

    void add_options(CLI::App &app)
    {
        app.add_option("--input_h264", input_h264_url, fmt::format("input h264 url (default {})", input_h264_url));
        app.add_option("--input_h265", input_h265_url, fmt::format("input h265 url (default {})", input_h265_url));
        app.add_option("--log_path", log_path, fmt::format("log path (default {})", log_path));
        app.add_option("--log_level", log_path, "log level (default spdlog::level::info)");
        app.add_option("--ffmpeg_log_level", ffmpeg_log_level, "ffmpeg log level (default AV_LOG_INFO)");
        app.add_option("--limit_input_frames", limit_input_frames, "limit input frames (default INT_MAX)");
        app.add_option("--threads", threads, fmt::format("concurrent threads (default {})", threads));
        app.add_option("--task", task, fmt::format("transcode task name (default {}, support list: {})", task, TranscodeTypeCvt::support_list()));
        app.add_option("--intel_quick_sync_video", intel_quick_sync_video, fmt::format("enable intel quick sync video (default {})", intel_quick_sync_video));
        app.add_option("--nvidia_video_codec", nvidia_video_codec, fmt::format("enable nvidia video codec (default {})", nvidia_video_codec));
        app.add_option("--amd_advanced_media_framework", amd_advanced_media_framework, fmt::format("enable amd advanced media framework (default {})", amd_advanced_media_framework));
    }

    std::string input_h264_url;
    std::string input_h265_url;
    std::string log_path;
    int log_level;
    int ffmpeg_log_level;
    int threads;
    int limit_input_frames;
    std::string task;
    bool intel_quick_sync_video;
    bool nvidia_video_codec;
    bool amd_advanced_media_framework;
};


int transcode(CommandArguments args) {
    TimeIt ti;
    TranscodeType task_type = TranscodeTypeCvt::from_string(args.task);
    if (task_type != TranscodeType::Invalid) {
        std::string input_codec;
        std::vector<std::string> output_codec;
        std::vector<int> output_width;
        std::vector<int> output_height;
        std::vector<int64_t> output_bitrate;

        if (task_type != TranscodeType::AllTasks) {
            FFmpegDemux demux = startswith(TranscodeTypeCvt::to_string(task_type), "h264_") ? FFmpegDemux(args.input_h264_url) : FFmpegDemux(args.input_h265_url);
            if (!demux.setup()) {
                return -1;
            }

            int width = demux.width();
            int height = demux.height();
            input_codec = demux.codec_name();
            std::vector<FFmpegPacket> frames_queue = demux.read_some_frames(args.limit_input_frames);

            ti.reset();
            SPDLOG_INFO("========== threads: {}, frames: {}, {} begin ==========", args.threads, frames_queue.size(), args.task);

            FFmpegTranscodeFactory factory;
            FFmpegTranscode *transcode = factory.create(
                task_type, input_codec, output_codec, output_width, output_height, output_bitrate,
                args.intel_quick_sync_video, args.nvidia_video_codec, args.amd_advanced_media_framework
            );
            transcode->multi_threading_test(args.threads, frames_queue, input_codec, width, height, output_codec, output_width, output_height, output_bitrate);

            SPDLOG_INFO("========== threads: {}, frames: {}, {} end with {:.2f}s ==========", args.threads, frames_queue.size(), args.task, ti.elapsed_seconds());
        }
        else {
            FFmpegDemux demux_h264(args.input_h264_url);
            if (!demux_h264.setup()) {
                return -2;
            }
            std::vector<FFmpegPacket> frames_queue_h264 = demux_h264.read_some_frames(args.limit_input_frames);
            int width_h264 = demux_h264.width();
            int height_h264 = demux_h264.height();

            FFmpegDemux demux_h265(args.input_h265_url);
            if (!demux_h265.setup()) {
                return -3;
            }
            std::vector<FFmpegPacket> frames_queue_h265 = demux_h265.read_some_frames(args.limit_input_frames);
            int width_h265 = demux_h265.width();
            int height_h265 = demux_h265.height();

            ti.reset();
            SPDLOG_INFO(
                "========== threads: {}, h264 frames: {}, h265 frames: {}, {} begin ==========",
                args.threads, frames_queue_h264.size(), frames_queue_h265.size(), args.task
            );

            for (auto e = (int)TranscodeType::H264DecodeOnly; e < (int)TranscodeType::AllTasks; e++) {
                FFmpegTranscodeFactory factory;
                FFmpegTranscode *transcode = factory.create(
                    (TranscodeType)e, input_codec, output_codec, output_width,
                    output_height, output_bitrate, args.intel_quick_sync_video, args.nvidia_video_codec, args.amd_advanced_media_framework
                );

                bool is_h264 = startswith(TranscodeTypeCvt::to_string((TranscodeType)e), "h264_");
                transcode->multi_threading_test(
                    args.threads, is_h264 ? frames_queue_h264 : frames_queue_h265, input_codec, is_h264 ? width_h264 : width_h265, is_h264 ? height_h264 : height_h265,
                    output_codec, output_width, output_height, output_bitrate
                );
            }

            SPDLOG_INFO(
                "========== threads: {}, h264 frames: {}, h265 frames: {}, {} end with {}s ==========",
                args.threads, frames_queue_h264.size(), frames_queue_h265.size(), args.task, ti.elapsed_seconds()
            );
        }
    }

    return 0;
}


int main(int argc, char **argv) {
    // parse cli
    CLI::App app("ffmpeg transcode");
    CommandArguments args;
    args.add_options(app);
    CLI11_PARSE(app, argc, argv);

    // setup logger
    ffmpeg_log_default(args.ffmpeg_log_level);

    auto file_logger = spdlog::basic_logger_mt("transcode", args.log_path);
    spdlog::set_default_logger(file_logger);
    spdlog::set_level((spdlog::level::level_enum)args.log_level);
    spdlog::flush_on((spdlog::level::level_enum)args.log_level);

    // transcode
    transcode(args);

    return 0;
}
