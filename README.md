Portable Event Library
======================

This small event library in C is based around `select()`.  The design
evolved over time in the [SMCRoute][] project, where I wanted to keep
the tool small and with as few (no) external dependencies as possible.

It started out as a refactor of the socket polling functionality, and
really came into its own in 2017 when I found Mr Rich Felker's mention
of the classic [timer-to-pipe pattern][1] on Stack Overflow.

In the years passed I've noticed many times the need for a really small
and, most importantly, *portable* event loop.  So here's code, extracted
from SMCRoute and free to used under the terms of the very liberal ISC
license.

You can use it as a library or just include the components into your own
project.  If you find bugs or have fixes to share, I'd very much like to
see.

> **Note:** The timer implementation uses `setitimer()` for maximum
> portability.  Depending on the system, this can potentially cause
> problems if your application also use `sleep()`, `usleep()`, or
> `alarm()`, which may all use the same back end implantation in your
> operating system.  Please check the documentation for your OS for more
> information on the subject.

Take care!  
 /Joachim :-)

[SMCRoute]: https://github.com/troglobit/SMCRoute
[1]: https://stackoverflow.com/questions/2328127/select-able-timers/6800676#6800676
