all: a

a.cmd:
	echo 'echo I am a > $(@:.cmd=)' > $@

%: %.cmd
	eval $$(cat $<)
