#pragma once

// c++
#include <string>

// project
#include "ffmpeg_types.hpp"



class FFmpegDecode {
public:
	FFmpegDecode(std::string codec_name, int pixel_format = 0);
	~FFmpegDecode();

	bool setup();
	void teardown();

	bool send_packet(FFmpegPacket &packet);
	FFmpegFrame receive_frame();


private:
	std::string m_codec_name;

	int m_pixel_format;

	FFmpegCodecContext *m_codec_context;
};

