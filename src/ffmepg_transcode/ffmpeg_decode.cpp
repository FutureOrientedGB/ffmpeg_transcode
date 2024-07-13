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



FFmpegDecode::FFmpegDecode(std::string decoder_name)
	: m_decoder_name(decoder_name)
	, m_codec_context(nullptr)
{
}


FFmpegDecode::~FFmpegDecode()
{
	teardown();
}


bool FFmpegDecode::setup() {
	m_codec_context = new FFmpegCodecContext("", m_decoder_name);
	if (m_codec_context->is_null()) {
		return false;
	}

	if (!m_codec_context->open({ {"threads", "1"} })) {
		return false;
	}

	return true;
}


void FFmpegDecode::teardown()
{
	if (m_codec_context != nullptr) {
		delete m_codec_context;
		m_codec_context = nullptr;
	}
	m_decoder_name.clear();
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
			break;
		}

		code = avcodec_receive_frame(m_codec_context->raw_ptr(), frame.raw_ptr());
		if (code == AVERROR(EAGAIN) || code == AVERROR_EOF) {
			break;
		}
		else if (code < 0) {
			SPDLOG_WARN("avcodec_receive_frame error, code: {}, msg: {}", code, ffmpeg_error_str(code));
			break;
		}

		return frame;
	}

	return FFmpegFrame(nullptr);
}


