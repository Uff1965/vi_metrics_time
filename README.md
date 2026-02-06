# vi_metrics_time

### Warning about project purpose

This repository was created for educational and experimental purposes. The code is intended as a learning playground and may include many prototypes, outdated fragments, and “dead” code. Solutions and approaches used here are not guaranteed to be optimal, may be overly complex, and should not be treated as production‑ready implementations.

### What to expect in the codebase

- Numerous prototypes and test files created during experimentation.  
- Duplicate code, unused functions, and remnants of previous attempts.  
- Complex or non‑idiomatic solutions introduced for exploration rather than clarity.  
- Limited error handling, incomplete validation, and minimal hardening for production use.

### Usage: vi_metrics_time [OPTIONS]

-h, --help
    Show this help and exit

-w, --warming[=<1|0>]
    Enable/disable warmup before measurement.
    Default: 1. Implicit (when used without value): OFF.

-s, --sort=<key>
    Sort by: name | resolution | duration | tick | type.
    Default: name. Implicit: resolution.

-i, --include=<name>
    Include only functions matching <name>.
    Repeatable: can be supplied multiple times.

-e, --exclude=<name>
    Exclude functions matching <name>.

-r, --repeat=<N>
    Number of measurement repeats.
    Default: 1. Implicit: 5.

--stat=<average|minimum|median>
    Statistic to report.
    Default: median. Implicit: minimum.

--version
    Print build type and compilation time.
