#!/bin/sh -
#@ s-bsdipa test

: ${KEEP_TESTS:=}
: ${PEBCAK:=} # some more output
: ${REDIR:='>/dev/null 2>/dev/null'}
: ${REDIROK:=} #'>/dev/null 2>/dev/null'}

: ${DP:=../s-bsdipa}
: ${DP32:=}
: ${DPBZ2:=}
: ${DPXZ:=}
: ${DPZSTD:=}

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
	eval echo ok $2 $REDIROK
	TNO=$((TNO + 1))
}

n() {
	[ $1 -ne 0 ] || { echo >&2 'bad '$2': '$1; exit 1; }
	eval echo ok $2 $REDIROK
	TNO=$((TNO + 1))
}

tx() {
	eval $DP $2 $3 diff t$1.b t$1.a t$1.$2.p $REDIR
	y $? $1.$2.1

	eval $DP patch t$1.a t$1.$2.p t$1.$2.r$4 $REDIR
	y $? $1.$2.2
	eval cmp t$1.b t$1.$2.r$4 $REDIR
	y $? $1.$2.3

	eval $DP -f $2 patch t$1.a t$1.$2.p t$1.$2.r$4 $REDIR
	y $? $1.$2.4
	eval cmp t$1.b t$1.$2.r$4 $REDIR
	y $? $1.$2.5

	# -H
	eval $DP $2 $3 -H diff t$1.b t$1.a t$1.$2.pH $REDIR
	y $? $1.$2.6

	eval $DP $2 -H patch t$1.a t$1.$2.pH t$1.$2.rH$4 $REDIR
	y $? $1.$2.7
	eval cmp t$1.b t$1.$2.rH$4 $REDIR
	y $? $1.$2.8

	eval $DP -Hf $2 patch t$1.a t$1.$2.pH t$1.$2.rH$4 $REDIR
	y $? $1.$2.9
	eval cmp t$1.b t$1.$2.rH$4 $REDIR
	y $? $1.$2.10

	eval cmp t$1.$2.p t$1.$2.pH $REDIR
	n $? $1.$2.11
}

> t1.b
> t1.a
tx 1 -R
tx 1 -z
[ -n "$DPBZ2" ] && tx 1 -j
[ -n "$DPXZ" ] && tx 1 -J
[ -n "$DPZSTD" ] && tx 1 -Z

> t2a.b
> t2a.a
i=0
while [ $i -le 100 ]; do
	ix=
	[ $i -ne 0 ] && ix=$i
	echo "$ix" >> t2a.a

	tx 2a -Rf '' "$i"
	tx 2a -zf -3 $i
	[ -n "$DPBZ2" ] && tx 2a -jf -3 $i
	[ -n "$DPXZ" ] && tx 2a -Jf -3 $i
	[ -n "$DPZSTD" ] && tx 2a -Zf -3 $i

	i=$((i + 1))
done

> t2b.b
> t2b.a
i=0
while [ $i -le 100 ]; do
	ix=
	[ $i -ne 0 ] && ix=$i
	echo "$ix" >> t2b.b

	tx 2b -Rf '' $i
	tx 2b -zf -4 $i
	[ -n "$DPBZ2" ] && tx 2b -jf -4 $i
	[ -n "$DPXZ" ] && tx 2b -Jf -4 $i
	[ -n "$DPZSTD" ] && tx 2b -Zf -4 $i

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

	tx 2c -Rf '' $i
	tx 2c -zf -1 $i
	[ -n "$DPBZ2" ] && tx 2c -jf -1 $i
	[ -n "$DPXZ" ] && tx 2c -Jf -1 $i
	[ -n "$DPZSTD" ] && tx 2c -Zf -1 $i

	i=$((i + 1))
done

($SEQ 100; echo 0; $SEQ 100) > t3a.b
($SEQ 100; echo 0; $SEQ 100) > t3a.a
tx 3a -R
tx 3a -z
[ -n "$DPBZ2" ] && tx 3a -j
[ -n "$DPXZ" ] && tx 3a -J
[ -n "$DPZSTD" ] && tx 3a -Z

($SEQ 100; echo 0; $SEQ 100) > t3b.b
($SEQ 100; echo 1; $SEQ 100) > t3b.a
tx 3b -R
tx 3b -z
[ -n "$DPBZ2" ] && tx 3b -j
[ -n "$DPXZ" ] && tx 3b -J
[ -n "$DPZSTD" ] && tx 3b -Z

($SEQ 100; echo 1; $SEQ 100) > t3c.b
($SEQ 100; echo 0; $SEQ 100) > t3c.a
tx 3c -R
tx 3c -z
[ -n "$DPBZ2" ] && tx 3c -j
[ -n "$DPXZ" ] && tx 3c -J
[ -n "$DPZSTD" ] && tx 3c -Z

(echo 0; $SEQ 200) > t4a.a
(echo 0; $SEQ 200) > t4a.b
tx 4a -R
tx 4a -z
[ -n "$DPBZ2" ] && tx 4a -j
[ -n "$DPXZ" ] && tx 4a -J
[ -n "$DPZSTD" ] && tx 4a -Z

(echo 0; $SEQ 200) > t4b.a
(echo 1; $SEQ 200) > t4b.b
tx 4b -R
tx 4b -z
[ -n "$DPBZ2" ] && tx 4b -j
[ -n "$DPXZ" ] && tx 4b -J
[ -n "$DPZSTD" ] && tx 4b -Z

(echo 1; $SEQ 200) > t4c.a
(echo 0; $SEQ 200) > t4c.b
tx 4c -R
tx 4c -z
[ -n "$DPBZ2" ] && tx 4c -j
[ -n "$DPXZ" ] && tx 4c -J
[ -n "$DPZSTD" ] && tx 4c -Z

($SEQ 200; echo 0) > t4d.a
($SEQ 200; echo 0) > t4d.b
tx 4d -R
tx 4d -z
[ -n "$DPBZ2" ] && tx 4d -j
[ -n "$DPXZ" ] && tx 4d -J
[ -n "$DPZSTD" ] && tx 4d -Z

($SEQ 200; echo 0) > t4e.a
($SEQ 200; echo 1) > t4e.b
tx 4e -R
tx 4e -z
[ -n "$DPBZ2" ] && tx 4e -j
[ -n "$DPXZ" ] && tx 4e -J
[ -n "$DPZSTD" ] && tx 4e -Z

($SEQ 200; echo 1) > t4f.a
($SEQ 200; echo 0) > t4f.b
tx 4f -R
tx 4f -z
[ -n "$DPBZ2" ] && tx 4f -j
[ -n "$DPXZ" ] && tx 4f -J
[ -n "$DPZSTD" ] && tx 4f -Z

# Note: t100 may use t5 tests
(echo 0; $SEQ 100; echo 1; $SEQ 100; echo 2; $SEQ 100; echo 4; $SEQ 100) > t5.b
(echo 1; $SEQ 100; echo 2; $SEQ 100; echo 3; $SEQ 100; echo 5) > t5.a
tx 5 -R
tx 5 -z
[ -n "$DPBZ2" ] && tx 5 -j
[ -n "$DPXZ" ] && tx 5 -J
[ -n "$DPZSTD" ] && tx 5 -Z

# Note: t100 may use t6 tests
if [ -f ../../lib/s-bsdiff.o ] && [ -f ../../lib/s-bspatch.o ]; then
	eval $DD if=../../lib/s-bsdiff.o of=t6.b $REDIR
	eval $DD if=../../lib/s-bspatch.o of=t6.a $REDIR
	tx 6 -R
	tx 6 -z -5
	[ -n "$DPBZ2" ] && tx 6 -j -5
	[ -n "$DPXZ" ] && tx 6 -J -5
	[ -n "$DPZSTD" ] && tx 6 -Z -5
else
	echo >&2 'SKIP TESTS 6: cannot find my object files'
fi

if [ -c /dev/urandom ]; then
	eval $DD if=/dev/urandom bs=512 count=1 of=t7.b $REDIR
	eval $DD if=/dev/urandom bs=768 count=1 of=t7.a $REDIR
	tx 7 -R
	tx 7 -z
	[ -n "$DPBZ2" ] && tx 7 -j
	[ -n "$DPXZ" ] && tx 7 -J
	[ -n "$DPZSTD" ] && tx 7 -Z

	eval $DD if=/dev/urandom bs=512 count=10 of=t8.a $REDIR
	eval $DD if=/dev/urandom bs=768 count=10 of=t8.b $REDIR
	tx 8 -R
	tx 8 -z -6
	[ -n "$DPBZ2" ] && tx 8 -j -6
	[ -n "$DPXZ" ] && tx 8 -J -6
	[ -n "$DPZSTD" ] && tx 8 -Z -6
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
tx 9 -R
tx 9 -z
[ -n "$DPBZ2" ] && tx 9 -j
[ -n "$DPXZ" ] && tx 9 -J
[ -n "$DPZSTD" ] && tx 9 -Z

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
		tx 10 -Rf '' $i
		tx 10 -zf -1 $i
		[ -n "$DPBZ2" ] && tx 10 -jf -1 $i
		[ -n "$DPXZ" ] && tx 10 -Jf -1 $i
		[ -n "$DPZSTD" ] && tx 10 -Zf -1 $i
	fi

	i=$ix
done

# Try -1 .. -9; uses t5 or t6 tests
[ -f t6.a ] && i=6 || i=5
$DD if=t$i.b of=t100.a >/dev/null 2>&1
$DD if=t$i.a of=t100.b >/dev/null 2>&1
i=1
while [ $i -le 9 ]; do
	tx 100 -zf -$i $i
	[ -n "$DPBZ2" ] && tx 100 -jf -$i $i
	[ -n "$DPXZ" ] && tx 100 -Jf -$i $i
	[ -n "$DPZSTD" ] && tx 100 -Zf -$i $i
	[ -n "$PEBCAK" ] && (set +o noglob && echo At level $i && ls -l t100.*.p*)
	i=$((i + 1))
done

echo 'Ran '$TNO' tests'
)
exit $?

# s-sht-mode
