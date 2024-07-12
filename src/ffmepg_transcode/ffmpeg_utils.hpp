#pragma once

// c++
#include <string>

// ffmpeg
struct AVPacket;
struct AVFrame;



void ffmpeg_log_default(int level);

std::string ffmpeg_error_str(int code);

void ffmpeg_free_packet(AVPacket **frame);

void ffmpeg_free_frame(AVFrame **frame);

