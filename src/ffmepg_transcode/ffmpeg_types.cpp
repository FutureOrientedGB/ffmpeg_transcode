// self
#include "ffmpeg_types.hpp"

// project
#include "ffmpeg_utils.hpp"
#include "string_utils.hpp"

// ffmpeg
extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavformat/avformat.h>
}

// spdlog
#include <spdlog/spdlog.h>



FFmpegPacket::FFmpegPacket()
    : m_packet(nullptr)
    , m_need_more(false)
{
    m_packet = av_packet_alloc();
    if (nullptr == m_packet) {
        SPDLOG_ERROR("av_packet_alloc error");
    }
}


FFmpegPacket::FFmpegPacket(AVPacket *packet)
    : m_packet(packet)
    , m_need_more(false)
{
}


FFmpegPacket::FFmpegPacket(const FFmpegPacket &other)
{
    free();

    m_packet = av_packet_clone(other.m_packet);
    if (nullptr == m_packet) {
        SPDLOG_ERROR("av_packet_clone error");
    }
    m_need_more = other.m_need_more;
}


FFmpegPacket::FFmpegPacket(FFmpegPacket &&other) noexcept
    : m_packet(other.m_packet)
    , m_need_more(other.m_need_more)
{
    other.m_packet = nullptr;
    other.m_need_more = false;
}


FFmpegPacket::~FFmpegPacket()
{
    free();
}


void FFmpegPacket::free()
{
    if (m_packet != nullptr) {
        ffmpeg_free_packet(&m_packet);
    }
    m_packet = nullptr;
    m_need_more = false;
}


AVPacket *FFmpegPacket::raw_ptr()
{
    return m_packet;
}


bool FFmpegPacket::is_null()
{
    return nullptr == m_packet;
}


void FFmpegPacket::need_more()
{
    free();
    m_need_more = true;
}


bool FFmpegPacket::does_need_more()
{
    return m_need_more;
}



FFmpegFrame::FFmpegFrame()
    : m_frame(nullptr)
    , m_need_more(false)
{
    m_frame = av_frame_alloc();
    if (nullptr == m_frame) {
        SPDLOG_ERROR("av_frame_alloc error");
    }
}


FFmpegFrame::FFmpegFrame(AVFrame *frame)
    : m_frame(frame)
    , m_need_more(false)
{
}


FFmpegFrame::FFmpegFrame(const FFmpegFrame &other)
{
    free();

    m_frame = av_frame_clone(other.m_frame);
    if (nullptr == m_frame) {
        SPDLOG_ERROR("av_frame_clone error");
    }
    m_need_more = other.m_need_more;
}


FFmpegFrame::FFmpegFrame(FFmpegFrame &&other) noexcept
    : m_frame(other.m_frame)
    , m_need_more(other.m_need_more)
{
    other.m_frame = nullptr;
    other.m_need_more = false;
}


FFmpegFrame::~FFmpegFrame()
{
    free();
}


void FFmpegFrame::free()
{
    if (m_frame != nullptr) {
        ffmpeg_free_frame(&m_frame);
    }
    m_frame = nullptr;
    m_need_more = false;
}


AVFrame *FFmpegFrame::raw_ptr()
{
    return m_frame;
}


bool FFmpegFrame::is_null()
{
    return nullptr == m_frame;
}


void FFmpegFrame::need_more()
{
    free();
    m_need_more = true;
}


bool FFmpegFrame::does_need_more()
{
    return m_need_more;
}



FFmpegCodec::FFmpegCodec()
    : m_codec(nullptr)
    , m_type(CodecType::Uninitialized)
{
}


FFmpegCodec::FFmpegCodec(int encoder_id, int decoder_id)
    : m_codec(nullptr)
    , m_type(CodecType::Uninitialized)
{
    if (encoder_id != 0) {
        m_codec = (AVCodec *)avcodec_find_encoder((AVCodecID)encoder_id);
        if (m_codec != nullptr) {
            m_type = CodecType::Encoder;
            return;
        }
        else {
            SPDLOG_ERROR("avcodec_find_encoder error, encoder_id: {}", encoder_id);
        }
    }
    if (decoder_id != 0) {
        m_codec = (AVCodec *)avcodec_find_decoder((AVCodecID)decoder_id);
        if (m_codec != nullptr) {
            m_type = CodecType::Decoder;
            return;
        }
        else {
            SPDLOG_ERROR("avcodec_find_decoder error, decoder_id: {}", decoder_id);
        }
    }

    free();
}


FFmpegCodec::FFmpegCodec(std::string encoder_name, std::string decoder_name)
    : m_codec(nullptr)
    , m_type(CodecType::Uninitialized)
{
    if (!encoder_name.empty()) {
        m_codec = (AVCodec *)avcodec_find_encoder_by_name(encoder_name.c_str());
        if (m_codec != nullptr) {
            m_type = CodecType::Encoder;
            m_codec_name = encoder_name;
            return;
        }
        else {
            SPDLOG_ERROR("avcodec_find_encoder_by_name error, encoder_name: {}", encoder_name);
        }
    }
    if (!decoder_name.empty()) {
        m_codec = (AVCodec *)avcodec_find_decoder_by_name(decoder_name.c_str());
        if (m_codec != nullptr) {
            m_type = CodecType::Decoder;
            m_codec_name = decoder_name;
            return;
        }
        else {
            SPDLOG_ERROR("avcodec_find_decoder_by_name error, decoder_name: {}", decoder_name);
        }
    }

    free();
}


FFmpegCodec::FFmpegCodec(const FFmpegCodec &other)
    : m_codec(nullptr)
    , m_type(CodecType::Uninitialized)
{
    free();

    if (other.m_type == CodecType::Encoder) {
        m_codec = (AVCodec *)avcodec_find_encoder(other.m_codec->id);
        if (nullptr == m_codec) {
            SPDLOG_ERROR("avcodec_find_encoder error, id: {}", (int)other.m_codec->id);
        }
    }
    else if (other.m_type == CodecType::Decoder) {
        m_codec = (AVCodec *)avcodec_find_decoder(other.m_codec->id);
        if (nullptr == m_codec) {
            SPDLOG_ERROR("avcodec_find_decoder error, id: {}", (int)other.m_codec->id);
        }
    }

    if (m_codec != nullptr) {
        m_type = other.m_type;
        m_codec_name = other.m_codec_name;
        return;
    }

    free();
}


FFmpegCodec::FFmpegCodec(FFmpegCodec &&other) noexcept
{
    m_codec = other.m_codec;
    m_type = other.m_type;
    m_codec_name = other.m_codec_name;

    other.m_codec = nullptr;
    other.m_type = CodecType::Uninitialized;
    other.m_codec_name.clear();
}


FFmpegCodec::~FFmpegCodec()
{
    free();
}


void FFmpegCodec::free()
{
    m_codec = nullptr;
    m_type = CodecType::Uninitialized;
    m_codec_name.clear();
}


AVCodec *FFmpegCodec::raw_ptr()
{
    return m_codec;
}


bool FFmpegCodec::is_null()
{
    return nullptr == m_codec;
}


std::string FFmpegCodec::codec_name()
{
    return m_codec_name;
}



enum AVPixelFormat get_pixel_format_callback(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    auto thiz = (FFmpegCodecContext *)ctx->opaque;

    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == (enum AVPixelFormat)thiz->pixel_format()) {
            return *p;
        }
    }

    return AV_PIX_FMT_NONE;
}


FFmpegCodecContext::FFmpegCodecContext(int encoder_id, int decoder_id, int pixel_format)
    : m_codec(nullptr)
    , m_codec_context(nullptr)
    , m_pixel_format(pixel_format)
    , m_hw_device_type(0)
    , m_hw_device_context(nullptr)
{
    do {
        m_codec = new FFmpegCodec(encoder_id, decoder_id);
        if (nullptr == m_codec || m_codec->is_null()) {
            break;
        }

        if (!configure_hw_accel()) {
            break;
        }

        m_codec_context = avcodec_alloc_context3(m_codec->raw_ptr());
        if (nullptr == m_codec_context) {
            SPDLOG_ERROR("avcodec_alloc_context3 error, encoder_id: {}, decoder_id: {}", encoder_id, decoder_id);
            break;
        }

        m_codec_context->opaque = this;
        if (m_hw_device_type > 0) {
            m_codec_context->hw_device_ctx = av_buffer_ref(m_hw_device_context);
            m_codec_context->get_format = get_pixel_format_callback;
        }

        return;
    } while (false);


    free();
}


FFmpegCodecContext::FFmpegCodecContext(std::string encoder_name, std::string decoder_name, int pixel_format)
    : m_codec(nullptr)
    , m_codec_context(nullptr)
    , m_pixel_format(pixel_format)
    , m_hw_device_type(0)
    , m_hw_device_context(nullptr)
{
    do {
        m_codec = new FFmpegCodec(encoder_name, decoder_name);
        if (nullptr == m_codec || m_codec->is_null()) {
            return;
        }

        if (!configure_hw_accel()) {
            break;
        }

        m_codec_context = avcodec_alloc_context3(m_codec->raw_ptr());
        if (nullptr == m_codec_context) {
            SPDLOG_ERROR("avcodec_alloc_context3 error, encoder_name: {}, decoder_name: {}", encoder_name, decoder_name);
            break;
        }

        m_codec_context->opaque = this;
        if (m_hw_device_type > 0) {
            m_codec_context->hw_device_ctx = av_buffer_ref(m_hw_device_context);
            m_codec_context->get_format = get_pixel_format_callback;
        }

        return;
    } while (false);


    free();
}


FFmpegCodecContext::FFmpegCodecContext(const FFmpegCodecContext &other)
{
    free();

    m_codec = other.m_codec;
    m_codec_context = other.m_codec_context;
    m_pixel_format = other.m_pixel_format;
    m_hw_device_type = other.m_hw_device_type;
    m_hw_device_context = other.m_hw_device_context;
}


FFmpegCodecContext::FFmpegCodecContext(FFmpegCodecContext &&other) noexcept
{
    m_codec = other.m_codec;
    m_codec_context = other.m_codec_context;
    m_pixel_format = other.m_pixel_format;
    m_hw_device_type = other.m_hw_device_type;
    m_hw_device_context = other.m_hw_device_context;

    other.m_codec = nullptr;
    other.m_codec_context = nullptr;
    other.m_pixel_format = 0;
    other.m_hw_device_type = 0;
    other.m_hw_device_context = nullptr;
}


FFmpegCodecContext::~FFmpegCodecContext()
{
    free();
}


bool FFmpegCodecContext::configure_hw_accel()
{
    if (endswith(m_codec->codec_name(),"_qsv")) {
        m_hw_device_type = AV_HWDEVICE_TYPE_QSV;
        m_pixel_format = AV_PIX_FMT_QSV;
    }
    else if (endswith(m_codec->codec_name(), "_cuvid")) {
        m_hw_device_type = AV_HWDEVICE_TYPE_CUDA;
        m_pixel_format = AV_PIX_FMT_CUDA;
    }
    else if (endswith(m_codec->codec_name(), "_amf")) {
        //m_hw_device_type = AV_HWDEVICE_TYPE_AMF;
        //m_pixel_format = AV_PIX_FMT_AMF;
    }

    int code = 0;
    if (m_hw_device_type > 0) {
        code = av_hwdevice_ctx_create(&m_hw_device_context, (AVHWDeviceType)m_hw_device_type, "auto", NULL, 0);
        if (code != 0) {
            SPDLOG_ERROR("av_hwdevice_ctx_create error, code: {}, msg: {}", code, ffmpeg_error_str(code));
            return false;
        }

        bool hw_config_found = false;
        for (int i = 0;; i++) {
            const AVCodecHWConfig *config = avcodec_get_hw_config(m_codec->raw_ptr(), i);
            if (config == nullptr) {
                SPDLOG_ERROR("avcodec_get_hw_config error, m_hw_device_type: {}", av_hwdevice_get_type_name((AVHWDeviceType)m_hw_device_type));
                break;
            }

            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == m_hw_device_type) {
                hw_config_found = true;
                if (config->pix_fmt != (AVHWDeviceType)m_pixel_format) {
                    m_pixel_format = (int)config->pix_fmt;
                }
                break;
            }
        }
        if (!hw_config_found) {
            SPDLOG_ERROR("hw pixel format not found");
            return false;
        }
    }

    return true;
}


bool FFmpegCodecContext::open(std::map<std::string, std::string> options)
{
    AVDictionary *dict = nullptr;

    for (auto iter = options.begin(); iter != options.end(); iter++) {
        av_dict_set(&dict, iter->first.c_str(), iter->second.c_str(), 0);
    }

    int code = avcodec_open2(m_codec_context, m_codec->raw_ptr(), &dict);
    if (code < 0) {
        SPDLOG_ERROR("avcodec_open2 error, code: {}, msg: {}", code, ffmpeg_error_str(code));
        free();
    }

    if (dict != nullptr) {
        av_dict_free(&dict);
    }

    return 0 == code;
}


void FFmpegCodecContext::free()
{
    if (m_codec_context != nullptr) {
        if (m_hw_device_context) {
            av_buffer_unref(&m_hw_device_context);
        }

        avcodec_close(m_codec_context);
        avcodec_free_context(&m_codec_context);
    }

    m_hw_device_context = nullptr;
    m_codec_context = nullptr;

    m_codec = nullptr;
    m_pixel_format = 0;
    m_hw_device_type = 0;
}


AVCodecContext *FFmpegCodecContext::raw_ptr()
{
    return m_codec_context;
}


bool FFmpegCodecContext::is_null()
{
    return nullptr == m_codec_context;
}


int FFmpegCodecContext::pixel_format()
{
    return m_pixel_format;
}

