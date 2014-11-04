# qake

GNU Make based build system with a different approach.

## Why?

GNU Make is an old dependency tracking program. Over the years, it gained a lot of features that are hard to use in a maintainable way. Such as, [built-in implicit rules](https://www.gnu.org/software/make/manual/html_node/Catalogue-of-Rules.html). It also improved in several areas, but these improvements are still optional to be compatible with older Makefiles.

For example:
* GNU Make 4.0 has option to sync output of multiple parallel jobs (-O);
* It also has option to disable built-in implicit recipes and variables (-r and -R);
* One can now use custom recipe prefix instead of TAB character, making for more explicit Makefiles;
* --trace enables tracing of full targets (with prerequisites that caused the target to update).

Let's *break* backwards compatibility. Let's squeeze every drop of goodness GNU Make has to offer. Let's make a build system that is easier to use and is more featureful.

## Goal

The user is supposed to never need to call `clean` goal. The single entry point is `qake`, and it should bring the project to the most-recent state without doing anything unnecessary.

## Features

* Compact syntax for definition of entire program build;
* Prevents lengthy meaningless rebuilds - prunes it as early as possible. Changing the Git branch doesn't cause the full rebuild anymore!;
* Tracks build commands - changes of build system itself are taken into account;
* Terse, but useful, output - we value users' attention and think it's easier to spot anything unusual when the build system doesn't spit at terminal every command it runs. At the same time, errors are colorized to be easy to notice and provide the full used command when user needs it the most;
* Extensible to other types of built entities. Supporting builds of static and dynamic libraries with dependencies between them is possible, as well as adding ways to build some custom artifacts;
* Tracks header dependencies (of course) - when user changes a common header, they don't need to remember to `clean` or do anything unusual. They just type `qake`, and whatever needs update gets updated;
* Uses GNU Make 4.0 internally and exposes it when needed - you can still pass custom flags to Make itself for debugging or any other purpose.

## Disclaimer

The project is in prototype stage and may or may not have significant limitations, including those making it unusable for a particular application.

For one, current implementation of object and source files tracking uses full hashing of corresponding file, what decreases the performance significantly.

## Tour (and a tutorial)

### Define program build

You need to call `PROGRAM` and `eval` the result:

```Make
$(eval $(call PROGRAM,\
              ,\           # name of directory with sources under 'src'
              program,\    # name of directory with results under 'build/res'
              $(SRC),\     # list of sources
              $(CFLAGS),\  # compiler flags for compilation of objects
              $(LDFLAGS),\ # compiler flags for linking of objects to program
              $(LDLIBS),\  # libraries for proper linking of objects to program
))
```

in your `Makefile`.

For this to work, you need Qake to be installed (see below). You also need yo define `SRC` variable to a list of paths to sources, relative to root directory of project. In case all your sources are directly in `src`, that's going to be `src/a.c src/b.c ...`:

```Make
SRC := $(wildcard src/*.c)
```

And a small touch on top: for tracking of commands used to build your program, we need the name of Makefile defining the build:

```Make
THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
```

This code is always the same, but you need to **put it** at the **beginning** of the Makefile, as `MAKEFILE_LIST` gets updated by Make as it goes and includes something.

That's it! With this, you get `make all` and `make clean`, with ability to separately build objects via path reference:

```Shell
make build/res/awesome_object.c.o
```

For concrete example, see `tests/circle/Makefile`. The rest of the tour will assume interaction with build of that program (it's an IRC chat named `circle`).

There's also a `test.sh`, which tests all the supposedly working modes of the build. You can run it from the `tests/circle/` directory.

### The full build

```Shell
➜  circle git:(master) ✗ qake
MKDIR build/res/circled
MKDIR build/aux
MKDIR build/aux/circled
GCC irclist.c.o
GCC irc.c.o
GCC ircfunc.c.o
GCC ircenv.c.o
GCC ircsock.c.o
GCC main.c.o
GCC ircq.c.o
GCC circled
➜  circle git:(master) ✗
```

As you can see, the default logging is terse and produces only the short description of command with name of a target.

This makes sense to not pollute the screen, and to make warnings easily detectable.

There currently is no "verbose" mode support (so that commands are output in full as in usual Make), but its' addition is trivial.

In case there's a compilation error, the build will stop and print the entire attempted command, so you'll be able to easily copy-paste it and retry it in manual mode:

```Shell
➜  circle git:(master) ✗ qake
GCC irc.c.o
src/irc.c:536:9: error: redefinition of ‘__irc_get_kicked_nick’
 field_t __irc_get_kicked_nick (const char * message)
         ^
src/irc.c:518:9: note: previous definition of ‘__irc_get_kicked_nick’ was here
 field_t __irc_get_kicked_nick (const char * message)
         ^
src/irc.c: In function ‘__irc_get_kicked_nick’:
src/irc.c:538:5: error: incompatible types when returning type ‘void *’ but ‘field_t’ was expected
     return NULL;
     ^
[ERROR] Failed command: gcc src/irc.c -o build/res/circled/irc.c.o -c -MD -MF build/aux/circled/irc.c.o.d -MP
➜  circle git:(master) ✗
```

I can't show it here, but `[ERROR]` is in red on any modern terminal, so the errors will really stand out.

And of course, null-build at this point is performed instantly and doesn't rebuild anything:
```Shell
➜  circle git:(master) ✗ qake
➜  circle git:(master) ✗ time qake
/home/mpankov/qake//qake  0,07s user 0,07s system 112% cpu 0,127 total
➜  circle git:(master) ✗
```

### Header files tracking

Let's start with basics: tracking of included headers.

You can edit the header and then everything depending on it will get rebuilt:

```Shell
➜  circle git:(master) ✗ qake
➜  circle git:(master) ✗ emacs src/ircfunc.h
Waiting for Emacs...
➜  circle git:(master) ✗ qake
GCC main.c.o
GCC ircfunc.c.o
GCC irclist.c.o
GCC ircsock.c.o
GCC ircenv.c.o
GCC ircq.c.o
GCC irc.c.o
GCC circled
➜  circle git:(master) ✗
```

### Build command tracking

Now, to some more magical features. You can change the build command of program, for example, by changing the `LDFLAGS`:

```Shell
➜  circle git:(master) ✗ qake
➜  circle git:(master) ✗ emacs Makefile
Waiting for Emacs...
```

in the following way:

```Make
LDLIBS_CIRCLE := \
  -ldl \
  -lpthread \
```

and the build notices that! See:

```Shell
➜  circle git:(master) ✗ qake
GCC circled
➜  circle git:(master) ✗
```

This is done so that the user never has to think about whether they need to `clean` anything. You simply use `qake` in all situations, even if you change the description of the build itself.

### Pruning of meaningless changes

More cool magic to come! In case you modify the source:

```Shell
➜  circle git:(master) ✗ qake
➜  circle git:(master) ✗ emacs src/irc.c
Waiting for Emacs...
```

by adding the following:

```C
// This is a super-useful comment about some function.
```

and re-build:

```Shell
➜  circle git:(master) ✗ qake
GCC irc.c.o
➜  circle git:(master) ✗
```

then you'll notice only the corresponding object file got rebuilt. What is it, a bug?.. Well, not actually - it's a way to cut propagation of meaningless changes further down the dependency graph.

Surely enough, object file we got after adding the comment is the same as before, so why re-link the program?

In more complex cases, this optimization allows us to produce nearly-null builds in situations where a full rebuild would happen in usual Make build. Just imagine several programs depending on that tiny object. Or possibly something uses the compiled program for other automated build actions, which would have to be re-run, too...

And just to make sure meaningful changes still propagate properly:

```Shell
➜  circle git:(master) ✗ emacs src/irc.c
Waiting for Emacs...
```

we add a new function:

```C
void awesome_func(void)
{
    printf("42");
}
```

and rebuild then:

```Shell
➜  circle git:(master) ✗ qake
GCC irc.c.o
GCC circled
➜  circle git:(master) ✗
```

Yay! We've rebuilt the program when needed.

It not only saves us from long rebuilds when you, say, change just the documentation. It also saves us from rebuilding when Git branch changes, or somebody touches the file accidentally, etc.

## Installation

### Automated

You can install **qake** via the command line with `wget`. `git` is also required.

GNU Make 4.0 will be downloaded, built and installed during to the installation.

#### Via Wget

If you're using `wget` type:

```bash
wget --no-check-certificate https://github.com/mkpankov/qake/raw/master/installer.sh -O - | sh
```
