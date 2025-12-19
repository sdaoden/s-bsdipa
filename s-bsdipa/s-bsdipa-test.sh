#!/bin/sh -
#@ s-bsdipa test (simple: full test part of BsDiPa perl XS module)

: ${KEEP_TESTS:=}
: ${REDIR:='>/dev/null 2>/dev/null'}

: ${DP:=../s-bsdipa}
: ${DP32:=}
: ${DPXZ:=}
: ${DPBZ2:=}

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
	eval $DP $2 $3 diff t$1.b t$1.a t$1.$2.p $REDIR
	y $? $1.$2.1

	eval $DP patch t$1.a t$1.$2.p t$1.$2.r$4 $REDIR
	y $? $1.$2.2
	cmp t$1.b t$1.$2.r$4
	y $? $1.$2.3

	eval $DP -f $2 patch t$1.a t$1.$2.p t$1.$2.r$4 $REDIR
	y $? $1.$2.4
	cmp t$1.b t$1.$2.r$4
	y $? $1.$2.5
}

> t1.b
> t1.a
tx 1 -z
[ -n "$DPXZ" ] && tx 1 -J
[ -n "$DPBZ2" ] && tx 1 -j
tx 1 -R

> t2a.b
> t2a.a
i=0
while [ $i -le 100 ]; do
	ix=
	[ $i -ne 0 ] && ix=$i
	echo "$ix" >> t2a.a

	tx 2a -zf -3 $i
	[ -n "$DPXZ" ] && tx 2a -Jf -3 $i
	[ -n "$DPBZ2" ] && tx 2a -jf -3 $i
	tx 2a -Rf '' "$i"

	i=$((i + 1))
done

> t2b.b
> t2b.a
i=0
while [ $i -le 100 ]; do
	ix=
	[ $i -ne 0 ] && ix=$i
	echo "$ix" >> t2b.b

	tx 2b -zf -9 $i
	[ -n "$DPXZ" ] && tx 2b -Jf -4 $i
	[ -n "$DPBZ2" ] && tx 2b -jf -9 $i
	tx 2b -Rf '' $i

	i=$((i + 1))
done

> t2c.b
> t2c.a
i=0
while [ $i -le 100 ]; do
	ix=
	[ $i -ne 0 ] && ix=$i
	echo "$ix" >> t2c.a
	echo "$ix" >> t2c.b

	tx 2c -zf -1 $i
	[ -n "$DPXZ" ] && tx 2c -Jf -1 $i
	[ -n "$DPBZ2" ] && tx 2c -jf -1 $i
	tx 2c -Rf '' $i

	i=$((i + 1))
done

($SEQ 100; echo 0; $SEQ 100) > t3.b
($SEQ 100; echo 0; $SEQ 100) > t3.a
tx 3 -z
[ -n "$DPXZ" ] && tx 3 -J
[ -n "$DPBZ2" ] && tx 3 -j
tx 3 -R

($SEQ 100; echo 0; $SEQ 100) > t4.b
($SEQ 100; echo 1; $SEQ 100) > t4.a
tx 4 -z
[ -n "$DPXZ" ] && tx 4 -J
[ -n "$DPBZ2" ] && tx 4 -j
tx 4 -R

(echo 0; $SEQ 100; echo 1; $SEQ 100; echo 2; $SEQ 100; echo 4; $SEQ 100) > t5.b
(echo 1; $SEQ 100; echo 2; $SEQ 100; echo 3; $SEQ 100; echo 5) > t5.a
tx 5 -z
[ -n "$DPXZ" ] && tx 5 -J
[ -n "$DPBZ2" ] && tx 5 -j
tx 5 -R

if [ -f ../../lib/s-bsdiff.o ] && [ -f ../../lib/s-bspatch.o ]; then
	eval $DD if=../../lib/s-bsdiff.o of=t6.b $REDIR
	eval $DD if=../../lib/s-bspatch.o of=t6.a $REDIR
	tx 6 -z
	[ -n "$DPXZ" ] && tx 6 -J -6
	[ -n "$DPBZ2" ] && tx 6 -j -6
	tx 6 -R
else
	echo >&2 'SKIP TESTS 6: cannot find my object files'
fi

if [ -c /dev/urandom ]; then
	eval $DD if=/dev/urandom bs=512 count=1 of=t7.b $REDIR
	eval $DD if=/dev/urandom bs=768 count=1 of=t7.a $REDIR
	tx 7 -z
	[ -n "$DPXZ" ] && tx 7 -J
	[ -n "$DPBZ2" ] && tx 7 -j
	tx 7 -R

	eval $DD if=/dev/urandom bs=512 count=10 of=t8.a $REDIR
	eval $DD if=/dev/urandom bs=768 count=10 of=t8.b $REDIR
	tx 8 -z -9
	[ -n "$DPXZ" ] && tx 8 -J -9
	[ -n "$DPBZ2" ] && tx 8 -j -9
	tx 8 -R
else
	echo >&2 'SKIP TESTS 7,8: no /dev/urandom'
fi

> t9.b
> t9.a
i=0
while [ $i -lt 7777 ]; do
	ix=$((i + 1))
	echo "$i$i$i$i$i$i$i$i" >> t9.b
	echo "$ix$ix$ix$ix$ix$ix$ix" >> t9.a
	i=$ix
done
tx 9 -z
[ -n "$DPXZ" ] && tx 9 -J
[ -n "$DPBZ2" ] && tx 9 -j
tx 9 -R

# (try increase ctrl dat)
> t10.b
> t10.a
i=0
while [ $i -le 1000 ]; do
	ix=$((i + 1))
	echo "$ix" >> t10.a
	iy=
	[ $((i % 5)) -eq 0 ] && ix="$ix " iy=y
	echo "$ix" >> t10.b

	if [ -n "$iy" ]; then
		tx 10 -zf -1 $i
		[ -n "$DPXZ" ] && tx 10 -Jf -1 $i
		[ -n "$DPBZ2" ] && tx 10 -jf -1 $i
		tx 10 -Rf '' $i
	fi

	i=$ix
done

echo 'Ran '$TNO' tests'
)
exit $?

# s-sht-mode
