#pragma once

// c++
#include <string>

// project
#include "ffmpeg_types.hpp"



class FFmpegDecode {
public:
	FFmpegDecode(std::string decoder_name);
	~FFmpegDecode();

	bool setup();
	void teardown();

	bool send_packet(FFmpegPacket &packet);
	FFmpegFrame receive_frame();


private:
	std::string m_decoder_name;

	FFmpegCodecContext *m_codec_context;
};

