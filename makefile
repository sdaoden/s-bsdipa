#@ Umbrella makefile for s-bsdipa-lib, plus example/program s-bsdipa.
#@ Please see top of lib/makefile for possible arguments to pass!

DESTDIR =
PREFIX = /usr
MANDIR = share/man

SUBDIRS = lib s-bsdipa

.SUFFIXES: # 4 smake
.DEFAULT:
	for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) $@) || exit; done

distclean: clean

install:
	mkdir -p -m 0755 \
		"$(DESTDIR)$(PREFIX)/bin" \
		"$(DESTDIR)$(PREFIX)/include" \
		"$(DESTDIR)$(PREFIX)/lib" \
		"$(DESTDIR)$(PREFIX)/$(MANDIR)/man1";\
	\
	cp -f s-bsdipa/s-bsdipa "$(DESTDIR)$(PREFIX)/bin"/;\
	cp -f s-bsdipa/s-bsdipa.1 "$(DESTDIR)$(PREFIX)/$(MANDIR)/man1"/;\
	\
	cp -f \
		lib/s-bsdipa-lib.h \
		lib/s-bsdipa-io.h \
		lib/s-bsdipa-config.h \
		"$(DESTDIR)$(PREFIX)/include"/;\
	cp -f lib/libsbsdipa.a "$(DESTDIR)$(PREFIX)/lib"/;\
	\
	chmod 0755 "$(DESTDIR)$(PREFIX)/bin"/s-bsdipa;\
	chmod 0644 "$(DESTDIR)$(PREFIX)/$(MANDIR)/man1"/s-bsdipa.1;\
	chmod 0644 \
		"$(DESTDIR)$(PREFIX)/include"/s-bsdipa-lib.h \
		"$(DESTDIR)$(PREFIX)/include"/s-bsdipa-io.h \
		"$(DESTDIR)$(PREFIX)/include"/s-bsdipa-config.h \
		"$(DESTDIR)$(PREFIX)/lib"/libsbsdipa.a

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin"/s-bsdipa \
		"$(DESTDIR)$(PREFIX)/$(MANDIR)/man1"/s-bsdipa.1
	rm -f \
		"$(DESTDIR)$(PREFIX)/include"/s-bsdipa-lib.h \
		"$(DESTDIR)$(PREFIX)/include"/s-bsdipa-io.h \
		"$(DESTDIR)$(PREFIX)/include"/s-bsdipa-config.h \
		"$(DESTDIR)$(PREFIX)/lib"/libsbsdipa.a

# s-mk-mode
