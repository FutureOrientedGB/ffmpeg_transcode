// c++
#include <future>
#include <thread>
#include <vector>

// project
#include "ffmpeg_utils.hpp"
#include "ffmpeg_types.hpp"
#include "ffmpeg_demux.hpp"
#include "ffmpeg_decode.hpp"
#include "ffmpeg_scale.hpp"
#include "ffmpeg_encode.hpp"
#include "math_utils.hpp"

// ffmpeg
extern "C" {
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

// fmt
#include <fmt/core.h>

// cli11
#include <CLI/CLI.hpp>



std::vector<FFmpegPacket> demux_all_packets(std::string input_url, int limit_count = INT_MAX)
{
	std::vector<FFmpegPacket> frames_queue;

	FFmpegDemux demux(input_url);
	if (!demux.setup()) {
		return frames_queue;
	}

	for (int i = 0; i < limit_count; i++) {
		// demux
		FFmpegPacket es_packet = demux.read_frame();
		if (es_packet.is_null()) {
			break;
		}

		frames_queue.push_back(es_packet);
	}

	return frames_queue;
}


double transcode(int task_id, std::vector<FFmpegPacket> &frames_queue, std::string input_codec, int input_width, int input_height, std::string output_codec, int output_width, int output_height, int64_t output_bitrate) {
	SPDLOG_INFO(
		"#{:2d}# transcode frames={} input_codec={} input_width={} input_height={} output_codec={} output_width={} output_height={} output_bitrate={}",
		task_id, frames_queue.size(), input_codec, input_width, input_height, output_codec, output_width, output_height, output_bitrate
	);

	FFmpegDecode decoder(input_codec);
	if (!decoder.setup()) {
		return -1;
	}

	FFmpegScale scaler(input_width, input_height, (int)AV_PIX_FMT_YUV420P, output_width, output_height, (int)AV_PIX_FMT_YUV420P);
	if (!scaler.setup()) {
		return -2;
	}

	FFmpegEncode encoder(output_codec, output_width, output_height, output_bitrate, (int)AV_PIX_FMT_YUV420P);
	if (!encoder.setup()) {
		return -3;
	}

	// statics
	TimeIt ti_task;
	TimeIt ti_step;
	size_t frames = frames_queue.size();
	MovingAverage ma_gop_decode(50);
	MovingAverage ma_all_decode(300);
	MovingAverage ma_gop_scale(50);
	MovingAverage ma_all_scale(300);
	MovingAverage ma_gop_encode(50);
	MovingAverage ma_all_encode(300);

	int index = 1;
	for (auto i = 0; i < frames_queue.size(); i++) {
		ti_step.reset();

		// decode
		if (!decoder.send_packet(frames_queue[i])) {
			break;
		}

		FFmpegFrame yuv_frame = decoder.receive_frame();
		if (yuv_frame.is_null()) {
			break;
		}

		double decode_elasped_ms = ti_step.elapsed_milliseconds();
		ma_gop_decode.add(decode_elasped_ms);
		ma_all_decode.add(ma_gop_decode.calc());
		ti_step.reset();

		// scale
		FFmpegFrame scaled_yuv_frame = scaler.scale(yuv_frame);
		if (scaled_yuv_frame.is_null()) {
			break;
		}

		// free decoed frame
		yuv_frame.free();

		double scale_elasped_ms = ti_step.elapsed_milliseconds();
		ma_gop_scale.add(scale_elasped_ms);
		ma_all_scale.add(ma_gop_scale.calc());
		ti_step.reset();

		// encode
		if (!encoder.send_frame(scaled_yuv_frame)) {
			break;
		}

		// free scaled frame
		scaled_yuv_frame.free();

		FFmpegPacket encoded_es_packet = encoder.receive_packet();
		if (encoded_es_packet.is_null()) {
			break;
		}

		// free encoed frame
		encoded_es_packet.free();

		double encode_elasped_ms = ti_step.elapsed_milliseconds();
		ma_gop_encode.add(encode_elasped_ms);
		ma_all_encode.add(ma_gop_encode.calc());
		ti_step.reset();

		if (0 == (i + 1) % 500) {
			double progress = 100.0 * i / frames;
			SPDLOG_INFO("#{:2d}# progress: {:.2f}%, ma_all_decode: {:.2f} ms/frame, ma_all_scale: {:.2f} ms/frame, ma_all_encode: {:.2f} ms/frame", task_id, progress, ma_all_decode.calc(), ma_all_scale.calc(), ma_all_encode.calc());
		}
		else if (0 == (i + 1) % 250) {
			double progress = 100.0 * i / frames;
			SPDLOG_INFO("#{:2d}# progress: {:.2f}%, ma_gop_decode: {:.2f} ms/frame, ma_gop_scale: {:.2f} ms/frame, ma_gop_encode: {:.2f} ms/frame", task_id, progress, ma_gop_decode.calc(), ma_gop_scale.calc(), ma_gop_encode.calc());
		}
	}

	double expect_ms = frames * 40.0;
	double task_elasped_ms = ti_task.elapsed_milliseconds();
	double speed = expect_ms / task_elasped_ms;


	SPDLOG_INFO("#{:2d}# progress: 100.00%, ma_all_decode: {:.2f} ms/frame, ma_all_scale: {:.2f} ms/frame, ma_all_encode: {:.2f} ms/frame, speed: {:.2f}x", task_id, ma_all_decode.calc(), ma_all_scale.calc(), ma_all_encode.calc(), speed);

	return speed;
}


double transcode_multiprocessing(std::vector<FFmpegPacket> &frames_queue, int concurrency_threads, std::string input_codec, int input_width, int input_height, std::string output_codec, int output_width, int output_height, int64_t output_bitrate)
{
	SPDLOG_INFO("========== {} ways {} decode + {} scale + {} encode begin ==========", concurrency_threads, input_codec, output_height, output_codec);

	double total_speed = 0.0;
	if (concurrency_threads <= 1) {
		total_speed = transcode(0, frames_queue, input_codec, input_width, input_height, output_codec, output_width, output_height, output_bitrate);
	}
	else {
		std::vector<std::thread> threads;
		std::vector<std::shared_ptr<std::promise<double>>> promises;
		std::vector<std::future<double>> futures;

		for (int i = 0; i < concurrency_threads; ++i) {
			std::shared_ptr<std::promise<double>> speed_promise = std::make_shared<std::promise<double>>();
			std::future<double> speed_future = speed_promise->get_future();

			int task_id = i;
			threads.emplace_back(
				[speed_promise, task_id, &frames_queue, input_codec, input_width, input_height, output_codec, output_width, output_height, output_bitrate]() {
					double result = transcode(task_id, frames_queue, input_codec, input_width, input_height, output_codec, output_width, output_height, output_bitrate);
					speed_promise->set_value(result);
				}
			);

			promises.push_back(speed_promise);
			futures.push_back(std::move(speed_future));
		}

		for (auto &thread : threads) {
			thread.join();
		}

		for (int i = 0; i < concurrency_threads; ++i) {
			total_speed += futures[i].get();
		}
	}

	SPDLOG_INFO("========== {} ways {} decode + {} scale + {} encode end with {:.2f}x speed)==========", concurrency_threads, input_codec, output_height, output_codec, total_speed);

	return total_speed;
}


#if defined(WIN32)
std::string log_path("transcode.log");
std::string h264_path("D:/__develop__/1080p.25fps.4M.264");
std::string h265_path("D:/__develop__/1080p.25fps.3M.265");
#else
std::string log_path("/media/transcode.log");
std::string h264_path("/media/1080p.25fps.4M.264");
std::string h265_path("/media/1080p.25fps.3M.265");
#endif


int main(int argc, char **argv) {
	auto file_logger = spdlog::basic_logger_mt("transcode", log_path);
	spdlog::set_default_logger(file_logger);
	spdlog::flush_on(spdlog::level::info);

	CLI::App app{ "test ffmpeg transcode" };
	argv = app.ensure_utf8(argv);

	int concurrency_threads;
	app.add_option("-c,--concurrency", concurrency_threads, "concurrency threads (default 1)");

	int limit_frames;
	app.add_option("-f,--frames", limit_frames, "limit frames (default INT_MAX)");

	std::string task_name = "transcode_h264_to_d1_h264";
	app.add_option("-t,--task", task_name, "transcode task name (default transcode_h264_to_d1_h264)");

	int ffmpeg_log_level = AV_LOG_INFO;
	app.add_option("-l,--level", ffmpeg_log_level, "ffmpeg log level (default 32)");

	CLI11_PARSE(app, argc, argv);

	ffmpeg_log_default(ffmpeg_log_level);

	SPDLOG_INFO("========== {} threads {} frames {} begin ==========", concurrency_threads, limit_frames, task_name);
	TimeIt it;
	std::vector<FFmpegPacket> frames_queue_264;
	std::vector<FFmpegPacket> frames_queue_265;
	if (task_name == "transcode_h264_to_d1_h264") {
		frames_queue_264 = demux_all_packets(h264_path, limit_frames);
		transcode_multiprocessing(frames_queue_264, concurrency_threads, "h264", 1920, 1080, "libx264", 720, 480, 1000 * 1000);
	}
	else if (task_name == "transcode_h264_to_cif_h264") {
		frames_queue_264 = demux_all_packets(h264_path, limit_frames);
		transcode_multiprocessing(frames_queue_264, concurrency_threads, "h264", 1920, 1080, "libx264", 352, 288, 128 * 1000);
	}
	else if (task_name == "transcode_h265_to_d1_h265") {
		frames_queue_265 = demux_all_packets(h265_path, limit_frames);
		transcode_multiprocessing(frames_queue_265, concurrency_threads, "hevc", 1920, 1080, "libx265", 720, 480, 1000 * 1000);
	}
	else if (task_name == "transcode_h265_to_cif_h265") {
		frames_queue_265 = demux_all_packets(h265_path, limit_frames);
		transcode_multiprocessing(frames_queue_265, concurrency_threads, "hevc", 1920, 1080, "libx265", 352, 288, 128 * 1000);
	}
	else if (task_name == "transcode_h265_to_d1_h264") {
		frames_queue_265 = demux_all_packets(h265_path, limit_frames);
		transcode_multiprocessing(frames_queue_265, concurrency_threads, "hevc", 1920, 1080, "libx264", 720, 480, 1000 * 1000);
	}
	else if (task_name == "transcode_h265_to_cif_h264") {
		transcode_multiprocessing(frames_queue_265, concurrency_threads, "hevc", 1920, 1080, "libx264", 352, 288, 128 * 1000);
	}
	else if (task_name == "transcode_all") {
		frames_queue_264 = demux_all_packets(h264_path, limit_frames);
		transcode_multiprocessing(frames_queue_264, concurrency_threads, "h264", 1920, 1080, "libx264", 720, 480, 1000 * 1000);
		SPDLOG_INFO("");
		transcode_multiprocessing(frames_queue_264, concurrency_threads, "h264", 1920, 1080, "libx264", 352, 288, 128 * 1000);
		SPDLOG_INFO("");
		frames_queue_264.clear();

		frames_queue_265 = demux_all_packets(h265_path, limit_frames);
		transcode_multiprocessing(frames_queue_265, concurrency_threads, "hevc", 1920, 1080, "libx265", 720, 480, 1000 * 1000);
		SPDLOG_INFO("");
		transcode_multiprocessing(frames_queue_265, concurrency_threads, "hevc", 1920, 1080, "libx265", 352, 288, 128 * 1000);
		SPDLOG_INFO("");
		transcode_multiprocessing(frames_queue_265, concurrency_threads, "hevc", 1920, 1080, "libx264", 720, 480, 1000 * 1000);
		SPDLOG_INFO("");
		transcode_multiprocessing(frames_queue_265, concurrency_threads, "hevc", 1920, 1080, "libx264", 352, 288, 128 * 1000);
	}
	SPDLOG_INFO("========== {} threads {} frames {} end in {:.2f}s ==========", concurrency_threads, limit_frames, task_name, it.elapsed_seconds());

	file_logger->flush();

	return 0;
}
