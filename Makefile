.PHONY: run
run: main.c3 rl.c3 interp.c3
	c3c compile-run main.c3 rl.c3 interp.c3 -g -l ./raylib/lib/libraylib.a
