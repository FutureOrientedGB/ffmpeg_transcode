#pragma once

// c
#include <stdint.h>

// c++
#include <map>
#include <string>

// ffmpeg
struct AVFrame;
struct AVPacket;
struct AVCodec;
struct AVCodecContext;



class FFmpegPacket {
public:
    FFmpegPacket();
    FFmpegPacket(AVPacket *packet);
    FFmpegPacket(const FFmpegPacket &other);
    FFmpegPacket(FFmpegPacket &&other) noexcept;
    ~FFmpegPacket();

    void free();

    AVPacket *raw_ptr();
    bool is_null();


private:
    AVPacket *m_packet;
};


class FFmpegFrame {
public:
    FFmpegFrame();
    FFmpegFrame(AVFrame *frame);
    FFmpegFrame(const FFmpegFrame &other);
    FFmpegFrame(FFmpegFrame &&other) noexcept;
    ~FFmpegFrame();

    void free();

    AVFrame *raw_ptr();
    bool is_null();


private:
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
    FFmpegCodec(const FFmpegCodec &other);
    FFmpegCodec(FFmpegCodec &&other) noexcept;
    ~FFmpegCodec();

    void free();

    AVCodec *raw_ptr();
    bool is_null();


private:
    AVCodec *m_codec;
    CodecType m_type;
};


class FFmpegCodecContext {
public:
    FFmpegCodecContext(int encoder_id, int decoder_id);
    FFmpegCodecContext(std::string encoder_name, std::string decoder_name);
    FFmpegCodecContext(const FFmpegCodecContext &other);
    FFmpegCodecContext(FFmpegCodecContext &&other) noexcept;
    ~FFmpegCodecContext();

    bool open(std::map<std::string, std::string> options);

    void free();

    AVCodecContext *raw_ptr();
    bool is_null();


private:
    FFmpegCodec *m_codec;
    AVCodecContext *m_codec_context;
};

