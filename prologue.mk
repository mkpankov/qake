# This is to precisely track what's going on.
# It's used by Make as command interpreter.
# By default, Make uses /bin/sh as interpreter for recipes.
# We overload that to pass everything through our script.
SHELL := $(QAKE_INCLUDE_DIR)/relay.sh

# This was to inhibit hash checking for phony targets.
# TODO: Remove this setting.
.SHELLFLAGS := --phony

# Check version of used Make.
# Most of the stuff mentioned here can actually be used in at least GNU Make 3.81.
#
# $(if) function checks first parameter:
#   if it expands to non-empty string, $(if) expands to the second parameter.
#   if it expands to     empty string, $(if) expands to the third  parameter
#     (if present).
# More on conditional functions in GNU Make:
# http://www.gnu.org/software/make/manual/html_node/Conditional-Functions.html#Conditional-Functions
#
# $(shell) function executes shell command and expands
#   to captured standard output.
# We effectively do make --version.
# We use $(MAKE) variable to account for different possible commands to launch
#   Make. It will expand to absolute path to Make,
#   so it works correctly with $PATH and everything.
# More about $(MAKE) variable:
# http://www.gnu.org/software/make/manual/html_node/MAKE-Variable.html#MAKE-Variable
#
# So, to branch on result of shell command, we output something in one case
#   (echo Fail) and nothing in another one.
# We also have to suppress all other output since otherwise the result of
#   $(shell) won't be empty even when we didn't explicitly output anything.
#
# In case you wonder why I don't write comments in-line:
#   GNU Make doesn't like that.
# It might want to cat the comment line to the previous one with backslash, etc.
$(if $(shell if ! $(MAKE) --version | grep 'GNU Make 4.' > /dev/null; \
               then echo Fail; \
             fi;),\
     $(error Only GNU Make 4+ is supported))

# By default, when target is not specified, Make will build the one
#   specified first in the Makefile.
# Confusing, let's disable and make it always build 'all' target.
# More about special variables:
# https://www.gnu.org/software/make/manual/html_node/Special-Variables.html
.DEFAULT_GOAL := all

# This is roughly equivalent to passing these flags on command line.
#
# -r removes built-in implicit rules (like %.o: %c ...).
# This is to ease debugging and improve performance.
# Make won't try every possible rule for every prerequisite anymore.
#
# -R removes built-in variables (like CC).
# Just to get rid of another source of confusion and make everything explicit.
#
# -j makes build parallel by default.
# Helps to discover underspecified dependencies and speeds up things.
#
# -O enables synchronization of output during parallel build.
# It is done on per-target basis: all commands of one target
#   are grouped together.
#
# How to override these flags:
# make MAKEFLAGS=-j
# will specify only '-j'. You can also pass empty string.
# More about overriding any Make variable on the command line:
# https://www.gnu.org/software/make/manual/html_node/Overriding.html#Overriding
MAKEFLAGS := -r -R -j -O -s

# Second expansion is used in this solution to seamlessly create
#   directories for target files.
# It means prerequisite list will be re-expanded second time before
#   executing the recipe. Usually prerequisites are evaluated just
#   when reading the Makefile.
# Secondary expansion:
# https://www.gnu.org/software/make/manual/html_node/Secondary-Expansion.html
.SECONDEXPANSION:

# This is to use only one shell process for entire execution of recipe
#   for target.
# We need it to store entire recipe in one .cmd file.
.ONESHELL:

# We'll introduce some convenience wrappers to ease debugging of
#   our new features. They'll be enabled or disabled by this switch.
# Variables can be overridden by command-line arguments, like this:
# make BUILD_DEBUG=y
# More info:
# https://www.gnu.org/software/make/manual/html_node/Overriding.html
BUILD_DEBUG := n

# By default, GNU Make uses tab character to determine
#   which line goes to recipe and which is Makefile syntax.
# Conventionally, recipe lines start with tab character.
# This is often source of confusion, as tab is not a printed character (obviously).
# This is even more so in the land of lousy editors which like
#   to convert indentation from tabs to spaces and vice versa.
# With this, an actual printed character is used to introduce recipe line.
#
# GNU Make 4.0 required.
.RECIPEPREFIX := >

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

# Function: Define a variable 'local' to the function.
#
# This is not a real local variable!
# It will have value it got during last function call later in Makefile.
# The only reason to do this is to perform name mangling to avoid
#   variable name conflicts.
# And, well, more convenient function debugging.
#
# $(strip) function removed spaces on both ends of the variable value.
# This way we don't have to care about spaces added by make
#   when continuing new lines.
# More here:
# https://www.gnu.org/software/make/manual/html_node/Splitting-Lines.html
#
# $(call) function performs expansion to the value of function definition,
#   with $0, $1 and so on in the function definition being replaced
#   with actual parameters.
#
# As you can see, variable names can be calculated via
#   other variables and functions ($(FUNCTION)_$(NAME)).
define DEFINE_LOCAL_VARIABLE
$(eval FUNCTION := $(strip $1))
$(eval NAME := $(strip $2))
$(eval VALUE := $(strip $3))
$(call TRACE1,$(FUNCTION)_$(NAME) := $(VALUE))
endef

# Function: Alias to DEFINE_LOCAL_VARIABLE.
# We can use any character, except '#', ':', '=' and space
#   in the name of variable.
# So, semantics: "move value on the right to the variable on the left"
# Okay, this is to illustrate our infinite possibilities,
#   but in the end, let's stick with something more conventional.
# See below.
define <<
$(call DEFINE_LOCAL_VARIABLE,$1,$2,$3)
endef

# Old-school lisp-ish (or haskell-ish, or whatever) statement.
# Not really a statement: you still have to use $(call let,...).
define let
$(call DEFINE_LOCAL_VARIABLE,$1,$2,$3)
endef

# Function: Reference a local variable introduced by DEFINE_LOCAL_VARIABLE.
# We need $(strip) to remove extra spaces produced by newlines and $(eval)
#   in the definition (result of the $(eval) here is empty).
define REFERENCE_LOCAL_VARIABLE
$(strip \
$(eval FUNCTION := $1)
$(eval NAME := $2)
$($(FUNCTION)_$(NAME))
)
endef

# Couldn't think of something more resembling of variable dereference
#   and not breaking GNU Make at the same time ('*' does break it).
define &
$(call REFERENCE_LOCAL_VARIABLE,$1,$2)
endef

# Function: Print a single passed parameter to stdout if debugging is enabled.
ifeq ($(BUILD_DEBUG),y)
define PRINT1
$(info $1)
endef
endif

# Function: Print a value as it's being evaluated.
define TRACE1
$(call PRINT1,$1)
$(eval $1)
endef

# We're going to put all the built stuff under 'build' directory.
# It allows to write easier .gitignore file and not accidentally commit
#   built files to version control.
# It also makes implementation of 'clean' target trivial:
#   just remove entire directory.
BUILD_DIR := build
AUX_DIR := $(BUILD_DIR)/aux
CMD_DIR := $(AUX_DIR)/
DU_DIR :=  $(AUX_DIR)/
RES_DIR := $(BUILD_DIR)/res

# This directory will hold sources.
# Good build system mirrors sources structure in build directory --
#   it allows to manage pattern rules easier.
SRC_DIR := src

# Function: Wrap a command.
# Print short description, output command only if it failed.
# Last lines is the shell command that will be invoked when making some target
#   that uses RUN in the recipe.
# If the command wasn't successful, we use tput to colorize part of the output.
# Double dollars are to prevent treatment as Make variable -
#   one dollar will get eaten by expansion, second will be left and
#   actually passed to the shell.
define RUN
$(strip \
$(call FUNCTION_DEBUG_HEADER,$0)
$(call let,$0,DESCRIPTION,$1)
$(call let,$0,COMMAND,$2)
)
echo $(call &,$0,DESCRIPTION); if ! $(call &,$0,COMMAND);\
  then echo "$$(tput setaf 1)[ERROR]$$(tput sgr0) Failed command:\n$(call &,$0,COMMAND)"; fi
endef


define GET_TARGET_PATH
$(patsubst build/aux/%.cmd,%,$1)
endef

define GET_CMD_PATH
$(patsubst %,build/aux/%.cmd,$1)
endef

define GET_CMD_DID_UPDATE
$(patsubst %,build/aux/%.cmd.did_update,$1)
endef


# Function: Define build of a program.
# Suppose we're calling this function as follows:
# $(call PROGRAM,a,b,c.c,d,e,f)
# (such flags make no sense, but we don't care, it's an example)
#
# We first define variable OBJ_b := build/c.c.o
#
# Then we define static pattern rule for objects:
# $(OBJ_b): build/%.o: \
#   src/% \
# | $(DIRECTORY)
# > $(COMPILE_OBJECT)
#
# Details:
# https://www.gnu.org/software/make/manual/html_node/Static-Pattern.html
#
# Then we specify CFLAGS for these objects via target-specific variable assignment:
# https://www.gnu.org/software/make/manual/html_node/Target_002dspecific.html
#
# After that, we define DEP_b := $(OBJ_b:=.d).
# This is called a substitution reference.
# We substitute empty suffix of each list element with suffix '.d'.
# This way, we get 'build/c.c.o.d' in DEP_b.
# These are the dependency files as generated by GCC.
# They contain all headers on which given object file depends on.
#
# Next, we define $(PROGRAM_b): $(OBJ_b) to specify prerequisites of program
#   and its' recipe on next line.
# LDFLAGS and LDLIBS are also specified in target-specific manner.
#
# Finally, we do ALL += $(PROGRAM_b) to make 'all' target build this program.
#
# You can see all the generated goodness by replacing
#   $(eval $(call PROGRAM, ...)) with
#   $(info $(call PROGRAM, ...))
define PROGRAM
$(call FUNCTION_DEBUG_HEADER,$0)
$(call let,$0,SOURCE_NAME,$1)
$(call let,$0,BUILT_NAME,$2)
$(call let,$0,SRC,$3)
$(call let,$0,CFLAGS,$4)
$(call let,$0,LDFLAGS,$5)
$(call let,$0,LDLIBS,$6)

.SHELLFLAGS = --target $$@

$(RES_DIR)/%: \
  $(AUX_DIR)/%.cmd.did_update \
| $(AUX_DIR)/%.cmd \
  $(DIRECTORY)
> eval $$$$(cat $$(firstword $$|))

$(call TRACE1,OBJ_$(call &,$0,BUILT_NAME)_CMD := $(strip \
  $(patsubst $(SRC_DIR)/$(call &,$0,SOURCE_NAME)%,\
             $(AUX_DIR)/$(call &,$0,BUILT_NAME)/%.o.cmd,\
             $(call &,$0,SRC))))

$(call TRACE1,OBJ_$(call &,$0,BUILT_NAME) := $(strip \
  $(patsubst $(SRC_DIR)/$(call &,$0,SOURCE_NAME)%,\
             $(RES_DIR)/$(call &,$0,BUILT_NAME)/%.o,\
             $(call &,$0,SRC))))

$(OBJ_$(call &,$0,BUILT_NAME)_CMD): .SHELLFLAGS = \
  --target $$@ --prerequisites $$? -- \
  --build-dir $(BUILD_DIR)


$(OBJ_$(call &,$0,BUILT_NAME)_CMD): \
  $(AUX_DIR)/$(call &,$0,BUILT_NAME)/%.o.cmd: \
  $(call NORM_PATH,$(DU_DIR)/$(call &,$0,SOURCE_NAME))/%.did_update \
  $(THIS_MAKEFILE) \
| $(call NORM_PATH,$(SRC_DIR)/$(call &,$0,SOURCE_NAME))/% \
  $$(DIRECTORY)
> echo '$$(COMPILE_OBJECT)' > $$@

$(OBJ_$(call &,$0,BUILT_NAME)_CMD): CFLAGS := $(call &,$0,CFLAGS)
$(OBJ_$(call &,$0,BUILT_NAME)_CMD): .SHELLFLAGS = \
  --target $$@ --prerequisites $$? -- \
  --build-dir $(BUILD_DIR)
$$(eval $$(call DEFINE_HASHED_CHAIN, \
                OBJ_$$(call &,$0,BUILT_NAME), \
                $(call NORM_PATH,./$(RES_DIR))/%, \
                $(call NORM_PATH,./$(DU_DIR)/)/%, \
                $$(OBJ_$(call &,$0,BUILT_NAME))))

$$(eval $$(call DEFINE_HASHED_CHAIN, \
                SRC_$$(call &,$0,SOURCE_NAME), \
                $(call NORM_PATH,./$(call &,$0,SOURCE_NAME))/%, \
                $(call NORM_PATH,$(DU_DIR)/$(call &,$0,SOURCE_NAME))/%, \
                $$(call &,$0,SRC)))

.PRECIOUS: build/aux/%.did_update

$(AUX_DIR)/%.did_update: \
$(AUX_DIR)/%.hash.new
>  if [ -f $$(patsubst %.hash.new,\
                       %.hash.old,\
                       $$<) ];\
   then \
     if ! diff -q $$< $$(patsubst %.hash.new,\
                                  %.hash.old,\
                                  $$<) > /dev/null; \
     then \
         touch $$@; \
     fi; \
   else \
     touch $$@; \
   fi; \
   cp $$< $$(patsubst %.hash.new,\
                      %.hash.old,\
                      $$<)

.PRECIOUS: build/aux/%.hash.new

$(AUX_DIR)/%.hash.new: \
$(RES_DIR)/%
> shasum $$< > $$@

$(AUX_DIR)/%.hash.new: \
$(SRC_DIR)/%
> shasum $$< > $$@

$(AUX_DIR)/%.hash.new: \
$(AUX_DIR)/%
> shasum $$< > $$@

$(call TRACE1,DEP_$(call &,$0,BUILT_NAME) := $(strip \
  $$(OBJ_$(call &,$0,BUILT_NAME):=.d)))

-include $$(DEP_$(call &,$0,BUILT_NAME))

$(call TRACE1,PROGRAM_$(call &,$0,BUILT_NAME)_CMD := $(strip \
  $(AUX_DIR)/$(call &,$0,BUILT_NAME)/$(call &,$0,BUILT_NAME)).cmd)

$(call TRACE1,PROGRAM_$(call &,$0,BUILT_NAME) := $(strip \
  $(RES_DIR)/$(call &,$0,BUILT_NAME)/$(call &,$0,BUILT_NAME)))

$$(PROGRAM_$(call &,$0,BUILT_NAME)_CMD): \
  $$(DID_UPDATE_OBJ_$(call &,$0,BUILT_NAME)) \
  $(THIS_MAKEFILE) \
| $$(OBJ_$(call &,$0,BUILT_NAME))
> echo '$$(LINK_PROGRAM)' > $$@

$$(PROGRAM_$(call &,$0,BUILT_NAME)_CMD): LDFLAGS := $(call &,$0,LDFLAGS)
$$(PROGRAM_$(call &,$0,BUILT_NAME)_CMD): LDLIBS := $(call &,$0,LDLIBS)
$$(PROGRAM_$(call &,$0,BUILT_NAME)_CMD): .SHELLFLAGS = \
  --target $$@ --prerequisites $$? -- \
  --build-dir $(BUILD_DIR)

ALL += $$(PROGRAM_$(call &,$0,BUILT_NAME))
endef

define DEFINE_HASHED_CHAIN
$(strip \
$(call FUNCTION_DEBUG_HEADER,$0)
$(call let,$0,VAR_NAME_SUFFIX,$1)
$(call let,$0,SOURCE_PATTERN,$2)
$(call let,$0,TARGET_PATTERN,$3)
$(call let,$0,SOURCE_LIST,$4)
)
DID_UPDATE_$$(call &,$0,VAR_NAME_SUFFIX) = $$(strip \
  $$(patsubst $(call &,$0,SOURCE_PATTERN),\
              $(call &,$0,TARGET_PATTERN).did_update, \
              $(call &,$0,SOURCE_LIST)))

HASH_NEW_$$(call &,$0,VAR_NAME_SUFFIX) = $$(strip \
  $$(patsubst $(call &,$0,SOURCE_PATTERN),\
              $(call &,$0,TARGET_PATTERN).hash.new, \
              $(call &,$0,SOURCE_LIST)))
endef

# This is called 'canned recipe'.
# It's essentially a function, which will get its' automatic variables
#   expanded in the context of target being built.
# Details:
# https://www.gnu.org/software/make/manual/html_node/Canned-Recipes.html
#
# $(@F) is file name part of the path of target.
# $< is first prerequisite of the target.
# $@ is the target.
# More on automatic variables:
# https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html
#
define COMPILE_OBJECT
$(call RUN,GCC $$(notdir $$(call GET_TARGET_PATH,$$@)),gcc $$(CFLAGS) $$(patsubst $(AUX_DIR)/%.did_update,$(SRC_DIR)/%,$$<) -o $(RES_DIR)/$$(call GET_TARGET_PATH,$$@) -c -MD -MF $(AUX_DIR)/$$(call GET_TARGET_PATH,$$@).d -MP); \
sed -i -e "s|\\b$(patsubst $(AUX_DIR)/%.did_update,$(SRC_DIR)/%,$<)\\b||g" $(AUX_DIR)/$(call GET_TARGET_PATH,$@).d
endef

# Another canned recipe - for linking program out of objects.
# We still call 'gcc' instead of direct invocation of 'ld'
#   since 'gcc' takes care about some additional parameters to 'ld' in many cases.
# Besides, there's 'collect2' in between of them.
# Just stick with compiler driver, it does the right thing most of the rime.
# $^ is all prerequisites of the target.
define LINK_PROGRAM
$(call RUN,GCC $$(notdir $$(call GET_TARGET_PATH,$$@)),gcc $$(LDFLAGS) $$(patsubst $(AUX_DIR)/%.did_update,$(RES_DIR)/%,$$(filter %.o,$$^)) -o $(RES_DIR)/$$(call GET_TARGET_PATH,$$@) $$(LDLIBS))
endef

# 'clean' just removes entire build directory.
.PHONY: clean
clean:
> $(call RUN,RM BUILD_DIR,rm -rf $(BUILD_DIR))

# This is a silly implementation of path normalization.
# We need this because there are paths with optional user-defined components.
# When these parts of paths are empty, we end up with double slashes and
#   Make's path pattern matching stops working.
# FIXME: There should be better ways to do it, as currently it completely
#   destroys performance (this is called several times per target).
define NORM_PATH
$(shell python -c 'import os, sys; print os.path.normpath(sys.argv[1])' $1)
endef
