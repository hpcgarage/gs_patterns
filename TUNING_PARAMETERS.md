# Usage:
`./gs_patterns [options] <trace.gz> [<binary>|-nv]`

Options accept both `--opt value` and `--opt=value` formats.
Argument parsing stops at `--`.

# Configuration Options:

## Triggers:
| Option | Short | Description                | Default  | Range                   |
|:------:|:-----:|:---------------------------|:--------:|:------------------------|
|`--per-sample`|`-ps`|Events per progress update|10000000|1000 - 1099511627776|

## Info Parameters:
| Option | Short | Description                          | Default | Range          |
|:------:|:-----:|:-------------------------------------|:-------:|:--------------:|
|`--cache-line-size`   |`-cls`|Cache line size in bytes               |64   |16 - 512       |
|`--trace-buffer-size` |`-tbs`|Trace buffer size (entries per chunk) |1024 |1 - 1048576   |
|`--iaddr-per-window`  |`-iw` |Instructions per window (static iaddrs)|1024|16 - 131072   |
|`--max-gather-scatter`|`-mgs`|Max gather/scatter elements tracked   |8096 |2 - 16384     |
|`--histogram-bounds`  |`-hb` |Histogram bounds (stride distance)    |512  |16 - 65536    |

## Pattern Parameters:
| Option | Short | Description                           | Default     | Range                |
|:------:|:-----:|:--------------------------------------|:-----------:|:--------------------:|
|`--unique-strides-threshold`   |`-ust`|Unique strides threshold               |1024        |16 - 65536           |
|`--unique-distances-threshold` |`-udt`|Unique distances threshold             |15          |1 - 128             |
|`--out-threshold`              |`-ot` |Out-of-bounds fraction threshold       |0.5         |0 - 1               |
|`--top-patterns`               |`-tp` |Number of top patterns to keep         |10          |1 - 100             |
|`--initial-pattern-size`       |`-ips`|Initial pattern size                   |32768       |1024 - 8589934592   |
|`--max-pattern-size`           |`-mps`|Maximum pattern size                   |1073741824  |1024 - 8589934592   |
|`--max-line-length`            |`-mll`|Maximum line length                    |1024        |80 - 8192           |

# Other flags:
| Option | Explanation                         |
|:------:|:------------------------------------|
|  -nv   | Interpret trace as NVBit (CUDA) trace |
|  -v    | Verbose logging                     |
|  -ow   | Overwrite outputs if present        |

# Invocation:
## For Pin/DynamoRIO traces:
    ./gs_patterns <pin_trace.gz> <binary>

## For NVBit (CUDA kernels):
    ./gs_patterns <nvbit_trace.gz> -nv

# Examples:
    ./gs_patterns app.pin.trace.gz ./app_with_symbols

    ./gs_patterns kernel.nvbit.trace.gz -nv

# Notes:
• Trace file must be gzipped (`.gz`) — not `tar.gz`.

• For Pin/DynamoRIO, the `<binary>` should be compiled with symbols (e.g., `-g`).

• For NVBit, compile CUDA kernels with line info (`--generate-line-info`).

• See `nvbit_tracing/README.md` for extracting compatible CUDA traces.