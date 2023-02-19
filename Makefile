# remove the # in the following line to enable reorg compilation (and running)
all: cruncher  reorg

cruncher: cruncher.c utils.h
	gcc -g -I. -O3 -o cruncher cruncher.c

loader: loader.c utils.h
	gcc -I. -O3 -o loader loader.c

reorg: test.cpp utils.h
	g++ -g -I. -O3 -o test test.cpp

reorg: reorg.cpp utils.h
	g++ -g -I. -O3 -o reorg reorg.cpp

cruncher_debug: cruncher.c utils.h
	gcc -I. -g -O0 -fsanitize=address cruncher.c

loader_debug: loader.c utils.h
	gcc -I. -g -O0 -fsanitize=address loader.c

reorg_debug: reorg.cpp utils.h
	g++ -I. -g -O0 -fsanitize=address reorg.cpp

clean:
	rm -f loader cruncher reorg
