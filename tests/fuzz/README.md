# RTT Scripting Fuzzer

The normal RTT test build always provides `scripting_fuzz_seed_test`. It runs
the corpus in `corpus/` through the expression, condition, value-statement,
program, state-machine, and direct-script parser boundaries.

The mutation target is opt-in and requires Clang with libFuzzer support:

```sh
cmake -S . -B /tmp/rtt-fuzz-build \
  -DENABLE_TESTS=ON \
  -DENABLE_SCRIPTING_FUZZING=ON \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_INSTALL_PREFIX=/tmp/rtt-fuzz-prefix
cmake --build /tmp/rtt-fuzz-build --target scripting_fuzzer
/tmp/rtt-fuzz-build/tests/scripting_fuzzer \
  tests/fuzz/corpus \
  -max_total_time=60 \
  -timeout=5 \
  -max_len=65536
```

Use a temporary build and install prefix. Sanitizer failures, crashes, hangs,
and unexpected exceptions are failures; parser and program-load exceptions
are expected outcomes for malformed input.
