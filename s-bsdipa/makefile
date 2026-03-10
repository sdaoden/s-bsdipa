#@ Makefile for s-bsdipa.
#@ Please see top of [../]lib/makefile for possible arguments to pass!

DD ?= dd
MKDIR ?= mkdir
RM ?= rm
SEQ ?= seq

PROG = s-bsdipa

# smake predefines this (cannot handle # in variables)
NUMBER_SIGN?=\#
SUFLVLC=-std=c99

SUFWW =# -Weverything -Wno-unsafe-buffer-usage -Wno-old-style-cast
SUFW = -W -Wall -pedantic $(SUFWW) \
	\
	-Wno-atomic-implicit-seq-cst \
	-Wno-documentation-unknown-command \
	-Wno-duplicate-enum \
	-Wno-reserved-identifier \
	-Wno-reserved-macro-identifier \
	-Wno-unused-macros \
	\
	-Werror=format-security -Werror=int-conversion \

SUFS = -fno-common \
	-fstrict-aliasing -fstrict-overflow \
	-fstack-protector-strong \
	-D_FORTIFY_SOURCE=3 \
	$$(x=$$(uname -m); [ "$${x}" != "$${x\#x86*}" ] && echo -fcf-protection=full) \
	\
#	-DHAVE_SANITIZER \
#		-fsanitize=undefined \
#		-fsanitize=address \

SUFX = $(SUFW) $(SUFS)

CFLAGS += $(SUFLVLC) $(s_BSDIPA_CFLAGS) -I../lib \
	$$($(MAKE) -f ../lib/ioconf.mk RM="$(RM)" bz2 >/dev/null 2>&1 && printf '%s' -Ds__BSDIPA_BZ2) \
	$$($(MAKE) -f ../lib/ioconf.mk RM="$(RM)" xz >/dev/null 2>&1 && printf '%s' -Ds__BSDIPA_XZ) \

LIBS = ../lib/libsbsdipa.a -lz \
	$$($(MAKE) -f ../lib/ioconf.mk RM="$(RM)" bz2 >/dev/null 2>&1 && printf '%s' -lbz2) \
	$$($(MAKE) -f ../lib/ioconf.mk RM="$(RM)" xz >/dev/null 2>&1 && printf '%s' -llzma) \

SRC = $(PROG).c

$(PROG): $(SRC) ../lib/s-bsdipa-io.h
	$(CC) $(CFLAGS) -o $(@) $(SRC) $(LIBS)
all: $(PROG)
test: all
	DP32=$(s_BSDIPA_32) \
	DPBZ2=$$($(MAKE) -f ../lib/ioconf.mk RM="$(RM)" bz2 >/dev/null 2>&1 && echo y) \
	DPXZ=$$($(MAKE) -f ../lib/ioconf.mk RM="$(RM)" xz >/dev/null 2>&1 && echo y) \
	DD="$(DD)" MKDIR="$(MKDIR)" RM="$(RM)" SEQ="$(SEQ)" \
	exec ./s-bsdipa-test.sh
clean:
	$(RM) -rf $(PROG) .test

# s-mk-mode
