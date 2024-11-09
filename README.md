# Fast Decoding of Large Huffman Codes

An attempt at fast decoding :)

## Problem

This project tackles the problem of decoding a sequence (such as a text) that contains multiple (>256) unique Huffman codes. 

While Huffman encoding is relatively straightforward, decoding becomes challenging when there are more than 256 codes. For the <=256 codes case (like byte-based encoding), state-of-the-art (SOTA) implementations exist. The advantage of this case is that decoding tables (typically up to 2^12 values) easily fit in the L1 cache. However, with large dictionaries, tables may no longer fit in the L2 cache, posing performance challenges.

The question is: how can we decode large Huffman codes as quickly as possible?

## Tests

- Test1: watch results and explanation in `Test1/results.txt` and `Test1/README.md`
- Test2: watch results and explanation in `Test2/results.txt` and `Test2/README.md`
