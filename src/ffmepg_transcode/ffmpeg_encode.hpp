#pragma once

// c
#include <stdint.h>

// c++
#include <string>

// project
#include "ffmpeg_types.hpp"

// ffmpeg
struct AVCodec;
struct AVCodecContext;



class FFmpegEncode {
public:
	FFmpegEncode(std::string encoder_name, int width, int height, int64_t bitrate, int pixel_format);

	~FFmpegEncode();

	bool setup();

	bool send_frame(FFmpegFrame &frame);

	FFmpegPacket receive_packet();


private:
	std::string m_encoder_name;

	int m_width;
	int m_height;
	int64_t m_bitrate;

	int m_pixel_format;

	AVCodec *m_encoder;
	AVCodecContext *m_encoder_context;
};

