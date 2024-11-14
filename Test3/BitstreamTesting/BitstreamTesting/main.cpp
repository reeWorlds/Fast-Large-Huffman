#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include "defines.h"
#include "TextParser.h"
#include "HuffV1_0_0.h"
#include "HuffV1_1_0.h"
#include "HuffV1_0_1.h"
#include "HuffV1_1_1.h"
#include "HuffV2_0_0.h"
#include "HuffV2_1_0.h"
#include "HuffV2_0_1.h"
#include "HuffV2_1_1.h"
using namespace std;


#define measure_start()  \
	QueryPerformanceFrequency(&freq); \
	QueryPerformanceCounter(&start);

#define measure_end() QueryPerformanceCounter(&finish); \
					m_time = ((finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

#define DECODING_ITERS 1000


#define HUFF_TABLE_CAPACITY (1 << 10)
#define ARRAY_BUFF (256 + 5)


string g_textName;
int32_t g_dictSize, g_cntWords;


void setupParams()
{
	g_textName = "alice29";
}

void generateDictAndCodes()
{
	if (0)
	{
		pair<int32_t, int32_t> stats = parseText(g_textName);
		g_dictSize = stats.first;
		g_cntWords = stats.second;
	}
	else
	{
		pair<int32_t, int32_t> stats = pseudoParseText(g_textName);
		g_dictSize = stats.first;
		g_cntWords = stats.second;
	}
}

void encode()
{
#if HUFF_TYPE == 1'0'0
	using namespace HuffEncodeV1_0_0;
#elif HUFF_TYPE == 1'1'0
	using namespace HuffEncodeV1_1_0;
#elif HUFF_TYPE == 1'0'1
	using namespace HuffEncodeV1_0_1;
#elif HUFF_TYPE == 1'1'1
	using namespace HuffEncodeV1_1_1;
#elif HUFF_TYPE == 2'0'0
	using namespace HuffEncodeV2_0_0;
#elif HUFF_TYPE == 2'1'0
	using namespace HuffEncodeV2_1_0;
#elif HUFF_TYPE == 2'0'1
	using namespace HuffEncodeV2_0_1;
#elif HUFF_TYPE == 2'1'1
	using namespace HuffEncodeV2_1_1;
#endif

	if (1)
	{
		uint32_t* codes = new uint32_t[g_cntWords];

		uint32_t streamLen = 0;
		uint32_t* stream = new uint32_t[g_cntWords];

		uint32_t huffTableLen = 0;
		uint8_t* huffTable = new uint8_t[HUFF_TABLE_CAPACITY];

		string codesFileName = "codes/" + g_textName + ".bin";
		ifstream fCodes(codesFileName, ios::binary);
		fCodes.read((char*)codes, g_cntWords * sizeof(uint32_t));
		fCodes.close();

		huffEncode(codes, g_cntWords, g_dictSize, stream, streamLen, huffTable, huffTableLen);

		delete[] codes;

		string encodedFileName = "codes/" + g_textName + ".enc";
		ofstream fEncoded(encodedFileName, ios::binary);
		fEncoded.write((char*)stream, streamLen * sizeof(uint32_t));
		fEncoded.close();

		delete[] stream;

		string huffTableFileName = "_aux/" + g_textName + ".tbl";
		ofstream fHuffTable(huffTableFileName, ios::binary);
		fHuffTable.write((char*)huffTable, huffTableLen);
		fHuffTable.close();
		delete[] huffTable;

		cout << "Huff table size = " << huffTableLen << " bytes\n";
		cout << "Encoded size = " << streamLen * sizeof(uint32_t) << " bytes\n\n";
	}
}

void decode()
{
#if HUFF_TYPE == 1'0'0
	using namespace HuffDecodeV1_0_0;
#elif HUFF_TYPE == 1'1'0
	using namespace HuffDecodeV1_1_0;
#elif HUFF_TYPE == 1'0'1
	using namespace HuffDecodeV1_0_1;
#elif HUFF_TYPE == 1'1'1
	using namespace HuffDecodeV1_1_1;
#elif HUFF_TYPE == 2'0'0
	using namespace HuffDecodeV2_0_0;
#elif HUFF_TYPE == 2'1'0
	using namespace HuffDecodeV2_1_0;
#elif HUFF_TYPE == 2'0'1
	using namespace HuffDecodeV2_0_1;
#elif HUFF_TYPE == 2'1'1
	using namespace HuffDecodeV2_1_1;
#endif

	if (1)
	{
		double m_time;
		LARGE_INTEGER start, finish, freq;

		uint32_t* codes = new uint32_t[g_cntWords + ARRAY_BUFF];

		uint32_t streamLen;
		uint32_t* stream;

		string encodedFileName = "codes/" + g_textName + ".enc";
		ifstream fEncoded(encodedFileName, ios::binary);
		fEncoded.seekg(0, ios::end);
		streamLen = fEncoded.tellg() / sizeof(uint32_t);
		fEncoded.seekg(0, ios::beg);
		stream = new uint32_t[streamLen];
		fEncoded.read((char*)stream, streamLen * sizeof(uint32_t));
		fEncoded.close();

		uint32_t huffTableLen;
		uint8_t* huffTable;

		string huffTableFileName = "_aux/" + g_textName + ".tbl";
		ifstream fHuffTable(huffTableFileName, ios::binary);
		fHuffTable.seekg(0, ios::end);
		huffTableLen = fHuffTable.tellg();
		fHuffTable.seekg(0, ios::beg);
		huffTable = new uint8_t[huffTableLen];
		fHuffTable.read((char*)huffTable, huffTableLen);
		fHuffTable.close();

		measure_start();
		precompute(huffTable, huffTableLen, g_dictSize);
		measure_end();

		delete[] huffTable;

		cout.precision(6);
		cout << "Precomputation done in " << fixed << m_time << " s\n";

		measure_start();
		for (int i = 0; i < DECODING_ITERS; i++)
		{
			huffDecode(codes, g_cntWords, g_dictSize, stream, streamLen);
		}
		measure_end();

		cout << "Decoding time = " << fixed << m_time << " s per " << DECODING_ITERS << " iters\n\n";

		delete[] stream;

		string decodedFileName = "codes/" + g_textName + ".dec";
		ofstream fDecoded(decodedFileName, ios::binary);
		fDecoded.write((char*)codes, g_cntWords * sizeof(uint32_t));
		fDecoded.close();

		delete[] codes;
	}
}



int main()
{
	setupParams();

	generateDictAndCodes();

	encode();

	decode();


	return 0;
}