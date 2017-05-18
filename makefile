###################################################################

###################################################################

.SUFFIXES: .h .o .c


CC = gcc -DMEMWATCH -DMW_STDIO
CCOPTS = -g
LIBS = -lm
OBJS = server.o client.o memwatch.o
CCEXEC = server client

all: server client

server: server.c
  $(CC) server.c memwatch.c -o lyrebird.server

client: client.c
  $(CC) client.c multiproc.c decrypt.c -lm memwatch.c -o lyrebird.client


clean:
  rm -f $(OBJS)
  rm -f lyrebird.server
  rm -f lyrebird.client
  rm -f core
  rm -f memwatch.log
