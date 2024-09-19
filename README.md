# FFmpeg 7 x86_64 Transcoding Performance Test Report

## 1. FFmpeg Information

    # ffmpeg -version
    ffmpeg version 7.0.1 Copyright (c) 2000-2024 the FFmpeg developers
    built with gcc 13 (Ubuntu 13.2.0-23ubuntu4)
    configuration: --disable-debug --disable-doc --disable-ffplay --enable-alsa --enable-cuvid --enable-ffprobe --enable-gpl --enable-libaom --enable-libass --enable-libfdk_aac --enable-libfontconfig --enable-libfreetype --enable-libfribidi --enable-libharfbuzz --enable-libkvazaar --enable-libmp3lame --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-libopenjpeg --enable-libopus --enable-libplacebo --enable-librav1e --enable-libshaderc --enable-libsvtav1 --enable-libtheora --enable-libv4l2 --enable-libvidstab --enable-libvmaf --enable-libvorbis --enable-libvpl --enable-libvpx --enable-libwebp --enable-libx264 --enable-libx265 --enable-libxml2 --enable-libxvid --enable-libzimg --enable-nonfree --enable-nvdec --enable-nvenc --enable-cuda-llvm --enable-opencl --enable-openssl --enable-stripping --enable-vaapi --enable-vdpau --enable-version3 --enable-vulkan
    libavutil      59.  8.100 / 59.  8.100
    libavcodec     61.  3.100 / 61.  3.100
    libavformat    61.  1.100 / 61.  1.100
    libavdevice    61.  1.100 / 61.  1.100
    libavfilter    10.  1.100 / 10.  1.100
    libswscale      8.  1.100 /  8.  1.100
    libswresample   5.  1.100 /  5.  1.100
    libpostproc    58.  1.100 / 58.  1.100

## 2. CPU Information

| CPU Brand | CPU Model          | CPU Frequency | CPU Cores | Threads per CPU | Total Threads |
|-----------|--------------------|---------------|-----------|------------------|---------------|
|  Intel  |  **Xeon Gold 5218R**  |  2.1GHz  |  2  |  40  |  **80**  |

## 3. Input Video Information

| Resolution | Frame Rate | Bitrate | Codec   |
| --- | --- | --- | --- |
|  1080P  |  25  |  **4M**  |  **H.264**  |
|  1080P  |  25  |  **3M**  |  **H.265**  |

## 4. tmpfs 4KiB Read/Write Performance

To avoid disk IO or network IO bottlenecks, we used a RAM disk for transcoding input and output. The tmpfs 4K read/write performance was 3.GB/s for read and 1.9GB/s for write, meeting our maximum requirement of 400MB/s.

    # df -h | grep "/tmp"
    tmpfs                    6.0G  3.9G  2.2G   64% /tmp

    dd if=/tmp/testfile of=/dev/null bs=4K count=10000000
    1000000+0 records in
    1000000+0 records out
    4096000000 bytes (4.1 GB) copied, 1.15974 seconds, 3.5 GB/s

    dd if=/dev/zero of=/tmp/testfile bs=4K count=10000000
    dd: error writing '/tmp/testfile': No space left on device
    1572848+0 records in
    1572847+0 records out
    6442381312 bytes (6.4 GB) copied, 3.32676 seconds, 1.9 GB/s

## 5. FFmpeg CLI Test Results

x10 represents 10 processes, x20 represents 20 processes, and -re x50 represents a fixed decoding frame rate with 50 processes.

    # Transcoding Command Example
    docker run -it -v /tmp:/media linuxserver/ffmpeg -nostdin -re -threads 1 -i /media/1080p.25fps.3M.265 -vf scale=720:480 -filter\_threads 1 -threads 1 -c:v libx264 -b:v 1M -bf 0 -g 50 -preset ultrafast -tune zerolatency -f h264 /media/outputs/d1\_1.25fps.1M.264

| Input Video Codec | Output Video Resolution | Output Video Frame Rate | Output Video Bitrate | Output Video Codec | Transcoding Paths | CPU Usage | Memory Usage |
| --- | --- | --- | --- | --- | --- | --- | --- |
|  H.264  |  D1  |  25  |  1M  |  H.264  |  **x10: 38** x20: 35 \-re x50: 37  |  x10: 80% x20: 99% \-re x50: 100%  |  x10: 3.2G x20: 5.5G \-re x50: 7.7G  |
|  H.264  |  CIF  |  25  |  128K  |  H.264  |
|  H.265  |  D1  |  25  |  1M  |  H.265  |  x10: 18 x20: 17 **\-re x50: 19**  |  x10: 92% x20: 99% \-re x50: 100%  |  x10: 3.9G x20: 8.2G \-re x50: 16.4G  |
|  H.265  |  CIF  |  25  |  128K  |  H.265  |
|  H.265  |  D1  |  25  |  1M  |  H.264  |  x10: 30 x20: 28 **\-re x50: 34**  |  x10: 84% x20: 100% \-re x50: 100%  |  x10: 3.5G x20: 8.8G \-re x50: 9.3G  |
|  H.265  |  CIF  |  25  |  128K  |  H.264  |
|  H.264  |  D1  |  25  |  1M  |  H.264  |  **x10: 51** x20: 51 \-re x50: 50  |  x10: 75% x20: 100% \-re x50: 50%  |  x10: 2.4G x20: 7.4G \-re x50: 7.6G  |
|  H.264  |  CIF  |  25  |  128K  |  H.264  |  **x10: 65** x20: 65 \-re x50: 50  |  x10: 67% x20: 100% \-re x50: 40%  |  x10: 2.3G x20: 7.1G \-re x50: 6.5G  |
|  H.265  |  D1  |  25  |  1M  |  H.265  |  **x10: 27** x20: 24 \-re x50: 24  |  x10: 83% x20: 95% \-re x50: 100%  |  X10: 3.2G x20: 7.8G \-re x50: 11.2G  |
|  H.265  |  CIF  |  25  |  128K  |  H.265  |  x10: 40 x20: 36 **\-re x50: 45**  |  x10: 79% x20: 98% \-re x50: 100%  |  x10: 2.8G x20: 6.9G \-re x50: 9.8G  |
|  H.265  |  D1  |  25  |  1M  |  H.264  |  x10: 43 x20: 39 **\-re x50: 47**  |  x10: 81% x20: 99% \-re x50: 100%  |  x10: 3.2G x20: 7.7G \-re x50: 8.1G  |
|  H.265  |  CIF  |  25  |  128K  |  H.264  |  **x10: 52** x20: 47 \-re x50: 50  |  x10: 73% x20: 96% \-re x50: 49%  |  x10: 3.1G x20: 6.4G \-re x50: 5.2G  |
|  H.264  |  FULL HD  |  25  |  74M  |  I420  |  **x10: 91** x20: 89 \-re x50: 50  |  x10: 98% x20: 100% \-re x50: 32%  |  x10: 2.2G x20: 4.9G \-re x50: 5.1G  |
|  H.265  |  FULL HD  |  25  |  74M  |  I420  |  x10: 61 **x20: 62** \-re x50:50   |  x10: 65% x20: 90% \-re x50: 43%  |  x10: 2.9G x20: 5.1G \-re x50: 5.6G  |


## 6. libavcodec Transcoding Load Performance

The following measurements show the 90th percentile under a load of 40 threads.

| Load     | Codec   | Input Resolution | Output Format | Output Resolution | Time        |
|----------|---------|------------------|---------------|-------------------|-------------|
| Decoding | H.264   | 1080P           | YUV420P       | 1080P             | 23 ms/frame |
| Decoding | H.265   | 1080P           | YUV420P       | 1080P             | 27 ms/frame |
|          |         |                  |               |                   |             |
| Scaling  | YUV420P | 1080P           | YUV420P       | D1                | 6 ms/frame  |
| Scaling  | YUV420P | 1080P           | YUV420P       | CIF               | 4 ms/frame  |
|          |         |                  |               |                   |             |
| Encoding | YUV420P | D1              | H.264         | D1                | 2.39 ms/frame |
| Encoding | YUV420P | CIF             | H.264         | CIF               | 1.02 ms/frame |
|          |         |                  |               |                   |             |
| Encoding | YUV420P | D1              | H.265         | D1                | 30.86 ms/frame |
| Encoding | YUV420P | CIF             | H.265         | CIF               | 14.67 ms/frame |



## 7. libavcodec Test Results

The following measurements show the 90th percentile under a load of 40 threads.

The results are slightly better than the FFmpeg CLI tests, likely because the video frames are pre-read into `std::vector`, and the encoding results are discarded directly.

CPU and memory usage are consistent with the FFmpeg CLI, so they will not be elaborated here.

| Input Video Codec | Output Video Resolution | Output Video Frame Rate | Output Video Bitrate | Output Video Codec | Transcoding Paths |
| --- | --- | --- | --- | --- | --- |
|  H.264  |  FULL HD  |  25  |  74M  |  YUV420P  |  **152**  |
|  H.265  |  FULL HD  |  25  |  74M  |  YUV420P  |  **86**  |
|   |   |   |   |   |   |
|  H.264  |  D1  |  25  |  1M  |  H.264  |  **65**  |
|  H.264  |  CIF  |  25  |  128K  |  H.264  |  **93**  |
|  H.265  |  D1  |  25  |  1M  |  H.265  |  **19**  |
|  H.265  |  CIF  |  25  |  128K  |  H.265  |  **40**  |
|  H.265  |  D1  |  25  |  1M  |  H.264  |  **53**  |
|  H.265  |  CIF  |  25  |  128K  |  H.264  |  **68**  |
|   |   |   |   |   |   |
|  H.264  |  D1+CIF  |  25  |  1M+128K  |  H.264  |  **49**  |
|  H.265  |  D1+CIF  |  25  |  1M+128K  |  H.265  |  **15**  |
|  H.265  |  D1+CIF  |  25  |  1M+128K  |  H.264  |  **41**  |


