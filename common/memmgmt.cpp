
#include <windows.h>

#include <iostream>

#include "memmgmt.h"


std::vector<void*> MemoryManager::MapSharedMemoryToSpecifiedAddresses(size_t numberOfAddresses, const uint64_t* targetAddresses, size_t viewSize, const uint32_t* protections) {
	std::vector<void*> views;

	// Create a memory-backed section object
	HANDLE hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, static_cast<DWORD>(viewSize), nullptr);
	if (!hMapping) {
		std::cerr << "CreateFileMapping failed. Error: " << GetLastError() << "\n";
		return views;
	}

	// Map the section to each specified address
	/*
	for (void* addr : targetAddresses) {
		//void* view = MapViewOfFileEx(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0, addr);
		void* view = MapViewOfFileEx(hMapping, 0, 0, 0, 0, addr);
		if (!view) {
			std::cerr << "MapViewOfFileEx failed at address " << addr << ". Error: " << GetLastError() << "\n";
			break;
		}
		views.push_back(view);
	}
	*/

	for (size_t i = 0; i < numberOfAddresses; i++) {
		//void* view = MapViewOfFileEx(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0, addr);
		void* view = MapViewOfFileEx(hMapping, protections[i], 0, 0, 0, (void*)(targetAddresses[i]));
		if (!view) {
			std::cerr << "MapViewOfFileEx failed at address " << targetAddresses[i] << ". Error: " << GetLastError() << "\n";
			break;
		}
		views.push_back(view);
	}

	CloseHandle(hMapping);
	return views;
}
