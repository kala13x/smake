####################################
# Automatically generated by SMake #
# https://github.com/kala13x/smake #
####################################

CFLAGS = -g -O2 -Wall
CFLAGS += -I./src -I./xutils/build/include
LD_LIBS = ./xutils/build/lib/libxutils.a
LIBS = -lpthread
NAME = smake
ODIR = ./obj
OBJ = o

OBJS = cfg.$(OBJ) \
	find.$(OBJ) \
	info.$(OBJ) \
	make.$(OBJ) \
	smake.$(OBJ)

OBJECTS = $(patsubst %,$(ODIR)/%,$(OBJS))
INSTALL_BIN = /usr/bin
VPATH = ./src

.c.$(OBJ):
	@test -d $(ODIR) || mkdir -p $(ODIR)
	$(CC) $(CFLAGS)  -c -o $(ODIR)/$@ $< $(LIBS)

$(NAME):$(OBJS)
	$(CC) $(CFLAGS) -o $(ODIR)/$(NAME) $(OBJECTS) $(LD_LIBS) $(LIBS)

.PHONY: install
install:
	@test -d $(INSTALL_BIN) || mkdir -p $(INSTALL_BIN)
	install -m 0755 $(ODIR)/$(NAME) $(INSTALL_BIN)/

.PHONY: clean
clean:
	$(RM) $(ODIR)/$(NAME) $(OBJECTS)
