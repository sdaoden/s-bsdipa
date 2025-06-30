#@ Makefile for s-bsdipa.
#@ Please see top of [../]lib/makefile for possible arguments to pass!

DD ?= dd
MKDIR ?= mkdir
RM ?= rm
SEQ ?= seq

PROG = s-bsdipa

SUFWW = #-Weverything
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

CFLAGS += -std=c99 $(s_BSDIPA_CFLAGS) -I../lib \
	$$(xz --version >/dev/null 2>&1 && printf '%s' -Ds__BSDIPA_XZ)
LIBS = ../lib/libsbsdipa.a -lz \
	$$(xz --version >/dev/null 2>&1 && printf '%s' -llzma)

SRC = $(PROG).c

$(PROG): $(SRC) ../lib/s-bsdipa-io.h
	$(CC) $(CFLAGS) -o $(@) $(SRC) $(LIBS)
all: $(PROG)
test: all
	DP32=$(s_BSDIPA_32) \
	DPXZ=$$(xz --version >/dev/null 2>&1 && echo y) \
	DD="$(DD)" MKDIR="$(MKDIR)" RM="$(RM)" SEQ="$(SEQ)" \
	exec ./s-bsdipa-test.sh
clean:
	$(RM) -rf $(PROG) .test

# s-mk-mode
