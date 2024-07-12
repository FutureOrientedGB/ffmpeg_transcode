#pragma once

// c++
#include <map>
#include <vector>
#include <string>

// project
#include "ffmpeg_types.hpp"



enum class TranscodeType : uint8_t {
	Invalid,
	DecodeH264Only,
	DecodeH265Only,
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
	static TranscodeType from_str(std::string s);
	static std::string to_str(TranscodeType e);
	static std::string help();


private:
	static std::map<std::string, TranscodeType> s_map_str_to_enum;
	static std::map<TranscodeType, std::string> s_map_enum_to_str;
};



std::vector<FFmpegPacket> demux_all_packets(std::string input_url, int limit_packets);



class FFmpegTranscode {
public:
	virtual double run(
		int task_id, std::vector<FFmpegPacket> &frames_queue,
		std::string input_codec, int input_width, int input_height,
		std::vector<std::string> output_codec, std::vector<int> output_width, std::vector<int> output_height, std::vector<int64_t> output_bitrate
	) = 0;

	double multi_threading_test(
		std::vector<FFmpegPacket> &frames_queue, int concurrency_threads,
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

