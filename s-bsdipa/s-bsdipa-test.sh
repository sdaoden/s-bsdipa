#!/bin/sh -
#@ s-bsdipa test (simple: full test part of BsDiPa perl XS module)

: ${KEEP_TESTS:=}
REDIR='>/dev/null 2>/dev/null'

: ${DP:=../s-bsdipa}
: ${DP32:=}
: ${DPXZ:=}

: ${DD:=dd}
: ${MKDIR:=mkdir}
: ${RM:=rm}
: ${SEQ:=seq}

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
TNO=0


y() {
	[ $1 -eq 0 ] || { echo >&2 'bad '$2': '$1; exit 1; }
	eval echo ok $2 $REDIR
	TNO=$((TNO + 1))
}

n() {
	[ $1 -ne 0 ] || { echo >&2 'bad '$2': '$1; exit 1; }
	eval echo ok $2 $REDIR
	TNO=$((TNO + 1))
}

tx() {
	eval $DP $2 diff t$1.b t$1.a t$1.$2.p $REDIR
	y $? $1.$2.1

	eval $DP patch t$1.a t$1.$2.p t$1.$2.r $REDIR
	y $? $1.$2.2
	cmp t$1.b t$1.$2.r
	y $? $1.$2.3

	eval $DP -f $2 patch t$1.a t$1.$2.p t$1.$2.r $REDIR
	y $? $1.$2.4
	cmp t$1.b t$1.$2.r
	y $? $1.$2.5
}

> t1.b
> t1.a
tx 1 -z
[ -n "$DPXZ" ] && tx 1 -J
tx 1 -R

echo > t2.b
echo > t2.a
tx 2 -z
[ -n "$DPXZ" ] && tx 2 -J
tx 2 -R

($SEQ 100; echo 0; $SEQ 100) > t3.b
($SEQ 100; echo 0; $SEQ 100) > t3.a
tx 3 -z
[ -n "$DPXZ" ] && tx 3 -J
tx 3 -R

($SEQ 100; echo 0; $SEQ 100) > t4.b
($SEQ 100; echo 1; $SEQ 100) > t4.a
tx 4 -z
[ -n "$DPXZ" ] && tx 4 -J
tx 4 -R

(echo 0; $SEQ 100; echo 1; $SEQ 100; echo 2; $SEQ 100; echo 4; $SEQ 100) > t5.b
(echo 1; $SEQ 100; echo 2; $SEQ 100; echo 3; $SEQ 100; echo 5) > t5.a
tx 5 -z
[ -n "$DPXZ" ] && tx 5 -J
tx 5 -R

if [ -f ../../lib/s-bsdiff.o ] && [ -f ../../lib/s-bspatch.o ]; then
	eval $DD if=../../lib/s-bsdiff.o of=t6.b $REDIR
	eval $DD if=../../lib/s-bspatch.o of=t6.a $REDIR
	tx 6 -z
	[ -n "$DPXZ" ] && tx 6 -J
	tx 6 -R
else
	echo >&2 'SKIP TESTS 6: cannot find my object files'
fi

if [ -c /dev/urandom ]; then
	eval $DD if=/dev/urandom bs=512 count=1 of=t7.b $REDIR
	eval $DD if=/dev/urandom bs=768 count=1 of=t7.a $REDIR
	tx 7 -z
	[ -n "$DPXZ" ] && tx 7 -J
	tx 7 -R

	eval $DD if=/dev/urandom bs=512 count=10 of=t8.b $REDIR
	eval $DD if=/dev/urandom bs=768 count=10 of=t8.a $REDIR
	tx 8 -z
	[ -n "$DPXZ" ] && tx 8 -J
	tx 8 -R
else
	echo >&2 'SKIP TESTS 7,8: no /dev/urandom'
fi

> t9.b
> t9.a
i=0
while [ $i -lt 7777 ]; do
	ix=$((i + 1))
	echo $i$i$i$i$i$i$i$i >> t9.b
	echo $ix$ix$ix$ix$ix$ix$ix >> t9.a
	i=$ix
done
tx 9 -z
[ -n "$DPXZ" ] && tx 9 -J
tx 9 -R

echo 'Ran '$TNO' tests'
)
exit $?

# s-sht-mode
