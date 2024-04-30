# Setup
Download NVBit from the folliwing locations:

https://github.com/NVlabs/NVBit

#### Tested with version 1.5.5

https://github.com/NVlabs/NVBit/releases/tag/1.5.5

```
# or (for example for Linux x86_64)

wget https://github.com/NVlabs/NVBit/releases/download/1.5.5/nvbit-Linux-x86_64-1.5.5.tar.bz2
```


```
module load gcc #or make sure you have gcc. Tested with 8.5.0 and 11.4.0

tar zxvf <nvbit-$platform-1.5.5.tar.gz>

export NVBIT_DIR=</location/of/nvbit/root/directory>  # full path

cp -rv nvbit_tracing/gsnv_trace $NVBIT_DIR/tools/

cd $NVBIT_DIR

#Compile tools and test apps. Make sure the gsnv_trace tool compiled. $NVBIT_DIR/tools/gsnv_trace/gsnv_trace.so
make -j
```


*** NOTE *** make sure you gzip the nvbit trace output file before attempting to use with gs_patterns.

# gsnv_trace

The gsnv_trace tool will instrument one or more CUDA kernels within a CUDA application and pass the resulting memory traces to the gs_patterns gs_patterns_core library.  
Once the application has completed and all kernels are retired the gs_patterns_core library will begin processing the trace data and automatically generate the pattern outputs and pattern output files.  
This includes the JSON file containing Gather/Scatter Patterns.

###  configuration
gsnv_trace tool can be configured by setting the GSNV_CONFIG_FILE environment variable to a config file.  
The config file should have 1 configuration setting perline.  Configuration settings take the form "<CONFIG_ITEM> <CONFIG_VALUE>" where there is a space between the config item and its value.

Example:

```
echo "GSNV_LOG_LEVEL 1" > ./gsnv_config.txt
echo "GSNV_TRACE_OUT_FILE trace_file.nvbit.bin" >> ./gsnv_config.txt
echo "GSNV_TARGET_KERNEL SweepUCBxyzKernel" >> ./gsnv_config.txt
echo "GSNV_FILE_PREFIX trace_file" >> ./gsnv_config.txt

export GSNV_CONFIG_FILE=./gsnv_config.txt
```

Additional settings which are supported by NVBit can also be set via additional environment variables.  To see these please visit the NVBit documentation.
Setting covered here are specific to the gsnv_trace tool.

### Instrumenting an application

To starat instrumenting a CUDA application using gsnv_trace.  The gsnv_trace.so libary previously built will need to be specified using LD_PRELOAD. 

Example:

```
LD_PRELOAD=$NVBIT_DIR/tools/gsnv_trace/gsnv_trace.so <application> <application options>
gzip trace_file.nvbit.bin
```

This will load gsnv_trace.so and then execute the specified application. NVBit will instrument the application using gsnv_trace.so which will call into libgs_patterns_core to write out the resulting trace file and generate memory patterns withn the trace.
The gzip command will compress the resulting trace file for use by gs_patterns in a subsequent run.

### Generating Memory Patterns using an existing trace file.

In the previous section on Instrumnenting an application, we used nvgs_trace.so to instrument an application, the resulting trace file was then compressed.
The instrumentation run also generated pattern files. 
If we want to rerun the pattern generation we can do so using the generated (and compressed) trace file without re-instrumenting the application as this is much faster.
To do this we just need to run the gs_pattern binary with the trace file and the "-nv " option.  The "-nv" option indicates that the trace file is an nvbit trace.  

Example:

```
export GS_PATTERNS_DIR=/path/to/gs_patterns/binary/
$GS_PATTERNS_DIR/gs_patterns <trace_file.nvbit.bin.gz> -nv
```

### Important Notes 

As of NVBit 1.5.5, when building gsnv_trace within the NVBit source tree it *may* be required to specify a version of the CUDA which is older
in order to enable NVBit to correctly emit the runtime instructions.  Without this the gsnv_trace libary will still be built but will be unable to instrument CUDA kernels.

For instance we were able to build a working nvgs_trace using CUDA api version 11.7 and lower and use that on higher versions of the CUDA environment such as CUDA 12.3.
However as of NVBit 1.5.5 it was not possible to get a working version of nvgs_trace when we build it using 12.3 directly.

Example:

```
export LD_LIBARY_PATH=/path/to/cuda/11.7/lib:$LD_LIBRARY_PATH
export PATH=/path/to/cuda/11.7/bin:$PATH
cd $NVBIT_DIR
make 
```

Then in another shell simply load the desired CUDA library version using module load or manually, e.g:

```
export LD_LIBARY_PATH=/path/to/new/cuda/12.3/lib:$LD_LIBRARY_PATH
export PATH=/path/to/new/cuda/12.3/bin:$PATH

# point to where you build gsnv_trace.so (We can now instrument under CUDA 12.3)
LD_PRELOAD=$NVBIT_DIR/tools/gsnv_trace/gsnv_trace.so <application> <application options>
gzip trace_file.nvbit.bin
```
