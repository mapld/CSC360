.phony all:
all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c diskutils.h
	gcc -g diskinfo.c -o diskinfo

disklist: disklist.c diskutils.h
	gcc -g disklist.c  -o disklist

diskget: diskget.c diskutils.h
	gcc -g diskget.c  -o diskget

diskput: diskput.c diskutils.h
	gcc -g diskput.c  -o diskput

.phony clean:
clean:
	-rm -rf *.o *.exe
