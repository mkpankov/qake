Build system is a crucial part of any project in compilable language, such as C, C++, Haskell or Java. It's critical part of infrastructure -- because broken or incorrect builds can lead to loss of productivity for the whole team.

That said, choice of build tool often doesn't receive proper attention. Many developers who just didn't care about it at start of the project, stick with something they've heard of -- and in many cases, that's GNU Make.

GNU Make may be a pioneer of the build systems, but by modern standards it's really lacking quite a lot of features. There is a lot of confusing stuff built on assumptions that is hard to maintain -- such as pattern rules. But basic things like C header dependencies tracking and one-line descriptions of built modules (programs and libraries) are missing.

Still, why bother with Make when you can just use a better build system? You can, for a new project. For an old and sufficiently big project, full conversion takes a lot of time and will inevitably discover some irregularities, tolerated by old build system.

So, it makes sense to bring some of the SCons goodness to the Make, while still having fully working build at any given time!

# Circles on the fields

We're going to implement an example build of a simple program called [circle](http://sourceforge.net/projects/circle/). It's an IRC chat bot, written in C.

We'll discuss all of major downsides of GNU Make and provide solutions, some of which will be nearly equivalent to the features from newer build tools.
