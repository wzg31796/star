CFLAGS = -g -O0 -Wall -Isrc -Ilib
LINK = -L/usr/local/lib -llua -lm -DLUA_USE_READLINE -ldl -lpthread


STAR = \
	star_main.c \
	star_queue.c \
	star_conf.c \
	star_proc.c \
	star_seri.c \

LUALIB = \
	lua_star.c \

star:
	gcc $(CFLAGS) $(addprefix src/,$(STAR)) $(addprefix lib/,$(LUALIB)) -o star $(LINK)