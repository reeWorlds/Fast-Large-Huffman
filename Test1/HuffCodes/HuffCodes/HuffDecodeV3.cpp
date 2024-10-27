#include <vector>
#include <array>
#include "defines.h"
#include "HuffDecodeV2.h"

#define LOOKUP_BITS 12
#define LOOKUP_SIZE (1 << LOOKUP_BITS)
#define LOOKUP_MASK (LOOKUP_SIZE - 1)

namespace HuffDecodeV3
{
	const int32_t MIN_VAL = -1000000007;

#pragma pack(push, 1)
	struct Node
	{
		int32_t remTreePos;
		uint8_t realLength;
	};
#pragma pack(pop)

#if DECODING_TYPE == 3'00
	Node t_firstTable[LOOKUP_SIZE]; // 5 high bits are code length, 27 low bits are ecoded code
	int32_t _remTreeSize = 0;
	int32_t t_remTree[(1 << 18)][2]; // if negative - leaf, otherwise - next tree node
#else
	Node t_firstTable[1 << 2];
	int32_t _remTreeSize = 0;
	uint32_t t_remTree[1 << 2][2];
#endif


	void fillFirstTableRec(int32_t remTreePos, int32_t realLength, uint32_t bits, uint32_t depth)
	{
		if (depth == LOOKUP_BITS)
		{
			t_firstTable[bits].remTreePos = remTreePos;
			t_firstTable[bits].realLength = realLength;

			return;
		}

		fillFirstTableRec(remTreePos, realLength, bits, depth + 1);
		fillFirstTableRec(remTreePos, realLength, bits | (1 << depth), depth + 1);
	}

	void buildRemTreeRec(vector <array<int32_t, 2> >& children, int32_t childrenPos, int32_t remTreePos)
	{
		if (children[childrenPos][0] < 0)
		{
			t_remTree[remTreePos][0] = children[childrenPos][0];
		}
		else
		{
			t_remTree[remTreePos][0] = _remTreeSize;
			_remTreeSize++;

			buildRemTreeRec(children, children[childrenPos][0], _remTreeSize - 1);
		}

		if (children[childrenPos][1] < 0)
		{
			t_remTree[remTreePos][1] = children[childrenPos][1];
		}
		else
		{
			t_remTree[remTreePos][1] = _remTreeSize;
			_remTreeSize++;

			buildRemTreeRec(children, children[childrenPos][1], _remTreeSize - 1);
		}
	}

	void buildTablesRec(vector <array<int32_t, 2> >& children, int32_t treePos, uint32_t bits, uint32_t depth)
	{
		if (treePos < 0)
		{
			fillFirstTableRec(treePos, depth, bits, depth);

			return;
		}

		if (depth == LOOKUP_BITS)
		{
			t_firstTable[bits].remTreePos = _remTreeSize;
			t_firstTable[bits].realLength = LOOKUP_BITS;

			_remTreeSize++;

			buildRemTreeRec(children, treePos, _remTreeSize - 1);

			return;
		}

		buildTablesRec(children, children[treePos][0], bits, depth + 1);
		buildTablesRec(children, children[treePos][1], bits | (1 << depth), depth + 1);
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

		buildTablesRec(children, dictSize - 2, 0, 0);
	}

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		uint64_t bitStream = stream[0];
		int32_t bitStreamLen = 32, streamPos = 1;

		for (int32_t codeI = 0; codeI < cntCodes; codeI++)
		{
			uint32_t bits = bitStream & LOOKUP_MASK;
			Node& node = t_firstTable[bits];

			int32_t pos = node.remTreePos;
			int32_t codeLen = node.realLength;

			while (pos >= 0)
			{
				int32_t bit = (bitStream >> codeLen) & 1;
				codeLen++;
				pos = t_remTree[pos][bit];
			}

			bitStream >>= codeLen;
			bitStreamLen -= codeLen;

			codes[codeI] = -(pos + 1);

			if (bitStreamLen <= 32)
			{
				bitStream |= ((uint64_t)stream[streamPos]) << bitStreamLen;
				bitStreamLen += 32;
				streamPos++;
			}
		}
	}
}
