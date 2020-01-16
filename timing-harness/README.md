More information about this profiler can be found in this paper.
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


Use it as follow, where <hex> the hex encoding of the basic block that you want to profile.
`./test <hex> <reps>`

For instance, you can profile the throughput of `pushq %rax` like this. 

```
$ ./test 50 100 # hex code of `pushq %rax` is `50`
Core_cyc	L1_read_misses	L1_write_misses	iCache_misses	Context_switches
868	         21	              -1	             0	           0
840	          0	              -1	             0	           0
790	          0	              -1	             0	           0
791	          0	              -1	             0	           0
790               0	              -1	             0	           0
791	          0	              -1	             0	           0
793	          0	              -1	             0	           0
791	          0	              -1	             0	           0
793	          0	              -1	             0	           0
794	          0	              -1	             0	           0
792	          0	              -1	             0	           0
791	          0	              -1	             0	           0
790	          0	              -1	             0	           0
794	          0	              -1	             0	           0
795	          0                   -1	             0	           0
```

Note that -1 signifies that the performance counter is not available in your hardware setup.

```
# Get latency for 200 iterations 
$ ./test 50 200
Core_cyc	L1_read_misses	L1_write_misses	iCache_misses	Context_switches
1142              24	              -1	             0	         0
933	            0	              -1	             0	         0
891	            0	              -1	             0	         0
892	            0	              -1	             0	         0
891	            0	              -1	             0	         0
897	            0	              -1	             0	         0
896	            0	              -1	             0	         0
894	            0	              -1	             0	         0
891	            0	              -1	             0	         0
895	            0	              -1	             0	         0
894	            0	              -1	             0	         0
892	            0	              -1	             0	         0
891	            0	              -1	             0	         0
895	            0	              -1	             0	         0
894	            0	              -1	             0	         0

# Core_cyc column reports latency (including measurement overhead) of executing the basic block 100 (200) iterations.
# We calculate the throughput as follows.
$ python <(echo 'print "Throughput:", (891.0 - 791.0)/100')
Throughput: 1.00
```

If you fancy using the harness as a library, just include `harness.h` and link against `harness.a`.
