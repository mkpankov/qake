THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))

RES = build/res/b

.SHELLFLAGS = --target $@

all: $(RES)

build/aux/build/res/a.cmd: $(THIS_MAKEFILE) | $(DIRECTORY)
> echo 'echo A; echo I am a > $(call GET_TARGET_PATH,$@)' > $@

build/aux/build/res/b.cmd: build/aux/build/res/a.did_update $(THIS_MAKEFILE) | $(DIRECTORY)
> echo 'echo B; cat $< > $(call GET_TARGET_PATH,$@);
> echo I am b buzzz >> $(call GET_TARGET_PATH,$@)' > $@

build/res/%: \
  build/aux/build/res/%.cmd.did_update \
| build/aux/build/res/%.cmd \
  $(DIRECTORY)
> eval $$(cat $(firstword $|))

build/res/b: \
  build/aux/build/res/a.did_update \
| build/res/a

define GET_TARGET_PATH
$(patsubst build/aux/%.cmd,%,$1)
endef

define GET_CMD_PATH
$(patsubst %,build/aux/%.cmd,$1)
endef

define GET_CMD_DID_UPDATE
$(patsubst %,build/aux/%.cmd.did_update,$1)
endef

.PRECIOUS: build/aux/%.did_update

build/aux/%.did_update: \
build/aux/%.hash.new
>  if [ -f $(<:.hash.new=.hash.old) ];\
   then \
     if ! diff -q $< $(<:.hash.new=.hash.old) > /dev/null; \
     then \
         touch $@; \
     fi; \
   else \
     touch $@; \
   fi; \
   cp $< $(<:.hash.new=.hash.old)

.PRECIOUS: build/aux/%.cmd.did_update

build/aux/%.cmd.did_update: \
build/aux/%.cmd.hash.new
>  if [ -f $(<:.hash.new=.hash.old) ];\
   then \
     if ! diff -q $< $(<:.hash.new=.hash.old) > /dev/null; \
     then \
         touch $@; \
     fi; \
   else \
     touch $@; \
   fi; \
   cp $< $(<:.hash.new=.hash.old)

.PRECIOUS: build/aux/%.hash.new

build/aux/%.hash.new: \
%
> echo Hashing $< to $@
> shasum $< > $@

build/aux/%.hash.new: \
build/aux/%
> echo Hashing $< to $@
> shasum $< > $@
