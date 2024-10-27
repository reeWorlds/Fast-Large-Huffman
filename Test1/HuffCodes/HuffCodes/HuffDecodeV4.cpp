#include <vector>
#include <array>
#include "defines.h"
#include "HuffDecodeV4.h"

#define STEP_BITS 8
#define STEP_SIZE (1 << STEP_BITS)
#define STEP_MASK (STEP_SIZE - 1)

namespace HuffDecodeV4
{
	const int32_t MIN_VAL = -1000000007;

	struct Node
	{
		int32_t nextNode[1 << STEP_BITS];
		uint8_t realLen[1 << STEP_BITS];
	};

#if DECODING_TYPE == 4'00
	int32_t _treeSize;
	Node t_tree[1 << (20 - STEP_BITS)];
#else
	int32_t _treeSize;
	Node t_tree[1 << 2];
#endif

	void buildTableRec(vector <array<int32_t, 2> >& children, int32_t childPos, int32_t treePos)
	{
		for (int32_t bits = 0; bits < STEP_SIZE; bits++)
		{
			int32_t finalNode = childPos;
			
			int32_t i;
			for (i = 0; i < STEP_BITS; i++)
			{
				finalNode = children[finalNode][(bits >> i) & 1];
				if (finalNode < 0)
				{
					break;
				}
			}

			if (finalNode < 0)
			{
				t_tree[treePos].nextNode[bits] = finalNode;
				t_tree[treePos].realLen[bits] = i + 1;
			}
			else
			{
				t_tree[treePos].nextNode[bits] = _treeSize;
				t_tree[treePos].realLen[bits] = STEP_BITS;
				_treeSize++;

				buildTableRec(children, finalNode, _treeSize - 1);
			}
		}
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

		_treeSize = 1;
		buildTableRec(children, dictSize - 2, 0);
	}

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		uint64_t bitStream = stream[0];
		int32_t bitStreamLen = 32, streamPos = 1;
		
		for (int32_t codeI = 0; codeI < cntCodes; codeI++)
		{
			int32_t pos = 0, shift = 0;

			while (pos >= 0)
			{
				uint32_t bits = (bitStream >> shift) & STEP_MASK;

				Node& node = t_tree[pos];

				pos = node.nextNode[bits];
				shift += node.realLen[bits];
			}

			bitStream >>= shift;
			bitStreamLen -= shift;
		
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
