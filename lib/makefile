#@ Makefile for s-bsdipa-lib (libsbsdipa.a).
#@  - pass s_BSDIPA_32 non-empty for 32-bit file size limits and control data.
#@  - pass s_BSDIPA_MAGIC_WINDOW=UINT to define the built-in default for
#@    s_bsdipa_diff_ctx::dc_magic_window (see there).
#@ [- pass s_BSDIPA_CFLAGS='$(SUFX)' for lots of compiler flags.]
#@
#@ For example:
#@	CFLAGS='-O1 -g' make s_BSDIPA_32=y s_BSDIPA_CFLAGS='$(SUFX)' all

LIB = libsbsdipa.a
VERSION = 0.5.0
LIBDIVSUFSORT = libdivsufsort

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

# smake predefines this (cannot handle # in variables)
NUMBER_SIGN?=\#
SUFS = -fno-common \
	-fstrict-aliasing -fstrict-overflow \
	-fstack-protector-strong \
	-D_FORTIFY_SOURCE=3 \
	$$(x=$$(uname -m); [ "$${x}" != "$${x$(NUMBER_SIGN)x86*}" ] && echo -fcf-protection=full) \
	\
#	-DHAVE_SANITIZER \
#		-fsanitize=undefined \
#		-fsanitize=address \

SUFX = $(SUFW) $(SUFS)

CFLAGS += -std=c99 $(s_BSDIPA_CFLAGS) -I$(LIBDIVSUFSORT) -I.

CONFIGH = s-bsdipa-config.h
SRCS = s-bsdiff.c \
	$(LIBDIVSUFSORT)/divsufsort.c $(LIBDIVSUFSORT)/sssort.c $(LIBDIVSUFSORT)/trsort.c \
	s-bspatch.c
OBJS = $(SRCS:.c=.o)

.SUFFIXES: .o .c .y
.c.o:
	$(CC) $(CFLAGS) -o $(<:.c=.o) -c $<

all: $(LIB)
$(CONFIGH):
	{ \
	echo '#ifndef s__BSDIPA_CONFIG_H';\
	echo '#define s__BSDIPA_CONFIG_H';\
	echo '#define s_BSDIPA_VERSION "'$(VERSION)'"';\
	[ -n "$(s_BSDIPA_32)" ] && echo '#define s_BSDIPA_32' || echo '#undef s_BSDIPA_32';\
	[ -n "$(s_BSDIPA_MAGIC_WINDOW)" ] && echo '#define s_BSDIPA_MAGIC_WINDOW $(s_BSDIPA_MAGIC_WINDOW)' ||\
		echo '#undef s_BSDIPA_MAGIC_WINDOW';\
	echo '#endif';\
	} > $(@)
$(LIB): $(CONFIGH) $(OBJS)
	$(AR) crs $(@) $(OBJS)
clean:
	rm -f $(LIB) $(OBJS) $(CONFIGH)

# s-mk-mode
