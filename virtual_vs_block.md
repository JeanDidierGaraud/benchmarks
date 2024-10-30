Benchmarking Mesh ideas
=======================

TL;DR
-----

- Eigen is faster than C (and a lot more readable)
- RowMajor gains 50% CPU
- virtual function ... not enough data to conclude yet


Principle
---------

Different methods to loop on cells (in order to compute volume in this case):
1. direct C implementation
2. eigen views and slices
3. virtual class to visit the array
4. non virtual class to visit the array
5. non virtual class to visit the array and avoid copy of `cell_coordinates`
6. same as 5, use `set_rank` to avoid Tetra2 re-creation inside inner loop





Compilation
-----------

        make && ./virtual_vs_block 100000000 100000000
        g++ -I../../../a-{core,solve}/src -std=c++20 -Wfatal-errors -frecord-gcc-switches -DEIGEN_USE_BLAS -DEIGEN_USE_MKL_ALL -DEIGEN_DEFAULT_TO_ROW_MAJOR -O3 -march=native -DNDEBUG    virtual_vs_block.cpp   -o virtual_vs_block




Results (seq, but on exclusive node):
----------------------------------------

On spiro-b321 (2x12 cascade lake, non hyperthreaded):
```
srun --nodelist=b321 --cpus-per-task=24 --export=ALL,SLURM_EXACT=1,SLURM_OVERLAP=1 --exclusive --immediate=2 --pty --qos c5_test_giga bash
make clean # to be sure  march=native on this node
make
```

Source code is now git:08f72de
- shows lots of variability (any idea why?)
- RowMajor makes a huge difference: +50% CPU on (2)
- ColMajor shows no variability at all on a single run (cache at work explains the variability?), but some (lots of) between two consecutive runs (12854ms vs. 17679ms)
- with RowMajor, C-style(1), slice(2), visitor-non-virtual-no-copy(5 or 6) are comparable: best run around 7780 ms.
- visitor-virtual(3) and visitor-non-virtual(4) are around 14340 ms
- virtual has negligible cost (was not expected, is this branch prediction at work when resolving the virtual table? --> need to add quads randomly to verify this idea)



### Detailed measures, on Friday:

#### 1st series
```
[jdgaraud@spiro-b321-clu benchmark]$ date
Fri Feb  3 18:27:19 CET 2023
[jdgaraud@spiro-b321-clu benchmark]$ ./virtual_vs_block 100000000 100000000
```

test (ms) | (1)     | (2)     | (3)     | (4)     | (5)     | (6)
----------|---------|---------|---------|---------|---------|---------
RUN1      | 7998.   | 7784.   | 13911   | 13903.8 | 7763.   | 7719.
RUN2      | 7997.37 | 7782.77 | 13916.2 | 13944.9 | 7763.26 | 7719.2
RUN3      | 7992.62 | 7780.22 | 13884.1 | 13900.3 | 7761.29 | 7718.04
RUN4      | 7999.3  | 7785.99 | 13912.3 | 13912.2 | 7768.01 | 7723.13
RUN5      | 7987.75 | 7768.79 | 13781.5 | 13782.5 | 7749.29 | 7706.96

No variability? (6) is systematically faster than the others?


#### 2nd series (taskset 0x1)

```
[jdgaraud@spiro-b321-clu benchmark]$ taskset 0x1 ./virtual_vs_block 100000000 100000000
```

test (ms) | (1)     | (2)     | (3)     | (4)     | (5)     | (6)
----------|---------|---------|---------|---------|---------|---------
RUN1      | 7957.8  | 7740.63 | 13624.8 | 13629.6 | 7721.41 | 7680.31
RUN2      | 7953.77 | 7738.58 | 13619.7 | 13624.4 | 7719.55 | 7676.33
RUN3      | 7958.24 | 7740.4  | 13625.4 | 13628.2 | 7723.39 | 7678.75


#### 3rd series (taskset 0x1, and compute twice the method (1) to throttle up CPU and preload L3 cache)

```
[jdgaraud@spiro-b321-clu benchmark]$ taskset 0x1 ./virtual_vs_block 100000000 100000000
```

test (ms) | (1)     | (1')    | (2)     | (3)     | (4)     | (5)     | (6)
----------|---------|---------|---------|---------|---------|---------|---------
RUN1      | 7956.92 | 8002.69 | 7739.91 | 13635.2 | 13777.3 | 8495.49 | 8493.25
RUN2      | 7956.1  | 8000.17 | 7737.35 | 13798.2 | 13820.7 | 8500.96 | 8497.51
RUN3      | 7953.8  | 7998.81 | 7737.02 | 13629.1 | 13774.3 | 8493.85 | 8494.42

Wowo! Ca a completement ralentit (5) et (6) ?


#### 4th series (taskset 0x1, sleep(10) between methods to let CPU cool down)

et j'ai supprimé le (1')

```
[jdgaraud@spiro-b321-clu benchmark]$ taskset 0x1 ./virtual_vs_block 100000000 100000000 10
```

test (ms) | (1)     | (2)     | (3)     | (4)     | (5)     | (6)
----------|---------|---------|---------|---------|---------|---------
RUN1      | 8006.87 | 7744.08 | 13637.2 | 13635.1 | 8501.8  | 8467.54
RUN2      | 8010.7  | 7745.55 | 13642.1 | 13627.8 | 8498.87 | 8464.58
RUN3      | 8009.34 | 7745.04 | 13631.2 | 13634.5 | 8494.04 | 8459.71



#### 5th series (taskset 0x1, sleep(0) so it is equal to 2nd series)

test (ms) | (1)     | (2)     | (3)     | (4)     | (5)     | (6)
----------|---------|---------|---------|---------|---------|---------
RUN1      | 8002.87 | 7740.14 | 13625.5 | 13619   | 8495.55 | 8459.77
RUN2      | 8006.76 | 7741.88 | 13633.4 | 13630.3 | 8495.01 | 8460.33
RUN3      | 8004.22 | 7742.02 | 13640.7 | 13636.6 | 8495.76 | 8460.14

Wowo, a ajouter sleep(0), je perds sur les methodes (5) et (6) ???

#### 6th series (taskset 0x1, #define sleep(A) {} so it is even more equal to 2nd series)

test (ms) | (1)     | (2)     | (3)     | (4)     | (5)     | (6)
----------|---------|---------|---------|---------|---------|---------
RUN1      | 7976.09 | 7740.86 | 13703.7 | 13631.2 | 7733.12 | 7680.6
RUN2      | 7977.96 | 7742.37 | 13707.5 | 13631.1 | 7730.28 | 7679.88
RUN3      | 7976.29 | 7740.87 | 13694.2 | 13628.1 | 7732.73 | 7679.44

On retombe bien sur 2nd series.


#### 7th series: ColMajor (pour rappel)

```
g++ -I../../../a-{core,solve}/src -std=c++20 -Wfatal-errors -frecord-gcc-switches -DEIGEN_USE_BLAS -DEIGEN_USE_MKL_ALL  -O3 -march=native -DNDEBUG    virtual_vs_block.cpp   -o virtual_vs_block
```

test (ms) | (1)     | (2)     | (3)     | (4)     | (5)     | (6)
----------|---------|---------|---------|---------|---------|---------
RUN1      | 9457.33 | 17679.1 | 24367.7 | 24259.1 | 17694.3 | 17693.9
RUN2      | 7999.74 | 12441.3 | 19555.1 | 19469.7 | 12452.2 | 12445.1
RUN3      | 7960.49 | 12292.6 | 19306.5 | 19223.1 | 12305.8 | 12296.8
RUN4      | 10290.5 | 12982.1 | 24937.1 | 24828.9 | 12974.1 | 12975.5

Costs +50% for any Eigen-based solution (and the C version result is wrong).

Has a 20% variability?
* RUN1 & 2: forgot taskset
* RUN3: `taskset 0x1`
* RUN4: `taskset --cpu-list 22`



### lscpu (spiro-b321, CascadeLake)
```
[jdgaraud@spiro-b321-clu benchmark]$ lscpu
CPU(s):              24
On-line CPU(s) list: 0-23
Thread(s) per core:  1
Core(s) per socket:  12
Socket(s):           2
NUMA node(s):        4
Vendor ID:           GenuineIntel
Model name:          Intel(R) Xeon(R) Gold 6226 CPU @ 2.70GHz
Stepping:            7
CPU MHz:             3499.227
L1d cache:           32K
L1i cache:           32K
L2 cache:            1024K
L3 cache:            19712K
NUMA node0 CPU(s):   0-5
NUMA node1 CPU(s):   6-11
NUMA node2 CPU(s):   12-17
NUMA node3 CPU(s):   18-23
```

Conclusion:
-----------

* RowMajor is optimal for this volume implementation and the loop on tetras
* lots of variability (no apparent reason), 10% to 50% between 2 runs ... on thursdays
* but next to zero variability on fridays??
* does virtual have a cost? cannot conclude on this test: it triggers a copy of Vector that has a cost. It may also be affected by branch prediction (need to shuffle Hexa and Tetra to break it).

TODO:
-----

- parallel (tbb or openmp or std::execution_policy)
- any idea how to get rid of the copy in 3 and 4 ?
- update VirtualCell.rank instead of recreate
- mixed elements (sorted and mixed)
- indirection par pointeur std::vector<Vertex*>   --> VC
