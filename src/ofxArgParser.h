#pragma once

#include "ofMain.h"

class ofxArgParser
{
public:
	
	static void init(int argc, const char** argv)
	{
		int n = 0;
		while (++n < argc)
		{
			string key = argv[n];
			
			if (key.substr(0, 2) == "--")
			{
				args[key.substr(2)] = argv[++n];
			}
			else if (key.substr(0, 1) == "-")
			{
				args[key.substr(1)] = argv[++n];
			}
		}
	}
	
	static bool hasKey(const string& key)
	{
		return args.find(key) != args.end();
	}
	
	static vector<string> allKeys()
	{
		vector<string> result;
		
		map<string, string>::iterator it = args.begin();
		while (it != args.end())
		{
			result.push_back((*it).first);
			it++;
		}
		
		return result;
	}
	
	static string getValue(const string& key)
	{
		return args[key];
	}
	
protected:
	
	static map<string, string> args;
	
};
