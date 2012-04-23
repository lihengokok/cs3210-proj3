bbfs : bbfs.o log.o date_utils.o btree.o
	gcc -g -o bbfs bbfs.o log.o date_utils.o btree.o `pkg-config fuse --libs`

bbfs.o : bbfs.c log.h params.h date_utils.h
	gcc -g -Wall -c bbfs.c `pkg-config fuse --cflags`

log.o : log.c log.h params.h
	gcc -g -Wall -c log.c `pkg-config fuse --cflags`
	
date_utils.o : date_utils.c date_utils.h
	gcc -g -Wall -c date_utils.c `pkg-config fuse --cflags`
	
btree.o : btree.c btree.h
	gcc -g -Wall -c btree.c `pkg-config fuse --cflags`

clean:
	rm -f bbfs *.o

dist:
	rm -rf fuse-tutorial/
	mkdir fuse-tutorial/
	cp ../*.html fuse-tutorial/
	mkdir fuse-tutorial/example/
	mkdir fuse-tutorial/example/mountdir/
	mkdir fuse-tutorial/example/rootdir/
	echo "a bogus file" > fuse-tutorial/example/rootdir/bogus.txt
	mkdir fuse-tutorial/src
	cp Makefile *.c *.h fuse-tutorial/src/
	tar cvzf ../../fuse-tutorial.tgz fuse-tutorial/
	rm -rf fuse-tutorial/
