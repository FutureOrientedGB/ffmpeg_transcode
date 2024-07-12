// self
#include "ffmpeg_scale.hpp"

// project
#include "ffmpeg_utils.hpp"
#include "ffmpeg_types.hpp"

// ffmpeg
extern "C" {
#include <libswscale/swscale.h>
}

// spdlog
#include <spdlog/spdlog.h>



FFmpegScale::FFmpegScale(int src_width, int src_height, int src_pixel_format, int dst_width, int dst_height, int dst_pixel_format)
	: m_src_width(src_width)
	, m_src_height(src_height)
	, m_src_pixel_format(src_pixel_format)
	, m_dst_width(dst_width)
	, m_dst_height(dst_height)
	, m_dst_pixel_format(dst_pixel_format)
	, m_SwsContext(nullptr)
{
}


FFmpegScale::~FFmpegScale()
{
	if (m_SwsContext != nullptr) {
		sws_freeContext(m_SwsContext);
		m_SwsContext = nullptr;
	}
}


bool FFmpegScale::setup() {
	m_SwsContext = sws_getContext(
		m_src_width, m_src_height, (enum AVPixelFormat)m_src_pixel_format,
		m_dst_width, m_dst_height, (enum AVPixelFormat)m_dst_pixel_format,
		SWS_FAST_BILINEAR, NULL, NULL, NULL
	);
	if (!m_SwsContext) {
		SPDLOG_ERROR("sws_getContext error");
		return false;
	}

	return true;
}


FFmpegFrame FFmpegScale::scale(FFmpegFrame &frame) {
	do {
		FFmpegFrame scaled_frame;
		if (scaled_frame.is_null()) {
			break;
		}

		scaled_frame.raw_ptr()->format = (enum AVPixelFormat)m_dst_pixel_format;
		scaled_frame.raw_ptr()->width = m_dst_width;
		scaled_frame.raw_ptr()->height = m_dst_height;

		int code = av_frame_get_buffer(scaled_frame.raw_ptr(), 1);
		if (code < 0) {
			SPDLOG_ERROR("av_frame_get_buffer error, code: {}, msg: {}", code, ffmpeg_error_str(code));
			break;
		}

		code = sws_scale(m_SwsContext, frame.raw_ptr()->data, frame.raw_ptr()->linesize, 0, m_src_height, scaled_frame.raw_ptr()->data, scaled_frame.raw_ptr()->linesize);
		if (code < 0) {
			SPDLOG_ERROR("sws_scale error, code: {}, msg: {}", code, ffmpeg_error_str(code));
			break;
		}

		return scaled_frame;

	} while (false);

	return FFmpegFrame(nullptr);
}

