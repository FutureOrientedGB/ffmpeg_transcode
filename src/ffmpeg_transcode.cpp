// self
#include "ffmpeg_transcode.hpp"

// c++
#include <future>
#include <thread>
#include <vector>

// project
#include "ffmpeg_utils.hpp"
#include "ffmpeg_types.hpp"
#include "ffmpeg_demux.hpp"
#include "ffmpeg_decode.hpp"
#include "ffmpeg_scale.hpp"
#include "ffmpeg_encode.hpp"
#include "math_utils.hpp"
#include "string_utils.hpp"

// ffmpeg
extern "C" {
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

// fmt
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

// cli11
#include <CLI/CLI.hpp>

// task-thread-pool
#include <task_thread_pool.hpp>



std::map<std::string, TranscodeType> TranscodeTypeCvt::s_map_string_to_enum{
    { "h264_decode_only", TranscodeType::H264DecodeOnly },
    { "h265_decode_only", TranscodeType::H265DecodeOnly },
    { "h264_to_d1_h264", TranscodeType::H264ToD1H264 },
    { "h264_to_cif_h264", TranscodeType::H264ToCifH264 },
    { "h265_to_d1_h265", TranscodeType::H265ToD1H265 },
    { "h265_to_cif_h265", TranscodeType::H265ToCifH265 },
    { "h265_to_d1_h264", TranscodeType::H265ToD1H264 },
    { "h265_to_cif_h264", TranscodeType::H265ToCifH264 },
    { "h264_to_d1_cif_h264", TranscodeType::H264ToD1CifH264 },
    { "h265_to_d1_cif_h265", TranscodeType::H265ToD1CifH265 },
    { "h265_to_d1_cif_h264", TranscodeType::H265ToD1CifH264 },
    { "all_task", TranscodeType::AllTasks },
};


std::map<TranscodeType, std::string> TranscodeTypeCvt::s_map_enum_to_string{
    { TranscodeType::H264DecodeOnly, "h264_decode_only" },
    { TranscodeType::H265DecodeOnly, "h265_decode_only" },
    { TranscodeType::H264ToD1H264, "h264_to_d1_h264" },
    { TranscodeType::H264ToCifH264, "h264_to_cif_h264" },
    { TranscodeType::H265ToD1H265, "h265_to_d1_h265" },
    { TranscodeType::H265ToCifH265, "h265_to_cif_h265" },
    { TranscodeType::H265ToD1H264, "h265_to_d1_h264" },
    { TranscodeType::H265ToCifH264, "h265_to_cif_h264" },
    { TranscodeType::H264ToD1CifH264, "h264_to_d1_cif_h264" },
    { TranscodeType::H265ToD1CifH265, "h265_to_d1_cif_h265" },
    { TranscodeType::H265ToD1CifH264, "h265_to_d1_cif_h264" },
    { TranscodeType::AllTasks, "all_task" },
};


TranscodeType TranscodeTypeCvt::from_string(std::string s)
{
    auto iter = s_map_string_to_enum.find(s);
    if (iter != s_map_string_to_enum.end()) {
        return iter->second;
    }
    return TranscodeType::Invalid;
}


std::string TranscodeTypeCvt::to_string(TranscodeType e)
{
    auto iter = s_map_enum_to_string.find(e);
    if (iter != s_map_enum_to_string.end()) {
        return iter->second;
    }
    return "";
}


std::string TranscodeTypeCvt::support_list()
{
    std::vector<std::string> result;
    for (auto e = (uint8_t)TranscodeType::Invalid + 1; e <= (uint8_t)TranscodeType::AllTasks; e++) {
        result.push_back(to_string((TranscodeType)e));
    }
    return fmt::to_string(fmt::join(result, ", "));
}



double FFmpegTranscode::multi_threading_test(
    int threads, std::vector<FFmpegPacket> &frames_queue,
    std::string input_codec, int input_width, int input_height,
    std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
) {
    SPDLOG_INFO(
        "========== threads: {}, frames: {}, decode: {},{}{} test begin ==========",
        threads, frames_queue.size(), input_codec,
        output_width.empty() ? "" : fmt::format(" scale: {},", fmt::join(output_height, ", ")),
        output_codec.empty() ? "" : fmt::format(" encode: {},", fmt::join(output_codec, ", "))
    );

    double total_speed = 0.0;
    if (threads <= 1) {
        total_speed = run(0, frames_queue, input_codec, input_width, input_height, output_codec, output_width, output_height, output_bitrate);
    }
    else {
        std::vector<std::thread> tasks;
        std::vector<std::shared_ptr<std::promise<double>>> promises;
        std::vector<std::future<double>> futures;

        for (int i = 0; i < threads; ++i) {
            std::shared_ptr<std::promise<double>> speed_promise = std::make_shared<std::promise<double>>();
            std::future<double> speed_future = speed_promise->get_future();

            int task_id = i;
            tasks.emplace_back(
                [this, speed_promise, task_id, &frames_queue, input_codec, input_width, input_height, output_codec, output_width, output_height, output_bitrate]() {
                    double result = run(task_id, frames_queue, input_codec, input_width, input_height, output_codec, output_width, output_height, output_bitrate);
                    speed_promise->set_value(result);
                }
            );

            promises.push_back(speed_promise);
            futures.push_back(std::move(speed_future));
        }

        for (auto &thread : tasks) {
            thread.join();
        }

        for (int i = 0; i < threads; ++i) {
            total_speed += futures[i].get();
        }
    }

    SPDLOG_INFO(
        "========== threads: {}, frames: {}, decode: {},{}{} test end with {:.2f}x speed ==========",
        threads, frames_queue.size(), input_codec,
        output_width.empty() ? "" : fmt::format(" scale: {},", fmt::join(output_height, ", ")),
        output_codec.empty() ? "" : fmt::format(" encode: {},", fmt::join(output_codec, ", ")),
        total_speed
    );

    return total_speed;
}


double FFmpegDecodeOnly::run(
    int task_id, std::vector<FFmpegPacket> &frames_queue,
    std::string input_codec, int input_width, int input_height,
    std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
) {
    SPDLOG_INFO(
        "task: {:2d}, frames: {}, input_codec: {}, input_width: {}, input_height: {}",
        task_id, frames_queue.size(), input_codec, input_width, input_height
    );

    FFmpegDecode decoder(input_codec);
    if (!decoder.setup()) {
        return -1;
    }

    // statics
    TimeIt ti_task;
    TimeIt ti_step;
    size_t frames = frames_queue.size();
    MovingAverage ma50_decode_frame(50);
    MovingAverage ma50_decode_gop(50);
    Percentile percentile_decode;

    int index = 1;
    for (auto i = 0; i < frames_queue.size(); i++) {
        ti_step.reset();

        // decode
        if (!decoder.send_packet(frames_queue[i])) {
            break;
        }

        FFmpegFrame yuv_frame = decoder.receive_frame();
        if (yuv_frame.does_need_more()) {
            continue;
        }
        if (yuv_frame.is_null()) {
            break;
        }

        // calc decode time
        double decode_elasped_ms = ti_step.elapsed_milliseconds();
        ma50_decode_frame.add(decode_elasped_ms);
        ma50_decode_gop.add(ma50_decode_frame.calc());
        percentile_decode.add(decode_elasped_ms);
        ti_step.reset();

        if (0 == (i + 1) % 1000) {
            double progress = 100.0 * i / frames;
            SPDLOG_INFO(
                "task: {:2d}, progress: {:.2f}%, ma50_decode_gop: {:.2f} (90%th={:.2f}) ms/frame",
                task_id, progress, ma50_decode_gop.calc(), percentile_decode.calc(0.9)
            );
        }
        else if (0 == (i + 1) % 250) {
            double progress = 100.0 * i / frames;
            SPDLOG_INFO(
                "task: {:2d}, progress: {:.2f}%, ma50_decode_frame: {:.2f} (90%th={:.2f}) ms/frame",
                task_id, progress, ma50_decode_frame.calc(), percentile_decode.calc(0.9)
            );
        }
    }

    double expect_ms = frames * 40.0;
    double task_elasped_ms = ti_task.elapsed_milliseconds();
    double speed = expect_ms / task_elasped_ms;

    SPDLOG_INFO(
        "task: {:2d}, progress: 100.00%, ma50_decode_frame: {:.2f} (90%th={:.2f}) ms/frame",
        task_id, ma50_decode_frame.calc(), percentile_decode.calc(0.9)
    );

    return speed;
}


std::string get_filter_text(std::string input_codec, int width, int height) {
    if (endswith(input_codec, "_qsv")) {
        return fmt::format("scale_qsv=w={}:h={}", width, height);
    }
    else if (endswith(input_codec, "_cuvid")) {
        return fmt::format("scale_npp=w={}:h={}", width, height);
    }
    else if (endswith(input_codec, "_amf")) {
        return fmt::format("scale_amf=w={}:h={}", width, height);  // not working
    }
    return fmt::format("scale={}:{}", width, height);
}


double FFmpegTranscodeOne::run(
    int task_id, std::vector<FFmpegPacket> &frames_queue,
    std::string input_codec, int input_width, int input_height,
    std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
) {
    SPDLOG_INFO(
        "task: {:2d}, frames: {}, input_codec: {}, input_width: {}, input_height: {}, output_codec: {}, output_width: {}, output_height: {}, output_bitrate: {}",
        task_id, frames_queue.size(), input_codec, input_width, input_height, fmt::join(output_codec, ", "), fmt::join(output_width, ", "),
        fmt::join(output_height, ", "), fmt::join(output_bitrate, ", ")
    );

    FFmpegDecode decoder(input_codec);
    if (!decoder.setup()) {
        return -1;
    }

    int pix_fmt = decoder.pixel_format();
    auto time_base = decoder.time_base();
    auto pixel_aspect = decoder.pixel_aspect();
    std::string scale_filter = get_filter_text(input_codec, output_width[0], output_height[0]);

    FFmpegScale scaler(input_width, input_height, pix_fmt, output_width[0], output_height[0], pix_fmt, pixel_aspect.first, pixel_aspect.second, time_base.first, time_base.second, scale_filter);

    FFmpegEncode encoder(output_codec[0], output_width[0], output_height[0], output_bitrate[0], pix_fmt);

    // statics
    TimeIt ti_task;
    TimeIt ti_step;
    size_t frames = frames_queue.size();
    MovingAverage ma50_decode_frame(50);
    MovingAverage ma50_decode_gop(50);
    Percentile percentile_decode;
    MovingAverage ma50_scale_frame(50);
    MovingAverage ma50_scale_gop(50);
    Percentile percentile_scale;
    MovingAverage ma50_encode_frame(50);
    MovingAverage ma50_encode_gop(50);
    Percentile percentile_encode;

    int index = 1;
    for (auto i = 0; i < frames_queue.size(); i++) {
        ti_step.reset();

        // decode
        if (!decoder.send_packet(frames_queue[i])) {
            break;
        }

        FFmpegFrame yuv_frame = decoder.receive_frame();
        if (yuv_frame.does_need_more()) {
            continue;
        }
        if (yuv_frame.is_null()) {
            break;
        }

        // calc decode time
        double decode_elasped_ms = ti_step.elapsed_milliseconds();
        ma50_decode_frame.add(decode_elasped_ms);
        ma50_decode_gop.add(ma50_decode_frame.calc());
        percentile_decode.add(decode_elasped_ms);
        ti_step.reset();

        if (!scaler.setup(yuv_frame.raw_ptr()->hw_frames_ctx)) {
            return -2;
        }

        // scale
        FFmpegFrame scaled_yuv_frame = scaler.scale(yuv_frame);
        if (scaled_yuv_frame.is_null()) {
            break;
        }

        // free decoed frame
        yuv_frame.free();

        // calc scale time
        double scale_elasped_ms = ti_step.elapsed_milliseconds();
        ma50_scale_frame.add(scale_elasped_ms);
        ma50_scale_gop.add(ma50_scale_frame.calc());
        percentile_scale.add(scale_elasped_ms);
        ti_step.reset();

        if (!encoder.setup(scaler.hw_frames_context())) {
            return -3;
        }

        // encode
        if (!encoder.send_frame(scaled_yuv_frame)) {
            break;
        }

        // free scaled frame
        scaled_yuv_frame.free();

        FFmpegPacket encoded_es_packet = encoder.receive_packet();
        if (encoded_es_packet.does_need_more()) {
            continue;
        }
        if (encoded_es_packet.is_null()) {
            break;
        }

        //printf("i: %d, size: %d\n", i, encoded_es_packet.raw_ptr()->size);

        // free encoded frame
        encoded_es_packet.free();

        // calc encode time
        double encode_elasped_ms = ti_step.elapsed_milliseconds();
        ma50_encode_frame.add(encode_elasped_ms);
        ma50_encode_gop.add(ma50_encode_frame.calc());
        percentile_encode.add(encode_elasped_ms);
        ti_step.reset();

        if (0 == (i + 1) % 1000) {
            double progress = 100.0 * i / frames;
            SPDLOG_INFO(
                "task: {:2d}, progress: {:.2f}%, ma50_decode_gop: {:.2f} (90%th={:.2f}) ms/frame, ma50_scale_gop: {:.2f} (90%th={:.2f}) ms/frame, ma50_encode_gop: {:.2f} (90%th={:.2f}) ms/frame",
                task_id, progress, ma50_decode_gop.calc(), percentile_decode.calc(0.9), ma50_scale_gop.calc(), percentile_scale.calc(0.9), ma50_encode_gop.calc(), percentile_encode.calc(0.9)
            );
        }
        else if (0 == (i + 1) % 250) {
            double progress = 100.0 * i / frames;
            SPDLOG_INFO(
                "task: {:2d}, progress: {:.2f}%, ma50_decode_frame: {:.2f} (90%th={:.2f}) ms/frame, ma50_scale_frame: {:.2f} (90%th={:.2f}) ms/frame, ma50_encode_frame: {:.2f} (90%th={:.2f}) ms/frame",
                task_id, progress, ma50_decode_frame.calc(), percentile_decode.calc(0.9), ma50_scale_frame.calc(), percentile_scale.calc(0.9), ma50_encode_frame.calc(), percentile_encode.calc(0.9)
            );
        }
    }

    double expect_ms = frames * 40.0;
    double task_elasped_ms = ti_task.elapsed_milliseconds();
    double speed = expect_ms / task_elasped_ms;

    SPDLOG_INFO(
        "task: {:2d}, progress: 100.00%, ma50_decode_gop: {:.2f} (90%th={:.2f}) ms/frame, ma50_scale_gop: {:.2f} (90%th={:.2f}) ms/frame, ma50_encode_gop: {:.2f} (90%th={:.2f}) ms/frame",
        task_id, ma50_decode_gop.calc(), percentile_decode.calc(0.9), ma50_scale_gop.calc(), percentile_scale.calc(0.9), ma50_encode_gop.calc(), percentile_encode.calc(0.9)
    );

    return speed;
}


double FFmpegTranscodeTwo::run(
    int task_id, std::vector<FFmpegPacket> &frames_queue,
    std::string input_codec, int input_width, int input_height,
    std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
) {
    SPDLOG_INFO(
        "task: {:2d}, frames: {} input_codec: {}, input_width: {}, input_height: {}, output_codec: {}, output_width: {}, output_height: {}, output_bitrate: {}",
        task_id, frames_queue.size(), input_codec, input_width, input_height, fmt::join(output_codec, ", "), fmt::join(output_width, ", "), fmt::join(output_height, ", "), fmt::join(output_bitrate, ", ")
    );

    FFmpegDecode decoder(input_codec);
    if (!decoder.setup()) {
        return -1;
    }

    int pix_fmt = decoder.pixel_format();
    auto time_base = decoder.time_base();
    auto pixel_aspect = decoder.pixel_aspect();
    std::string scale_filter1 = get_filter_text(input_codec, output_width[0], output_height[0]);
    std::string scale_filter2 = get_filter_text(input_codec, output_width[1], output_height[1]);

    FFmpegScale scaler1(input_width, input_height, pix_fmt, output_width[0], output_height[0], pix_fmt, pixel_aspect.first, pixel_aspect.second, time_base.first, time_base.second, scale_filter1);
    FFmpegScale scaler2(input_width, input_height, pix_fmt, output_width[1], output_height[1], pix_fmt, pixel_aspect.first, pixel_aspect.second, time_base.first, time_base.second, scale_filter2);

    FFmpegEncode encoder1(output_codec[0], output_width[0], output_height[0], output_bitrate[0], pix_fmt);
    FFmpegEncode encoder2(output_codec[1], output_width[1], output_height[1], output_bitrate[1], pix_fmt);

    // statics
    TimeIt ti_task;
    TimeIt ti_step;
    size_t frames = frames_queue.size();
    MovingAverage ma50_decode_frame(50);
    MovingAverage ma50_decode_gop(50);
    Percentile percentile_decode;
    MovingAverage ma_scale_encode_50frames(50);
    MovingAverage ma_scale_encode_200gops(50);
    Percentile percentile_scale_encode;

    task_thread_pool::task_thread_pool thread_pool(2);
    for (auto i = 0; i < frames_queue.size(); i++) {
        ti_step.reset();

        // decode
        if (!decoder.send_packet(frames_queue[i])) {
            break;
        }

        FFmpegFrame yuv_frame = decoder.receive_frame();
        if (yuv_frame.does_need_more()) {
            continue;
        }
        if (yuv_frame.is_null()) {
            break;
        }

        // calc decode time
        double decode_elasped_ms = ti_step.elapsed_milliseconds();
        ma50_decode_frame.add(decode_elasped_ms);
        ma50_decode_gop.add(ma50_decode_frame.calc());
        percentile_decode.add(decode_elasped_ms);
        ti_step.reset();

        if (!scaler1.setup(yuv_frame.raw_ptr()->hw_frames_ctx)) {
            return -2;
        }

        if (!scaler2.setup(yuv_frame.raw_ptr()->hw_frames_ctx)) {
            return -3;
        }

        bool has_error1 = false;
        bool need_more1 = false;
        thread_pool.submit(
            [&yuv_frame, &scaler1, &encoder1, &need_more1, &has_error1]() {
                // scale
                FFmpegFrame scaled_yuv_frame = scaler1.scale(yuv_frame);
                if (scaled_yuv_frame.is_null()) {
                    return;
                }

                if (!encoder1.setup(scaler1.hw_frames_context())) {
                    has_error1 = true;
                    return;
                }

                // encode
                if (!encoder1.send_frame(scaled_yuv_frame)) {
                    return;
                }

                // free scaled frame
                scaled_yuv_frame.free();

                FFmpegPacket encoded_es_packet = encoder1.receive_packet();
                if (encoded_es_packet.does_need_more()) {
                    need_more1 = true;
                }
                if (encoded_es_packet.is_null()) {
                    return;
                }

                // free encoded frame
                encoded_es_packet.free();
            }
        );

        bool has_error2 = false;
        bool need_more2 = false;
        thread_pool.submit(
            [&yuv_frame, &scaler2, &encoder2, &need_more2, &has_error2]() {
                // scale
                FFmpegFrame scaled_yuv_frame = scaler2.scale(yuv_frame);
                if (scaled_yuv_frame.is_null()) {
                    return;
                }

                if (!encoder2.setup(scaler2.hw_frames_context())) {
                    has_error2 = true;
                    return;
                }

                // encode
                if (!encoder2.send_frame(scaled_yuv_frame)) {
                    return;
                }

                // free scaled frame
                scaled_yuv_frame.free();

                FFmpegPacket encoded_es_packet = encoder2.receive_packet();
                if (encoded_es_packet.does_need_more()) {
                    need_more2 = true;
                }
                if (encoded_es_packet.is_null()) {
                    return;
                }

                // free encoded frame
                encoded_es_packet.free();
            }
        );

        thread_pool.wait_for_tasks();

        if (has_error1 || has_error2) {
            return -4;
        }

        if (need_more1 || need_more2) {
            continue;
        }

        // calc scale & decode time
        double encode_elasped_ms = ti_step.elapsed_milliseconds();
        ma_scale_encode_50frames.add(encode_elasped_ms);
        ma_scale_encode_200gops.add(ma_scale_encode_50frames.calc());
        percentile_scale_encode.add(encode_elasped_ms);
        ti_step.reset();

        if (0 == (i + 1) % 1000) {
            double progress = 100.0 * i / frames;
            SPDLOG_INFO(
                "task: {:2d}, progress: {:.2f}%, ma50_decode_gop: {:.2f} (90%th={:.2f}) ms/frame, ma_scale_encode_200gops: {:.2f} (90%th={:.2f}) ms/frame",
                task_id, progress, ma50_decode_gop.calc(), percentile_decode.calc(0.9), ma_scale_encode_200gops.calc(), percentile_scale_encode.calc(0.9)
            );
        }
        else if (0 == (i + 1) % 250) {
            double progress = 100.0 * i / frames;
            SPDLOG_INFO(
                "task: {:2d}, progress: {:.2f}%, ma50_decode_frame: {:.2f} (90%th={:.2f}) ms/frame, ma_scale_encode_50frames: {:.2f} (90%th={:.2f}) ms/frame",
                task_id, progress, ma50_decode_frame.calc(), percentile_decode.calc(0.9), ma_scale_encode_50frames.calc(), percentile_scale_encode.calc(0.9)
            );
        }
    }

    double expect_ms = frames * 40.0;
    double task_elasped_ms = ti_task.elapsed_milliseconds();
    double speed = expect_ms / task_elasped_ms;

    SPDLOG_INFO(
        "task: {:2d}, progress: 100.00%, ma50_decode_frame: {:.2f} (90%th={:.2f}) ms/frame, ma_scale_encode_50frames: {:.2f} (90%th={:.2f}) ms/frame",
        task_id, ma50_decode_frame.calc(), percentile_decode.calc(0.9), ma_scale_encode_50frames.calc(), percentile_scale_encode.calc(0.9)
    );

    return speed;
}



FFmpegTranscodeFactory::FFmpegTranscodeFactory()
    : m_transcode(nullptr)
{
}


FFmpegTranscodeFactory::~FFmpegTranscodeFactory()
{
    if (m_transcode != nullptr) {
        delete m_transcode;
        m_transcode = nullptr;
    }
}



std::string get_h264_encoder_name(bool intel_quick_sync_video, bool nvidia_video_codec, bool amd_advanced_media_framework) {
    if (intel_quick_sync_video) {
        return "h264_qsv";
    }
    else if (nvidia_video_codec) {
        return "h264_nvenc";
    }
    else if (nvidia_video_codec) {
        return "h264_amf";
    }
    else {
        return "libx264";
    }
}


std::string get_h265_encoder_name(bool intel_quick_sync_video, bool nvidia_video_codec, bool amd_advanced_media_framework) {
    if (intel_quick_sync_video) {
        return "hevc_qsv";
    }
    else if (nvidia_video_codec) {
        return "hevc_nvenc";
    }
    else if (nvidia_video_codec) {
        return "hevc_amf";
    }
    else {
        return "libx265";
    }
}


void get_h264_decoder_name(std::string &codec_name, bool intel_quick_sync_video, bool nvidia_video_codec, bool amd_advanced_media_framework) {
    if (intel_quick_sync_video) {
        codec_name = "h264_qsv";
    }
    else if (nvidia_video_codec) {
        codec_name = "h264_cuvid";
    }
    else if (amd_advanced_media_framework) {
        codec_name = "h264_amf";
    }
    else {
        codec_name = "h264";
    }
}


void get_h265_decoder_name(std::string &codec_name, bool intel_quick_sync_video, bool nvidia_video_codec, bool amd_advanced_media_framework) {
    if (intel_quick_sync_video) {
        codec_name = "hevc_qsv";
    }
    else if (nvidia_video_codec) {
        codec_name = "hevc_cuvid";
    }
    else if (amd_advanced_media_framework) {
        codec_name = "hevc_amf";
    }
    else {
        codec_name = "hevc";
    }
}


void get_decoder_name(std::string &codec_name, bool intel_quick_sync_video, bool nvidia_video_codec, bool amd_advanced_media_framework) {
    if (codec_name == "h264") {
        get_h264_decoder_name(codec_name, intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework);
    }
    else if (codec_name == "hevc") {
        get_h265_decoder_name(codec_name, intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework);
    }
}


FFmpegTranscode *FFmpegTranscodeFactory::create(
    TranscodeType t, std::string &intput_codec, std::vector<std::string> &output_codec, std::vector<int> &output_width, std::vector<int> &output_height,
    std::vector<int64_t> &output_bitrate, bool intel_quick_sync_video, bool nvidia_video_codec, bool amd_advanced_media_framework
) {
    output_codec.clear();
    output_width.clear();
    output_height.clear();
    output_bitrate.clear();

    m_transcode = nullptr;

    get_decoder_name(intput_codec, intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework);

    switch (t)
    {
    case TranscodeType::H264DecodeOnly:
    {
        m_transcode = new FFmpegDecodeOnly();
    }
    break;

    case TranscodeType::H265DecodeOnly:
    {
        m_transcode = new FFmpegDecodeOnly();
    }
    break;

    case TranscodeType::H264ToD1H264:
    {
        output_codec.push_back(get_h264_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(720);
        output_height.push_back(480);
        output_bitrate.push_back(1000 * 1000);
        m_transcode = new FFmpegTranscodeOne();
    }
    break;
    case TranscodeType::H264ToCifH264:
    {
        output_codec.push_back(get_h264_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(352);
        output_height.push_back(288);
        output_bitrate.push_back(128 * 1000);
        m_transcode = new FFmpegTranscodeOne();
    }
    break;
    case TranscodeType::H265ToD1H265:
    {
        output_codec.push_back(get_h265_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(720);
        output_height.push_back(480);
        output_bitrate.push_back(1000 * 1000);
        m_transcode = new FFmpegTranscodeOne();
    }
    break;
    case TranscodeType::H265ToCifH265:
    {
        output_codec.push_back(get_h265_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(352);
        output_height.push_back(288);
        output_bitrate.push_back(128 * 1000);
        m_transcode = new FFmpegTranscodeOne();
    }
    break;
    case TranscodeType::H265ToD1H264:
    {
        output_codec.push_back(get_h264_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(720);
        output_height.push_back(480);
        output_bitrate.push_back(1000 * 1000);
        m_transcode = new FFmpegTranscodeOne();
    }
    break;
    case TranscodeType::H265ToCifH264:
    {
        output_codec.push_back(get_h264_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(352);
        output_height.push_back(288);
        output_bitrate.push_back(128 * 1000);
        m_transcode = new FFmpegTranscodeOne();
    }
    break;

    case TranscodeType::H264ToD1CifH264:
    {
        output_codec.push_back(get_h264_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_codec.push_back(get_h264_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(720);
        output_height.push_back(480);
        output_width.push_back(352);
        output_height.push_back(288);
        output_bitrate.push_back(1000 * 1000);
        output_bitrate.push_back(128 * 1000);
        m_transcode = new FFmpegTranscodeTwo();
    }
    break;
    case TranscodeType::H265ToD1CifH265:
    {
        output_codec.push_back(get_h265_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_codec.push_back(get_h265_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(720);
        output_height.push_back(480);
        output_width.push_back(352);
        output_height.push_back(288);
        output_bitrate.push_back(1000 * 1000);
        output_bitrate.push_back(128 * 1000);
        m_transcode = new FFmpegTranscodeTwo();
    }
    break;
    case TranscodeType::H265ToD1CifH264:
    {
        output_codec.push_back(get_h264_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_codec.push_back(get_h264_encoder_name(intel_quick_sync_video, nvidia_video_codec, amd_advanced_media_framework));
        output_width.push_back(720);
        output_height.push_back(480);
        output_width.push_back(352);
        output_height.push_back(288);
        output_bitrate.push_back(1000 * 1000);
        output_bitrate.push_back(128 * 1000);
        m_transcode = new FFmpegTranscodeTwo();
    }
    break;
    }

    return m_transcode;
}

