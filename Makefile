CC     ?= cc
CFLAGS ?= -g -O2
RM     ?= rm -f

EXEC    = demo
OBJS    = demo.o

LIB	= libpev.a
LIBOBJS = pev.o
ARCHIVE = $(LIB)($(LIBOBJS))

all: $(EXEC) $(ARCHIVE)

$(EXEC): $(OBJS) $(LIB)
	$(CC) -o $@ $(OBJS) $(LIB) $(LDLIBS)

$(LIB): $(LIBOBJS)
	$(AR) $(ARFLAGS) $@ $(LIBOBJS)

clean:
	$(RM) *.o $(LIB) demo

distclean: clean
	$(RM) *~
