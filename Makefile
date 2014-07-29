# By default, when target is not specified, Make will build the one
# specified first in the Makefile.
# Confusing, let's disable and make it always build 'all' target.
# Special variables:
# https://www.gnu.org/software/make/manual/html_node/Special-Variables.html
.DEFAULT_GOAL := all

# Second expansion is used in this solution to seamlessly create
# directories for target files.
# Secondary expansion:
# https://www.gnu.org/software/make/manual/html_node/Secondary-Expansion.html
.SECONDEXPANSION:

# We'll introduce some convenience wrappers to ease debugging of
# our new features. They'll be enabled or disabled by this switch.
# Variables can be overridden by command-line arguments, like this:
# make BUILD_DEBUG=y
# More info:
# https://www.gnu.org/software/make/manual/html_node/Overriding.html
BUILD_DEBUG := n

# Function: Wrap a command. Print short description, output command only if it failed.
# Multi-line definitions:
# https://www.gnu.org/software/make/manual/html_node/Multi_002dLine.html#Multi_002dine
# Eval built-in function:
# https://www.gnu.org/software/make/manual/html_node/Eval-Function.html#Eval-Function
define RUN
$(call FUNCTION_DEBUG_HEADER,$0)
$(call FUNCTION_DEBUG,DESCRIPTION,$1)
$(call FUNCTION_DEBUG,COMMAND,$2)
@echo $(DESCRIPTION); if ! $(COMMAND); then echo '[ERROR] Failed command:\n$(COMMAND)'; fi
endef

# Function: Print header of function call trace.
# Conditional parts of Makefiles:
# https://www.gnu.org/software/make/manual/html_node/Conditional-Syntax.html
ifeq ($(BUILD_DEBUG),y)
define FUNCTION_DEBUG_HEADER
$(eval FUNCTION_NAME := $1)
$(info Calling function $(FUNCTION_NAME):)
endef
endif

# Function: Print value of function parameter.
ifeq ($(BUILD_DEBUG),y)
define FUNCTION_DEBUG
$(eval NAME = $1)
$(eval VALUE = $2)
$(eval $(NAME) = $(VALUE))
$(info --$(NAME): $(VALUE))
endef
else
define FUNCTION_DEBUG
$(eval $1 = $2)
endef
endif

# Function: Print a value as it's being evaluated.
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

# Directory creation:
# Use secondary expansion to get name of directory of target.
# We insert additional dollar sign to prevent expansion of @D automatic variable.
# @D contains directory part of the path to the target file.
# More on this:
# https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
define DIRECTORY
$$(@D)/.directory.marker
endef

# Implicit rule for all marker files.
# We can't write a rule for directories themselves,
# since Make only matches targets by file paths,
# and path to directory looks just like path to any file.
# I.e., we would have to change first line to
# %:
# which obviously wouldn't work, since it would match any file.
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
	$$(call RUN,GCC $$(@F),gcc $$< -o $$@ $4 -c -MD -MF $$@.d -MP)

$(call TRACE1,DEP_$2 := $$(OBJ_$2:=.d))

-include $$(DEP_$2)

PROGRAM_$2 := $(BUILD_DIR)/$2/$2

$$(PROGRAM_$2): $(OBJ_$2)
	$$(call RUN,GCC $$(@F),gcc $$^ -o $$@ $5)

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
