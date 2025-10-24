CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -D_DEFAULT_SOURCE -g

RAYLIB_SRC = ./contrib/raylib-5.5/src

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	LDLIBS = -lraylib -lm -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
else
	LDLIBS = -lraylib -lm -lpthread -ldl -lX11 -lGL -lrt
endif


all: montage

raylib:
	cd $(RAYLIB_SRC) && make PLATFORM=PLATFORM_DESKTOP

montage: raylib montage.c
	$(CC) $(CFLAGS) -I$(RAYLIB_SRC) -o $@ montage.c -L$(RAYLIB_SRC) $(LDLIBS)

run: montage
	./montage montage.c

format:
	clang-format --style=LLVM -i -- montage.c

clean:
	rm -f montage
