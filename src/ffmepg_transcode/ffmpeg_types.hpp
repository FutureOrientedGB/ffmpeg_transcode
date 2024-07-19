#pragma once

// c
#include <stdint.h>

// c++
#include <map>
#include <string>

// ffmpeg
struct AVBufferRef;
struct AVFrame;
struct AVPacket;
struct AVCodec;
struct AVCodecContext;



class FFmpegPacket {
public:
    FFmpegPacket();
    FFmpegPacket(AVPacket *packet);
    FFmpegPacket(const FFmpegPacket &other) = delete;
    FFmpegPacket(FFmpegPacket &&other) noexcept;
    ~FFmpegPacket();

    void free();

    AVPacket *raw_ptr();
    bool is_null();

    void need_more();
    bool does_need_more();


private:
    bool m_need_more;
    AVPacket *m_packet;
};


class FFmpegFrame {
public:
    FFmpegFrame();
    FFmpegFrame(AVFrame *frame);
    FFmpegFrame(const FFmpegFrame &other) = delete;
    FFmpegFrame(FFmpegFrame &&other) noexcept;
    ~FFmpegFrame();

    void free();

    AVFrame *raw_ptr();
    bool is_null();

    void need_more();
    bool does_need_more();


private:
    bool m_need_more;
    AVFrame *m_frame;
};


class FFmpegCodec {
public:
    enum class CodecType : uint8_t {
        Uninitialized,
        Encoder,
        Decoder,
    };

    FFmpegCodec();
    FFmpegCodec(int encoder_id, int decoder_id);
    FFmpegCodec(std::string encoder_name, std::string decoder_name);
    FFmpegCodec(const FFmpegCodec &other) = delete;
    FFmpegCodec(FFmpegCodec &&other) noexcept;
    ~FFmpegCodec();

    void free();

    AVCodec *raw_ptr();
    bool is_null();

    std::string codec_name();


private:
    AVCodec *m_codec;
    CodecType m_type;
    std::string m_codec_name;
};


class FFmpegCodecContext {
public:
    FFmpegCodecContext(int encoder_id, int decoder_id, int pixel_format = 0);
    FFmpegCodecContext(std::string encoder_name, std::string codec_name, int pixel_format = 0);
    FFmpegCodecContext(const FFmpegCodecContext &other) = delete;
    FFmpegCodecContext(FFmpegCodecContext &&other) noexcept;
    ~FFmpegCodecContext();

    bool auto_hw_accel();

    bool open(std::map<std::string, std::string> options);

    void free();

    AVCodecContext *raw_ptr();
    bool is_null();

    int pixel_format();
    int hw_device_type();
    AVBufferRef *hw_device_context();
    std::pair<int, int> pixel_aspect();
    std::pair<int, int> time_base();


private:
    FFmpegCodec *m_codec;
    AVCodecContext *m_codec_context;

    int m_pixel_format;
    int m_hw_device_type;
    AVBufferRef *m_hw_device_context;
};

