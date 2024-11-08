#pragma once

#include <stdint.h>
using namespace std;

namespace HuffEncode
{
	void huffEncode(uint32_t* codes, uint32_t cntCodes, uint32_t dictSize, uint32_t* stream,
		uint32_t& streamLen, uint8_t* huffTable, uint32_t& huffTableLen);
}
