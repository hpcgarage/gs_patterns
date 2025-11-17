# Usage:

`./gs_patterns [options] <trace.gz> [<binary>|-nv]`

Options accept both `--opt value` and `--opt=value` formats.
Argument parsing stops at `--`.

> For large values, you can use shell arithmetic.
Example: `--max-pattern-size=$((2**24))`

# Configuration Options:

## Triggers:

| Option | Short | Description | Default | Range |
|:---:|:---:|:---|:---:|:---:|
|`--per-sample`|`-ps`|Min memory operations before printing a progress dot|10000000 (≈2\*\*23)|2\*\*10 - 2\*\*30|

## Info Parameters:

| Option | Short | Description | Default | Range |
|:---:|:---:|:---|:---:|:---:|
|`--cache-line-size` |`-cls`|Cache line size (in bytes) to consider. |2\*\*6 |2\*\*4 - 2\*\*9 |
|`--trace-buffer-size` |`-tbs`|Number of trace entries to read at once (limits read buffer memory)|2\*\*10 |2\*\*0 - 2\*\*20 |
|`--iaddr-per-window` |`-iw` |Instruction window size (static iaddrs), prevents 1st pass slowdown|2\*\*10|2\*\*4 - 2\*\*12 |
|`--max-gather-scatter`|`-mgs`|Max number of unique gather/scatter iaddrs to track|8096 (≈2\*\*13) |2\*\*1 - 2\*\*14 |
|`--histogram-bounds` |`-hb` |Bound for the stride histogram (total bins = 2\*bounds+3)|2\*\*9 |2\*\*4 - 2\*\*12 |

## Pattern Parameters:

| Option | Short | Description | Default | Range |
|:---:|:---:|:---|:---:|:---:|
|`--unique-strides-threshold` |`-ust`|Min number of accesses a pattern must have to be considered|2\*\*10 |2\*\*4 - 2\*\*14 |
|`--unique-distances-threshold`|`-udt`|Max number of unique strides, used to identify complex patterns|15 (≈2\*\*4) |2\*\*0 - 2\*\*7 |
|`--out-threshold` |`-ot` |Max percentage (0.0-1.0) of accesses allowed "out of bounds"|0.5 |0.0 - 1.0 |
|`--top-patterns` |`-tp` |Max number of top gather/scatter patterns to save|10 |1 - 100 |
|`--initial-pattern-size` |`-ips`|Min *initial* size (indices) to allocate for a pattern|2\*\*15 |2\*\*10 - 2\*\*24|
|`--max-pattern-size` |`-mps`|Absolute *maximum* size (indices) a pattern can grow to (prevents OOM)|2\*\*30|2\*\*10 - 2\*\*31|
|`--max-line-length` |`-mll`|Max buffer size (chars) for reading source code lines (addr2line)|2\*\*10 |2\*\*10 - 2\*\*13 |

# Other flags:

| Option | Explanation |
|:---:|:---|
|  -nv  | Interpret trace as NVBit (CUDA) trace |
|  -v  | Verbose logging |
|  -ow  | Overwrite outputs if present |

# Invocation:

## For Pin/DynamoRIO traces:

```
./gs_patterns <pin_trace.gz> <binary>
```

## For NVBit (CUDA kernels):

```
./gs_patterns <nvbit_trace.gz> -nv
```

# Examples:

```
./gs_patterns app.pin.trace.gz ./app_with_symbols

./gs_patterns kernel.nvbit.trace.gz -nv
```

# Notes:

• Trace file must be gzipped (`.gz`) — not `tar.gz`.

• For Pin/DynamoRIO, the `<binary>` should be compiled with symbols (e.g., `-g`).

• For NVBit, compile CUDA kernels with line info (`--generate-line-info`).

• See `nvbit_tracing/README.md` for extracting compatible CUDA traces.
