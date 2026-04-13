# CSC3060 Project 4

Cache simulator for CSC3060 Project 4.

## Team

- Yuezhe Meng (`124090475`)
- Mingze Zhou (`124090947`)
- Chosen trace seed student ID: `124090475`

## Build

```bash
make
```

Clean build artifacts:

```bash
make clean
```

## Run

Task 1:

```bash
make task1
```

Task 2:

```bash
make task2
```

Task 3:

```bash
make task3
```

Build the trace generator:

```bash
make trace_gen
```

Generate the personalized trace again with the chosen seed:

```bash
./trace_generator/workload_gen student 124090475 > my_trace.txt
```

Run the trace analyzer:

```bash
python3 trace_analyzer.py my_trace.txt
```

## Project Files

- `main.cpp`: command-line parsing and hierarchy construction
- `memory_hierarchy.cpp/.h`: cache and memory access logic
- `repl_policy.cpp/.h`: replacement policies
- `prefetcher.cpp/.h`: prefetchers
- `trace_analyzer.py`: helper script for trace analysis
- `trace_sanity.txt`: validation trace for Task 1 and Task 2
- `my_trace.txt`: personalized trace used for Task 3
- `report.md`: project report source

## Notes

- `task1` runs a single-level `L1 -> Memory` hierarchy.
- `task2` and `task3` enable `L1 -> L2 -> Memory`.
- The final Task 3 configuration is controlled by the `TASK3_*` variables in `Makefile`.