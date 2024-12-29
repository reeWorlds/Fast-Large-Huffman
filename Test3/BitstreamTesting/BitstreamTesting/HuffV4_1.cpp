#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include "defines.h"
#include "HuffV4_1.h"
using namespace std;


namespace HuffEncodeV4_1
{
	void fillHuffLens(vector <array<int32_t, 2> >& cntPar, vector <uint32_t>& huffLens, uint32_t dictSize,
		int32_t i, uint32_t depth)
	{
		if (i < dictSize)
		{
			huffLens[i] = depth;
			return;
		}

		fillHuffLens(cntPar, huffLens, dictSize, cntPar[i][0], depth + 1);
		fillHuffLens(cntPar, huffLens, dictSize, cntPar[i][1], depth + 1);
	}

	void huffEncode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t& streamLen, uint8_t* huffTable, uint32_t& huffTableLen)
	{
		vector <array<int32_t, 2> > cntPar(2 * dictSize - 1); // weight, parent
		for (auto& it : cntPar) { it[0] = 0; it[1] = -1; }

		// count frequencies
		for (int32_t i = 0; i < cntCodes; i++)
		{
			cntPar[codes[i]][0]++;
		}

		// build some tree to get huffman codes lengths
		int32_t iLeaf = dictSize - 1, iNode = dictSize, nextNode = dictSize;

		for (int32_t iter = 0; iter < dictSize - 1; iter++)
		{
			if (iter == 0 || (iLeaf > 1 && cntPar[iLeaf - 1][0] <
				cntPar[iNode][0]))
			{
				cntPar[nextNode][0] = cntPar[iLeaf][0] + cntPar[iLeaf - 1][0];
				cntPar[iLeaf][1] = nextNode;
				cntPar[iLeaf - 1][1] = nextNode;
				iLeaf -= 2;
				nextNode++;
			}
			else if (iLeaf == -1 || (abs(iNode - nextNode) > 1 &&
				cntPar[iNode + 1][0] < cntPar[iLeaf][0]))
			{
				cntPar[nextNode][0] = cntPar[iNode][0] + cntPar[iNode + 1][0];
				cntPar[iNode][1] = nextNode;
				cntPar[iNode + 1][1] = nextNode;
				iNode += 2;
				nextNode++;
			}
			else
			{
				cntPar[nextNode][0] = cntPar[iLeaf][0] + cntPar[iNode][0];
				cntPar[iLeaf][1] = nextNode;
				cntPar[iNode][1] = nextNode;
				iLeaf--;
				iNode++;
				nextNode++;
			}
		}

		// now cntPar is represented as {child 1, child 2}
		for (auto& it : cntPar) { it[0] = -1; }
		for (int32_t i = int32_t(cntPar.size()) - 2; i >= 0; i--)
		{
			int32_t parent = cntPar[i][1];

			if (cntPar[parent][0] == -1)
			{
				cntPar[parent][0] = i;
			}
			else
			{
				cntPar[parent][1] = i;
			}
		}

		// prepare canonical huffman codes arrays
		vector <uint32_t> huffLens(dictSize);
		vector <uint32_t> huffCodes(dictSize);

		fillHuffLens(cntPar, huffLens, dictSize, int32_t(cntPar.size()) - 1, 0);

		uint32_t minHuffCodeLen = huffLens[0];
		uint32_t maxHuffCodeLen = huffLens[dictSize - 1];

		vector <int32_t> cntCodesPerLen(32, 0);
		for (int32_t i = 0; i < dictSize; i++)
		{
			cntCodesPerLen[huffLens[i]]++;
		}

		// make codes
		huffCodes[0] = 0;
		for (int32_t i = 1; i < dictSize; i++)
		{
			huffCodes[i] = (huffCodes[i - 1] + 1) << (huffLens[i] - huffLens[i - 1]);
		}

		cout << "Min/Max huff code len: " << minHuffCodeLen << " " << maxHuffCodeLen << "\n";

		// encode
		uint64_t buffer = 0;
		uint32_t bufferShift = 64;
		streamLen = 0;

		for (int32_t i = 0; i < cntCodes; i++)
		{
			uint32_t code = codes[i];
			uint32_t codeLen = huffLens[code];
			uint64_t huffCode = huffCodes[code];
			bufferShift -= codeLen;
			buffer |= huffCode << bufferShift;

			if (bufferShift <= 32)
			{
				stream[streamLen] = buffer >> 32;
				buffer <<= 32;
				bufferShift += 32;
				streamLen++;
			}
		}

		stream[streamLen] = buffer >> 32;
		stream[streamLen + 1] = buffer & 0xFFFFFFFF;
		stream[streamLen + 2] = 0;
		stream[streamLen + 3] = 0;
		stream[streamLen + 4] = 0;
		stream[streamLen + 5] = 0;
		stream[streamLen + 6] = 0;
		stream[streamLen + 7] = 0;
		streamLen += 8;

		huffTableLen = 0;
		*(uint32_t*)(huffTable + huffTableLen) = minHuffCodeLen;
		huffTableLen += 4;
		*(uint32_t*)(huffTable + huffTableLen) = maxHuffCodeLen;
		huffTableLen += 4;
		for (int32_t i = minHuffCodeLen; i <= maxHuffCodeLen; i++)
		{
			*(uint32_t*)(huffTable + huffTableLen) = cntCodesPerLen[i];
			huffTableLen += 4;
		}
	}
}


#define LOOKUP_BITS 12
#define LOOKUP_SIZE (1 << LOOKUP_BITS)
#define LOOKUP_MASK (LOOKUP_SIZE - 1)

namespace HuffDecodeV4_1
{
	uint32_t t_minHuffLen, t_maxHuffLen;
	uint32_t t_cntCodesPerLen[32];
	uint32_t t_firstCode[32], t_firstHuffCode[32];
	uint64_t t_limit[32];

#if HUFF_TYPE == 4'1
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
		lastHuffCode = t_firstHuffCode[t_minHuffLen] + t_cntCodesPerLen[t_minHuffLen] - 1;
		lastHuffCodeLen = t_minHuffLen;

		for (int32_t i = t_minHuffLen + 1; i <= t_maxHuffLen; i++)
		{
			if (t_cntCodesPerLen[i] > 0)
			{
				t_firstCode[i] = t_firstCode[i - 1] + t_cntCodesPerLen[i - 1];
				t_firstHuffCode[i] = (lastHuffCode + 1) << (i - lastHuffCodeLen);
				lastHuffCode = t_firstHuffCode[i] + t_cntCodesPerLen[i] - 1;
				lastHuffCodeLen = i;
			}
			else
			{
				t_firstCode[i] = t_firstHuffCode[i] = -1;
			}
		}

		t_limit[t_maxHuffLen] = -1;
		lastHuffCode = t_firstHuffCode[t_maxHuffLen];
		lastHuffCodeLen = t_maxHuffLen;
		for (int32_t i = t_maxHuffLen - 1; i >= t_minHuffLen; i--)
		{
			if (t_cntCodesPerLen[i] > 0)
			{
				t_limit[i] = (((uint64_t)lastHuffCode) << (64 - lastHuffCodeLen)) - 1;
				lastHuffCode = t_firstHuffCode[i];
				lastHuffCodeLen = i;
			}
			else
			{
				t_limit[i] = (((uint64_t)lastHuffCode) << (64 - lastHuffCodeLen)) - 1;
			}
		}

		for (int32_t firstBits = 0; firstBits < LOOKUP_SIZE; firstBits++)
		{
			int32_t l = t_minHuffLen;
			uint64_t blockCode = ((uint64_t) firstBits) << (64 - LOOKUP_BITS);

			while (blockCode > t_limit[l])
			{
				l++;
			}

			t_first[firstBits] = l;
		}
	}

	uint32_t* g_stream;
	uint64_t g_bitStream;
	uint32_t g_bitStreamRequiredShift;

	void setBitStream(uint32_t* stream)
	{
		g_stream = stream;
		g_bitStream = (((uint64_t)stream[0]) << 32) | stream[1];
		g_bitStreamRequiredShift = 0;
		g_stream += 1;
	}

	uint32_t inputBits(int32_t l)
	{
		uint32_t result = g_bitStream >> (64 - l);

		g_bitStream <<= l;
		g_bitStreamRequiredShift += l;

		if (g_bitStreamRequiredShift >= 32)
		{
			g_bitStreamRequiredShift -= 32;
			g_stream++;
		}

		g_bitStream |= ((uint64_t)g_stream[0]) << g_bitStreamRequiredShift;

		return result;
	}

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		uint64_t buffer = (((uint64_t)stream[0]) << 32) | stream[1];
		setBitStream(stream + 2);
		int32_t lookupShift = 64 - LOOKUP_BITS;

		for (int32_t codeI = 0; codeI < cntCodes; codeI++)
		{
			uint32_t p = buffer >> lookupShift;
			int32_t l = t_first[p];

			while (buffer > t_limit[l])
			{
				l++;
			}

			uint32_t codeword = buffer >> (64 - l);
			codeword = t_firstCode[l] + (codeword - t_firstHuffCode[l]);

			codes[codeI] = codeword;

			buffer = (buffer << l) | inputBits(l);
		}
	}
}