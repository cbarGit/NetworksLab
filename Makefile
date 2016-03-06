GCCFLAGS= -ansi -Wall -Wunused -pedantic -ggdb -DDEBUG
LINKERFLAGS=-lpthread -lm

all:  ProxySender ProxyReceiver

Util.o: Util.c 
	gcc -c ${GCCFLAGS} Util.c

ProxySender.o: ProxySender.c Util.h
	gcc -c ${GCCFLAGS} ProxySender.c
	
ProxyReceiver.o: ProxyReceiver.c Util.h
	gcc -c ${GCCFLAGS} ProxyReceiver.c

ProxySender:	ProxySender.o Util.o
	gcc -o ProxySender ${GCCFLAGS} ProxySender.o Util.o ${LINKERFLAGS} 

ProxyReceiver:	ProxyReceiver.o Util.o
	gcc -o ProxyReceiver ${GCCFLAGS} ProxyReceiver.o Util.o ${LINKERFLAGS} 

clean:
	rm -f core* *.stackdump
	rm -f ProxySender ProxySender.o 
	rm -f ProxyReceiver ProxyReceiver.o
	rm -f Util.o
