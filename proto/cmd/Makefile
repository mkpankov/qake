RES = build/res/b

all: $(RES)

build/aux/build/res/a.cmd.new: Makefile | $(DIRECTORY)
> echo 'echo A; echo I am a\nWell aaaa azaza > $(call GET_TARGET_PATH,$@)' > $@

build/aux/build/res/b.cmd.new: build/aux/build/res/a.did_update Makefile | $(DIRECTORY)
> echo 'echo B; cat $< > $(call GET_TARGET_PATH,$@);
> echo And I am b buzzz >> $(call GET_TARGET_PATH,$@)' > $@

build/res/%: \
  build/aux/build/res/%.cmd.did_update \
| build/aux/build/res/%.cmd.new \
  $(DIRECTORY)
> eval $$(cat $(firstword $|))

build/res/b: \
  build/aux/build/res/a.did_update \
| build/res/a

define GET_TARGET_PATH
$(patsubst build/aux/%.cmd.new,%,$1)
endef

define GET_CMD_PATH
$(patsubst %,build/aux/%.cmd.new,$1)
endef

define GET_CMD_DID_UPDATE
$(patsubst %,build/aux/%.cmd.did_update,$1)
endef

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


build/aux/%.cmd.did_update: \
build/aux/%.cmd.new.hash.new
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


build/aux/%.hash.new: \
%
> echo Hashing $< to $@
> shasum $< > $@

build/aux/%.hash.new: \
build/aux/%
> echo Hashing $< to $@
> shasum $< > $@
