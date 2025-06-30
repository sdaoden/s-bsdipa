#!/bin/sh -
#@ s-bsdipa test (simple: full test part of BsDiPa perl XS module)

: ${KEEP_TESTS:=}
: ${SANITIZER:=}
REDIR= #'2>/dev/null'

: ${DP:=../s-bsdipa}
: ${AWK:=awk}
: ${MKDIR:=mkdir}
#: ${PWD:=pwd}
: ${RM:=rm}

###

# For heaven's sake auto-redirect on SunOS/Solaris
if [ -z "$__BSDIPA_UP" ] && [ -d /usr/xpg4 ]; then
	if [ "x$SHELL" = x ] || [ "x$SHELL" = x/bin/sh ]; then
		echo >&2 'SunOS/Solaris, redirecting through $SHELL=/usr/xpg4/bin/sh'
		__BSDIPA_UP=y PATH=/usr/xpg4/bin:$PATH SHELL=/usr/xpg4/bin/sh
		export __BSDIPA_UP PATH SHELL
		exec /usr/xpg4/bin/sh "$0" "$@"
	fi
fi

LC_ALL=C SOURCE_DATE_EPOCH=844221007
export LC_ALL SOURCE_DATE_EPOCH

[ -d .test ] || $MKDIR .test || exit 1
trap "trap '' EXIT; [ -z \"$KEEP_TESTS\" ] && $RM -rf ./.test" EXIT
trap 'exit 1' HUP INT TERM

(
cd ./.test || exit 2
#pwd=$($PWD) || exit 3
pwd=.



)
exit $?

# s-sht-mode
