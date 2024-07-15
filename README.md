# FFmpeg 7.0.1 x86\_64 转码性能测试报告

# 一、FFmpeg信息

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

# 二、CPU信息

|  CPU品牌  |  CPU型号  |  CPU主频  |  CPU颗数  |  单个CPU线程数  |  总线程数  |
| --- | --- | --- | --- | --- | --- |
|  Intel  |  **Xeon Gold 5218R**  |  2.1GHz  |  2  |  40  |  **80**  |

# 三、输入视频信息

|  分辨率  |  帧率  |  码率  |  编码格式  |
| --- | --- | --- | --- |
|  1080P  |  25  |  **4M**  |  **H.264**  |
|  1080P  |  25  |  **3M**  |  **H.265**  |

# 四、tmpfs 4KiB读写性能

为了避免遇到磁盘IO或者网络IO瓶颈，我们使用内存盘来作为转码输入输出，而tmpfs 4K读写性能为3.GB/秒，1.9GB/秒，满足我们最大400MB/秒的读写要求

    # df -h | grep "/tmp"
    tmpfs                    6.0G  3.9G  2.2G   64% /tmp
    # dd if=/tmp/testfile of=/dev/null bs=4K count=10000000
    记录了1000000+0 的读入
    记录了1000000+0 的写出
    4096000000字节(4.1 GB)已复制，1.15974 秒，3.5 GB/秒
    # dd if=/dev/zero of=/tmp/testfile bs=4K count=10000000
    dd: 写入"/tmp/testfile" 出错: 设备上没有空间
    记录了1572848+0 的读入
    记录了1572847+0 的写出
    6442381312字节(6.4 GB)已复制，3.32676 秒，1.9 GB/秒

# 五、FFmpeg Cli测试结果

x10为10进程，x20为20进程，-re x50为固定解码帧率50进程

转码命令行示例 docker run -it -v /tmp:/media linuxserver/ffmpeg -nostdin -re -threads 1 -i /media/1080p.25fps.3M.265 -vf scale=720:480 -filter\_threads 1 -threads 1 -c:v libx264 -b:v 1M -bf 0 -g 50 -preset ultrafast -tune zerolatency -f h264 /media/outputs/d1\_1.25fps.1M.264

|  输入视频编码  |  输出视频分辨率  |  输出视频帧率  |  输出视频码率  |  输出视频编码  |  转码路数  |  CPU占用  |  内存占用  |
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


# 六、libavcodec 转码负载性能

以下测量值40线程下的90%分位数

|  负载  |  编码格式  |  输入分辨率  |  输出格式  |  输出分辨率  |  耗时  |
| --- | --- | --- | --- | --- | --- |
|  解码  |  H.264  |  1080P  |  YUV420P  |  1080P  |  23 ms / 帧  |
|  解码  |  H.265  |  1080P  |  YUV420P  |  1080P  |  27 ms / 帧  |
|   |   |   |   |   |   |
|  缩放  |  YUV420P  |  1080P  |  YUV420P  |  D1  |  6 ms / 帧  |
|  缩放  |  YUV420P  |  1080P  |  YUV420P  |  CIF  |  4 ms / 帧  |
|   |   |   |   |   |   |
|  编码  |  YUV420P  |  D1  |  H.264  |  D1  |  2.39 ms / 帧  |
|  编码  |  YUV420P  |  CIF  |  H.264  |  CIF  |  1.02 ms / 帧  |
|   |   |   |   |   |   |
|  编码  |  YUV420P  |  D1  |  H.265  |  D1  |  30.86 ms / 帧  |
|  编码  |  YUV420P  |  CIF  |  H.265  |  CIF  |  14.67 ms / 帧  |

# 七、libavcodec 测试结果

以下测量值40线程下的90%分位数

比ffmpeg cli测试结果稍好，应该是因为这里将视频帧提前读进std::vector，编码结果直接丢弃

CPU与MEM内存与ffmpeg cli并无区别，这里就不赘叙了

|  输入视频编码  |  输出视频分辨率  |  输出视频帧率  |  输出视频码率  |  输出视频编码  |  转码路数  |
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


