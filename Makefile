CFLAGS = -g -O0 -Wall -Isrc -Ilib -I/usr/local/include
LINK = -L/usr/local/lib -llua -lm -DLUA_USE_READLINE -ldl -lpthread


STAR = \
	star_main.c \
	star_queue.c \
	star_seri.c \
	star_conf.c \
	star_proc.c \
	star_tcp.c \
	star_timer.c \

LUALIB = \
	lua_star.c \
	lua_sock.c \
	lua_timer.c \

star:
	gcc $(CFLAGS) $(addprefix src/,$(STAR)) $(addprefix lib/,$(LUALIB)) -o star $(LINK)