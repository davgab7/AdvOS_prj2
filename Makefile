CC = gcc
CFLAGS = -Wall -Werror -pthread
LIBS = -lsnappyc
DIRS = -Lsnappy-c/

SRC = src/service.c
OBJ = $(SRC:.c=.o)


service:
	$(CC) $(CFLAGS) src/service.c -o bin/service $(DIRS) $(LIBS)

library:
	$(CC) $(CFLAGS) src/library.c -o bin/lib



clean :
	@rm src/*.o bin/*.a
	@echo Cleaned!
