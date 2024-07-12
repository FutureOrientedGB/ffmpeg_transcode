#pragma once

// c++
#include <string>

// project
#include "ffmpeg_types.hpp"

// ffmpeg
struct AVCodec;
struct AVCodecContext;



class FFmpegDecode {
public:
	FFmpegDecode(std::string decoder_name);
	~FFmpegDecode();

	bool setup();

	bool send_packet(FFmpegPacket &packet);
	FFmpegFrame receive_frame();


private:
	std::string m_decoder_name;

	AVCodec *m_decoder;
	AVCodecContext *m_decoder_context;
};

