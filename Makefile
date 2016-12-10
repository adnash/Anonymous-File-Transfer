#
#	Makefile for awget
#	Aaron Smith, Alan Nash
#	
#

CC = /usr/bin/cc

all:	awget ss 

awget:	awget.c awget.h
	$(CC) -o awget awget.c awget.h

ss:	ss.c
	$(CC) -o ss ss.c

target:	
	tar cf P2.tar *.c *.h Makefile README

clean:
	rm -f awget ss P2.tar
