a:
	echo ZZZ > $@
	cp a.hash.new a.hash.old

b: a.hash.do_update
	cat a > $@
	echo YYY >> $@


a.hash.do_update: a.hash.new
	if [ -f a.hash.old ]; \
	then \
	  if ! diff -q a.hash.new a.hash.old; \
	  then \
	    touch $@; \
	  fi; \
	else \
	  touch $@; \
	fi


a.hash.new: a
	shasum a > $@
