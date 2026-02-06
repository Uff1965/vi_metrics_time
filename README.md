# vi_metrics_time

This is a learning program for measuring and analyzing the basic time characteristics of time functions.

### Warning about project purpose

This repository was created for educational and experimental purposes. The code is intended as a learning playground and may include many prototypes, outdated fragments, and “dead” code. Solutions and approaches used here are not guaranteed to be optimal, may be overly complex, and should not be treated as production‑ready implementations.

### What to expect in the codebase

- Numerous prototypes and test files created during experimentation.  
- Duplicate code, unused functions, and remnants of previous attempts.  
- Complex or non‑idiomatic solutions introduced for exploration rather than clarity.  
- Limited error handling, incomplete validation, and minimal hardening for production use.

### Usage: vi_metrics_time [OPTIONS]

```
-h, --help                 Show this help and exit  
-w, --warming[=1|0]        Warmup before measurement (default: 1; implicit: OFF)  
-s, --sort <key>           Sort by name|resolution|duration|tick|type (default: name)  
-i, --include <name>       Include function name (repeatable)  
-e, --exclude <name>       Exclude function name  
-r, --repeat <N>           Number of measurements (default: 1; implicit: 5)  
    --stat <avg|min|med>   Statistic to report (default: median)  
    --version              Show build type and compilation time  
```
