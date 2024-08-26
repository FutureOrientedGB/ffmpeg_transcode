// self
#include "ffmpeg_utils.hpp"

// c++
#include <string>
#include <vector>

// ffmpeg
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/log.h>
#include <libavcodec/avcodec.h>
}



void ffmpeg_log_default(int log_level) {
	av_log_set_level(log_level);
	av_log_set_callback(av_log_default_callback);
}


std::string ffmpeg_error_str(int code) {
	std::vector<char> buf;
	buf.resize(AV_ERROR_MAX_STRING_SIZE);

	av_strerror(code, buf.data(), buf.size());
	return std::string(buf.data());
}


void ffmpeg_free_packet(AVPacket **frame) {
	if (frame != nullptr && *frame != nullptr) {
		av_packet_free(frame);
		*frame = nullptr;
	}
}


void ffmpeg_free_frame(AVFrame **frame) {
	if (frame != nullptr && *frame != nullptr) {
		av_frame_free(frame);
		*frame = nullptr;
	}
}

