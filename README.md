Portable Event Library (PEV)
============================

This is a small event library in C based around `select()` available
free for use in the [*public domain*](UNLICENSE).

> **NOTE:**
>   * v1.3 changes timer resolution from 1 second to 1 microsecond
>   * v1.5 changes timer API to add support for one-shot timers


The Code
--------

The event loop consists of two files: `pev.c` and `pev.h`.  You can use
it as a library or just include the components into your own project.

See the header file for the API description.  If you find bugs or have
fixes to share, please report them using [GitHub issues][] or, preferably,
[pull requests][].


Implementation Notes
--------------------

**Note:** The timer implementation uses `setitimer()` for maximum
	portability.  Depending on the system, this can potentially cause
	problems if your application also use `sleep()`, `usleep()`, or
	`alarm()`, which may all use the same back end implantation in your
	operating system.  Please check the documentation for your OS for
	more information on the subject.


Building and Testing
--------------------

For those that don't just take the sources and integrate into their own
project.  You may have to set `CC`, to install you must set `prefix`.
Example:

    make clean
    make CC=clang
    make install prefix=/usr/local
    ...
    make uninstall prefix=/usr/local


History and Background
----------------------

The design evolved over time in the [SMCRoute][] project, where I wanted
to keep the tool small and with as few (no) external dependencies as
possible.

It started out as a refactor of the socket polling functionality, and
really came into its own in 2017 when I found Mr Rich Felker's mention
of the classic [timer-to-pipe pattern][1] on Stack Overflow.

In the years passed I've noticed many times the need for a really small
and, most importantly, *portable* event loop.  So here's the code I
wrote for SMCRoute, cleaned up and now fully free in the public domain.

Take care!  
 /Joachim :-)

[1]: https://stackoverflow.com/questions/2328127/select-able-timers/6800676#6800676
[SMCRoute]: https://github.com/troglobit/SMCRoute
[GitHub issues]: https://github.com/troglobit/pev/issues
[pull requests]: https://github.com/troglobit/pev/pulls
