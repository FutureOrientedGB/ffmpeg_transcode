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
        SPDLOG_WARN("av_packet_alloc error");
    }
    //else {
    //    SPDLOG_WARN("av_packet_alloc ok, p: {}", fmt::ptr(m_packet));
    //}
}


FFmpegPacket::FFmpegPacket(AVPacket *packet)
    : m_packet(packet)
{

}


FFmpegPacket::FFmpegPacket(const FFmpegPacket &other)
    : m_packet(nullptr)
{
    if(m_packet != nullptr) {
        ffmpeg_free_packet(&m_packet);
    }

    m_packet = av_packet_clone(other.m_packet);
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
        //SPDLOG_WARN("ffmpeg_free_packet ok, p: {}", fmt::ptr(m_packet));
        ffmpeg_free_packet(&m_packet);
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
        SPDLOG_WARN("av_frame_alloc error");
    }
    //else {
    //    SPDLOG_WARN("av_frame_alloc ok, p: {}", fmt::ptr(m_frame));
    //}
}


FFmpegFrame::FFmpegFrame(AVFrame *frame)
    : m_frame(frame)
{

}


FFmpegFrame::FFmpegFrame(const FFmpegFrame &other)
    : m_frame(nullptr)
{
    if(m_frame != nullptr) {
        ffmpeg_free_frame(&m_frame);
    }

    m_frame = av_frame_clone(other.m_frame);
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
        //SPDLOG_WARN("ffmpeg_free_frame ok, p: {}", fmt::ptr(m_frame));
        ffmpeg_free_frame(&m_frame);
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

