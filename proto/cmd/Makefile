RES = build/res/b

all: $(RES)

build/aux/build/res/a.cmd.new: Makefile | $(DIRECTORY)
> echo 'echo A; echo I am a\nWell aaaa > $(call GET_TARGET_PATH,$@)' > $@

build/aux/build/res/b.cmd.new: Makefile build/aux/build/res/a.did_update | $(DIRECTORY)
> echo 'echo B; cat $< > $(call GET_TARGET_PATH,$@);
> echo And I am b >> $(call GET_TARGET_PATH,$@)' > $@

build/res/a: \
  build/aux/build/res/a.cmd.did_update \
| build/aux/build/res/a.cmd.new \
  $(DIRECTORY)
> eval $$(cat $(firstword $|))

build/res/b: \
  build/aux/build/res/b.cmd.did_update \
  build/aux/build/res/a.did_update \
| build/aux/build/res/b.cmd.new \
$(DIRECTORY)
> eval $$(cat $(firstword $|))

build/res/%: \
  $$(call GET_CMD_DID_UPDATE,$$@) \
| $$(call GET_CMD_PATH,$$@) \
  $(DIRECTORY)
> eval $$(cat $(firstword $|))

define GET_TARGET_PATH
$(patsubst build/aux/%.cmd.new,%,$1)
endef

define GET_CMD_PATH
$(patsubst %,build/aux/%.cmd.new,$1)
endef

define GET_CMD_DID_UPDATE
$(patsubst %,build/aux/%.cmd.did_update,$1)
endef

build/aux/build/res/a.did_update: \
build/aux/build/res/a.hash.new
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


build/aux/build/res/a.hash.new: \
build/res/a
> echo Hashing $< to $@
> shasum $< > $@

build/aux/build/res/a.cmd.new.hash.new: \
build/aux/build/res/a.cmd.new
> echo Hashing $< to $@
> shasum $< > $@

build/aux/build/res/b.hash.new: \
build/res/b
> echo Hashing $< to $@
> shasum $< > $@

build/aux/build/res/b.cmd.new.hash.new: \
build/aux/build/res/b.cmd.new
> echo Hashing $< to $@
> shasum $< > $@

build/aux/build/res/b.cmd.did_update: \
build/aux/build/res/b.cmd.new.hash.new
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


build/aux/build/res/a.cmd.did_update: \
build/aux/build/res/a.cmd.new.hash.new
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


build/aux/%.hash_new: \
build/res/%
> echo Hashing $< to $@
> shasum $< > $@
