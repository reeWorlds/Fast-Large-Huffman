#include <vector>
#include "defines.h"
#include "HuffDecodeV1a.h"

namespace HuffDecodeV1a
{
	const int32_t MIN_VAL = -1000000007;

#if DECODING_TYPE == 1'00
	int32_t t_startPosition;
	int32_t t_children[(1 << 18)][2];
#else
	int32_t t_startPosition;
	int32_t t_children[2][2];
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
			t_children[i][0] = t_children[i][1] = MIN_VAL;
		}

		for (int32_t i = 0; i < 2 * dictSize - 2; i++)
		{
			int32_t parent = parents[i];
			int32_t children = i < dictSize ? (-i - 1) : i - dictSize;

			if (t_children[parent][0] == MIN_VAL)
			{
				t_children[parent][0] = children;
			}
			else
			{
				t_children[parent][1] = children;
			}
		}
	}

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		uint64_t bitStream = stream[0];
		int32_t bitStreamLen = 32, streamPos = 1;

		for (int32_t codeI = 0; codeI < cntCodes; codeI++)
		{
			int32_t tablePos = t_startPosition;
			while (tablePos >= 0)
			{
				int32_t bit = bitStream & 1;
				tablePos = t_children[tablePos][bit];

				bitStream >>= 1;
				bitStreamLen--;
			}

			codes[codeI] = -(tablePos + 1);

			if (bitStreamLen <= 32)
			{
				bitStream |= ((uint64_t)stream[streamPos]) << bitStreamLen;
				bitStreamLen += 32;
				streamPos++;
			}
		}
	}

	void huffDecode_v2(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen)
	{
		// Different implementation of the same idea
		// Seems to work ~5% slower (which is around statistical/noise error)
		// It is more aapropriate for `HuffDecodeV1b` implementation

		int32_t streamLenM2 = streamLen - 2, codesI = 0;
		int32_t tablePos = t_startPosition;

		for (int32_t streamI = 0; streamI < streamLenM2; streamI++)
		{
			uint32_t bits = stream[streamI];

			for (int32_t i = 0; i < 32; i++)
			{
				int32_t bit = bits & 1;
				tablePos = t_children[tablePos][bit];
				bits >>= 1;

				if (tablePos < 0)
				{
					codes[codesI] = -(tablePos + 1);
					tablePos = t_startPosition;
					codesI++;
				}
			}
		}
	}
}