#pragma once

// c
#include <limits.h>

// c++
#include <string>
#include <vector>

// project
#include "ffmpeg_types.hpp"

// ffmpeg
struct AVStream;
struct AVFormatContext;;



class FFmpegDemux {
public:
	FFmpegDemux(std::string input_url);
	~FFmpegDemux();

	bool setup();
	void teardown();

	FFmpegPacket read_frame();
	std::vector<FFmpegPacket> read_some_frames(int limit_packets = INT_MAX);

	int codec_id();
	std::string codec_name();
	int width();
	int height();
	double fps();


private:
	std::string m_input_url;

	int m_video_stream_index;
	AVStream *m_video_stream;

	AVFormatContext *m_format_context;

	int m_codec_id;
	int m_width;
	int m_height;
	double m_fps;
};

