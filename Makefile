CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -D_DEFAULT_SOURCE -g

RAYLIB_SRC = ./contrib/raylib-5.5/src

LDLIBS = -lraylib -lm -lpthread -ldl -lX11 -lGL -lrt

all: montage

raylib:
	cd $(RAYLIB_SRC) && make PLATFORM=PLATFORM_DESKTOP

montage: raylib montage.c
	$(CC) $(CFLAGS) -I$(RAYLIB_SRC) -o $@ montage.c -L$(RAYLIB_SRC) $(LDLIBS)

clean:
	rm -f montage
