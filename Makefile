CC     ?= cc
RM     ?= rm -f

EXEC    = demo
OBJS    = demo.o

LIB	= libpev.a
ARCHIVE = $(LIB)(pev.o)

all: $(EXEC) $(ARCHIVE)

$(EXEC): $(OBJS) $(ARCHIVE)
#	$(CC) -o $@ $^

clean:
	$(RM) *.o $(LIB) demo

distclean: clean
	$(RM) *~
