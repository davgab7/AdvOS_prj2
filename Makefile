# CC = gcc
# CFLAGS = -Wall -Werror -pthread
# LIBS = -lsnappyc
# DIRS = -Lsnappy-c/

# SRC = src/tinyfile.c
# OBJ = $(SRC:.c=.o)


# tinyfile:
# 	$(CC) $(CFLAGS) src/tinyfile.c -o bin/tinyfile $(DIRS) $(LIBS)

# library:
# 	$(CC) $(CFLAGS) src/library.c -o bin/lib


# clean :
# 	@rm src/*.o bin/*.a
# 	@echo Cleaned!

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LIBS = -lrt

SRCDIR = src
OBJDIR = bin

tinyfile_SRCS = $(SRCDIR)/service.c $(SRCDIR)/shared_mem.c
LIBRARY_SRCS = $(SRCDIR)/library.c $(SRCDIR)/shared_mem.c
SAMPLE_SRCS = $(SRCDIR)/sample_app.c
OBJS = $(OBJDIR)/tinyfile.o $(OBJDIR)/library.o $(OBJDIR)/shared_mem.o $(OBJDIR)/sample_app.o

all: tinyfile sample_app 

tinyfile: $(OBJDIR)/tinyfile.o $(OBJDIR)/shared_mem.o src/libsnappyc.a
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

sample_app: $(OBJDIR)/sample_app.o $(OBJDIR)/library.o $(OBJDIR)/shared_mem.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

$(OBJDIR)/tinyfile.o: $(SRCDIR)/service.c $(SRCDIR)/shared_mem.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/library.o: $(SRCDIR)/library.c $(SRCDIR)/library.h $(SRCDIR)/shared_mem.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sample_app.o: $(SRCDIR)/sample_app.c $(SRCDIR)/library.h
	$(CC) $(CFLAGS) -c $< -o $@	

$(OBJDIR)/shared_mem.o: $(SRCDIR)/shared_mem.c $(SRCDIR)/shared_mem.h
	$(CC) $(CFLAGS) -c $< -o $@

# snappy-c: snappy-c/snappy.o
# 	ar cr src/libsnappy.a snappy-c/snappy.o

clean:
	rm -f $(OBJS) tinyfile sample_app
