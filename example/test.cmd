rem extract raw audio 48000Hz 16bit stereo
rem ffmpeg -i out_test.mkv -f s16le -acodec pcm_s16le audio.raw

rem Video only
rem ffmpeg -re -i out_test.mkv -an -f rawvideo -pix_fmt yuv420p - | rtpcli.exe

rem Audio only
rem rtpcli.exe - 8888 8899 audio.raw

rem Video and Audio
ffmpeg -re -i out_test.mkv -an -f rawvideo -pix_fmt yuv420p - | rtpcli.exe - 8888 8899 audio.raw
