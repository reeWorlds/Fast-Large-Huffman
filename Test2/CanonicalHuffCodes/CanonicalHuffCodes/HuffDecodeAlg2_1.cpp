#include <vector>
#include <immintrin.h>
#include "defines.h"
#include "HuffDecodeAlg2.h"

#define CODES_BLOCK_SIZE (256u)

#define LOOKUP_BITS 12
#define LOOKUP_SIZE (1 << LOOKUP_BITS)
#define LOOKUP_MASK (LOOKUP_SIZE - 1)

namespace HuffDecodeAlg2_1
{
	uint32_t t_minHuffLen, t_maxHuffLen;
	uint32_t t_cntCodesPerLen[32];
	uint32_t t_firstCode[32], t_firstHuffCode[32];
	uint32_t t_firstCodeDiff[32]; // = t_firstCode[i] - t_firstHuffCode[i] (2 operation -> 1 operation)
	uint64_t t_limit[32];

#if DECODING_TYPE == 2'10
	int8_t t_first[LOOKUP_SIZE];
#else
	int8_t t_first[2];
#endif

	void precompute(uint8_t* huffTable, uint32_t huffTableLen, uint32_t dictSize)
	{
		int32_t huffTablePos = 0;

		t_minHuffLen = *(uint32_t*)(huffTable + huffTablePos);
		huffTablePos += 4;
		t_maxHuffLen = *(uint32_t*)(huffTable + huffTablePos);
		huffTablePos += 4;
		for (int32_t i = t_minHuffLen; i <= t_maxHuffLen; i++)
		{
			t_cntCodesPerLen[i] = *(uint32_t*)(huffTable + huffTablePos);
			huffTablePos += 4;
		}

		int32_t lastHuffCode, lastHuffCodeLen;

		t_firstCode[t_minHuffLen] = 0;
		t_firstHuffCode[t_minHuffLen] = 0;
		t_firstCodeDiff[t_minHuffLen] = t_firstCode[t_minHuffLen] - t_firstHuffCode[t_minHuffLen];
		lastHuffCode = t_firstHuffCode[t_minHuffLen] + t_cntCodesPerLen[t_minHuffLen] - 1;
		lastHuffCodeLen = t_minHuffLen;

		for (int32_t i = t_minHuffLen + 1; i <= t_maxHuffLen; i++)
		{
			if (t_cntCodesPerLen[i] > 0)
			{
				t_firstCode[i] = t_firstCode[i - 1] + t_cntCodesPerLen[i - 1];
				t_firstHuffCode[i] = (lastHuffCode + 1) << (i - lastHuffCodeLen);
				t_firstCodeDiff[i] = t_firstCode[i] - t_firstHuffCode[i];
				lastHuffCode = t_firstHuffCode[i] + t_cntCodesPerLen[i] - 1;
				lastHuffCodeLen = i;
			}
			else
			{
				t_firstCode[i] = t_firstHuffCode[i] = t_firstCodeDiff[i] = -1;
			}
		}

		t_limit[t_maxHuffLen] = 1ull << 32;
		lastHuffCode = t_firstHuffCode[t_maxHuffLen];
		lastHuffCodeLen = t_maxHuffLen;
		for (int32_t i = t_maxHuffLen - 1; i >= t_minHuffLen; i--)
		{
			if (t_cntCodesPerLen[i] > 0)
			{
				t_limit[i] = (((uint64_t)lastHuffCode) << (32 - lastHuffCodeLen));
				lastHuffCode = t_firstHuffCode[i];
				lastHuffCodeLen = i;
			}
			else
			{
				t_limit[i] = (((uint64_t)lastHuffCode) << (32 - lastHuffCodeLen));
			}
		}

		for (int32_t firstBits = 0; firstBits < LOOKUP_SIZE; firstBits++)
		{
			int32_t l = t_minHuffLen;
			uint32_t blockCode = (firstBits << (32 - LOOKUP_BITS));

			while (blockCode >= t_limit[l])
			{
				l++;
			}

			t_first[firstBits] = l;
		}
	}

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		uint32_t tmpCodes[CODES_BLOCK_SIZE];
		uint32_t tmpLens[CODES_BLOCK_SIZE];

		uint64_t bitStream = (((uint64_t)stream[0]) << 32) | stream[1];
		int32_t bitStreamRequiredShift = 0, streamPos = 1;
		int32_t lookupShift = 64 - LOOKUP_BITS;

		__m256i v32 = _mm256_set1_epi32(32);

		for (int32_t codeI = 0; codeI < cntCodes; codeI += CODES_BLOCK_SIZE)
		{
			int32_t codesInBlock = min(CODES_BLOCK_SIZE, cntCodes - codeI);
			
			for (int32_t i = 0; i < codesInBlock; i++)
			{
				uint32_t blockCode = bitStream >> 32;
				int32_t l = t_first[bitStream >> lookupShift];
			
				while (blockCode >= t_limit[l])
				{
					l++;
				}
			
				bitStreamRequiredShift += l;
				bitStream <<= l;
			
				if (bitStreamRequiredShift >= 32)
				{
					bitStreamRequiredShift -= 32;
					streamPos++;
					bitStream |= ((uint64_t)stream[streamPos]) << bitStreamRequiredShift;
				}
			
				tmpCodes[i] = blockCode;
				tmpLens[i] = l;
			}
			
			for (int32_t i = 0; i < CODES_BLOCK_SIZE; i += 8)
			{
				// load intermediate value of `blockCode` and `l`
				__m256i codes_8 = _mm256_loadu_si256((__m256i*) & tmpCodes[i]);
				__m256i lens_8 = _mm256_loadu_si256((__m256i*) & tmpLens[i]);
			
				// blockCode >>= 32 - l
				__m256i shifted_8 = _mm256_srlv_epi32(codes_8, _mm256_sub_epi32(v32, lens_8));
				// load (lookup) value of t_firstCodeDiff[l]
				__m256i lookup_8 = _mm256_i32gather_epi32((const int32_t*)t_firstCodeDiff, lens_8, 4);
			
				// blockCode += t_firstCodeDiff[l]
				__m256i result_8 = _mm256_add_epi32(shifted_8, lookup_8);
				// store decoded codes
				_mm256_storeu_si256((__m256i*) & codes[codeI + i], result_8);
			}
		}
	}
}
