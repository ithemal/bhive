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
@inproceedings{delimitrou2013ibench,
  title={ibench: Quantifying interference for datacenter applications},
  author={Delimitrou, Christina and Kozyrakis, Christos},
  booktitle={2013 IEEE international symposium on workload characterization (IISWC)},
  pages={23--33},
  year={2013},
  organization={IEEE}
}
```
