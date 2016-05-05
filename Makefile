CFLAGS = -g -O2 -Wall -pthread
LIB = -lpthread 

RM = rm -rf
VPATH = src
INSTALL = bin

OBJS = smake.o \
		vector.o \
		version.o \
		config.o \
		makes.o \
		files.o \
		slog.o 

.c.o: $(VPATH)
	$(CC) $(CFLAGS) -c $< $(LIB)

smake: $(OBJS)
	$(CC) $(CFLAGS) -o smake $(OBJS) $(LIB)
	@test -d $(INSTALL) || mkdir $(INSTALL)
	@install -m 0755 smake $(INSTALL)/

.PHONY: clean
clean:
	$(RM) smake $(OBJS)
