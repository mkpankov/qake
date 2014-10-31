THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))

RES = build/res/b

.SHELLFLAGS = --target $@

all: $(RES)

CMD_DIR := build/aux/build/res
DU_DIR := build/aux/build/res
RES_DIR := build/res
AUX_DIR := build/aux

$(CMD_DIR)/a.cmd:
> echo 'echo A; echo I am a > $(call GET_TARGET_PATH,$@)' > $@

$(CMD_DIR)/b.cmd: $(DU_DIR)/a.did_update
> echo 'echo B; cat $< > $(call GET_TARGET_PATH,$@);
> echo I am b buzzz >> $(call GET_TARGET_PATH,$@)' > $@

COMMANDS := $(CMD_DIR)/a.cmd $(CMD_DIR)/b.cmd

$(COMMANDS): $(THIS_MAKEFILE) | $(DIRECTORY)

$(RES_DIR)/%: \
  $(DU_DIR)/%.cmd.did_update \
| $(CMD_DIR)/%.cmd \
  $(DIRECTORY)
> eval $$(cat $(firstword $|))

$(RES_DIR)/b: \
  $(DU_DIR)/a.did_update \
| $(RES_DIR)/a

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

$(AUX_DIR)/%.did_update: \
$(AUX_DIR)/%.hash.new
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

$(AUX_DIR)/%.cmd.did_update: \
$(AUX_DIR)/%.cmd.hash.new
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

$(AUX_DIR)/%.hash.new: \
%
> echo Hashing $< to $@
> shasum $< > $@

$(AUX_DIR)/%.hash.new: \
$(AUX_DIR)/%
> echo Hashing $< to $@
> shasum $< > $@
