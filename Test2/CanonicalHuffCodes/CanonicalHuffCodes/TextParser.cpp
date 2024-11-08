#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include "TextParser.h"


bool isAlnum2(char c, char prev)
{
	return isalnum(c) || c == '-' || (int32_t(c) == 39 && isalnum(prev));
}

string getWord(stringstream& stream)
{
	string sa = "", sn = "";
	char a, c = 0, prev;

	if (stream.eof())
	{
		return "";
	}

	do
	{
		prev = c;
		stream >> noskipws >> c;
		sn += c == 10 ? '#' : c;
	} while (!isAlnum2(c, prev) && !stream.eof());

	if (stream.eof())
	{
		return sn;
	}

	stream.unget();
	sn = sn.substr(0, int32_t(sn.length()) - 1);

	do
	{
		prev = c;
		stream >> noskipws >> c;
		sa += c;
	} while (isAlnum2(c, prev) && !stream.eof());

	if (stream.eof())
	{
		return sa;
	}

	stream.unget();
	sa = sa.substr(0, int32_t(sa.length()) - 1);

	return sa;
}

pair<int32_t, int32_t> parseText(string textName)
{
	string textFileName = "texts/" + textName + ".txt";
	string codesFileName = "codes/" + textName + ".bin";
	string dictFileName = "_aux/" + textName + ".txt"; // frequency + word
	//string splitersFileName = "aux/" + textName + "_spliters";

	ifstream fIn(textFileName);
	stringstream stream;
	stream << fIn.rdbuf();
	fIn.close();

	map <string, int32_t> freqMap;
	map <string, int32_t> strToCode;
	vector <int32_t> codes;

	double entropy = 0.0;
	string word;

	while (!stream.eof())
	{
		word = getWord(stream);

		if (strToCode.count(word) == 0)
		{
			codes.push_back(strToCode.size());
			strToCode[word] = codes.back();
		}
		else
		{
			codes.push_back(strToCode[word]);
		}

		freqMap[word]++;
	}

	int32_t dictSize = freqMap.size(), cntWords = 0;
	vector <pair<int32_t, string> > freqVec;

	for (auto& it : freqMap)
	{
		freqVec.push_back(make_pair(it.second, it.first));
		cntWords += it.second;
	}

	cout << "Dictionary size = " << dictSize << "\n";
	cout << "Total words = " << cntWords << "\n";

	sort(freqVec.begin(), freqVec.end());
	reverse(freqVec.begin(), freqVec.end());

	vector <int32_t> codeTransform(dictSize, -1);
	for (int32_t i = 0; i < freqVec.size(); i++)
	{
		codeTransform[strToCode[freqVec[i].second]] = i;
	}

	for (int32_t i = 0; i < cntWords; i++)
	{
		codes[i] = codeTransform[codes[i]];
	}

	ofstream fCodes(codesFileName, ios::binary);
	fCodes.write((char*)&codes[0], cntWords * sizeof(int32_t));
	fCodes.close();

	ofstream fDict(dictFileName);

	for (auto& it : freqVec)
	{
		fDict << it.first << " " << it.second << "\n";
		entropy -= it.first * log2(double(it.first) / cntWords);
	}

	fDict.close();

	cout.precision(3);
	cout << "H0 entropy = " << fixed << entropy / 8.0 << " bytes\n\n";

	return make_pair(dictSize, cntWords);
}

pair<int32_t, int32_t> pseudoParseText(string textName)
{
	int32_t maxCode = 0, cntWords, code;

	ifstream fIn("codes/" + textName + ".bin", ios::binary);

	fIn.seekg(0, ios::end);
	cntWords = fIn.tellg() / sizeof(int32_t);
	fIn.seekg(0, ios::beg);

	for (int32_t i = 0; i < cntWords; i++)
	{
		fIn.read((char*)&code, sizeof(int32_t));
		maxCode = max(maxCode, code);
	}

	fIn.close();

	cout << "Dictionary size = " << maxCode + 1 << "\n";
	cout << "Total words = " << cntWords << "\n\n";

	return make_pair(maxCode + 1, cntWords);
}
