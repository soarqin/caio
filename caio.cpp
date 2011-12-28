/*
 * Copyright (c) 2011, Soar Qin<soarchin@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <functional>
#include "filechain.h"

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>	
#endif

FileChain fc;
bool recursive = false;

template<typename F>
void findFiles(const std::string& path, const std::string& pattern, F func)
{
#ifdef WIN32
		WIN32_FIND_DATAA FindFileData;
		HANDLE hFind = FindFirstFile((path + "*").c_str(), &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
			return;
		do {
			if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(FindFileData.cFileName[0] == '.')
				{
					if(FindFileData.cFileName[1] == 0)
						continue;
					if(FindFileData.cFileName[1] == '.' && FindFileData.cFileName[2] == 0)
						continue;
				}
				if(recursive)
					findFiles(path + FindFileData.cFileName + '\\', pattern, func);
				else
					continue;
			}
			else if(PathMatchSpec(FindFileData.cFileName, pattern.c_str()))
			{
				func(path + FindFileData.cFileName);
			}
		} while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
#endif
}

void showHelp()
{
	std::cout << "Usage: caio <args> ..." << std::endl;
	std::cout << "  arguments:" << std::endl;
	std::cout << "    -r          enable recursive search" << std::endl;
	std::cout << "    -i<file>    pattern of include file(s)" << std::endl;
	std::cout << "    -o<file>    name of output file" << std::endl;
	std::cout << "    -e<file>    pattern of exclude file(s)" << std::endl;
	std::cout << "    -I<file>    add include directory" << std::endl;
	exit(0);
}

int main(int argc, char **argv)
{
	if(argc < 2)
		exit(0);
	using namespace std::placeholders;
	std::string outputFilename = "output.cpp";
	std::set<std::string> excludes;
	for(int i = 1; i < argc; ++ i)
	{
		if(argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
			case 'r':
				recursive = true;
				break;
			case 'I':
				if(argv[i][2] == 0)
				{
					if(++i >= argc)
						break;
					fc.addIncludeDir(argv[i]);
				}
				else
					fc.addIncludeDir(argv[i] + 2);
				break;
			case 'i':
				{
					std::string path;
					if(argv[i][2] == 0)
					{
						if(++i >= argc)
							break;
						path = argv[i];
					}
					else
						path = argv[i] + 2;
					if(path.empty())
						continue;
					size_t spos = path.find_last_of("\\/");
					std::string pattern;
					if(spos != std::string::npos)
					{
						pattern = std::string(path.begin() + spos + 1, path.end());
						if(pattern.empty())
							pattern = "*";
						path = path.substr(0, spos + 1);
					}
					else
					{
						pattern = path;
						path = "";
					}
					findFiles(path, pattern, std::bind(&FileChain::pushFile, &fc, _1));
				}
				break;
			case 'o':
				if(argv[i][2] == 0)
				{
					if(++i >= argc)
						break;
					outputFilename = argv[i];
				}
				else
					outputFilename = argv[i] + 2;
				break;
			case 'e':
				if(argv[i][2] == 0)
				{
					if(++i >= argc)
						break;
					excludes.insert(argv[i]);
				}
				else
					excludes.insert(argv[i] + 2);
				break;
			default:
				showHelp();
				break;
			}
			continue;
		}
	}
	std::for_each(excludes.begin(), excludes.end(), [](const std::string& filename) {
		std::string path = filename;
		if(path.empty())
			return;
		size_t spos = path.find_last_of("\\/");
		std::string pattern;
		if(spos != std::string::npos)
		{
			pattern = std::string(path.begin() + spos + 1, path.end());
			if(pattern.empty())
				pattern = "*";
			path = path.substr(0, spos + 1);
		}
		else
		{
			pattern = path;
			path = "";
		}
		findFiles(path, pattern, std::bind(&FileChain::excludeFile, &fc, _1));
	} );
	fc.generate(outputFilename.c_str());
	return 0;
}
