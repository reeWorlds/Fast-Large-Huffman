#pragma once

#include <stdint.h>
using namespace std;

namespace HuffDecodeV4
{
	void precompute(uint8_t* huffTable, uint32_t huffTableLen, uint32_t dictSize);

	void huffDecode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t streamLen);
}
