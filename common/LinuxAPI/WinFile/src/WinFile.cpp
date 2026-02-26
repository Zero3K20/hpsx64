/*
	Copyright (C) 2012-2030

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Linux/POSIX implementation of the WinFile API

#include "WinFile.h"

#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;
using namespace WinApi;

bool File::Create(string fileName, unsigned long /*Creation*/, unsigned long /*Access*/, unsigned long /*Attributes*/)
{
	hFile = fopen(fileName.c_str(), "wb");
	return hFile != nullptr;
}

bool File::CreateSync(string fileName, unsigned long /*Creation*/, unsigned long /*Access*/, unsigned long /*Attributes*/)
{
	hFile = fopen(fileName.c_str(), "rb");
	return hFile != nullptr;
}

bool File::CreateAsync(string fileName, unsigned long /*Creation*/, unsigned long /*Access*/, unsigned long /*Attributes*/)
{
	// On Linux we use synchronous I/O run asynchronously via thread
	hFile = fopen(fileName.c_str(), "rb");
	return hFile != nullptr;
}

unsigned long File::WriteString(string Data)
{
	if (!hFile) return 0;
	return (unsigned long)fwrite(Data.c_str(), 1, Data.size(), hFile);
}

unsigned long File::Read(char* DataOut, unsigned long BytesToRead)
{
	if (!hFile) return 0;
	return (unsigned long)fread(DataOut, 1, BytesToRead, hFile);
}

bool File::ReadSync(char* DataOut, unsigned long BytesToRead, unsigned long long SeekPosition)
{
	if (!hFile) return false;
	if (fseeko(hFile, (off_t)SeekPosition, SEEK_SET) != 0) return false;
	size_t count = fread(DataOut, 1, BytesToRead, hFile);
	return (count == BytesToRead);
}

bool File::ReadAsync(char* DataOut, unsigned long BytesToRead, unsigned long long SeekPosition, void* /*Callback_Function*/)
{
	// Simplified: perform synchronously (async via thread if needed)
	Overlapped.Offset = (unsigned long)SeekPosition;
	Overlapped.OffsetHigh = (unsigned long)(SeekPosition >> 32);
	m_async_pending = true;

	bool ret = ReadSync(DataOut, BytesToRead, SeekPosition);
	m_async_pending = false;
	return ret;
}

bool File::WaitAsync()
{
	// No-op: async is synchronous in this implementation
	return true;
}

bool File::Seek(unsigned long long offset)
{
	if (!hFile) return false;
	return fseeko(hFile, (off_t)offset, SEEK_SET) == 0;
}

bool File::Close()
{
	if (!hFile) return false;
	int ret = fclose(hFile);
	hFile = nullptr;
	return ret == 0;
}

long long File::Size()
{
	if (!hFile) return 0;
	struct stat st;
	if (fstat(fileno(hFile), &st) != 0) return 0;
	return (long long)st.st_size;
}
