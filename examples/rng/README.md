# Examples of multi-threaded random number generation in C++

This folder contains example codes for random number gneration in C++ considering mutti-threading. It shows both good and bad examples of random number generation in multi-threaded environment using OpenMP.

## The Examples

In order of simplicity:

1. `basic_rand.cpp`: A simple example of random number generation using `rand()` from the `cstdlib` library. Thi s is not a great example of random number generation; The way `rand()` should work is not specified in the C++ standard, and may either not be thread-safe, block in multi-threaded environments, or not be random enough.
2. `slow_random.cpp` A naive example using the `<random>` STL library. This version is slow because it creates a new random number generator (and initialises from a random device) for each random number generated. This is not necessary, and is slow.
3. `race_random.cpp` A bad example of random number generation in a multi-threaded environment. This example shows how not to generate random numbers in a multi-threaded environment using `<random>`. It uses the same random number generator in multiple threads, which is not thread-safe.
4. `fast_random.cpp` A good example of random number generation in a multi-threaded environment. This example shows how to generate random numbers in a multi-threaded environment using `<random>`. It uses a different random number generator in each thread, which is thread-safe, but does not require a new random number generator to be created for each random number generated.

## Building the Examples

To build the examples, you will need a C++ compiler that supports OpenMP. You can build the examples using the following commands:

```bash
cmake .
cmake --build .
```

## Running the Examples

You are likely to want to time the examples to see how they perform. On Mac/Linux can do this using the `time` command in bash. For example:

```bash
> export OMP_NUM_THREADS=4
> time ./fast_random
Fraction of multiples of 3: 0.333194

real    0m0.295s
user    0m1.109s
sys     0m0.021s
```

On Windows, you can use the Measure-Command cmdlet in PowerShell. For example:

```powershell
> $Env:OMP_NUM_THREADS =1
> Measure-Command { .\race_random.exe }

Days              : 0
Hours             : 0
Minutes           : 0
Seconds           : 0
Milliseconds      : 633
Ticks             : 6330871
TotalDays         : 7.32739699074074E-06
TotalHours        : 0.000175857527777778
TotalMinutes      : 0.0105514516666667
TotalSeconds      : 0.6330871
TotalMilliseconds : 633.0871
```

## Performance

In single-threaded mode `basic_rand.cpp` will be the fastest on many systems, although there can be small issues with the quality of the early random numbers generated. Here `fast_random.cpp` and `race_random.cpp` will take about the same time.

In multi-threaded mode, `fast_random.cpp` will often be the fastest approach, while still giving good quality random numbers. `race_random.cpp` will show speed-up but give poor quality random numbers, meanwhile `basic_rand.cpp` and `slow_random.cpp` may even be slower than in single-threaded mode.

## Notes

- This exmaple is set up to use the STL Mersenne Twister random number generator. This is a good quality random number generator, but it is not the fastest. If your group have the time, you may want to investigate other random number generators (but try to stick to reputable ones).

## Summary

- Follow patterns like in `fast_random.cpp` to generate random numbers in a multi-threaded environment across multiple platforms.
- If you really can't get it to work, use `basic_rand.cpp` in single-threaded mode to generate your random numbers.
- Don't follow the patterns in `race_random.cpp` or `slow_random.cpp` under any circumstances.
