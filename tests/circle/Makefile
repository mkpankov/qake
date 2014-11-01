THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))

# Get list of C sources in 'src' directory.
# More on how $(wildcard) works:
# https://www.gnu.org/software/make/manual/html_node/Wildcard-Function.html#Wildcard-Function
SRC_CIRCLE := $(wildcard src/*.c)

# Just all libraries which we need
LDLIBS_CIRCLE := \
  -ldl \
  -lpthread \
  -rdynamic \

# The meat of the build: generate all the goodness.
$(eval $(call PROGRAM,\
              ,\
              circled,\
              $(SRC_CIRCLE),\
              $(CFLAGS_CIRCLE),\
              $(LDFLAGS_CIRCLE),\
              $(LDLIBS_CIRCLE),\
))
