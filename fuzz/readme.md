# Fuzzing

Building the fuzz tests requires [AFLplusplus](https://github.com/AFLplusplus/AFLplusplus/).

Make sure that AFLplusplus's binaries are in your path. It is recommended to run ```sudo afl-system-config``` to smooth out your fuzzing experience.
The ```afl_helper.sh``` script can be used to prepare fuzzing input, aswell as to build and run the actual fuzzing session.
