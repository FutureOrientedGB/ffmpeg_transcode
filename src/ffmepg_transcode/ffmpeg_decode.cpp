// self
#include "ffmpeg_decode.hpp"

// project
#include "ffmpeg_utils.hpp"
#include "ffmpeg_types.hpp"

// ffmpeg
extern "C" {
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
	do {
		m_codec_context = new FFmpegCodecContext("", m_codec_name, m_pixel_format);
		if (m_codec_context->is_null()) {
			break;
		}

		if (!m_codec_context->open({ {"threads", "1"} })) {
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


int FFmpegDecode::hw_device_type()
{
	return m_codec_context->hw_device_type();
}


AVBufferRef *FFmpegDecode::hw_device_context()
{
	return m_codec_context->hw_device_context();
}


std::pair<int, int> FFmpegDecode::pixel_aspect()
{
	return m_codec_context->pixel_aspect();
}


std::pair<int, int> FFmpegDecode::time_base()
{
	return m_codec_context->time_base();
}
