#@ Umbrella makefile for s-bsdipa-lib, plus example/program s-bsdipa.
#@ Please see top of lib/makefile for possible arguments to pass!

SUBDIRS = lib s-bsdipa

.SUFFIXES: # 4 smake
.DEFAULT:
	for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) $@) || exit; done

distclean: clean

# s-mk-mode
