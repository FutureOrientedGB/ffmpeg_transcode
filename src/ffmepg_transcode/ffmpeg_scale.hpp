#pragma once

// project
#include "ffmpeg_types.hpp"

// ffmpeg
struct SwsContext;



class FFmpegScale {
public:
	FFmpegScale(int src_width, int src_height, int src_pixel_format, int dst_width, int dst_height, int dst_pixel_format);
	~FFmpegScale();

	bool setup();

	FFmpegFrame scale(FFmpegFrame &frame);


private:
	int m_src_width;
	int m_src_height;
	int m_src_pixel_format;
	int m_dst_width;
	int m_dst_height;
	int m_dst_pixel_format;

	SwsContext *m_SwsContext;
};

