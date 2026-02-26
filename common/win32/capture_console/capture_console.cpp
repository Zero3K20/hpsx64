
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "capture_console.h"

void CaptureConsoleToFile(const std::string& filename) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
		std::cerr << "Failed to get console screen buffer info\n";
		return;
	}

	// Use cursor position to determine actual content area
	int maxRow = csbi.dwCursorPosition.Y + 1; // +1 because cursor position is 0-based
	int bufferWidth = csbi.dwSize.X;

	// Don't read beyond cursor position
	COORD bufferSize = { static_cast<SHORT>(bufferWidth), static_cast<SHORT>(maxRow) };
	COORD bufferCoord = { 0, 0 };
	SMALL_RECT readRegion = { 0, 0,
		static_cast<SHORT>(bufferWidth - 1),
		static_cast<SHORT>(maxRow - 1) };

	std::vector<CHAR_INFO> buffer(bufferWidth * maxRow);

	if (!ReadConsoleOutput(hConsole, buffer.data(), bufferSize,
		bufferCoord, &readRegion)) {
		std::cerr << "Failed to read console output\n";
		return;
	}

	std::ofstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << filename << "\n";
		return;
	}

	std::vector<std::string> lines;

	// Process each line
	for (int y = 0; y < maxRow; ++y) {
		std::string line;

		// Build the line
		for (int x = 0; x < bufferWidth; ++x) {
			char ch = buffer[y * bufferWidth + x].Char.AsciiChar;
			line += (ch == 0) ? ' ' : ch; // Convert null chars to spaces
		}

		// Trim trailing whitespace (spaces and tabs)
		while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
			line.pop_back();
		}

		lines.push_back(line);
	}

	// Remove trailing empty lines (optional - comment out if you want to keep them)
	while (!lines.empty() && lines.back().empty()) {
		lines.pop_back();
	}

	// Write all lines to file
	for (const auto& line : lines) {
		file << line << '\n';
	}

	std::cout << "Console content saved to: " << filename << "\n";
}
