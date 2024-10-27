#include <vector>
#include <array>
#include "defines.h"
#include "HuffDecodeV2.h"

namespace HuffDecodeV2
{
	const int32_t MIN_VAL = -1000000007;

#if DECODING_TYPE == 2'00
	uint32_t t_maxCodeLen, t_maxCodeMask;
	uint32_t t_table[(1 << 21)]; // 8 high bits are code length, 24 low bits are ecoded code
#else
	uint32_t t_maxCodeLen, t_maxCodeMask;
	uint32_t t_table[(1 << 4)];
#endif

	int32_t getMaxLength(vector <array<int32_t, 2> >& children, int32_t pos)
	{
		if (pos < 0)
		{
			return 0;
		}

		return max(getMaxLength(children, children[pos][0]), getMaxLength(children, children[pos][1])) + 1;
	}

	void precompute(uint8_t* huffTable, uint32_t huffTableLen, uint32_t dictSize)
	{
		vector <int32_t> parents(2 * dictSize - 1, -1);

		for (int32_t i = 0; i < 2 * dictSize - 2; i++)
		{
			uint32_t parent = 0;
			parent |= ((uint32_t)huffTable[3 * i]) << 16;
			parent |= ((uint32_t)huffTable[3 * i + 1]) << 8;
			parent |= ((uint32_t)huffTable[3 * i + 2]);

			parents[i] = parent;
		}

		vector <array<int32_t, 2> > children(dictSize - 1);

		for (int32_t i = 0; i < children.size(); i++)
		{
			children[i][0] = children[i][1] = MIN_VAL;
		}

		for (int32_t i = 0; i < 2 * dictSize - 2; i++)
		{
			int32_t curParent = parents[i];
			int32_t curChildren = i < dictSize ? (-i - 1) : i - dictSize;

			if (children[curParent][0] == MIN_VAL)
			{
				children[curParent][0] = curChildren;
			}
			else
			{
				children[curParent][1] = curChildren;
			}
		}

		t_maxCodeLen = getMaxLength(children, dictSize - 2);
		t_maxCodeMask = (1 << t_maxCodeLen) - 1;

		for (int32_t mask = 0; mask <= t_maxCodeMask; mask++)
		{
			int32_t treePos = dictSize - 2, maskCopy = mask, bit;
			uint32_t codeLen = 0, decodedCode;

			while (treePos >= 0)
			{
				bit = maskCopy & 1;
				maskCopy >>= 1;
				codeLen++;
				treePos = children[treePos][bit];
			}

			decodedCode = -(treePos + 1);

			t_table[mask] = (codeLen << 24) | decodedCode;
		}
	}

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		uint64_t bitStream = stream[0];
		int32_t bitStreamLen = 32, streamPos = 1;

		for (int32_t codeI = 0; codeI < cntCodes; codeI++)
		{
			uint32_t bits = bitStream & t_maxCodeMask;
			uint32_t tableInfo = t_table[bits];
			
			uint32_t codeLen = tableInfo >> 24;
			codes[codeI] = tableInfo & 0x00FFFFFF;

			bitStreamLen -= codeLen;
			bitStream >>= codeLen;

			if (bitStreamLen <= 32)
			{
				bitStream |= ((uint64_t)stream[streamPos]) << bitStreamLen;
				bitStreamLen += 32;
				streamPos++;
			}
		}
	}
}
