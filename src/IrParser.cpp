/*
 * ITachIRParser.cpp
 *
 *  Created on: Feb 16, 2017
 *      Author: sle118
 */

#include <IrParser.h>


namespace iTach {

IrParser::IrParser(long startPos, const String& data):
		commandDict( LinkedList<ParserDictionaryEntry * >([](ParserDictionaryEntry *h) {delete h;})),
		_data(data)
{
	memset(&_val,0,sizeof(_val));
	_valpos=0;
	value1=0;
	value2=0;
	_startString = _data.c_str();
	_currentChar = &_startString[startPos];
	_prevChar = _currentChar;
	totalNumberOfValues=0;
	totalNumberOfPairs=0;
	_compressed = "";
	DEBUG_PRINTF("New parser for : %s\n",data.c_str());
}
bool IrParser::isSameWithTolerance(unsigned int val1a, unsigned int val2a,unsigned int val1b, unsigned int val2b)
{
	return abs((long)val1a-(long)val1b)<=3 && abs((long)val2a-(long)val2b)<=3;
}


ParserDictionaryEntry * IrParser::getPair(unsigned int value1, unsigned int value2)
{
	// this is a dictionary item, let's see if it's been already entered
	for (const auto& d : commandDict) {
		if(isSameWithTolerance(d->val[0],d->val[1], value1, value2))
		{
			DEBUG_PRINTF("Found dict[%c]=%d,%d\n", d->index, d->val[0],d->val[1]);
			return d;
		}
	}
	return NULL;
}
void IrParser::moveToNextValue(bool wasNumericEnding)
{
	if(*_currentChar!='\0' && !wasNumericEnding)
		_currentChar++;
	while(*_currentChar == ',')
		_currentChar++;
	_prevChar=_currentChar;
}
ParserDictionaryEntry * IrParser::getPair(char index)
{
	// this is a dictionary item, let's see if it's been already entered
	for (const auto& d : commandDict) {
		if(d->index == index)
		{
			DEBUG_PRINTF("Found dict[%c]=%d,%d\n", d->index, d->val[0],d->val[1]);
			return d;
		}
	}
	return NULL;
}


bool IrParser::getNext()
{
	ParserDictionaryEntry * dictionaryEntry=NULL;
	if(*_prevChar=='\0')
		return false;
	DEBUG_PRINTF("Next: %s\n",_currentChar);

	//we're going to cram a lot of data through the serial port and hold
	// processing for a while, so better reset the wdt with every pass
	DEBUG_RESET_WDT_IF_PRINT;

	while(true)
	{
		dictionaryEntry = NULL;
		if ((*_currentChar == ',' || *_currentChar=='\0' || (isAlpha(*_currentChar) && _prevChar != _currentChar)))

		{
			// The following combinarions signal the end of a pair
			// nnn, : numbers followed by a comma
			// nnna : numbers followed by an index
			// nnn  : numbers at the end of the string

			// Grab the value
			size_t startPos = _prevChar-_startString;
			size_t endPos = _currentChar-_startString;
			_val[_valpos] = _data.substring(startPos, endPos).toInt();

			moveToNextValue(true);
			DEBUG_PRINTF("val[%d] @pos %d,%d ( next char %c): [%s]\n",
							_valpos,
							startPos,
							endPos,
							*_prevChar,
							_data.substring(startPos, endPos).c_str());

			totalNumberOfValues++;
			_valpos++;
			if(_valpos > 1)
			{
				break;
			}
		}
		else if (isAlpha(*_currentChar) && _currentChar == _prevChar)
		{
			// Processing a compression index
			dictionaryEntry = getPair(*_currentChar);
			moveToNextValue(false);
			break;
		}
		else
			_currentChar++;
	}
	if(dictionaryEntry!=NULL)
	{
		// we have both values found!
		value1 = dictionaryEntry->val[0];
		value2 = dictionaryEntry->val[1];
		totalNumberOfValues+=2;
		DEBUG_PRINTF("Using dict [%d, %d], next char %c\n",value1,value2,*_prevChar);
		// append the char to the current compressed string
		_compressed+= (char)dictionaryEntry->index;
	}
	else if(_valpos > 1 || _prevChar == '\0')
	{
		char szStringBuffer[25]={0};
		// we have both values found!
		value1 = _val[0];
		value2 = _val[1];


		if(*_prevChar !='\0')
		{
			// check if the value pair is in the dictionary already
			dictionaryEntry = getPair(value1,value2);

			if(dictionaryEntry ==NULL )
			{
				// the pair isn't in the dictionary yet, so add it
				// only if we're not at the end of the string
				dictionaryEntry = new ParserDictionaryEntry();
				dictionaryEntry->val[0] = _val[0];
				dictionaryEntry->val[1] = _val[1];
				dictionaryEntry->index = 'A'+totalNumberOfPairs;
				commandDict.add(dictionaryEntry);
				DEBUG_PRINTF("New dict [%c], values [%d,%d]\n",
						dictionaryEntry->index,
						dictionaryEntry->val[0],
								dictionaryEntry->val[1]);
				totalNumberOfPairs++;
				// so append values to the compressed string
				snprintf(szStringBuffer, sizeof(szStringBuffer)-1,",%d,%d", dictionaryEntry->val[0],dictionaryEntry->val[1]);
				_compressed+= szStringBuffer;
			}
			else
			{
				// This value was stored already as a dictionary,
				// so append it to the compressed string
				_compressed+= (char)dictionaryEntry->index;
			}
		}
		else
		{
			// last chars are never compressed, so add them as-is to the compressed
			// string
			snprintf(szStringBuffer, sizeof(szStringBuffer)-1,",%d,%d", _val[0],_val[1]);
			_compressed+= szStringBuffer;
		}
	}
	else
	{
		DEBUG_PRINTF("Well, something went wrong after %d values... \n",totalNumberOfValues);
		return false; //we're definitely not expecting this
	}


	DEBUG_PRINTF("returning %d,%d\n", value1,value2);
	// reset the values for the next pass
	_valpos = 0;
	memset(&_val,0,sizeof(_val));

	return true;
	}

} /* namespace ITach */
