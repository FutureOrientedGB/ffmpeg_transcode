// self
#include "ffmpeg_scale.hpp"

// project
#include "ffmpeg_utils.hpp"
#include "ffmpeg_types.hpp"
#include "string_utils.hpp"

// ffmpeg
extern "C" {
#include <libavutil/opt.h>
#include <libavutil/hwcontext.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

// spdlog
#include <spdlog/spdlog.h>



FFmpegScale::FFmpegScale(int src_width, int src_height, int src_pixel_format, int dst_width, int dst_height, int dst_pixel_format, int pixel_aspect_num, int pixel_aspect_den, int time_base_num, int time_base_den, std::string filter_text)
    : m_src_width(src_width)
    , m_src_height(src_height)
    , m_src_pixel_format(src_pixel_format)
    , m_dst_width(dst_width)
    , m_dst_height(dst_height)
    , m_dst_pixel_format(dst_pixel_format)
    , m_pixel_aspect_num(pixel_aspect_num)
    , m_pixel_aspect_den(pixel_aspect_den)
    , m_time_base_num(time_base_num)
    , m_time_base_den(time_base_den)
    , m_filter_text(filter_text)
    , m_filter_graph(nullptr)
    , m_buffer_src_filter_context(nullptr)
    , m_buffer_sink_filter_context(nullptr)
{
}


FFmpegScale::~FFmpegScale()
{
    teardown();
}


bool FFmpegScale::setup(AVBufferRef *hw_frames_context) {
    if (m_filter_graph != nullptr) {
        return true;
    }

    AVFilterInOut *inputs = nullptr;
    AVFilterInOut *outputs = nullptr;

    do {
        AVFilter *buffer_src_filter = (AVFilter *)avfilter_get_by_name("buffer");
        if (nullptr == buffer_src_filter) {
            SPDLOG_ERROR("avfilter_get_by_name(buffer) error");
            break;
        }

        AVFilter *buffer_sink_filter = (AVFilter *)avfilter_get_by_name("buffersink");
        if (nullptr == buffer_sink_filter) {
            SPDLOG_ERROR("avfilter_get_by_name(buffersink) error");
            break;
        }

        inputs = avfilter_inout_alloc();
        if (nullptr == inputs) {
            SPDLOG_ERROR("avfilter_inout_alloc -> inputs error");
            break;
        }

        outputs = avfilter_inout_alloc();
        if (nullptr == outputs) {
            SPDLOG_ERROR("avfilter_inout_alloc -> outputs error");
            break;
        }

        m_filter_graph = avfilter_graph_alloc();
        if (nullptr == m_filter_graph) {
            SPDLOG_ERROR("avfilter_graph_alloc error");
            break;
        }

        std::string args = fmt::format(
            "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
            m_src_width, m_src_height, m_src_pixel_format, m_time_base_num, m_time_base_den, m_pixel_aspect_num, m_pixel_aspect_den
        );

        int code = avfilter_graph_create_filter(&m_buffer_src_filter_context, buffer_src_filter, "in", args.c_str(), NULL, m_filter_graph);
        if (code < 0) {
            SPDLOG_ERROR("avfilter_graph_create_filter(in) error, code: {}, msg: {}", code, ffmpeg_error_str(code));
            break;
        }

        code = avfilter_graph_create_filter(&m_buffer_sink_filter_context, buffer_sink_filter, "out", NULL, NULL, m_filter_graph);
        if (code < 0) {
            SPDLOG_ERROR("avfilter_graph_create_filter(in) error, code: {}, msg: {}", code, ffmpeg_error_str(code));
            break;
        }

        if (hw_frames_context != nullptr) {
            AVBufferSrcParameters *buffer_src_params = av_buffersrc_parameters_alloc();
            if (nullptr == buffer_src_params) {
                SPDLOG_ERROR("av_buffersrc_parameters_alloc error");
                break;
            }

            buffer_src_params->format = m_src_pixel_format;
            buffer_src_params->hw_frames_ctx = av_buffer_ref(hw_frames_context);
            int code = av_buffersrc_parameters_set(m_buffer_src_filter_context, buffer_src_params);
            av_free(buffer_src_params);
            if (code < 0) {
                SPDLOG_ERROR("av_buffersrc_parameters_set error");
                break;
            }
        }

        AVPixelFormat pix_fmts[] = { (AVPixelFormat)m_src_pixel_format, AV_PIX_FMT_NONE };
        code = av_opt_set_int_list(m_buffer_sink_filter_context, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
        if (code < 0) {
            SPDLOG_ERROR("avfilter_graph_create_filter(in) error, code: {}, msg: {}", code, ffmpeg_error_str(code));
            break;
        }

        outputs->name = av_strdup("in");
        outputs->filter_ctx = m_buffer_src_filter_context;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = m_buffer_sink_filter_context;
        inputs->pad_idx = 0;
        inputs->next = NULL;

        code = avfilter_graph_parse_ptr(m_filter_graph, m_filter_text.c_str(), &inputs, &outputs, NULL);
        if (code < 0) {
            SPDLOG_ERROR("avfilter_graph_parse_ptr error, code: {}, msg: {}", code, ffmpeg_error_str(code));
            break;
        }

        code = avfilter_graph_config(m_filter_graph, NULL);
        if (code < 0) {
            SPDLOG_ERROR("avfilter_graph_config error, code: {}, msg: {}", code, ffmpeg_error_str(code));
            break;
        }

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);

        return true;

    } while (false);

    if (inputs != nullptr) {
        avfilter_inout_free(&inputs);
    }

    if (outputs != nullptr) {
        avfilter_inout_free(&outputs);
    }

    teardown();

    return false;
}


void FFmpegScale::teardown()
{
    if (m_filter_graph != nullptr) {
        avfilter_graph_free(&m_filter_graph);
    }
    m_filter_graph = nullptr;
}


FFmpegFrame FFmpegScale::scale(FFmpegFrame &frame) {
    FFmpegFrame scaled_frame;
    if (scaled_frame.is_null()) {
        return scaled_frame;
    }

    int code = 0;
    if (startswith(m_filter_text, "scale=")) {
        scaled_frame.raw_ptr()->format = (enum AVPixelFormat)m_dst_pixel_format;
        scaled_frame.raw_ptr()->width = m_dst_width;
        scaled_frame.raw_ptr()->height = m_dst_height;

        code = av_frame_get_buffer(scaled_frame.raw_ptr(), 1);
        if (code < 0) {
            SPDLOG_ERROR("av_frame_get_buffer error, code: {}, msg: {}", code, ffmpeg_error_str(code));
            return scaled_frame;
        }
    }

    code = av_buffersrc_add_frame_flags(m_buffer_src_filter_context, frame.raw_ptr(), AV_BUFFERSRC_FLAG_KEEP_REF);
    if (code < 0) {
        SPDLOG_ERROR("av_buffersrc_add_frame_flags error, code: {}, msg: {}", code, ffmpeg_error_str(code));
        return scaled_frame;
    }

    while (true) {
        code = av_buffersink_get_frame(m_buffer_sink_filter_context, scaled_frame.raw_ptr());
        if (code == AVERROR(EAGAIN)) {
            scaled_frame.does_need_more();
        }
        if (code == AVERROR_EOF) {
        }
        else if (code < 0) {
            SPDLOG_ERROR("av_buffersink_get_frame error, code: {}, msg: {}", code, ffmpeg_error_str(code));
        }

        return scaled_frame;
    }

    return scaled_frame;
}


AVBufferRef *FFmpegScale::hw_frames_context()
{
    return av_buffersink_get_hw_frames_ctx(m_buffer_sink_filter_context);
}

