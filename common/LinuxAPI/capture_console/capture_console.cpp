// Linux stub for CaptureConsoleToFile - no-op since Linux terminals work differently
#include "capture_console.h"
#include <iostream>

void CaptureConsoleToFile(const std::string& /*filename*/)
{
	// No-op on Linux: console output can be captured via shell redirection (e.g. ./hps1x64 > output.txt)
}
