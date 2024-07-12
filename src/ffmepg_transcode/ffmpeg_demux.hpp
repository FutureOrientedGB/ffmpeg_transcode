#pragma once

// c++
#include <string>

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

	FFmpegPacket read_frame();


private:
	std::string m_input_url;

	int m_video_stream_index;
	AVStream *m_video_stream;

	AVFormatContext *m_format_context;
};

