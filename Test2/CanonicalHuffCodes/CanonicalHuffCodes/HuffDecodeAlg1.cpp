#include <vector>
#include "defines.h"
#include "HuffDecodeAlg1.h"

namespace HuffDecodeAlg1
{
	uint32_t t_minHuffLen, t_maxHuffLen;
	uint32_t t_cntCodesPerLen[32];
	uint32_t t_firstCode[32], t_firstHuffCode[32];
	uint32_t t_firstCodeDiff[32]; // = t_firstCode[i] - t_firstHuffCode[i] (2 operation -> 1 operation)
	uint64_t t_limit[32];

#if DECODING_TYPE == 1'00
#else
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
	}

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		uint64_t bitStream = (((uint64_t)stream[0]) << 32) | stream[1];
		int32_t bitStreamRequiredShift = 0, streamPos = 1;
		
		for (int32_t codeI = 0; codeI < cntCodes; codeI++) // line 3
		{
			uint32_t blockCode = bitStream >> 32;
			int32_t l = t_minHuffLen; // line 5

			while (blockCode >= t_limit[l]) // lines 6-7 (t_limit is modified)
			{
				l++;
			}

			bitStreamRequiredShift += l;
			bitStream <<= l;

			int32_t posPP = bitStreamRequiredShift >= 32;
			bitStreamRequiredShift -= posPP << 5;
			streamPos += posPP;
			bitStream |= ((uint64_t)stream[streamPos]) << bitStreamRequiredShift; // line 12 (kind of)

			blockCode >>= 32 - l; // line 8
			blockCode += t_firstCodeDiff[l]; // line 9
			
			codes[codeI] = blockCode; // line 11
		}
	}
}