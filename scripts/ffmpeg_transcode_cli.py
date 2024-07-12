import os
import logging
import subprocess

import fire  # python3 -m pip install fire


class FFmpegTranscode:
    def __init__(self, output_type='de_264_only', read_rate=False, max_processes=1, input_net_stream=False, output_net_stream=False, timeout_seconds=60, dry=False):
        self.output_type = output_type
        self.read_rate = read_rate
        self.max_processes = max_processes
        self.input_net_stream = input_net_stream
        self.output_net_stream = output_net_stream
        self.timeout_seconds = timeout_seconds
        self.dry = dry

        self.file_name = os.path.splitext(os.path.basename(__file__))[0]
        logging.basicConfig(
            filename='{}.log'.format(self.file_name), level=logging.INFO, 
            format='%(asctime)s - %(name)s - %(lineno)d - %(levelname)s - %(message)s'
        )

    def run(self):
        logging.info(
            'python3 {}.py run --output_type={} --read_rate={} --max_processes={} --input_net_stream={} --output_net_stream={} --dry={}'
            ' \n'.format(self.file_name, self.output_type, self.read_rate, self.max_processes, self.input_net_stream, self.output_net_stream, self.dry)
        )

        logging.info('{}\n'.format(' '.join(self.build_cli(1))))

        tasks = []
        for i in range(0, self.max_processes):
            args = self.build_cli(i)
            if not self.dry:
                process = subprocess.Popen(
                    args,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    universal_newlines=True,
                )
                tasks.append(process)

        all_speed = []
        for t in tasks:
            my_speed = []
            stdout, stderr = t.communicate()
            stdout = stdout.strip()
            stderr = stderr.strip()
            for line in stdout.splitlines():
                if 'speed=' in line:
                    s = line.partition('speed=')[-1].strip().strip('x')
                    if s != 'N/A':
                        speed = float(s)
                        my_speed.append(speed)
                        all_speed.append(speed)
            if len(my_speed) > 0:
                logging.info('my average speed: {}'.format(sum(my_speed) / float(len(my_speed))))
            if len(stderr) > 0:
                logging.error(stderr)
        if len(all_speed) > 0:
            logging.info('all average speed: {}\n\n\n\n\n'.format(self.max_processes * sum(all_speed) / float(len(all_speed))))

    def build_cli(self, index):
        subprocess.run('rm -rf /tmp/outputs/*', shell=True)

        input_stream_h264 = 'rtsp://192.168.91.64/live/h264'
        input_file_h264 = '/media/1080p.25fps.4M.264'

        input_stream_h265 = 'rtsp://192.168.91.64/live/h265'
        input_file_h265 = '/media/1080p.25fps.3M.265'

        source = input_file_h264
        if self.output_type.startswith('de_264') or self.output_type.startswith('d_264'):
            if self.input_net_stream:
                source = input_stream_h264
        elif self.output_type.startswith('de_265') or self.output_type.startswith('d_265'):
            if self.input_net_stream:
                source = input_stream_h265
            else:
                source = input_file_h265
        else:
            logging.error('output_type {} and input_net_stream {} not support\n'.format(self.output_type, self.input_net_stream))

        output_stream_h264 = 'rtsp://192.168.91.64/my/h264/%s/%d'
        output_file_h264 = '/media/outputs/%s_%d.25fps.264'

        output_stream_h265 = 'rtsp://192.168.91.64/my/h265/%s/%d'
        output_file_h265 = '/media/outputs/%s_%d.25fps.265'

        destination = output_file_h264
        output_format = 'h264'
        if self.output_type.endswith('_264'):
            if self.output_net_stream:
                destination = output_stream_h264
                output_format = 'rtsp'
        elif self.output_type.endswith('_265'):
            if self.output_net_stream:
                destination = output_stream_h265
                output_format = 'rtsp'
            else:
                destination = output_file_h265
                output_format = 'hevc'
        elif self.output_type.endswith('_only'):
            pass
        else:
            logging.error('output_type {} and output_net_stream {} not support\n'.format(self.output_type, self.output_net_stream))
     
        video_filter_d1 = 'scale=720:480'
        video_filter_cif = 'scale=352:288'
        video_filter_complex = '[0:v]split=2[v1][v2];[v1]scale=720:480,setsar=1[b1];[v2]scale=352:288,setsar=1[b2]'

        args = [
            'docker', 'run', '-it',
            '-v', '/tmp:/media',
            'linuxserver/ffmpeg',
            '-nostdin',
        ]
        if self.read_rate:
            args += [
                '-re',
                '-threads', '1',
            ]

        if self.input_net_stream:
            args += [
                '-rtsp_transport', 'tcp',
                '-t', str(self.timeout_seconds),
            ]

        args += [
            '-i', source,
        ]

        return args + {
            'de_264_to_264': [
                '-filter_complex', video_filter_complex, '-filter_complex_threads', '2', '-threads', '2',
                '-map', '[b1]', '-c:v:0', 'libx264', '-b:v:0', '1M', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('d1', index),
                '-map', '[b2]', '-c:v:1', 'libx264', '-b:v:1', '128K', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('cif', index)
            ],
            'de_265_to_265': [
                '-filter_complex', video_filter_complex, '-filter_complex_threads', '2', '-threads', '2',
                '-map', '[b1]', '-c:v:0', 'libx265', '-b:v:0', '1M', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('d1', index),
                '-map', '[b2]', '-c:v:1', 'libx265', '-b:v:1', '128K', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('cif', index)
            ],
            'de_265_to_264': [
                '-filter_complex', video_filter_complex, '-filter_complex_threads', '2', '-threads', '2',
                '-map', '[b1]', '-c:v:0', 'libx264', '-b:v:0', '1M', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('d1', index),
                '-map', '[b2]', '-c:v:1', 'libx264', '-b:v:1', '128K', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('cif', index)
            ],
            'de_264_to_d1_264': [
                '-vf', video_filter_d1, '-filter_threads', '1', '-threads', '1',
                '-c:v', 'libx264', '-b:v', '1M', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('d1', index),
            ],
            'de_264_to_cif_264': [
                '-vf', video_filter_cif, '-filter_threads', '1', '-threads', '1',
                '-c:v', 'libx264', '-b:v', '128K', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('cif', index),
            ],
            'de_265_to_d1_265': [
                '-vf', video_filter_d1, '-filter_threads', '1', '-threads', '1',
                '-c:v', 'libx265', '-b:v', '1M', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('d1', index),
            ],
            'de_265_to_cif_265': [
                '-vf', video_filter_cif, '-filter_threads', '1', '-threads', '1',
                '-c:v', 'libx265', '-b:v', '128K', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('cif', index),
            ],
            'de_265_to_d1_264': [
                '-vf', video_filter_d1, '-filter_threads', '1', '-threads', '1',
                '-c:v', 'libx264', '-b:v', '1M', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('d1', index),
            ],
            'de_265_to_cif_264': [
                '-vf', video_filter_cif, '-filter_threads', '1', '-threads', '1',
                '-c:v', 'libx264', '-b:v', '128K', '-bf', '0', '-g', '50', '-preset', 'ultrafast', '-tune', 'zerolatency', '-f', output_format, destination % ('cif', index),
            ],
            'd_264_only': [
                '-f', 'null', '-'
            ],
            'd_265_only': [
                '-f', 'null', '-'
            ],
        }[self.output_type]


if __name__ == '__main__':
    fire.Fire(FFmpegTranscode)