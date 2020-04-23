CFLAGS =  -I./src
LIBS = -lpthread
NAME = smake
ODIR = obj

OBJS = smake.o \
	xlog.o \
	sver.o \
	file.o \
	array.o \
	xstr.o \
	cfg.o \
	make.o

OBJECTS = $(patsubst %,$(ODIR)/%,$(OBJS))
INSTALL = /usr/bin
VPATH = ./src

.c.o:
	@test -d $(ODIR) || mkdir $(ODIR)
	$(CC) $(CFLAGS) -c -o $(ODIR)/$@ $< $(LIBS)

$(NAME):$(OBJS)
	$(CC) $(CFLAGS) -o $(ODIR)/$(NAME) $(OBJECTS) $(LIBS)

.PHONY: install
install:
	@test -d $(INSTALL) || mkdir $(INSTALL)
	@install -m 0755 $(ODIR)/$(NAME) $(INSTALL)/

.PHONY: clean
clean:
	$(RM) $(ODIR)/$(NAME) $(OBJECTS)
