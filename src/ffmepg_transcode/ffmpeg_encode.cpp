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
	, m_encoder(nullptr)
	, m_encoder_context(nullptr)
{

}


FFmpegEncode::~FFmpegEncode()
{
	if (m_encoder_context != nullptr) {
		avcodec_close(m_encoder_context);
		avcodec_free_context(&m_encoder_context);
		m_encoder_context = nullptr;
	}
	m_encoder = nullptr;
	m_pixel_format = AV_PIX_FMT_NONE;
	m_encoder_name.clear();
}


bool FFmpegEncode::setup() {
	do {
		m_encoder = (AVCodec *)avcodec_find_encoder_by_name(m_encoder_name.c_str());
		if (nullptr == m_encoder) {
			SPDLOG_ERROR("avcodec_find_encoder_by_name error, m_encoder_name: {}", m_encoder_name);
			return false;
		}

		m_encoder_context = avcodec_alloc_context3(m_encoder);
		if (!m_encoder_context) {
			SPDLOG_ERROR("avcodec_alloc_context3 error, m_encoder_name: {}", m_encoder_name);
			return false;
		}

		m_encoder_context->pix_fmt = (enum AVPixelFormat)m_pixel_format;
		m_encoder_context->width = m_width;
		m_encoder_context->height = m_height;
		m_encoder_context->time_base = { 1, 25 };
		m_encoder_context->framerate = { 1, 25 };
		m_encoder_context->bit_rate = m_bitrate;
		m_encoder_context->bit_rate_tolerance = (int)(m_bitrate / 4);
		m_encoder_context->gop_size = 50;
		m_encoder_context->max_b_frames = 0;

		int code = 0;
		code = av_opt_set(m_encoder_context->priv_data, "preset", "ultrafast", 0);
		if (code < 0) {
			SPDLOG_ERROR("av_opt_set(preset, ultrafast) error, code: {}, msg: {}, m_encoder_name: {}", code, ffmpeg_error_str(code), m_encoder_name);
		}
		code = av_opt_set(m_encoder_context->priv_data, "tune", "zerolatency", 0);
		if (code < 0) {
			SPDLOG_ERROR("av_opt_set(tune, zerolatency) error, code: {}, msg: {}, m_encoder_name: {}", code, ffmpeg_error_str(code), m_encoder_name);
		}

		AVDictionary *options = nullptr;
		av_dict_set(&options, "threads", "1", 0);

		if (m_encoder_name == "libx265") {
			av_opt_set(m_encoder_context->priv_data, "x265-params", "threads=1", 0);
		}

		code = avcodec_open2(m_encoder_context, m_encoder, &options);
		if (code < 0) {
			SPDLOG_ERROR("avcodec_open2 error, code: {}, msg: {}, m_encoder_name: {}", code, ffmpeg_error_str(code), m_encoder_name);
			return false;
		}
	} while (false);

	return true;
}


bool FFmpegEncode::send_frame(FFmpegFrame &frame) {
	int code = avcodec_send_frame(m_encoder_context, frame.raw_ptr());
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

		code = avcodec_receive_packet(m_encoder_context, packet.raw_ptr());
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



