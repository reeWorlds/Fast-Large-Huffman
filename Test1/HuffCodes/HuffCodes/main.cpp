#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include "defines.h"
#include "TextParser.h"
#include "HuffEncode.h"
#include "HuffDecodeV1a.h"
#include "HuffDecodeV1b.h"
#include "HuffDecodeV2.h"
#include "HuffDecodeV3.h"
#include "HuffDecodeV4.h"
#include "HuffDecodeV5.h"
using namespace std;


#define measure_start()  \
	QueryPerformanceFrequency(&freq); \
	QueryPerformanceCounter(&start);

#define measure_end() QueryPerformanceCounter(&finish); \
					m_time = ((finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

#define DECODING_ITERS 1000


#define HUFF_TABLE_CAPACITY (1 << 18)
#define ARRAY_BUFF 32


string g_textName;
int32_t g_dictSize, g_cntWords;


void setupParams()
{
	g_textName = "hp1";
}

void generateDictAndCodes()
{
	if (1)
	{
		pair<int32_t, int32_t> stats =  parseText(g_textName);
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

		HuffEncode::huffEncode(codes, g_cntWords, g_dictSize, stream, streamLen, huffTable,
			huffTableLen);

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
#if DECODING_TYPE == 1'00
	using namespace HuffDecodeV1a;
#elif DECODING_TYPE == 1'10
	using namespace HuffDecodeV1b;
#elif DECODING_TYPE == 2'00
	using namespace HuffDecodeV2;
#elif DECODING_TYPE == 3'00
	using namespace HuffDecodeV3;
#elif DECODING_TYPE == 4'00
	using namespace HuffDecodeV4;
#elif DECODING_TYPE == 5'00
	using namespace HuffDecodeV5;
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