all: build/res/b

build/aux/a.cmd: | $(DIRECTORY)
> echo 'echo I am a > $(call GET_TARGET_PATH,$@)' > $@

build/aux/b.cmd: build/res/a | $(DIRECTORY)
> echo 'cat $< > $(call GET_TARGET_PATH,$@);
> echo And I am b >> $(call GET_TARGET_PATH,$@)' > $@

build/res/%: \
  $$(call GET_CMD_PATH,$$@) \
| $(DIRECTORY)
> eval $$(cat $<)

build/res/b: | $(DIRECTORY)

define GET_TARGET_PATH
$(patsubst build/aux/%.cmd,build/res/%,$1)
endef

define GET_CMD_PATH
$(patsubst build/res/%,build/aux/%.cmd,$1)
endef
