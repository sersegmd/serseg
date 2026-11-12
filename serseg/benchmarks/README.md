# Checked array iteration microbenchmark

Run `python3 benchmarks/iteration.py build/serseg` from the `serseg` directory. The script builds the same source with `-O0` and `-O1`, checks every output, warms each binary once and records seven alternating wall-time samples per level by default. Pass `--samples N` to change the count or `--output new-file.json` to save raw results without overwriting an existing file.

This measures one workload only: creation of a 1,000-element integer array followed by 30 million checked array visits and additions. Recorded times include process startup, and build time is a single observation per level. Peak memory is not measured. Results are affected by hardware, system load, compiler and libraries. The optimization levels differ in the embedded runtime's GCC settings and AST constant folding; the benchmark does not isolate their contributions. Do not generalize these results to other programs or compare them to other languages.

A local Linux observation, with raw samples and environment, is provided in `results/`. It is not a Windows result or a comprehensive language benchmark.
