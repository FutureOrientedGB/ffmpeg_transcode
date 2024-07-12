#!/bin/bash

rm -f ffmpeg_transcode_ways.log

#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_264_to_264 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_265_to_265 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_265_to_264 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_264_to_d1_264 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_264_to_cif_264 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_265_to_d1_265 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_265_to_cif_265 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_265_to_d1_264 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=de_265_to_cif_264 --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=d_264_only --max_processes=20
#python3 ffmpeg_transcode_ways.py run --read_rate False --output_type=d_265_only --max_processes=20

python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_264_to_264 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_265_to_265 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_265_to_264 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_264_to_d1_264 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_264_to_cif_264 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_265_to_d1_265 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_265_to_cif_265 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_265_to_d1_264 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=de_265_to_cif_264 --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=d_264_only --max_processes=50
python3 ffmpeg_transcode_ways.py run --read_rate True --output_type=d_265_only --max_processes=50

