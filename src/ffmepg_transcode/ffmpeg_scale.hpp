#pragma once

// project
#include "ffmpeg_types.hpp"

// ffmpeg
struct AVFilter;
struct AVFilterGraph;
struct AVFilterInOut;
struct AVFilterContext;



class FFmpegScale {
public:
	FFmpegScale(int src_width, int src_height, int src_pixel_format, int dst_width, int dst_height, int dst_pixel_format, int pixel_aspect_num = 1, int pixel_aspect_den = 1, int time_base_num = 1, int time_base_den = 1, std::string filter_text = "");
	~FFmpegScale();

	bool setup(FFmpegFrame &frame);
	void teardown();

	FFmpegFrame scale(FFmpegFrame &frame);


private:
	int m_src_width;
	int m_src_height;
	int m_src_pixel_format;
	int m_dst_width;
	int m_dst_height;
	int m_dst_pixel_format;

	int m_pixel_aspect_num;
	int m_pixel_aspect_den;
	int m_time_base_num;
	int m_time_base_den;
	std::string m_filter_text;

	AVFilterGraph *m_filter_graph;
	AVFilterContext *m_buffer_src_filter_context;
	AVFilterContext *m_buffer_sink_filter_context;
};

