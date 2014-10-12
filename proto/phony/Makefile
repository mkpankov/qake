all: c d

c: b
	echo c
	echo c > $@
	cat $< >> $@

b: a
	echo b
	echo b > $@
	cat $< >> $@

a: Makefile
	echo a
	echo a > $@

d: c
	echo d
	echo d > $@
	cat $< >> $@
