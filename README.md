libprom
=======
This repo is a fork of the [Prometheus client libraries for C](https://github.com/digitalocean/prometheus-client-c) repository.

It provides shared libraries for instrumenting software and exposing metrics in text format as defined by the Prometheus exposition format (see https://prometheus.io/docs/instrumenting/exposition_formats/ for more details). libprom provides the core API implementation, libpromhttp a simple web handler to expose metrics for scraping.

Improvements wrt. to the forked source:
---------------------------------------
- bug and test fixes
- much less useless bloat
- compact function and type names, reduced code dups
- better performance
- enhanced and re-usable logging
- better documentation
- better process metrics about the collector itself incl. scrapetimes
- compact output, i.e. option that allows to supress the huge
  `# HELP` and `# TYPE` overhead in prometheus formatted output
- has now a `prom_counter_reset()` function
- `prom_string_builder()` is now public, i.e. usable by libprom consumers
- improved/KISSed process limit scraping (10..200x faster)
- libprom's process_collector is enhanced and now public, i.e. can now be used
  by libprom consumers to collect metrics of other processes easily
- new functions `prom_collector_data_set()` and `prom_collector_data_get()`
  allow to attach custom data by reference to every collector as well as read
  them back
- Solaris/Smartos/OmniOS/Illumos support

Build, Install, Test
--------------------
Requirements: A recent cmake, gcc and libmicrohttpd version incl. development
aka header files. Other compilers than gcc may work, too (but have not been
tested yet).

To build libprom and libpromhttp just run `make`. To build the API docs, 
run `make docs`. To install the libs, etc. change the working directory
to __target__/build/ and run `make install` (e.g.
`cd prom/build && DESTDIR=/tmp/libprom make install`).

To test the libs, run `make test`. 

If you do not want to compile libprom by yourself, any successful CI job on the [Github Action Page](https://github.com/jelmd/libprom/actions) contains the libprom archive for x86_64 Ubuntu, where the CI tests have been run. Just click on a CI job and scroll down to the **Artifacts** section. On Linux they probably work on most more or less recent distributions.

Last but not least you may try the Ubuntu packages hosted on [https://pkg.cs.ovgu.de/LNF/linux/ubuntu](https://pkg.cs.ovgu.de/LNF/linux/ubuntu)/**release**/.

API, Documentation, Usage
-------------------------
- https://jelmd.github.io/libprom/
- [example directory](https://github.com/jelmd/libprom/tree/main/example) of libprom
- To compile your own libprom enhanced app, just add `#include <libprom/prom.h>`
  and optionally `#include <libprom/promhttp.h>` to the related source and link
  it with `-lprom` and optionally with `-lpromhttp`.

Branch info
-----------
The **master** branch is used to track the main development branch of the upstream repo, the **main** branch is the main development branch of this repo.
