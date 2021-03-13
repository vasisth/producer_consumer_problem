nc: distcommon.h NCont.o com.o
	gcc com.o NCont.o -lpthread -o nc
NCont.o : distcommon.h NCont.c
	gcc -c NCont.c
com.o : distcommon.h com.c
	gcc -c com.c
