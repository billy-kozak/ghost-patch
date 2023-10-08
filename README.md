ghost-patch
===========

ghost-patch is a utility for injecting a lua interpreter into the address space
of a target process. Using lua, ghost-patch can inspect or even modify, the
behaviour of the target. While lua is running as a thread within the target's
memory space, it also ptrace attaches to the target so that it can control
the target like a debugger.

Building
========

ghost-patch is built with a custom makefile. After building, binaries are saved
to ./bin.

## Main Build

Builds with optimizations on and debug symbols off.

```
$ make all
```

## Debug Build

Builds with optimizations off and debug symbols on.

```
$ make debug
```

## Test Build

Builds tests. Tests are run from ./bin/tests/ghost-patch-tests (run with -h
for test run help documentation). The test build turns on debug symbols and
turns off optimizations.

```
make tests
```

Build tests with optimizations turned off.

```
make fast_tests
```

Using
=====

Run ghost-patch -h for usage information.


TODO
====

All of the below is meant to be addressed at some point.

## Limitations

- Can't attach to targets which aren't dynamically linked to libc or which
aren't boot-strapped using libc (i.e. using crt0/crt1).
- Can't attach to targets which clean their environments (ghost-patch relies
on LD_PRELOAD to load code).


## Bugs

- Segfaults when run in Chromuim on Arch Linux. Chromium seems to be doing
some relocation or manipulation of dynamic symbols which runs before
ghost-patch and subsequently breaks ghost-patch.
- ghost-patch does not correctly identify indivdual threads when raising events
in lua.

## Unfinished Features

- lua facilities to locate symbols in the running target
- lua facilitie so override a target function with a lua function
- lua facilities for reading/writing/interpreting memory addresses within the
target


