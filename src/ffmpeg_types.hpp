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
    virtual ~FFmpegPacket();

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
    virtual ~FFmpegFrame();

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
    FFmpegCodec() = delete;
    FFmpegCodec(std::string codec_name);
    FFmpegCodec(const FFmpegCodec &other) = delete;
    FFmpegCodec(FFmpegCodec &&other) noexcept;
    ~FFmpegCodec();

    void init(std::string codec_name);
    virtual bool find(std::string codec_name) = 0;

    void free();

    AVCodec *raw_ptr();
    bool is_null();

    std::string codec_name();


protected:
    AVCodec *m_codec;
    std::string m_codec_name;
};


class FFmpegEncoder : public FFmpegCodec {
public:
    FFmpegEncoder() = delete;
    FFmpegEncoder(std::string codec_name);

    virtual bool find(std::string codec_name) override;
};


class FFmpegDecoder : public FFmpegCodec {
public:
    FFmpegDecoder() = delete;
    FFmpegDecoder(std::string codec_name);

    virtual bool find(std::string codec_name) override;
};


class FFmpegCodecContext {
public:
    FFmpegCodecContext() = delete;
    FFmpegCodecContext(std::string codec_name, int pixel_format = 0);
    FFmpegCodecContext(const FFmpegCodecContext &other) = delete;
    FFmpegCodecContext(FFmpegCodecContext &&other) noexcept;
    virtual ~FFmpegCodecContext();

    void init(std::string codec_name, int pixel_format);
    virtual bool new_codec(std::string codec_name) = 0;

    bool auto_hw_accel();

    bool open(std::map<std::string, std::string> options);

    void free();

    AVCodecContext *raw_ptr();
    bool is_null();

    int pixel_format();
    std::pair<int, int> pixel_aspect();
    std::pair<int, int> time_base();


protected:
    FFmpegCodec *m_codec;
    AVCodecContext *m_codec_context;
    std::string m_codec_name;

    int m_pixel_format;
    int m_hw_device_type;
    AVBufferRef *m_hw_device_context;
};



class FFmpegEncoderContext : public FFmpegCodecContext {
public:
    FFmpegEncoderContext() = delete;
    FFmpegEncoderContext(std::string codec_name, int pixel_format = 0);

    virtual bool new_codec(std::string codec_name) override;
};


class FFmpegDecoderContext : public FFmpegCodecContext {
public:
    FFmpegDecoderContext() = delete;
    FFmpegDecoderContext(std::string codec_name, int pixel_format = 0);

    virtual bool new_codec(std::string codec_name) override;
};

