# Fast Huffman Coding

This project implements several variants of Huffman coding designed for larger dictionaries.

## How to Run
1. Open `main.cpp` in `BitstreamTesting/BitstreamTesting/*`.

2. In `setupParams()`, set the input file name.
   Dont forget to put file (`*.txt`) inside `BitstreamTesting/BitstreamTesting/texts/*`.

3. In `main()`, change  
   ```cpp
   if (0) generateDictAndCodes();
   ```
   to
   ```cpp
   if (1) generateDictAndCodes();
   ```
   on the first run (to build dictionary and codes).
   After that, keep it `if (0)`.
   
4. Don't forget to run the adjust parameters if algorithm have any (`CODES_PER_BITSTREAM`, `LOOKUP_BITS`, `SKIP_BITS`).

## Selecting Algorithm

Open `defines.h` and set the value of:

```cpp
#define HUFF_TYPE X
```

Where possible X values could be seen in `defines.h`.

## Short description of optimizations

Opt. 1 – Merge `baseSym` and `baseCwd` into a single array (`Base`), one less memory load, one less addition operation.

Opt. 2 – Hybrid method: combine small table lookup with canonical decoding, can decode multiple symbols per iteration.

Opt. 3 – Faster bit-buffer handling: refill in 64-bit chunks instead of per symbol, process several iteration of decoding per 64-bit buffer.

Opt. 4 – SIMD parallelization of part of decoding algorithm (AVX/AVX2/AVX-512).
