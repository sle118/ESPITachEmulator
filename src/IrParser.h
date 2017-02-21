/*
 * ITachIRParser.h
 *
 *  Created on: Feb 16, 2017
 *      Author: sle118
 */
#include <Arduino.h>
#include "functional"
#include "IrServiceBase.h"
#include "StringArray.h"

#ifndef SRC_ITACHIRPARSER_H_
#define SRC_ITACHIRPARSER_H_

namespace iTach {

class ParserDictionaryEntry
{
public:
	char index;
	unsigned int val[2];
};


class IrParser {
public:
	unsigned int value1;
	unsigned int value2;
	unsigned int totalNumberOfValues;
	unsigned int totalNumberOfPairs;

	IrParser(long startPos, const String& data);
	~IrParser(){commandDict.free();}
	bool isSameWithTolerance(unsigned int val1a, unsigned int val2a,unsigned int val1b, unsigned int val2b);
	bool getNext() ;
	String& getCompressedString(){return _compressed;}

private:
	LinkedList<ParserDictionaryEntry *> commandDict;
	const String& _data;
	String _compressed;
	unsigned int _val[2];
	const char * _currentChar;
	unsigned int _valpos;
	const char * _prevChar;
	const char * _startString;

	ParserDictionaryEntry * getPair(unsigned int value1, unsigned int value2);
	ParserDictionaryEntry * getPair(char index);
	void moveToNextValue(bool wasNumericEnding=false);


};

} /* namespace ITach */

#endif /* SRC_ITACHIRPARSER_H_ */
