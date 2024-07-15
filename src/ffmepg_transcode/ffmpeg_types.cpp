// self
#include "ffmpeg_types.hpp"

// project
#include "ffmpeg_utils.hpp"

// ffmpeg
extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

// spdlog
#include <spdlog/spdlog.h>



FFmpegPacket::FFmpegPacket()
    : m_packet(nullptr)
{
    m_packet = av_packet_alloc();
    if (nullptr == m_packet) {
        SPDLOG_ERROR("av_packet_alloc error");
    }
}


FFmpegPacket::FFmpegPacket(AVPacket *packet)
    : m_packet(packet)
{

}


FFmpegPacket::FFmpegPacket(const FFmpegPacket &other)
    : m_packet(nullptr)
{
    free();

    m_packet = av_packet_clone(other.m_packet);
    if (nullptr == m_packet) {
        SPDLOG_ERROR("av_packet_clone error");
    }
}


FFmpegPacket::FFmpegPacket(FFmpegPacket &&other) noexcept
    : m_packet(nullptr)
{
    m_packet = other.m_packet;
    other.m_packet = nullptr;
}


FFmpegPacket::~FFmpegPacket()
{
    free();
}


void FFmpegPacket::free()
{
    if (m_packet != nullptr) {
        ffmpeg_free_packet(&m_packet);
        m_packet = nullptr;
    }
}


AVPacket *FFmpegPacket::raw_ptr()
{
    return m_packet;
}


bool FFmpegPacket::is_null()
{
    return nullptr == m_packet;
}



FFmpegFrame::FFmpegFrame()
    : m_frame(nullptr)
{
    m_frame = av_frame_alloc();
    if (nullptr == m_frame) {
        SPDLOG_ERROR("av_frame_alloc error");
    }
}


FFmpegFrame::FFmpegFrame(AVFrame *frame)
    : m_frame(frame)
{

}


FFmpegFrame::FFmpegFrame(const FFmpegFrame &other)
    : m_frame(nullptr)
{
    free();

    m_frame = av_frame_clone(other.m_frame);
    if (nullptr == m_frame) {
        SPDLOG_ERROR("av_frame_clone error");
    }
}


FFmpegFrame::FFmpegFrame(FFmpegFrame &&other) noexcept
    : m_frame(nullptr)
{
    m_frame = other.m_frame;
    other.m_frame = nullptr;
}


FFmpegFrame::~FFmpegFrame()
{
    free();
}


void FFmpegFrame::free()
{
    if (m_frame != nullptr) {
        ffmpeg_free_frame(&m_frame);
        m_frame = nullptr;
    }
}


AVFrame *FFmpegFrame::raw_ptr()
{
    return m_frame;
}


bool FFmpegFrame::is_null()
{
    return nullptr == m_frame;
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
        }
        else {
            SPDLOG_ERROR("avcodec_find_encoder error, encoder_id: {}", encoder_id);
        }
    }
    if (decoder_id != 0) {
        m_codec = (AVCodec *)avcodec_find_decoder((AVCodecID)decoder_id);
        if (m_codec != nullptr) {
            m_type = CodecType::Decoder;
        }
        else {
            SPDLOG_ERROR("avcodec_find_decoder error, decoder_id: {}", decoder_id);
        }
    }
}


FFmpegCodec::FFmpegCodec(std::string encoder_name, std::string decoder_name)
    : m_codec(nullptr)
    , m_type(CodecType::Uninitialized)
{
    if (!encoder_name.empty()) {
        m_codec = (AVCodec *)avcodec_find_encoder_by_name(encoder_name.c_str());
        if (m_codec != nullptr) {
            m_type = CodecType::Encoder;
        }
        else {
            SPDLOG_ERROR("avcodec_find_encoder_by_name error, encoder_name: {}", encoder_name);
        }
    }
    if (!decoder_name.empty()) {
        m_codec = (AVCodec *)avcodec_find_decoder_by_name(decoder_name.c_str());
        if (m_codec != nullptr) {
            m_type = CodecType::Decoder;
        }
        else {
            SPDLOG_ERROR("avcodec_find_decoder_by_name error, decoder_name: {}", decoder_name);
        }
    }
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
    }
}


FFmpegCodec::FFmpegCodec(FFmpegCodec &&other) noexcept
{
    m_codec = other.m_codec;
    m_type = other.m_type;

    other.m_codec = nullptr;
    other.m_type = CodecType::Uninitialized;
}


FFmpegCodec::~FFmpegCodec()
{
    free();
}


void FFmpegCodec::free()
{
    m_codec = nullptr;
    m_type = CodecType::Uninitialized;
}


AVCodec *FFmpegCodec::raw_ptr()
{
    return m_codec;
}


bool FFmpegCodec::is_null()
{
    return nullptr == m_codec;
}




FFmpegCodecContext::FFmpegCodecContext(int encoder_id, int decoder_id)
    : m_codec(nullptr)
    , m_codec_context(nullptr)
{
    m_codec = new FFmpegCodec(encoder_id, decoder_id);
    if (nullptr == m_codec || m_codec->is_null()) {
        return;
    }

    m_codec_context = avcodec_alloc_context3(m_codec->raw_ptr());
    if (nullptr == m_codec_context) {
        SPDLOG_ERROR("avcodec_alloc_context3 error, encoder_id: {}, decoder_id: {}", encoder_id, decoder_id);
    }
}


FFmpegCodecContext::FFmpegCodecContext(std::string encoder_name, std::string decoder_name)
    : m_codec(nullptr)
    , m_codec_context(nullptr)
{
    m_codec = new FFmpegCodec(encoder_name, decoder_name);
    if (nullptr == m_codec || m_codec->is_null()) {
        return;
    }

    m_codec_context = avcodec_alloc_context3(m_codec->raw_ptr());
    if (nullptr == m_codec_context) {
        SPDLOG_ERROR("avcodec_alloc_context3 error, encoder_name: {}, decoder_name: {}", encoder_name, decoder_name);
    }
}


FFmpegCodecContext::FFmpegCodecContext(const FFmpegCodecContext &other)
{
    free();

    m_codec = other.m_codec;
    m_codec_context = other.m_codec_context;
}


FFmpegCodecContext::FFmpegCodecContext(FFmpegCodecContext &&other) noexcept
{
    m_codec = other.m_codec;
    m_codec_context = other.m_codec_context;

    other.m_codec = nullptr;
    other.m_codec_context = nullptr;
}


FFmpegCodecContext::~FFmpegCodecContext()
{
    free();
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
        avcodec_close(m_codec_context);
        avcodec_free_context(&m_codec_context);
        m_codec_context = nullptr;
    }

    m_codec = nullptr;
}


AVCodecContext *FFmpegCodecContext::raw_ptr()
{
    return m_codec_context;
}


bool FFmpegCodecContext::is_null()
{
    return nullptr == m_codec_context;
}

