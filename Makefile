EXEC    = demo
OBJS    = demo.o
LDLIBS  = -lrt

LIB	= libpev.a
ARCHIVE = $(LIB)(pev.o)

all: $(EXEC) $(ARCHIVE)

$(EXEC): $(OBJS) $(ARCHIVE)
#	$(CC) -o $@ $^

clean:
	$(RM) *.o $(LIB) demo

distclean: clean
	$(RM) *~
