CFLAGS = -g -Wall -O2 -I./src
LIBS = -lpthread
NAME = smake
ODIR = obj
OBJ = o

OBJS = smake.$(OBJ) \
	xlog.$(OBJ) \
	sver.$(OBJ) \
	file.$(OBJ) \
	array.$(OBJ) \
	xstr.$(OBJ) \
	cfg.$(OBJ) \
	make.$(OBJ)

OBJECTS = $(patsubst %,$(ODIR)/%,$(OBJS))
INSTALL = /usr/bin
VPATH = ./src

.c.$(OBJ):
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
