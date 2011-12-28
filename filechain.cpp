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

#include "filechain.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <io.h>

#ifdef WIN32
#include <windows.h>
#define IS_SEP(n) ((n) == '\\' || (n) == '/')
#define SEP_CHAR '\\'
#else
#define IS_SEP(n) ((n) == '/')
#define SEP_CHAR '/'
#endif

#define IS_SPACE(n) ((n) == ' ' || (n) == '\t' || (n) == '\n' || (n) == '\r')

static std::string absolutePath(const std::string& filename)
{
	char fn[1024];
#ifdef WIN32
	GetFullPathNameA(filename.c_str(), 1024, fn, NULL);
#else
	realpath(filename.c_str(), fn);
#endif
	return fn;
}

static std::string fixPath(const std::string& dirname, bool addTrailer = false)
{
	std::string ns;
	
	for(auto it = dirname.begin(); it != dirname.end(); ++ it)
	{
		if(IS_SEP(*it))
		{
			ns += SEP_CHAR;
			++ it;
			while(it != dirname.end() && IS_SEP(*it))
				++ it;
			-- it;
		}
		else
			ns += *it;
	}
	if(addTrailer && !ns.empty() && ns[ns.length() - 1] != SEP_CHAR)
		ns.push_back(SEP_CHAR);
	return ns;
}

static bool fileExists(const std::string& fn)
{
	return access(fn.c_str(), 0) == 0;
}

static bool fileNameEqual(const std::string& f1, const std::string& f2)
{
#ifdef WIN32
		return stricmp(f1.c_str(), f2.c_str()) == 0;
#else
		return f1 == f2;
#endif
}

FileChain::~FileChain()
{
	for(auto it = _files.begin(); it != _files.end(); ++ it)
		delete it->second;
	_files.clear();
}

void FileChain::addIncludeDir(const std::string& dirname)
{
	std::string alterName = fixPath(dirname, true);
	if(alterName.empty())
		return;
	for(auto it = _includedirs.begin(); it != _includedirs.end(); ++ it)
	{
		if(fileNameEqual(*it, alterName))
			return;
	}
	_includedirs.push_back(alterName);
}

void FileChain::pushFile(const std::string& file)
{
	_allFiles.insert(absolutePath(file));
}

void FileChain::excludeFile(const std::string& file)
{
	_allFiles.erase(absolutePath(file));
}

bool FileChain::processFile(const std::string& filename, std::string& output)
{
	auto it = _files.find(filename);
	if(it != _files.end())
	{
		if(it->second->status == 0)
		{
			std::cout << "Ciruculate including found in " << filename << "! Halts now!" << std::endl;
			exit(0);
		}
		return true;
	}

	std::cout << "Analyzing " << filename << " ..." << std::endl;
	std::ifstream fs(filename);
	if(!fs)
		return false;
	FileInfo * fi = new FileInfo;
	fi->filePath = filename;
	fi->status = 0;
	_files[filename] = fi;
	std::string line, l;
	output += "\n/********** BEGIN OF ";
	output += filename;
	output += " **********/\n\n";
	while(std::getline(fs, l))
	{
		if(l.back() == '\\')
		{
			line.insert(line.end(), l.begin(), l.end() - 1);
			continue;
		}
		else
			line += l;
		if(!line.empty())
		{
			std::string incf;
			if(!processLine(output, fi, line))
			{
				output += line;
				output += '\n';
			}
			line.clear();
		}
		else
		{
			output += line;
			output += '\n';
		}
	}
	if(!line.empty())
	{
		std::string incf;
		if(!processLine(output, fi, line))
		{
			output += line;
			output += '\n';
		}
	}
	output += "\n\n/************ END OF ";
	output += filename;
	output += " **********/\n";
	fi->status = 1;
	return true;
}

bool FileChain::processLine(std::string& output, FileInfo* fi, std::string& s)
{
#define SKIP_SPACES \
	while(it != s.end() && IS_SPACE(*it)) ++ it; \
	if(it == s.end()) return false

	auto it = s.begin();
	SKIP_SPACES;
	if(*it != '#')
		return false;
	++ it;
	SKIP_SPACES;
	if(strncmp(&s[it - s.begin()], "include", 7) != 0)
		return false;
	it += 7;
	SKIP_SPACES;
	char lquote;
	if(*it == '<' || *it == '"')
	{
		if(*it == '<')
			lquote = '>';
		else
			lquote = '"';
		++ it;
		size_t pos = s.find(lquote, it - s.begin());
		if(pos == std::string::npos)
			return false;
		std::string incfile(it, s.begin() + pos);
		std::cout << "    found include: " << incfile << std::endl;
		return tryInclude(output, fi, incfile);
	}
	return false;
}

bool FileChain::tryInclude(std::string& output, FileInfo* fi, std::string& s)
{
	std::string incf = absolutePath(fi->filePath + SEP_CHAR + ".." + SEP_CHAR + s);
	if(fileExists(incf) && _allFiles.find(incf) != _allFiles.end())
	{
		std::cout << "        found real file: " << incf << std::endl;
		return processFile(incf, output);
	}
	for(auto it = _includedirs.begin(); it != _includedirs.end(); ++ it)
	{
		incf = absolutePath(*it + s);
		if(fileExists(incf) && _allFiles.find(incf) != _allFiles.end())
		{
			std::cout << "        found real file: " << incf << std::endl;
			return processFile(incf, output);
		}
	}
	return false;
}

void FileChain::generate(const std::string& filename)
{
	using namespace std::placeholders;
	std::string output;
	std::for_each(_allFiles.begin(), _allFiles.end(), [&](const std::string& rfilename) {
		processFile(rfilename, output);
	});
	std::ofstream f(filename);
	f << output;
	f.close();
}
