# CC = gcc
# CFLAGS = -Wall -Werror -pthread
# LIBS = -lsnappyc
# DIRS = -Lsnappy-c/

# SRC = src/service.c
# OBJ = $(SRC:.c=.o)


# service:
# 	$(CC) $(CFLAGS) src/service.c -o bin/service $(DIRS) $(LIBS)

# library:
# 	$(CC) $(CFLAGS) src/library.c -o bin/lib



# clean :
# 	@rm src/*.o bin/*.a
# 	@echo Cleaned!

CC = gcc
CFLAGS = -Wall -Wextra -g
LIBS = -lrt

SRCDIR = src
OBJDIR = bin

SERVICE_SRCS = $(SRCDIR)/service.c $(SRCDIR)/shared_mem.c
LIBRARY_SRCS = $(SRCDIR)/library.c $(SRCDIR)/shared_mem.c
OBJS = $(OBJDIR)/service.o $(OBJDIR)/library.o $(OBJDIR)/shared_mem.o

all: service library

service: $(OBJDIR)/service.o $(OBJDIR)/shared_mem.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

library: $(OBJDIR)/library.o $(OBJDIR)/shared_mem.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

$(OBJDIR)/service.o: $(SRCDIR)/service.c $(SRCDIR)/shared_mem.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/library.o: $(SRCDIR)/library.c $(SRCDIR)/shared_mem.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/shared_mem.o: $(SRCDIR)/shared_mem.c $(SRCDIR)/shared_mem.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) service library
