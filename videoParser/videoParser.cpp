// videoParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MediaDataParser.h"


int _tmain(int argc, _TCHAR* argv[])
{
    MediaDataParser parser;
    parser.parseVideo();
    parser.testCrc();
	return 0;
}

