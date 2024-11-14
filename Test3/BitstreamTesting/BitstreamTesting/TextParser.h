#pragma once

#include <string>
#include <vector>
#include <map>
using namespace std;


pair<int32_t, int32_t> parseText(string textName); // {dictSize, cntWords}
pair<int32_t, int32_t> pseudoParseText(string textName); // {dictSize, cntWords}
