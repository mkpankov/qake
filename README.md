# qake

GNU Make based build system with a different approach.

## Why?

GNU Make is an old dependency tracking program. Over the years, it gained a lot of features that are hard to use in a maintainable way. Such as, [built-in implicit rules](https://www.gnu.org/software/make/manual/html_node/Catalogue-of-Rules.html). It also improved in several areas, but these improvements are still optional to be compatible with older Makefiles.

Let's *break* backwards compatibility. Let's squeeze every drop of goodness GNU Make has to offer. Let's make a build system that is easier to use and is more featureful.

## Installation

### Automated

You can install **qake** via the command line with `wget`. `git` is also required.

GNU Make 4.0 will be downloaded, built and installed during to the installation.

#### Via Wget

If you're using `wget` type:

```bash
wget --no-check-certificate https://github.com/mkpankov/qake/raw/master/installer.sh -O - | sh
```
