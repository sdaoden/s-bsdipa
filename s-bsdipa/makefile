#@ Makefile for s-bsdipa.
#@ Please see top of [../]lib/makefile for possible arguments to pass!

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

CFLAGS += -std=c99 $(s_BSDIPA_CFLAGS) -I../lib
LIBS = ../lib/libsbsdipa.a -lz

SRC = $(PROG).c

$(PROG): $(SRC) ../lib/s-bsdipa-io.h
	$(CC) $(CFLAGS) -o $(@) $(SRC) $(LIBS)
all: $(PROG)
clean:
	rm -f $(PROG)

# s-mk-mode
