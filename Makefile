.DEFAULT_GOAL := all

.SECONDEXPANSION:

BUILD_DEBUG := n

# 1 - description
# 2 - command
# 3 - where to write command to
define RUN
@echo $1; if ! $2; then echo '[ERROR] Failed command:\n$2'; fi
endef

# 1 - function name
ifeq ($(BUILD_DEBUG),y)
define FUNCTION_DEBUG_HEADER
$(info Calling function $1:)
endef
endif

# 1 - parameter name
# 2 - parameter value
ifeq ($(BUILD_DEBUG),y)
define FUNCTION_DEBUG
$(info --$1: $2)
endef
endif

ifeq ($(BUILD_DEBUG),y)
define TRACE1
$(info $1)
$(eval $1)
endef
else
define TRACE1
$(eval $1)
endef
endif

define DIRECTORY
$$(@D)/.directory.marker
endef

%/.directory.marker:
	$(call RUN,MKDIR $(@D),mkdir -p $(@D))
	@touch $@

BUILD_DIR := build
SRC_DIR := src

define PROGRAM
$(call FUNCTION_DEBUG_HEADER,$0)
$(call FUNCTION_DEBUG,source name,$1)
$(call FUNCTION_DEBUG,built name,$2)
$(call FUNCTION_DEBUG,source files,$3)
$(call FUNCTION_DEBUG,CFLAGS,$4)
$(call FUNCTION_DEBUG,LDFLAGS,$5)

$(call TRACE1,OBJ_$2 := $(patsubst $(SRC_DIR)/$1%,$(BUILD_DIR)/$2/%.o,$3))

$(OBJ_$2): $(BUILD_DIR)/$2/%.o: \
  $(SRC_DIR)/$1/% \
| $$(DIRECTORY)
	$(call RUN,GCC $$(@F),gcc $$< -o $$@ $4 -c -MD -MF $$@.d -MP)

$(call TRACE1,DEP_$2 := $$(OBJ_$2:=.d))

-include $$(DEP_$2)

PROGRAM_$2 := $(BUILD_DIR)/$2/$2

$$(PROGRAM_$2): $(OBJ_$2)
	$(call RUN,GCC $$(@F),gcc $$^ -o $$@ $5)

ALL += $$(PROGRAM_$2)
endef

SRC := $(wildcard src/*.c)
LDFLAGS := \
  -ldl \
  -lpthread \
  -rdynamic \

$(eval $(call PROGRAM,,circled,$(SRC),$(CFLAGS),$(LDFLAGS)))

.PHONY: all
all: $(ALL)

.PHONY: clean
clean:
	$(call RUN,RM BUILD_DIR,rm -rf $(BUILD_DIR))
