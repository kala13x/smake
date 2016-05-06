CFLAGS = -g -O2 -Wall -pthread
LIB = -lpthread 

RM = rm -rf
VPATH = src
BUILD = bin
INSTALL = /usr/bin

OBJS = smake.o \
		version.o \
		vector.o \
		config.o \
		files.o \
		make.o \
		slog.o 

.c.o: $(VPATH)
	$(CC) $(CFLAGS) -c $< $(LIB)

smake: $(OBJS)
	$(CC) $(CFLAGS) -o smake $(OBJS) $(LIB)
	@test -d $(BUILD) || mkdir $(BUILD)
	@install -m 0755 smake $(BUILD)/

.PHONY: install
install: smake
	@test -d $(INSTALL) || mkdir $(INSTALL)
	@echo @install -m 0755 smake $(INSTALL)/
	@install -m 0755 smake $(INSTALL)/

.PHONY: clean
clean:
	$(RM) smake $(OBJS)
