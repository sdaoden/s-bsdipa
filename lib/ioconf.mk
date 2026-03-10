#@ Makefile to test for supported s-bsdipa-io.h I/O layers.

RM ?= rm

bz2:
	@echo '#include <bzlib.h>' > iomk.c; $(CC) -c iomk.c >/dev/null 2>&1; res=$$?; $(RM) -f iomk.*; exit $$res
xz:
	@echo '#include <lzma.h>' > iomk.c; $(CC) -c iomk.c >/dev/null 2>&1; res=$$?; $(RM) -f iomk.*; exit $$res
zstd:
	@echo '#include <zstd.h>' > iomk.c; $(CC) -c iomk.c >/dev/null 2>&1; res=$$?; $(RM) -f iomk.*; exit $$res

# s-mk-mode
