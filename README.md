# BHive: A Benchmark Suite and Measurement Framework for Validating x86-64 Basic Block Performance Models

What's here?
* `categories.csv` lists all the basic blocks and their categories.
* `throughput/` contains the measured throughput of basic blocks. The throughput of some basic blocks are missing because we can't get a clean measurement.
* `sources/` lists the source applications of all the basic blocks. For each basic block, we additionally show the (static) frequency with which it shows up in an application.
* `disasm` is a tool that disassembles hex representation of a basic block. Use it like this `./disasm 85c044897c2460`. It uses `llvm-mc` and assumes that you have `llvm` installed.

Furthermore, if you want to measure the throughput of a basic block yourself, you can use our [profiler](https://github.com/ithemal/timing-harness).

More information about our tool/benchmark can be found in this paper.
* BHive: A Benchmark Suite and Measurement Framework for Validating x86-64 Basic Block Performance Models

  Yishen Chen, Ajay Brahmakshatriya, Charith Mendis, Alex Renda, Eric Atkinson, Ondrej Sykora, Saman Amarasinghe, and Michael Carbin

  2019 IEEE International Symposium on Workload Characterization


```
@inproceedings{bhive,
  title={BHive: A Benchmark Suite and Measurement Framework for Validating x86-64 Basic Block Performance Models},
  author={Chen, Yishen and Brahmakshatriya, Ajay and Mendis,  Charith and Renda, Alex and Atkinson, Eric and Sykora, Ondrej and Amarasinghe, Saman and Carbin, Michael},
  booktitle={2019 IEEE international symposium on workload characterization (IISWC)},
  year={2019},
  organization={IEEE}
}
```
