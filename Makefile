.PHONY: compile

all: libffmpeg_linux.a libjimp.a main

libffmpeg_linux.a: ffmpeg_linux.c
	gcc -c ffmpeg_linux.c -o ffmpeg_linux.o
	ar rcs libffmpeg_linux.a ffmpeg_linux.o
	rm ffmpeg_linux.o

libjimp.a: jimp.h
	gcc -x c jimp.h -c -DJIMP_IMPLEMENTATION
	ar rcs libjimp.a jimp.o
	rm jimp.o

main: main.c3 animator.c3 rl.c3 ffmpeg.c3 jimp.c3
	c3c compile main.c3 animator.c3 rl.c3 ffmpeg.c3 jimp.c3 -g -l ./libffmpeg_linux.a -l ./raylib/lib/libraylib.a -l ./libjimp.a

