// self
#include "ffmpeg_encode.hpp"

// project
#include "ffmpeg_utils.hpp"
#include "ffmpeg_types.hpp"

// ffmpeg
extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
}

// spdlog
#include <spdlog/spdlog.h>



FFmpegEncode::FFmpegEncode(std::string codec_name, int width, int height, int64_t bitrate, int pixel_format)
	: m_codec_name(codec_name)
	, m_width(width)
	, m_height(height)
	, m_bitrate(bitrate)
	, m_pixel_format(pixel_format)
	, m_codec_context(nullptr)
{
}


FFmpegEncode::~FFmpegEncode()
{
	teardown();
}


bool FFmpegEncode::setup() {
	do {
		m_codec_context = (FFmpegCodecContext *)new FFmpegEncoderContext(m_codec_name, m_pixel_format);
		if (m_codec_context->is_null()) {
			break;
		}

		m_codec_context->raw_ptr()->pix_fmt = (enum AVPixelFormat)m_pixel_format;
		m_codec_context->raw_ptr()->width = m_width;
		m_codec_context->raw_ptr()->height = m_height;
		m_codec_context->raw_ptr()->time_base = { 1, 25 };
		m_codec_context->raw_ptr()->framerate = { 25, 1 };
		m_codec_context->raw_ptr()->bit_rate = m_bitrate;
		m_codec_context->raw_ptr()->bit_rate_tolerance = (int)(m_bitrate / 4);
		m_codec_context->raw_ptr()->gop_size = 50;
		m_codec_context->raw_ptr()->max_b_frames = 0;

		int code = 0;
		if (m_codec_name == "libx264" || m_codec_name == "libx265") {
			code = av_opt_set(m_codec_context->raw_ptr()->priv_data, "preset", "ultrafast", 0);
			if (code < 0) {
				SPDLOG_ERROR("av_opt_set(preset, ultrafast) error, code: {}, msg: {}, m_encoder_name: {}", code, ffmpeg_error_str(code), m_codec_name);
			}
			code = av_opt_set(m_codec_context->raw_ptr()->priv_data, "tune", "zerolatency", 0);
			if (code < 0) {
				SPDLOG_ERROR("av_opt_set(tune, zerolatency) error, code: {}, msg: {}, m_encoder_name: {}", code, ffmpeg_error_str(code), m_codec_name);
			}

			if (m_codec_name == "libx265") {
				av_opt_set(m_codec_context->raw_ptr()->priv_data, "x265-params", "threads=1", 0);
			}
		}

		if (!m_codec_context->open({ {"threads", "1"} })) {
			break;
		}

		return true;
	} while (false);

	teardown();

	return false;
}


void FFmpegEncode::teardown()
{
	if (m_codec_context != nullptr) {
		delete m_codec_context;
	}
	m_codec_context = nullptr;

	m_codec_name.clear();
	m_pixel_format = AV_PIX_FMT_NONE;
}


bool FFmpegEncode::send_frame(FFmpegFrame &frame) {
	int code = avcodec_send_frame(m_codec_context->raw_ptr(), frame.raw_ptr());
	if (code < 0) {
		SPDLOG_WARN("avcodec_send_frame error, code: {}, msg: {}", code, ffmpeg_error_str(code));
		return false;
	}

	return true;
}


FFmpegPacket FFmpegEncode::receive_packet() {
	int code = 0;
	while (code >= 0) {
		FFmpegPacket packet;
		if (packet.is_null()) {
			code = AVERROR(ENOMEM);
			return packet;
		}

		code = avcodec_receive_packet(m_codec_context->raw_ptr(), packet.raw_ptr());
		if (code == AVERROR(EAGAIN)) {
			packet.need_more();
		}
		else if (code == AVERROR_EOF) {
		}
		else if (code < 0) {
			SPDLOG_WARN("avcodec_receive_packet error, code: {}, msg: {}", code, ffmpeg_error_str(code));
		}

		return packet;
	}

	return FFmpegPacket(nullptr);
}



