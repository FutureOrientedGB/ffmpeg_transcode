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

	int pixel_format();
	int hw_device_type();
	AVBufferRef *hw_device_context();
	std::pair<int, int> pixel_aspect();
	std::pair<int, int> time_base();


private:
	std::string m_codec_name;

	int m_pixel_format;

	FFmpegCodecContext *m_codec_context;
};

