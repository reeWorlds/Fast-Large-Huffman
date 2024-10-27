#include <vector>
#include "defines.h"
#include "HuffDecodeV1b.h"

namespace HuffDecodeV1b
{
	const int32_t MIN_VAL = -1000000007;

	struct Node
	{
		int32_t nextNode[2];
		uint32_t nextCode[2];
	};

#if DECODING_TYPE == 1'10
	int32_t t_startPosition;
	Node t_table[(1 << 18)];
#else
	int32_t t_startPosition;
	Node t_table[2];
#endif

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

		t_startPosition = dictSize - 2;

		for (int32_t i = 0; i <= t_startPosition; i++)
		{
			t_table[i].nextNode[0] = t_table[i].nextNode[1] = MIN_VAL;
		}

		for (int32_t i = 0; i < 2 * dictSize - 2; i++)
		{
			int32_t parent = parents[i];

			if (t_table[parent].nextNode[0] == MIN_VAL)
			{
				if (i < dictSize)
				{
					t_table[parent].nextNode[0] = t_startPosition;
					t_table[parent].nextCode[0] = i;
				}
				else
				{
					t_table[parent].nextNode[0] = i - dictSize;
					t_table[parent].nextCode[0] = 0;
				}
			}
			else
			{
				if (i < dictSize)
				{
					t_table[parent].nextNode[1] = t_startPosition;
					t_table[parent].nextCode[1] = i;
				}
				else
				{
					t_table[parent].nextNode[1] = i - dictSize;
					t_table[parent].nextCode[1] = 0;
				}
			}
		}
	}

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		int32_t streamLenM2 = streamLen - 2, codesI = 0;
		
		uint32_t bits, nextCode;
		int32_t bit, nextNode = t_startPosition;

		for (int32_t streamI = 0; streamI < streamLenM2; streamI++)
		{
			bits = stream[streamI];

			for (int32_t i = 0; i < 32; i++)
			{
				bit = bits & 1;
				Node& node = t_table[nextNode];
				bits >>= 1;

				nextNode = node.nextNode[bit];
				codes[codesI] = node.nextCode[bit];

				codesI += nextNode == t_startPosition;
			}
		}
	}
}
