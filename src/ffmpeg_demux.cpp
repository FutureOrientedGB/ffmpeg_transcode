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
	, m_codec_id(-1)
	, m_width(-1)
	, m_height(-1)
	, m_fps(-1.0)
{
}


FFmpegDemux::~FFmpegDemux()
{
	teardown();
}


bool FFmpegDemux::setup() {
	int code = 0;

	do {
		AVDictionary *options = 0;
		av_dict_set(&options, "rtsp_transport", "tcp", 0);

		code = avformat_open_input(&m_format_context, m_input_url.c_str(), NULL, &options);
		if (code < 0) {
			SPDLOG_ERROR("avformat_open_input error, code: {}, msg: {}, m_input_url: {}", code, ffmpeg_error_str(code), m_input_url);
			break;
		}

		code = avformat_find_stream_info(m_format_context, NULL);
		if (code < 0) {
			SPDLOG_ERROR("avformat_find_stream_info error, code: {}, msg: {}, m_input_url: {}", code, ffmpeg_error_str(code), m_input_url);
			break;
		}

		code = av_find_best_stream(m_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		if (code < 0) {
			SPDLOG_ERROR("av_find_best_stream error, code: {}, msg: {}, m_input_url: {}", code, ffmpeg_error_str(code), m_input_url);
			break;
		}

		m_video_stream_index = code;
		m_video_stream = m_format_context->streams[m_video_stream_index];

		AVCodecParameters *codec_params = m_video_stream->codecpar;
		m_codec_id = (int)codec_params->codec_id;
		m_width = codec_params->width;
		m_height = codec_params->height;

		if (m_video_stream->avg_frame_rate.num > 0 && m_video_stream->avg_frame_rate.den > 0) {
			// fps
			m_fps = av_q2d(m_video_stream->avg_frame_rate);
		}
		else if (m_video_stream->r_frame_rate.num > 0 && m_video_stream->r_frame_rate.den > 0) {
			// tbr
			m_fps = av_q2d(m_video_stream->r_frame_rate);
		}
		else if (m_video_stream->time_base.num > 0 && m_video_stream->time_base.den > 0) {
			// tbn
			m_fps = 1.0 / av_q2d(m_video_stream->time_base);
		}
		else {
			// default
			m_fps = 25.0;
			SPDLOG_WARN("can't find fps from frame_rate and timebase, set default to 25.0");
		}

		return true;
	} while (false);

	teardown();

	return false;
}


void FFmpegDemux::teardown()
{
	if (m_format_context != nullptr) {
		avformat_close_input(&m_format_context);
	}
	m_format_context = nullptr;

	m_video_stream_index = -1;
	m_video_stream = nullptr;
	m_input_url.clear();
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


std::vector<FFmpegPacket> FFmpegDemux::read_some_frames(int limit_packets)
{
	std::vector<FFmpegPacket> vec;

	for (int i = 0; i < limit_packets; i++) {
		FFmpegPacket packet = read_frame();
		if (packet.is_null()) {
			break;
		}

		vec.push_back(std::move(packet));
	}

	return vec;
}


int FFmpegDemux::codec_id()
{
	return m_codec_id;
}


std::string FFmpegDemux::codec_name()
{
	return avcodec_get_name((AVCodecID)m_codec_id);
}


int FFmpegDemux::width()
{
	return m_width;
}


int FFmpegDemux::height()
{
	return m_height;
}


double FFmpegDemux::fps()
{
	return m_fps;
}

