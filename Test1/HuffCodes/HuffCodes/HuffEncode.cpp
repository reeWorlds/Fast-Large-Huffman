#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include "HuffEncode.h"
using namespace std;

namespace HuffEncode
{
	void huffEncode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t& streamLen, uint8_t* huffTable, uint32_t& huffTableLen)
	{
		vector <array<int32_t, 2> > cntPar(2 * dictSize - 1); // weight, parent
		for (auto& it : cntPar) { it[0] = 0; it[1] = -1; }

		for (int32_t i = 0; i < cntCodes; i++)
		{
			cntPar[codes[i]][0]++;
		}

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

		// now cntPar is represented as {1-bit child, parent}
		for (auto& it : cntPar) { it[0] = -1; }
		for (int32_t i = 0; i < int32_t(cntPar.size()) - 1; i++)
		{
			cntPar[cntPar[i][1]][0] = i;
		}

		vector <uint32_t> codeBits;
		codeBits.reserve(32);
		vector <uint32_t> huffCodes(dictSize, 0);
		vector <uint32_t> huffLens(dictSize);

		uint32_t maxHuffCodeLen = 0;

		for (int32_t i = 0; i < dictSize; i++)
		{
			codeBits.clear();
			
			int32_t pos = i;
			while (pos + 1 != cntPar.size())
			{
				codeBits.push_back(pos == cntPar[cntPar[pos][1]][0]);
				pos = cntPar[pos][1];
			}

			huffLens[i] = codeBits.size();

			reverse(codeBits.begin(), codeBits.end());
			for (uint32_t j = 0; j < codeBits.size(); j++)
			{
				huffCodes[i] |= codeBits[j] << j;
			}

			maxHuffCodeLen = max(maxHuffCodeLen, huffLens[i]);
		}

		cout << "Max huff code len: " << maxHuffCodeLen << "\n";

		uint64_t buffer = 0;
		uint32_t bufferLen = 0;
		streamLen = 0;

		for (int32_t i = 0; i < cntCodes; i++)
		{
			uint32_t code = codes[i];
			buffer |= ((uint64_t) huffCodes[code]) << bufferLen;
			bufferLen += huffLens[code];

			if (bufferLen >= 32)
			{
				stream[streamLen] = buffer & 0xFFFFFFFFull;
				bufferLen -= 32;
				buffer >>= 32;
				streamLen++;
			}
		}

		stream[streamLen] = buffer & 0xFFFFFFFFull;
		stream[streamLen + 1] = buffer >> 32;
		stream[streamLen + 2] = 0;
		stream[streamLen + 3] = 0;
		streamLen += 4;

		for (int32_t i = 0; i < 2 * dictSize - 2; i++)
		{
			uint32_t parent = cntPar[i][1] - dictSize;
			huffTable[huffTableLen] = (parent & 0xFF0000) >> 16;
			huffTable[huffTableLen + 1] = (parent & 0xFF00) >> 8;
			huffTable[huffTableLen + 2] = parent & 0xFF;
			huffTableLen += 3;
		}
	}
}
