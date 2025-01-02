
#include <random>
#include <algorithm>

void testSimulated()
{
	int32_t nCodes = 1 << 13;

	uint32_t* codes = new uint32_t[nCodes];
	
	uint32_t streamLen = 0;
	uint32_t* stream = new uint32_t[nCodes];

	uint32_t huffTableLen = 0;
	uint8_t* huffTable = new uint8_t[HUFF_TABLE_CAPACITY];

	uint32_t* codes_decoded = new uint32_t[nCodes + ARRAY_BUFF];

	int32_t numFreqsI = 0;
	vector <pair<int32_t, int32_t> > numfreqs;
	for (int32_t i = 0; i < 2; i++) { numfreqs.push_back(make_pair(i, nCodes / 4)); }
	for (int32_t i = 2; i < 10; i++) { numfreqs.push_back(make_pair(i, nCodes / 16)); }
	for (auto it : numfreqs)
	{
		int32_t numValue = it.first;
		int32_t numNumbers = it.second;

		for (int32_t i = 0; i < it.second; i++)
		{
			codes[numFreqsI] = numValue;
			numFreqsI++;
		}
	}
	mt19937 rng(47);
	shuffle(codes, codes + nCodes, rng);

	HuffEncodeV1_0_0::huffEncode(codes, nCodes, 10, stream, streamLen, huffTable, huffTableLen);

	HuffDecodeV1_0_0::precompute(huffTable, huffTableLen, 10);

	HuffDecodeV1_0_0::huffDecode(codes_decoded, nCodes, 10, stream, streamLen);

	// 0 - The memory blocks are equal.
	// < 0 — The first differing byte in ptr1 is less than the corresponding byte in ptr2.
	// > 0 — The first differing byte in ptr1 is greater than the corresponding byte in ptr2.
	int cmpResult = memcmp(codes, codes_decoded, nCodes * sizeof(uint32_t));
	cout << "Result of comparison = " << cmpResult << "\n";

	delete[] codes;
	delete[] stream;
	delete[] huffTable;
	delete[] codes_decoded;
}