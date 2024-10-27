# Fast Decoding of Large Huffman Codes

An attempt at fast decoding :)

## Problem

This project tackles the problem of decoding a sequence (such as a text) that contains multiple (>256) unique Huffman codes. 

While Huffman encoding is relatively straightforward, decoding becomes challenging when there are more than 256 codes. For the <=256 codes case (like byte-based encoding), state-of-the-art (SOTA) implementations exist. The advantage of this case is that decoding tables (typically up to 2^12 values) easily fit in the L1 cache. However, with large dictionaries, tables may no longer fit in the L2 cache, posing performance challenges.

The question is: how can we decode large Huffman codes as quickly as possible?

## Possible Solutions

### Option 1: Basic Implementation

Decode one bit at a time by traversing the Huffman tree. There are two possible approaches:
- **1.a**: With `if` statements.
- **1.b**: Use a branchless implementation with an additional array.

### Option 2: Large Table

Decode each Huffman code with a single table lookup. The table size will be $2^{`\texttt{max_huff_code_len}`}$, where `max_huff_code_len` is the length of the longest Huffman code. 

Note: most bits from one lookup will be reused in the next lookup, except when encountering the longest Huffman codes.

### Option 3: L1 Table + Basic Implementation

Shorter Huffman codewords tend to appear more frequently. Thus, even if `max_huff_code_len = 15`, most codes can be decoded with a 13-bit lookup table.

This approach involves using a table that fits in the L1 cache and, if necessary, decodes the remaining bits by traversing the tree bit by bit.

### Option 4: Multiple Smaller Tables

This method is similar to the basic implementation but decodes several bits (chunks) at a time, using multiple smaller tables.

In the basic implementation, tables of size $2^1$ are used (one element per bit). Here, tables are sized $2^{chunk\_size}$, where `chunk_size` could be, for example, 5.

### Option 5: Combination of Option 3 and Option 4

Decode the first bits using the approach in Option 3, and then decode the remaining bits as in Option 4.

## Notes

The current implementations are optimized on a conceptual level, though further, potentially somewhat significant, optimizations may be possible.
