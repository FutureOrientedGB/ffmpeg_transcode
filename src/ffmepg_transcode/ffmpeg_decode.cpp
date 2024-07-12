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
	, m_decoder(nullptr)
	, m_decoder_context(nullptr)
{
}


FFmpegDecode::~FFmpegDecode()
{
	if (m_decoder_context != nullptr) {
		avcodec_close(m_decoder_context);
		avcodec_free_context(&m_decoder_context);
		m_decoder_context = nullptr;
	}
	m_decoder = nullptr;
	m_decoder_name.clear();
}


bool FFmpegDecode::setup() {
	do {
		m_decoder = (AVCodec *)avcodec_find_decoder_by_name(m_decoder_name.c_str());
		if (nullptr == m_decoder) {
			SPDLOG_ERROR("avcodec_find_decoder_by_name error, m_decoder_name: {}", m_decoder_name);
			return false;
		}

		m_decoder_context = avcodec_alloc_context3(m_decoder);
		if (!m_decoder_context) {
			SPDLOG_ERROR("avcodec_alloc_context3 error, m_decoder_name: {}", m_decoder_name);
			return false;
		}
		
		AVDictionary *options = nullptr;
		av_dict_set(&options, "threads", "1", 0);

		int code = avcodec_open2(m_decoder_context, m_decoder, &options);
		if (code < 0) {
			SPDLOG_ERROR("avcodec_open2 error, code: {}, msg: {}, m_decoder_name: {}", code, ffmpeg_error_str(code), m_decoder_name);
			return false;
		}
	} while (false);

	return true;
}


bool FFmpegDecode::send_packet(FFmpegPacket &packet) {
	int code = avcodec_send_packet(m_decoder_context, packet.raw_ptr());
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

		code = avcodec_receive_frame(m_decoder_context, frame.raw_ptr());
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


