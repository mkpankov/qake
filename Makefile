# By default, when target is not specified, Make will build the one
#   specified first in the Makefile.
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
#   our new features. They'll be enabled or disabled by this switch.
# Variables can be overridden by command-line arguments, like this:
# make BUILD_DEBUG=y
# More info:
# https://www.gnu.org/software/make/manual/html_node/Overriding.html
BUILD_DEBUG := n

.RECIPEPREFIX = >

# Function: Print header of function call trace.
# Conditional parts of Makefiles:
# https://www.gnu.org/software/make/manual/html_node/Conditional-Syntax.html
# Undefined functions and variables are safe to call and reference -
#   they just expand to empty string.
# $1 is first parameter of function as passed by Make.
# $0 is function name, and parameters are accessed as $2, $3 and so on.
# $(eval) evaluates what is being passed to it as syntax of Makefile.
# It allows one to implement "local variables".
# Such variables will-be re-evaluated on each expansion of function call.
# Usual variables (like BUILD_DEBUG above) are global and
#   are visible from everywhere.
# Should we not use $(eval) here, FUNCTION_NAME would be defined by Make once
#   when parsing the Makefile and not reset on each function call.
# More on $(eval) here:
# http://www.gnu.org/software/make/manual/html_node/Eval-Function.html#Eval-Function
#
# $(info) Outputs the given string to stdout.
# http://www.gnu.org/software/make/manual/html_node/Make-Control-Functions.html#Make-Control-Functions
ifeq ($(BUILD_DEBUG),y)
define FUNCTION_DEBUG_HEADER
$(eval FUNCTION_NAME := $1)
$(info Calling function $(FUNCTION_NAME):)
endef
endif

# Function: print variable assignment if build debugging is enabled.
# You might be wondering why I decided to start parameter trace line with '--'.
# It's because Make is very inconsistent about treatment of spaces in
#   its' syntax. For example, $(info) will strip everything in front
#   of first function parameters.
# To not go deeper inside Make's peculiar space and escaping rules,
#   we'll just sidestep the problem by using dashes.
ifeq ($(BUILD_DEBUG),y)
define MAYBE_PRINT_VARIABLE_ASSIGNMENT
$(eval NAME := $1)
$(eval VALUE := $2)
$(info --$(NAME): $(VALUE))
endef
endif

ifeq ($(BUILD_DEBUG),y)
define PRINT1
$(info $1)
endef
endif

# Function: Print value of function parameter.
# $(call) function performs expansion to the value of function definition,
#   with $0, $1 and so on in the function definition being replaced
#   with actual parameters.
define DEFINE_LOCAL_VARIABLE
$(eval FUNCTION := $(strip $1))
$(eval NAME := $(strip $2))
$(eval VALUE := $(strip $3))
$(call TRACE1,$(FUNCTION)_$(NAME) := $(VALUE))
endef

define REFERENCE_LOCAL_VARIABLE
$(strip \
$(eval FUNCTION := $1)
$(eval NAME := $2)
$($(FUNCTION)_$(NAME))
)
endef

define RLV
$(call REFERENCE_LOCAL_VARIABLE,$1,$2)
endef

define &
$(call RLV,$1,$2)
endef

# Function: Print a value as it's being evaluated.
define TRACE1
$(call PRINT1,$1)
$(eval $1)
endef

# Directory creation:
# Use secondary expansion to get name of directory of target.
# We insert additional dollar sign to prevent expansion of $(@D) automatic variable.
# $(@D) contains directory part of the path to the target file.
# But it's so only when execution is inside of recipe part
#   of target.
# That is, automatic variables like $@, $(@D) and so on, are target-local.
# More on this:
# https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
define DIRECTORY
$$(@D)/.directory.marker
endef

# Implicit rule for all marker files.
# % is called stem of pattern rule.
# It matches the part of file path of target.
# I.e. if target is a/b/c/.directory.marker,
#   % will match a/b/c/.
# We can't write a rule for directories themselves,
# since Make only matches targets by file paths,
# and path to directory looks just like path to any file.
# I.e., we would have to change first line to
# %:
# which obviously wouldn't work, since it would match any file.
# In case you think about rule matching on last slash and trying
#   to create directories themselves: don't.
# Slashes processing rules are no less obscure than space processing rules,
#   and there is a known bug during moving from Make 3.81 to Make 3.82
#   due to changes in processing of paths with double slashes.
%/.directory.marker:
> $(call RUN,MKDIR $(@D),mkdir -p $(@D))
> @touch $@

# We're going to put all the built stuff under 'build' directory.
# It allows to write easier .gitignore file and not accidentally commit
#   built files to version control.
# It also makes implementation of 'clean' target trivial:
#   just remove entire directory.
BUILD_DIR := build
# This directory will hold sources.
# Good build system mirrors sources structure in build directory --
#   it allows to manage pattern rules easier.
SRC_DIR := src

# Function: Wrap a command.
# Print short description, output command only if it failed.
# Last line is the shell command that will be invoked when making some target
#   that uses RUN in the recipe.
# '@' in front of it is to disable echoing of the command itself.
define RUN
$(strip \
$(call FUNCTION_DEBUG_HEADER,$0)
$(call DEFINE_LOCAL_VARIABLE,$0,DESCRIPTION,$1)
$(call DEFINE_LOCAL_VARIABLE,$0,COMMAND,$2)
)
@echo $(call &,$0,DESCRIPTION); if ! $(call &,$0,COMMAND);\
  then echo '[ERROR] Failed command:\n$(call &,$0,COMMAND)'; fi
endef

# Function: Define build of a program.
define PROGRAM
$(call FUNCTION_DEBUG_HEADER,$0)
$(call DEFINE_LOCAL_VARIABLE,$0,SOURCE_NAME,$1)
$(call DEFINE_LOCAL_VARIABLE,$0,BUILT_NAME,$2)
$(call DEFINE_LOCAL_VARIABLE,$0,SRC,$3)
$(call DEFINE_LOCAL_VARIABLE,$0,CFLAGS,$4)
$(call DEFINE_LOCAL_VARIABLE,$0,LDFLAGS,$5)
$(call DEFINE_LOCAL_VARIABLE,$0,LDLIBS,$6)

$(call TRACE1,OBJ_$(call &,$0,BUILT_NAME) := $(strip \
  $(patsubst $(SRC_DIR)/$(call &,$0,SOURCE_NAME)%,\
             $(BUILD_DIR)/$(call &,$0,BUILT_NAME)/%.o,\
             $(call &,$0,SRC))))

$(call TRACE1,$(OBJ_$(call &,$0,BUILT_NAME)): \
  $(BUILD_DIR)/$(call &,$0,BUILT_NAME)/%.o: \
  $(SRC_DIR)/$(call &,$0,SRC_NAME)/% \
| $$(DIRECTORY)
> $$(COMPILE_OBJECT))
$(OBJ_$2): CFLAGS := $(call &,$0,CFLAGS)

$(call TRACE1,DEP_$(call &,$0,BUILT_NAME) := $(strip \
  $$(OBJ_$(call &,$0,BUILT_NAME):=.d)))

-include $$(DEP_$(call &,$0,BUILT_NAME))

$(call TRACE1,PROGRAM_$(call &,$0,BUILT_NAME) := $(strip \
  $(BUILD_DIR)/$(call &,$0,BUILT_NAME)/$(call &,$0,BUILT_NAME)))

$$(PROGRAM_$(call &,$0,BUILT_NAME)): $(OBJ_$(call &,$0,BUILT_NAME))
> $$(LINK_PROGRAM)

$$(PROGRAM_$(call &,$0,BUILT_NAME)): LDFLAGS := $(call &,$0,LDFLAGS)
$$(PROGRAM_$(call &,$0,BUILT_NAME)): LDLIBS := $(call &,$0,LDLIBS)

ALL += $$(PROGRAM_$(call &,$0,BUILT_NAME))
endef

define COMPILE_OBJECT
$(call RUN,GCC $$(@F),gcc $$(CFLAGS) $$< -o $$@ -c -MD -MF $$@.d -MP)
endef

define LINK_PROGRAM
$(call RUN,GCC $$(@F),gcc $$(LDFLAGS) $$^ -o $$@ $$(LDLIBS))
endef

SRC_CIRCLE := $(wildcard src/*.c)
LDLIBS_CIRCLE := \
  -ldl \
  -lpthread \
  -rdynamic \

$(eval $(call PROGRAM,\
              ,\
              circled,\
              $(SRC_CIRCLE),\
              $(CFLAGS_CIRCLE),\
              $(LDFLAGS_CIRCLE),\
              $(LDLIBS_CIRCLE)))

.PHONY: all
all: $(ALL)

.PHONY: clean
clean:
> $(call RUN,RM BUILD_DIR,rm -rf $(BUILD_DIR))
