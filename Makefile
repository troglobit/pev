VERSION	 = 2.0
NAME     = pev
PKG      = $(NAME)-$(VERSION)
ARCHIVE	 = $(PKG).tar.gz

incdir   = $(prefix)/include
libdir   = $(prefix)/lib
docdir   = $(prefix)/share/doc/$(NAME)

RM    	 = rm -f
INSTALL  = install

EXEC   	 = demo example

LIB    	 = libpev.a
CFLAGS  += -W -Wall -Wextra -Wno-unused-parameter
LDLIBS	+= $(LIB)
LIBINC   = pev.h
LIBOBJS	 = pev.o
DOCFILES = README.md UNLICENSE

all: $(EXEC)

$(EXEC): $(LIB)

$(LIB): $(LIBOBJS)
	$(AR) $(ARFLAGS) $@ $(LIBOBJS)

install: $(LIB)
	$(INSTALL) -d $(DESTDIR)$(incdir)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(docdir)
	$(INSTALL) -m 0644 $(LIBINC) $(DESTDIR)$(incdir)/$(LIBINC)
	$(INSTALL) -m 0644 $(LIB) $(DESTDIR)$(libdir)/$(LIB)
	for file in $(DOCFILES); do					\
		$(INSTALL) -m 0644 $$file $(DESTDIR)$(docdir)/$$file;	\
	done

uninstall:
	-$(RM) $(DESTDIR)$(incdir)/$(LIBINC)
	-$(RM) $(DESTDIR)$(libdir)/$(LIB)
	-$(RM) -r $(DESTDIR)$(docdir)

clean:
	$(RM) *.o $(LIB) demo

distclean: clean
	$(RM) *~ GPATH GRTAGS GTAGS ID

dist:
	git archive --format=tar.gz --prefix=$(PKG)/ -o ../$(ARCHIVE) v$(VERSION)
	(cd ..; md5sum $(ARCHIVE) | tee $(ARCHIVE).md5)
