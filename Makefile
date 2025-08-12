main: src/main.c src/animator.h src/animator.c ./src/ffmpeg.h ./src/ffmpeg_linux.c
	gcc -ggdb -Wall -Wextra -o main src/main.c src/animator.c ./src/ffmpeg_linux.c -L./raylib/lib/ -l:libraylib.a -lm
