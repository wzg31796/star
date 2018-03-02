#include "star.h"
#include "star_tcp.h"

Star *star;

int SIZEINT;
int SIZEPTR;


Queue *
next_queue()
{
	Queue *q;
	star->i++;
	if (star->i == star->n)
		star->i = 0;
	q = star->func[star->i]->queue;
	return q;
}

Star *
new_star(Conf *conf)
{
	Star *s = malloc(sizeof(Star) + conf->nthread * sizeof(Process *));

	s->conf = conf;
	s->n = conf->nthread;
	s->i = -1;

	s->server = 0;
	s->main = new_proc();

	for (int i = 0; i < s->n; ++i)
	{
		s->func[i] = new_proc();
	}

	return s;
}

void
free_star(Star *star)
{
	pthread_kill(star->server, SIGQUIT);

	for (int i = 0; i < star->n; ++i)
	{
		free_proc(star->func[i]);
	}

	free_proc(star->main);

	free_conf(star->conf);
	free(star);
	printf("free star\n");
}

void star_stop(int signo)
{  
    printf("star stop\n");
    free_star(star);
    exit(0);  
}  


static inline void 
read_conf_str(lua_State *L, const char *key, char **p)
{
	const char *str;
	char *newstr;
	size_t sz;
	lua_getglobal(L, key);
	
	str = luaL_checklstring(L, -1, &sz);
	newstr = malloc(sz + 1);
	strcpy(newstr, str);
	*p = newstr;
	lua_pop(L, 1);
}

static inline void 
read_conf_int(lua_State *L, const char *key, int *p)
{
	lua_getglobal(L, key);
	int n = (int)lua_tointeger(L, -1);
	*p = n;
	lua_pop(L, 1);
}

void
read_conf(const char *file, Conf *conf)
{
	lua_State *L = luaL_newstate();

	if (luaL_loadfile(L, file) != 0 ) {
		lua_close(L);
	 	fprintf(stderr, "Load conf error from %s: %s", file, lua_tostring(L, -1));
	 	exit(1);
	}
	luaL_openlibs(L);

	if (lua_pcall(L, 0, 0, 0) != 0) {
		lua_close(L);
		fprintf(stderr, "Run conf error: %s\n", lua_tostring(L, -1));
		exit(1);
	}

	read_conf_int(L, "thread", 		&conf->nthread);
	read_conf_str(L, "main", 		&conf->main);
	read_conf_str(L, "func", 		&conf->func);

	read_conf_str(L, "server", 		&conf->server);
	read_conf_str(L, "ip", 			&conf->ip);
	read_conf_int(L, "port", 		&conf->port);
	read_conf_int(L, "maxclient", 	&conf->maxclient);

	lua_close(L);
}


void
star_run()
{
	tcp_thread_run();

	for (int i = 0; i < star->n; ++i)
	{
		run_funcproc(star->func[i], star->conf->func);
	}

	// must last start main
	run_mainproc(star->main, star->conf->main);
}


void
start(Conf *conf)
{
	dump_conf(conf);
	star = new_star(conf);
	star_run();
}


void
star_init()
{
	SIZEPTR = sizeof(void *);
	SIZEINT = sizeof(int);

	signal(SIGINT, star_stop);
}


int
main(int argc, char const *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: star main.lua\n");
		return 1;
	}

	star_init();

	Conf *conf = new_conf();
	read_conf(argv[1], conf);

	start(conf);

	free_star(star);

	return 0;
}