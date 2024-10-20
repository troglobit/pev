Change Log
==========

All relevant changes to the project are documented in this file.


[v2.0][] - 2024-10-20
---------------------

**News:** breaking ABI change, timer callbacks are now passed the timer
id as the first argument instead of the timeout value.

### Changes
  * Timer callbacks are now passed the timer id as their first argument,
    instead of the timeout value.  This to allow callbacks an easy way
    to call `pev_timer_set()`
  * Add optional destructor callback for private socket, signal, and
    timer data, by Jacques de Laval, Westermo

### Fixes
  * The first argument to `select()` can now handle users calling
    `pev_sock_del()`.  May fix issues seen in this use-case


[v1.8][] - 2022-08-23
---------------------

### Fixes
  * Fix spurious timer triggers on Aarch6 (Arm64), or any arch where a
    `char` is default unsigned.  Found and fixed by Tobias Waldekranz


[v1.7][] - 2022-03-05
---------------------

### Fixes
  * Fix problem with timers expiring with old timeout when having called
    `pev_timer_set()` to reset them
  * Update timer API description, timeout and period are in microseconds


[v1.6][] - 2022-01-09
---------------------

### Changes
  * Add support for resetting one-shot timers
  * Simplify signal handling, no implicit masking


[v1.5][] - 2022-01-06
---------------------

Support for one-shot timers, causes incompatible API change.

### Changes
  * Add support for one-shot timers.  Such callbacks will get the
    timeout value as their first argument.  For periodic tasks this
	means, apart from the API change, that the first callback will
	get the timeout value, not the period time, unless the timeout
	value is zero, or the values are equal

### Fixes
  * Check return value from `fcntl()`, which is responsible for
    ensuring all file descriptors are in non-blocking state.  As well
    as the `O_CLOEXEC` flag to prevent forked children from modifying
    the descriptors of the parent process.  On failure, `pev_sock_add()`
    now returns error, which must be dealt with
  * Safely iterate over all events, callbacks may add or remove events
  * Fix install target, looked for missing file LICENSE


[v1.4][] - 2020-11-21
---------------------

Bug fix release, focusing on timers.

### Fixes
  * Fix timer nsec overflow correction
  * Verify result of `timer_start()` to prevent disabling event loop
    timer altogether
  * Don't update timers from timer callback context
  * Check for new timers at runtime


[v1.3][] - 2020-11-21
---------------------

Bug fix release, including incompatible API change.

### Changes
  * Timer resolution has changed from seconds to microseconds, affects
    period argument in `pev_timer_add()`
  * Sanity check API input arguments, returns `EINVAL` if arguments
    are not OK, e.g. `NULL` callbacks, invalid signal numbers, or bad
    sockets
### Fixes
  * Linked list fix, regression introduced in [v1.2][], removing an
    event at runtime caused loss more events: A B C D E, removing B
    caused loss of A as well, only: C D E remain
  * Memory safety fixes: uninitialized timers, checking for socket
    activity on non-sockets, etc.
  * Free memory properly on `pev_exit()`


[v1.2][] - 2020-12-02
---------------------

### Changes
  * Dropped local `queue.h` and BSD queue API entirely for basic
    doubly-linked list implementation
  * Placed in the public domain, fully free to use without any
    restrictions.


[v1.1][] - 2020-11-21
---------------------

Minor fixes.

### Changes
  * Use BSD semantics for signals, restart syscalls automatically
  * Support Illumos/OpenSolaris, simplify default `Makefile`
    * You now may need to set `CC` and `prefix` environment variables
      to build and install, respectively
  * Support *BSD
    * Verified on OpenBSD, FreeBSD, and NetBSD

### Fixes
  * Fix leaking of local variable `running`
  * Fix build on macOS, does not have `SOCK_CLOEXEC`
  * Handle `EINTR` when reading (signal/timer) from event pipe


[v1.0][] - 2020-11-21
---------------------

Initial release.

Support for periodic timers, signals, and sockets/descriptors.


[UNRELEASED]: https://github.com/troglobit/pev/compare/v2.0...HEAD
[v2.0]: https://github.com/troglobit/pev/compare/v1.8...v2.0
[v1.8]: https://github.com/troglobit/pev/compare/v1.7...v1.8
[v1.7]: https://github.com/troglobit/pev/compare/v1.6...v1.7
[v1.6]: https://github.com/troglobit/pev/compare/v1.5...v1.6
[v1.5]: https://github.com/troglobit/pev/compare/v1.4...v1.5
[v1.4]: https://github.com/troglobit/pev/compare/v1.3...v1.4
[v1.3]: https://github.com/troglobit/pev/compare/v1.2...v1.3
[v1.2]: https://github.com/troglobit/pev/compare/v1.1...v1.2
[v1.1]: https://github.com/troglobit/pev/compare/v1.0...v1.1
[v1.0]: https://github.com/troglobit/pev/compare/TAIL...v1.0
