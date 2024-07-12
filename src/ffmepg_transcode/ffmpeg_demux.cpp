// self
#include "ffmpeg_demux.hpp"

// project
#include "ffmpeg_utils.hpp"
#include "ffmpeg_types.hpp"

// ffmpeg
extern "C" {
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
}

// spdlog
#include <spdlog/spdlog.h>



FFmpegDemux::FFmpegDemux(std::string input_url)
	: m_input_url(input_url)
	, m_video_stream_index(-1)
	, m_video_stream(nullptr)
	, m_format_context(nullptr)
{
}


FFmpegDemux::~FFmpegDemux()
{
	if (m_format_context != nullptr) {
		avformat_close_input(&m_format_context);
		m_format_context = nullptr;
	}
	m_video_stream_index = -1;
	m_video_stream = nullptr;
	m_input_url.clear();
}


bool FFmpegDemux::setup() {
	int code = 0;
	do {
		AVDictionary *options = 0;
		av_dict_set(&options, "rtsp_transport", "tcp", 0);

		code = avformat_open_input(&m_format_context, m_input_url.c_str(), NULL, &options);
		if (code < 0) {
			SPDLOG_ERROR("avformat_open_input error, code: {}, msg: {}, m_input_url: {}", code, ffmpeg_error_str(code), m_input_url);
			return false;
		}

		code = avformat_find_stream_info(m_format_context, NULL);
		if (code < 0) {
			SPDLOG_ERROR("avformat_find_stream_info error, code: {}, msg: {}, m_input_url: {}", code, ffmpeg_error_str(code), m_input_url);
			return false;
		}

		code = av_find_best_stream(m_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		if (code < 0) {
			SPDLOG_ERROR("av_find_best_stream error, code: {}, msg: {}, m_input_url: {}", code, ffmpeg_error_str(code), m_input_url);
			return false;
		}

		m_video_stream_index = code;
		m_video_stream = m_format_context->streams[m_video_stream_index];

	} while (false);

	return true;
}


FFmpegPacket FFmpegDemux::read_frame() {
	FFmpegPacket packet;
	if (packet.is_null()) {
		return packet;
	}

	int code = 0;
	while ((code = av_read_frame(m_format_context, packet.raw_ptr())) >= 0) {
		if (packet.raw_ptr()->stream_index == m_video_stream_index) {
			break;
		}
	}

	if (code < 0) {
		packet.free();
	}

	return packet;
}
