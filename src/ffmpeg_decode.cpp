// self
#include "ffmpeg_decode.hpp"

// project
#include "ffmpeg_utils.hpp"
#include "ffmpeg_types.hpp"
#include "string_utils.hpp"

// ffmpeg
extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
}

// spdlog
#include <spdlog/spdlog.h>



FFmpegDecode::FFmpegDecode(std::string codec_name, int pixel_format)
    : m_codec_name(codec_name)
    , m_codec_context(nullptr)
    , m_pixel_format(pixel_format)
{
}


FFmpegDecode::~FFmpegDecode()
{
    teardown();
}


bool FFmpegDecode::setup() {
    if (m_codec_context != nullptr) {
        return true;
    }

    do {
        m_codec_context = (FFmpegCodecContext *)new FFmpegDecoderContext(m_codec_name, m_pixel_format);
        if (m_codec_context->is_null()) {
            break;
        }

        int code = 0;
        std::map<std::string, std::string> options;
        if (m_codec_name == "libx264") {
            options.insert(std::make_pair("threads", "1"));
        }
        else if (m_codec_name == "libx265") {

        }
        else if (endswith(m_codec_name, "_qsv")) {
            code = av_opt_set(m_codec_context->raw_ptr()->priv_data, "async_depth", "1", 0);
            if (code < 0) {
                SPDLOG_ERROR("av_opt_set(async_depth, 1) error, code: {}, msg: {}, m_encoder_name: {}", code, ffmpeg_error_str(code), m_codec_name);
            }
        }
        else if (endswith(m_codec_name, "_cuvid")) {

        }
        else if (endswith(m_codec_name, "_amf")) {

        }

        if (!m_codec_context->open(options)) {
            break;
        }

        return true;
    } while (false);

    teardown();

    return false;
}


void FFmpegDecode::teardown()
{
    if (m_codec_context != nullptr) {
        delete m_codec_context;
    }
    m_codec_context = nullptr;

    m_codec_name.clear();
    m_pixel_format = AV_PIX_FMT_NONE;
}


bool FFmpegDecode::send_packet(FFmpegPacket &packet) {
    int code = avcodec_send_packet(m_codec_context->raw_ptr(), packet.raw_ptr());
    if (code < 0) {
        SPDLOG_WARN("avcodec_send_packet error, code: {}, msg: {}", code, ffmpeg_error_str(code));
        return false;
    }

    return true;
}


FFmpegFrame FFmpegDecode::receive_frame() {
    int code = 0;
    while (code >= 0) {
        FFmpegFrame frame;
        if (frame.is_null()) {
            code = AVERROR(ENOMEM);
            return frame;
        }

        code = avcodec_receive_frame(m_codec_context->raw_ptr(), frame.raw_ptr());
        if (code == AVERROR(EAGAIN)) {
            frame.need_more();
        }
        else if (code == AVERROR_EOF) {
        }
        else if (code < 0) {
            SPDLOG_WARN("avcodec_receive_frame error, code: {}, msg: {}", code, ffmpeg_error_str(code));
        }

        return frame;
    }

    return FFmpegFrame(nullptr);
}


int FFmpegDecode::pixel_format()
{
    return m_codec_context->pixel_format();
}


std::pair<int, int> FFmpegDecode::pixel_aspect()
{
    return m_codec_context->pixel_aspect();
}


std::pair<int, int> FFmpegDecode::time_base()
{
    return m_codec_context->time_base();
}
