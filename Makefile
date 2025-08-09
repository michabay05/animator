.PHONY: ffmpeg jimp compile

all: jimp ffmpeg compile

ffmpeg: ffmpeg_linux.c
	gcc -c ffmpeg_linux.c -o ffmpeg_linux.o
	ar rcs libffmpeg_linux.a ffmpeg_linux.o
	rm ffmpeg_linux.o

jimp: jimp.c jimp.h
	gcc -c jimp.c -o jimp.o
	ar rcs libjimp.a jimp.o
	rm jimp.o

compile: main.c3 animator.c3 rl.c3 ffmpeg.c3 jimp.c3
	c3c compile main.c3 animator.c3 rl.c3 ffmpeg.c3 jimp.c3 -g -l ./libffmpeg_linux.a -l ./raylib/lib/libraylib.a -l ./libjimp.a

