# Epilogue: unfortunately, we can't move this above,
#   as before or during the $(eval $(call PROGRAM,...))
#   ALL variable is undefined.
# Making it recursively expanded doesn't help, as it's value
#   is received by evaluating a generated part of the Makefile.
.PHONY: all
all: $(ALL)
