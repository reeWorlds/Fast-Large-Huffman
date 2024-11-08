#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include "HuffEncode.h"
using namespace std;

namespace HuffEncode
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
		streamLen += 4;
		
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
