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

#ifndef _FILECHAIN_H_
#define _FILECHAIN_H_

#include <vector>
#include <set>
#include <map>
#include <list>
#include <string>
#include <string.h>

struct FileNameLess
{
	inline bool operator()(const std::string& f1, const std::string& f2) const
	{
	#ifdef WIN32
			return stricmp(f1.c_str(), f2.c_str()) < 0;
	#else
			return f1 < f2;
	#endif
	}
};

class FileChain
{
	struct FileInfo
	{
		FileInfo(): status(0) { }
		std::string filePath;
		int status; /* 0 - processing   1 - processed */
	};
public:
	~FileChain();
public:
	void addIncludeDir(const std::string& dirname);
	void pushFile(const std::string& file);
	void excludeFile(const std::string& file);
	void generate(const std::string& filename);
private:
	bool processFile(const std::string& filename, std::string& output);
	bool processLine(std::string& output, FileInfo * fi, std::string& s);
	bool tryInclude(std::string& output, FileInfo * fi, std::string& s);
private:
	std::vector<std::string> _includedirs;
	std::map<std::string, FileInfo *, FileNameLess> _files;
	std::set<std::string, FileNameLess> _allFiles;
};

#endif
