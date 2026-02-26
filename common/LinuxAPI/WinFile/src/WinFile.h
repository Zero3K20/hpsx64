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

// Linux/POSIX implementation of the WinFile API (same interface as WindowsAPI/WinFile)

//#pragma once
#ifndef __WINFILE_H__
#define __WINFILE_H__

#include <string>
#include <cstdio>
#include <cstdint>
#include <thread>
#include <functional>

using namespace std;

namespace WinApi
{

	class File
	{
		FILE* hFile;
		bool m_async_pending;
		std::thread m_async_thread;

	public:
		// dummy for API compatibility
		struct OverlappedCompat {
			unsigned long Offset;
			unsigned long OffsetHigh;
		} Overlapped;

		unsigned long DistanceHigh;

		File() : hFile(nullptr), m_async_pending(false), DistanceHigh(0) {}
		~File() { if (hFile) fclose(hFile); }

		bool Create(string fileName, unsigned long Creation = 0, unsigned long Access = 0, unsigned long Attributes = 0);
		bool CreateSync(string fileName, unsigned long Creation = 0, unsigned long Access = 0, unsigned long Attributes = 0);
		bool CreateAsync(string fileName, unsigned long Creation = 0, unsigned long Access = 0, unsigned long Attributes = 0);

		unsigned long WriteString(string Data);

		unsigned long Read(char* DataOut, unsigned long BytesToRead);
		bool ReadSync(char* DataOut, unsigned long BytesToRead, unsigned long long SeekPosition);
		bool ReadAsync(char* DataOut, unsigned long BytesToRead, unsigned long long SeekPosition, void* Callback_Function);

		bool WaitAsync();

		bool Seek(unsigned long long offset);

		bool Close();

		long long Size();
	};

}

#endif	// end #ifndef __WINFILE_H__
