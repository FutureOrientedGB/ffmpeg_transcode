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



FFmpegEncode::FFmpegEncode(std::string encoder_name, int width, int height, int64_t bitrate, int pixel_format)
	: m_encoder_name(encoder_name)
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
	m_codec_context = new FFmpegCodecContext(m_encoder_name, "");
	if (m_codec_context->is_null()) {
		return false;
	}

	m_codec_context->raw_ptr()->pix_fmt = (enum AVPixelFormat)m_pixel_format;
	m_codec_context->raw_ptr()->width = m_width;
	m_codec_context->raw_ptr()->height = m_height;
	m_codec_context->raw_ptr()->time_base = { 1, 25 };
	m_codec_context->raw_ptr()->framerate = { 1, 25 };
	m_codec_context->raw_ptr()->bit_rate = m_bitrate;
	m_codec_context->raw_ptr()->bit_rate_tolerance = (int)(m_bitrate / 4);
	m_codec_context->raw_ptr()->gop_size = 50;
	m_codec_context->raw_ptr()->max_b_frames = 0;

	int code = 0;
	code = av_opt_set(m_codec_context->raw_ptr()->priv_data, "preset", "ultrafast", 0);
	if (code < 0) {
		SPDLOG_ERROR("av_opt_set(preset, ultrafast) error, code: {}, msg: {}, m_encoder_name: {}", code, ffmpeg_error_str(code), m_encoder_name);
	}
	code = av_opt_set(m_codec_context->raw_ptr()->priv_data, "tune", "zerolatency", 0);
	if (code < 0) {
		SPDLOG_ERROR("av_opt_set(tune, zerolatency) error, code: {}, msg: {}, m_encoder_name: {}", code, ffmpeg_error_str(code), m_encoder_name);
	}

	if (m_encoder_name == "libx265") {
		av_opt_set(m_codec_context->raw_ptr()->priv_data, "x265-params", "threads=1", 0);
	}

	if (!m_codec_context->open({ {"threads", "1"} })) {
		return false;
	}

	return true;
}


void FFmpegEncode::teardown()
{
	if (m_codec_context != nullptr) {
		delete m_codec_context;
		m_codec_context = nullptr;
	}

	m_pixel_format = AV_PIX_FMT_NONE;
	m_encoder_name.clear();
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
			break;
		}

		code = avcodec_receive_packet(m_codec_context->raw_ptr(), packet.raw_ptr());
		if (code == AVERROR(EAGAIN) || code == AVERROR_EOF) {
			break;
		}
		else if (code < 0) {
			SPDLOG_WARN("avcodec_receive_packet error, code: {}, msg: {}", code, ffmpeg_error_str(code));
			break;
		}

		return packet;
	}

	return FFmpegPacket(nullptr);
}



