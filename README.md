# BHive: A Benchmark Suite and Measurement Framework for Validating x86-64 Basic Block Performance Models

More information about our tool/benchmark can be found in this paper.
* **BHive: A Benchmark Suite and Measurement Framework for Validating x86-64 Basic Block Performance Models**</br>
  Yishen Chen, Ajay Brahmakshatriya, Charith Mendis, Alex Renda, Eric Atkinson, Ondrej Sykora, Saman Amarasinghe, and Michael Carbin</br>
  2019 IEEE International Symposium on Workload Characterization</br>
  
```
@inproceedings{bhive,
  title={BHive: A Benchmark Suite and Measurement Framework for Validating x86-64 Basic Block Performance Models},
  author={Chen, Yishen and Brahmakshatriya, Ajay and Mendis,  Charith and Renda, Alex and Atkinson, Eric and Sykora, Ondrej and Amarasinghe, Saman and Carbin, Michael},
  booktitle={2019 IEEE international symposium on workload characterization (IISWC)},
  year={2019},
  organization={IEEE}
}
```


What's here?
# The Benchmark
* `benchmark/categories.csv` lists all the basic blocks and their categories.
* `benchmark/throughput/` contains the measured throughput of basic blocks. The throughput of some basic blocks are missing because we can't get a clean measurement.
* `benchmark/sources/` lists the source applications of all the basic blocks. For each basic block, we additionally show the (static) frequency with which it shows up in an application.
* `benchmark/disasm` is a tool that disassembles hex representation of a basic block. Use it like this `./disasm 85c044897c2460`. It uses `llvm-mc` and assumes that you have `llvm` installed.

# The Basic Block Profiler
`timing-harness/` is a profiler that allows you to profile arbitrary, memory accessing basic blocks such as those in `sources/`.
