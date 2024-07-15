#pragma once

// c++
#include <map>
#include <vector>
#include <string>

// project
#include "ffmpeg_types.hpp"



enum class TranscodeType : uint8_t {
	Invalid,
	H264DecodeOnly,
	H265DecodeOnly,
	H264ToD1H264,
	H264ToCifH264,
	H265ToD1H265,
	H265ToCifH265,
	H265ToD1H264,
	H265ToCifH264,
	H264ToD1CifH264,
	H265ToD1CifH265,
	H265ToD1CifH264,
	AllTasks,
};


class TranscodeTypeCvt {
public:
	static TranscodeType from_string(std::string s);
	static std::string to_string(TranscodeType e);
	static std::string support_list();


private:
	static std::map<std::string, TranscodeType> s_map_string_to_enum;
	static std::map<TranscodeType, std::string> s_map_enum_to_string;
};



class FFmpegTranscode {
public:
	virtual double run(
		int task_id, std::vector<FFmpegPacket> &frames_queue,
		std::string input_codec, int input_width, int input_height,
		std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
	) = 0;

	double multi_threading_test(
		int threads, std::vector<FFmpegPacket> &frames_queue,
		std::string input_codec, int input_width, int input_height,
		std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
	);
};


class FFmpegDecodeOnly : public FFmpegTranscode {
public:
	// decode 1 input only
	double run(
		int task_id, std::vector<FFmpegPacket> &frames_queue,
		std::string input_codec, int input_width, int input_height,
		std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
	) override;
};


class FFmpegTranscodeOne : public FFmpegTranscode {
public:
	// 1 input 1 outputs
	double run(
		int task_id, std::vector<FFmpegPacket> &frames_queue,
		std::string input_codec, int input_width, int input_height,
		std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
	) override;
};


class FFmpegTranscodeTwo : public FFmpegTranscode {
public:
	// 1 input 2 outputs
	double run(
		int task_id, std::vector<FFmpegPacket> &frames_queue,
		std::string input_codec, int input_width, int input_height,
		std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
	) override;
};



class FFmpegTranscodeFactory {
public:
	FFmpegTranscodeFactory();
	~FFmpegTranscodeFactory();

	// create transcoder and fill parameters
	FFmpegTranscode *create(TranscodeType t, std::string &intput_codec, std::vector<std::string> &output_codec, std::vector<int> &output_width, std::vector<int> &output_height, std::vector<int64_t> &output_bitrate);

private:
	FFmpegTranscode *m_transcode;
};

