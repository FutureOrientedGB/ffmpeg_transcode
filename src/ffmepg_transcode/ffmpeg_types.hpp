#pragma once

// ffmpeg
struct AVFrame;
struct AVPacket;


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

