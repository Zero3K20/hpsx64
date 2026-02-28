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




#include "hps2x64.h"
#include "WinApiHandler.h"
#include <fstream>
//#include "ConfigFile.h"

//#include "WinJoy.h"

//#include "VU_Print.h"

#include "json.hpp"

#include "guicon.h"

//#include "vulkan_setup.h"

#include "R5900_Recompiler.h"

#include "capture_console.h"

#include <immintrin.h>

using namespace WinApi;

using namespace Playstation2;
using namespace Utilities::Strings;
//using namespace Config;


#ifdef _DEBUG_VERSION_

// debug defines go in here

#endif


#define USE_NEW_PROGRAM_WINDOW



#define ALLOW_PS2_HWRENDER


// the type of window, either opengl or vulkan
int hps2x64::iWindowType;

GamepadConfigWindow::GamepadConfig hps2x64::m_gamepadConfig;


hps2x64 _HPS2X64;


volatile hps2x64::MenuClicked hps2x64::_MenuClick;
volatile hps2x64::RunMode hps2x64::_RunMode;
volatile u32 hps2x64::_MenuWasClicked;


WindowClass::Window *hps2x64::ProgramWindow;
std::unique_ptr<OpenGLWindow> hps2x64::m_prgwindow;


// the path for the last bios that was selected
string hps2x64::sLastBiosPath;

string hps2x64::ExecutablePath;
char ExePathTemp [ hps2x64::c_iExeMaxPathLength + 1 ];


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{

	
#if !defined(__GNUC__)
	// don't need this for gcc
	RedirectIOToConsole();
#endif

	WindowClass::Register ( hInstance, "testSystem" );


	cout << "\nGet handle to the console output window";

	HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	COORD oScrBufSize;
	oScrBufSize.X = 300;
	oScrBufSize.Y = 30000;
	

	if (!SetConsoleScreenBufferSize(hConsoleOutput, oScrBufSize))
	{
		cout << "\nProblem setting console screen buffer size. Error#0x" << hex << GetLastError() << " " << dec << GetLastError();
	}
	else
	{
		cout << "\nSet console screen buffer size ok.";
	}

	cout << "\nGet max size of the console window.";

	COORD oMaxConsoleSize = GetLargestConsoleWindowSize(hConsoleOutput);

	//PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo;
	CONSOLE_SCREEN_BUFFER_INFO oConsoleScreenBufferInfo;
	GetConsoleScreenBufferInfo(hConsoleOutput, &oConsoleScreenBufferInfo);

	cout << "\nConsole max x size: " << dec << oMaxConsoleSize.X << " max y size: " << oMaxConsoleSize.Y;
	cout << "\nConsole max x size: " << dec << oConsoleScreenBufferInfo.dwMaximumWindowSize.X << " max y size: " << oConsoleScreenBufferInfo.dwMaximumWindowSize.Y;


	cout << "Initializing program...\n";
	
	_HPS2X64.Initialize ();
	

	// initialize direct input joysticks here for now

	Playstation1::SIO::gamepad.Initialize(hps2x64::m_prgwindow->GetHandle());


	cout << "\nStarting run of program...\n";
	
	_HPS2X64.RunProgram ();
	
	
	//cin.ignore ();
	
	return 0;
}


hps2x64::hps2x64 ()
{
	cout << "Running hps2x64 constructor...\n";
	
	
	// zero object
	// *** PROBLEM *** this clears out all the defaults for the system
	//memset ( this, 0, sizeof( hps2x64 ) );
}


hps2x64::~hps2x64 ()
{
	cout << "Running hps2x64 destructor...\n";
	
	// end the timer resolution
	if ( timeEndPeriod ( 1 ) == TIMERR_NOCANDO )
	{
		cout << "\nhpsx64 ERROR: Problem ending timer period.\n";
	}
}

void hps2x64::Reset ()
{
	// clear static variables too
	_RunMode.Value = 0;
	_MenuClick.Value = 0;
	_MenuWasClicked = 0;
	
	_SYSTEM.Reset ();
}





// returns 0 if menu was not clicked, returns 1 if menu was clicked
int hps2x64::HandleMenuClick ()
{
	int i;
	int MenuWasClicked = 0;
	
	//if ( _MenuClick.Value )
	if ( _MenuWasClicked )
	{
		cout << "\nA menu item was clicked.\n";

		// a menu item was clicked
		MenuWasClicked = 1;
		
		
		
		
		// update anything that was checked/unchecked
		Update_CheckMarksOnMenu ();
		
		// clear anything that was clicked
		//x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&)_MenuClick.Value, 0 );
		
		DebugWindow_Update ();
		
		_MenuWasClicked = 0;
	}
	
	return MenuWasClicked;
}


void hps2x64::Update_CheckMarksOnMenu ()
{
#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetMenuItemChecked("load|bios", false);
	m_prgwindow->SetMenuItemChecked("load|ps2gamedisk", false);
	m_prgwindow->SetMenuItemChecked("load|ps1gamedisk", false);
	m_prgwindow->SetMenuItemChecked("load|audiodisk", false);

	m_prgwindow->SetMenuItemChecked("pad1|digital", false);
	m_prgwindow->SetMenuItemChecked("pad1|analog", false);
	m_prgwindow->SetMenuItemChecked("pad1|dualshock2", false);
	m_prgwindow->SetMenuItemChecked("pad1|none", false);
	m_prgwindow->SetMenuItemChecked("pad1|device0", false);
	m_prgwindow->SetMenuItemChecked("pad1|device1", false);

	m_prgwindow->SetMenuItemChecked("pad2|digital", false);
	m_prgwindow->SetMenuItemChecked("pad2|analog", false);
	m_prgwindow->SetMenuItemChecked("pad2|dualshock2", false);
	m_prgwindow->SetMenuItemChecked("pad2|none", false);
	m_prgwindow->SetMenuItemChecked("pad2|device0", false);
	m_prgwindow->SetMenuItemChecked("pad2|device1", false);

	m_prgwindow->SetMenuItemChecked("card1|connect", false);
	m_prgwindow->SetMenuItemChecked("card1|disconnect", false);
	m_prgwindow->SetMenuItemChecked("card2|connect", false);
	m_prgwindow->SetMenuItemChecked("card2|disconnect", false);

	m_prgwindow->SetMenuItemChecked("region|europe", false);
	m_prgwindow->SetMenuItemChecked("region|japan", false);
	m_prgwindow->SetMenuItemChecked("region|america", false);

	m_prgwindow->SetMenuItemChecked("audio|enable", false);
	m_prgwindow->SetMenuItemChecked("volume|100", false);
	m_prgwindow->SetMenuItemChecked("volume|75", false);
	m_prgwindow->SetMenuItemChecked("volume|50", false);
	m_prgwindow->SetMenuItemChecked("volume|25", false);

	m_prgwindow->SetMenuItemChecked("buffer|8k", false);
	m_prgwindow->SetMenuItemChecked("buffer|16k", false);
	m_prgwindow->SetMenuItemChecked("buffer|32k", false);
	m_prgwindow->SetMenuItemChecked("buffer|64k", false);
	m_prgwindow->SetMenuItemChecked("buffer|128k", false);

	m_prgwindow->SetMenuItemChecked("audio|filter", false);

	m_prgwindow->SetMenuItemChecked("opengl|software", false);
	m_prgwindow->SetMenuItemChecked("vulkan|software", false);
	m_prgwindow->SetMenuItemChecked("vulkan|hardware", false);

	m_prgwindow->SetMenuItemChecked("threads|x0", false);
	m_prgwindow->SetMenuItemChecked("threads|x1", false);

	//m_prgwindow->SetMenuItemChecked("scanlines|enable", false);
	//m_prgwindow->SetMenuItemChecked("scanlines|disable", false);

	m_prgwindow->SetMenuItemChecked("windowsize|x1", false);
	m_prgwindow->SetMenuItemChecked("windowsize|x1.5", false);
	m_prgwindow->SetMenuItemChecked("windowsize|x2", false);

	m_prgwindow->SetMenuItemChecked("displaymode|stretch", false);
	m_prgwindow->SetMenuItemChecked("displaymode|fit", false);

	m_prgwindow->SetMenuItemChecked("r3000a|interpreter", false);
	m_prgwindow->SetMenuItemChecked("r3000a|recompiler", false);
	m_prgwindow->SetMenuItemChecked("r5900|interpreter", false);
	m_prgwindow->SetMenuItemChecked("r5900|recompiler", false);
	m_prgwindow->SetMenuItemChecked("vu0|interpreter", false);
	m_prgwindow->SetMenuItemChecked("vu0|recompiler", false);
	m_prgwindow->SetMenuItemChecked("vu1|interpreter", false);
	m_prgwindow->SetMenuItemChecked("vu1|recompiler", false);

	// check if a bios is loaded into memory or not
	if (sLastBiosPath.compare(""))
	{
		// looks like there is a bios currently loaded into memory //

		m_prgwindow->SetMenuItemChecked("load|bios", true);
	}

	// check box for audio output enable //
	if (_SYSTEM._PS1SYSTEM._SPU2.AudioOutput_Enabled)
	{
		m_prgwindow->SetMenuItemChecked("audio|enable", true);
	}

	// check box for if disk is loaded and whether data/audio //

	if (!_SYSTEM._PS1SYSTEM._CD.isLidOpen)
	{
		switch (_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType)
		{
		case CDVD_TYPE_PS2CD:
		case CDVD_TYPE_PS2DVD:
			m_prgwindow->SetMenuItemChecked("load|ps2gamedisk", true);
			break;

		case CDVD_TYPE_PSCD:
			m_prgwindow->SetMenuItemChecked("load|ps1gamedisk", true);
			break;
		}



	}	// end if ( !_SYSTEM._PS1SYSTEM._CD.isLidOpen )


	switch (_SYSTEM._PS1SYSTEM._SIO.ControlPad_Type[0])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("pad1|digital", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("pad1|analog", true);
		break;
	case Playstation1::SIO::PADTYPE_DUALSHOCK2:
		m_prgwindow->SetMenuItemChecked("pad1|dualshock2", true);
		break;
	}

	switch (_SYSTEM._PS1SYSTEM._SIO.PortMapping[0])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("pad1|device0", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("pad1|device1", true);
		break;

	default:
		m_prgwindow->SetMenuItemChecked("pad1|none", true);
		break;
	}

	// do pad 2
	switch (_SYSTEM._PS1SYSTEM._SIO.ControlPad_Type[1])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("pad2|digital", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("pad2|analog", true);
		break;

	case Playstation1::SIO::PADTYPE_DUALSHOCK2:
		m_prgwindow->SetMenuItemChecked("pad2|dualshock2", true);
		break;
	}

	switch (_SYSTEM._PS1SYSTEM._SIO.PortMapping[1])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("pad2|device0", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("pad2|device1", true);
		break;

	default:
		m_prgwindow->SetMenuItemChecked("pad2|none", true);
		break;
	}


	// check box for memory card 1/2 connected/disconnected //

	// do card 1
	switch (_SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState[0])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("card1|connect", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("card1|disconnect", true);
		break;
	}

	// do card 2
	switch (_SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState[1])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("card2|connect", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("card2|disconnect", true);
		break;
	}


	// check box for region //
	switch (_SYSTEM._PS1SYSTEM._CDVD.Region)
	{
	case 'A':
		m_prgwindow->SetMenuItemChecked("region|america", true);
		break;

	case 'E':
		m_prgwindow->SetMenuItemChecked("region|europe", true);
		break;

	case 'I':
		m_prgwindow->SetMenuItemChecked("region|japan", true);
		break;
	}

	// check box for audio volume //
	switch (_SYSTEM._PS1SYSTEM._SPU2.GlobalVolume)
	{
	case 0x400:
		m_prgwindow->SetMenuItemChecked("volume|25", true);
		break;

	case 0x1000:
		m_prgwindow->SetMenuItemChecked("volume|50", true);
		break;

	case 0x3000:
		m_prgwindow->SetMenuItemChecked("volume|75", true);
		break;

	case 0x7fff:
		m_prgwindow->SetMenuItemChecked("volume|100", true);
		break;
	}

	// audio filter enable/disable //
	if (_SYSTEM._PS1SYSTEM._SPU2.AudioFilter_Enabled)
	{
		m_prgwindow->SetMenuItemChecked("audio|filter", true);
	}

	// check box for audio buffer size //
	switch (_SYSTEM._PS1SYSTEM._SPU2.NextPlayBuffer_Size)
	{
	case 8192:
		m_prgwindow->SetMenuItemChecked("buffer|8k", true);
		break;

	case 16384:
		m_prgwindow->SetMenuItemChecked("buffer|16k", true);
		break;

	case 32768:
		m_prgwindow->SetMenuItemChecked("buffer|32k", true);
		break;

	case 65536:
		m_prgwindow->SetMenuItemChecked("buffer|64k", true);
		break;

	case 131072:
		m_prgwindow->SetMenuItemChecked("buffer|128k", true);
		break;
	}


	// scanlines enable/disable //
	//if (_SYSTEM._GPU.Get_Scanline())
	//{
	//	m_prgwindow->SetMenuItemChecked("scanlines|enable", true);
	//}
	//else
	//{
	//	m_prgwindow->SetMenuItemChecked("scanlines|disable", true);
	//}

	if (m_prgwindow->GetDisplayMode() == OpenGLWindow::DisplayMode::STRETCH)
	{
		m_prgwindow->SetMenuItemChecked("displaymode|stretch", true);

	}
	if (m_prgwindow->GetDisplayMode() == OpenGLWindow::DisplayMode::FIT)
	{
		m_prgwindow->SetMenuItemChecked("displaymode|fit", true);
	}

	if (_HPS2X64._SYSTEM._GPU.ulNumberOfThreads)
	{
		m_prgwindow->SetMenuItemChecked("threads|x1", true);
	}
	else
	{
		m_prgwindow->SetMenuItemChecked("threads|x0", true);
	}

	// R3000A Interpreter/Recompiler //
	if (_SYSTEM._PS1SYSTEM._CPU.bEnableRecompiler)
	{
		if (_SYSTEM._CPU.rs->OptimizeLevel == 1)
		{
			m_prgwindow->SetMenuItemChecked("r3000a|recompiler", true);
		}
		else
		{
			m_prgwindow->SetMenuItemChecked("r3000a|recompiler2", true);
		}
	}
	else
	{
		m_prgwindow->SetMenuItemChecked("r3000a|interpreter", true);
	}

	if (_HPS2X64._SYSTEM._CPU.bEnableRecompiler)
	{
		if (_HPS2X64._SYSTEM._CPU.rs->OptimizeLevel == 1)
		{
			m_prgwindow->SetMenuItemChecked("r5900|recompiler", true);
		}
		else
		{
			m_prgwindow->SetMenuItemChecked("r5900|recompiler2", true);
		}
	}
	else
	{
		m_prgwindow->SetMenuItemChecked("r5900|interpreter", true);
	}

	if (_HPS2X64._SYSTEM._VU0.VU0.bEnableRecompiler)
	{
		m_prgwindow->SetMenuItemChecked("vu0|recompiler", true);
	}
	else
	{
		m_prgwindow->SetMenuItemChecked("vu0|interpreter", true);
	}

	if (_HPS2X64._SYSTEM._VU1.VU1.bEnableRecompiler)
	{
		m_prgwindow->SetMenuItemChecked("vu1|recompiler", true);
	}
	else
	{
		m_prgwindow->SetMenuItemChecked("vu1|interpreter", true);
	}


	if (_SYSTEM._GPU.bEnable_OpenCL)
	{
		m_prgwindow->SetMenuItemChecked("vulkan|hardware", true);
	}
	else
	{
		switch (_SYSTEM._GPU.iCurrentDrawLibrary)
		{
		case Playstation1::GPU::DRAW_LIBRARY_VULKAN:
			m_prgwindow->SetMenuItemChecked("vulkan|software", true);
			break;

		default:
			m_prgwindow->SetMenuItemChecked("opengl|software", true);
			break;
		}
	}

#else
	// uncheck all first
	ProgramWindow->Menus->UnCheckItem("Bios");
	ProgramWindow->Menus->UnCheckItem("Insert/Remove Audio Disk");
	ProgramWindow->Menus->UnCheckItem("Insert/Remove PS1 Game Disk");
	ProgramWindow->Menus->UnCheckItem ( "Insert/Remove PS2 Game Disk" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1 Digital" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1 Analog" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1 DualShock2" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1: None" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1: Device0" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1: Device1" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 2 Digital" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 2 Analog" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 2 DualShock2" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 2: None" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 2: Device0" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 2: Device1" );
	ProgramWindow->Menus->UnCheckItem ( "Disconnect Card1" );
	ProgramWindow->Menus->UnCheckItem ( "Connect Card1" );
	ProgramWindow->Menus->UnCheckItem ( "Disconnect Card2" );
	ProgramWindow->Menus->UnCheckItem ( "Connect Card2" );
	ProgramWindow->Menus->UnCheckItem ( "North America" );
	ProgramWindow->Menus->UnCheckItem ( "Europe" );
	ProgramWindow->Menus->UnCheckItem ( "Japan" );
	ProgramWindow->Menus->UnCheckItem ( "Enable" );
	ProgramWindow->Menus->UnCheckItem ( "100%" );
	ProgramWindow->Menus->UnCheckItem ( "75%" );
	ProgramWindow->Menus->UnCheckItem ( "50%" );
	ProgramWindow->Menus->UnCheckItem ( "25%" );
	ProgramWindow->Menus->UnCheckItem ( "8 KB" );
	ProgramWindow->Menus->UnCheckItem ( "16 KB" );
	ProgramWindow->Menus->UnCheckItem ( "32 KB" );
	ProgramWindow->Menus->UnCheckItem ( "64 KB" );
	ProgramWindow->Menus->UnCheckItem ( "128 KB" );
	ProgramWindow->Menus->UnCheckItem ( "Filter" );
	ProgramWindow->Menus->UnCheckItem ( "Interpreter: R3000A" );
	ProgramWindow->Menus->UnCheckItem ( "Recompiler: R3000A" );
#ifdef ENABLE_RECOMPILE2
	ProgramWindow->Menus->UnCheckItem ( "Recompiler2: R3000A" );
#endif
	ProgramWindow->Menus->UnCheckItem ( "Interpreter: R5900" );
	ProgramWindow->Menus->UnCheckItem ( "Recompiler: R5900" );
#ifdef ENABLE_RECOMPILE2
	ProgramWindow->Menus->UnCheckItem ( "Recompiler2: R5900" );
#endif
	ProgramWindow->Menus->UnCheckItem ( "Interpreter: VU0" );
	ProgramWindow->Menus->UnCheckItem ( "Recompiler: VU0" );
	ProgramWindow->Menus->UnCheckItem ( "Interpreter: VU1" );
	ProgramWindow->Menus->UnCheckItem ( "Recompiler: VU1" );
	ProgramWindow->Menus->UnCheckItem ( "VU1: 1 [multi-thread]" );
	ProgramWindow->Menus->UnCheckItem ( "VU1: 0 [single-thread]" );
	//ProgramWindow->Menus->UnCheckItem ( "Skip Idle Cycles" );
	ProgramWindow->Menus->UnCheckItem ( "1 [multi-thread]" );
	ProgramWindow->Menus->UnCheckItem ( "0 [single-thread]" );
	ProgramWindow->Menus->UnCheckItem ( "SPU Multi-Thread" );
	ProgramWindow->Menus->UnCheckItem ( "Enable Vsync" );
	
	ProgramWindow->Menus->UnCheckItem("OpenGL: Software");
	ProgramWindow->Menus->UnCheckItem("Vulkan: Software");
	//ProgramWindow->Menus->UnCheckItem("Renderer: Vulkan: Hardware");

	ProgramWindow->Menus->UnCheckItem("Shader Threads: 1");
	ProgramWindow->Menus->UnCheckItem("Shader Threads: 2");
	ProgramWindow->Menus->UnCheckItem("Shader Threads: 4");
	ProgramWindow->Menus->UnCheckItem("Shader Threads: 8");

	// check if a bios is loaded into memory or not
	if (sLastBiosPath.compare(""))
	{
		// looks like there is a bios currently loaded into memory //

		// add a tick mark next to File | Load | Bios
		ProgramWindow->Menus->CheckItem("Bios");
	}
	
	// check box for audio output enable //
	if ( _SYSTEM._PS1SYSTEM._SPU2.AudioOutput_Enabled )
	{
		ProgramWindow->Menus->CheckItem ( "Enable" );
	}
	
	// check box for if disk is loaded and whether data/audio //
	
	if ( !_SYSTEM._PS1SYSTEM._CD.isLidOpen )
	{
		switch (_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType)
		{
		case CDVD_TYPE_PS2CD:
		case CDVD_TYPE_PS2DVD:
			ProgramWindow->Menus->CheckItem("Insert/Remove PS2 Game Disk");
			break;

		case CDVD_TYPE_PSCD:
			ProgramWindow->Menus->CheckItem("Insert/Remove PS1 Game Disk");
			break;
		}


		/*
		switch ( _SYSTEM._PS1SYSTEM._CD.isGameCD )
		{
			case true:
				ProgramWindow->Menus->CheckItem ( "Insert/Remove Game Disk" );
				break;
			
			case false:
				ProgramWindow->Menus->CheckItem ( "Insert/Remove Audio Disk" );
				break;
		}
		*/


	}	// end if ( !_SYSTEM._PS1SYSTEM._CD.isLidOpen )



	
	// check box for analog/digital pad 1/2 //
	
	// do pad 1
	switch ( _SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 0 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Pad 1 Digital" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Pad 1 Analog" );
			break;
			
		case Playstation1::SIO::PADTYPE_DUALSHOCK2:
			ProgramWindow->Menus->CheckItem ( "Pad 1 DualShock2" );
			break;
	}

	switch ( _SYSTEM._PS1SYSTEM._SIO.PortMapping [ 0 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Pad 1: Device0" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Pad 1: Device1" );
			break;
			
		default:
			ProgramWindow->Menus->CheckItem ( "Pad 1: None" );
			break;
	}
	
	// do pad 2
	switch ( _SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 1 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Pad 2 Digital" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Pad 2 Analog" );
			break;
			
		case Playstation1::SIO::PADTYPE_DUALSHOCK2:
			ProgramWindow->Menus->CheckItem ( "Pad 2 DualShock2" );
			break;
	}

	switch ( _SYSTEM._PS1SYSTEM._SIO.PortMapping [ 1 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Pad 2: Device0" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Pad 2: Device1" );
			break;
			
		default:
			ProgramWindow->Menus->CheckItem ( "Pad 2: None" );
			break;
	}
	
	
	// check box for memory card 1/2 connected/disconnected //
	
	// do card 1
	switch ( _SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Connect Card1" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Disconnect Card1" );
			break;
	}
	
	// do card 2
	switch ( _SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Connect Card2" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Disconnect Card2" );
			break;
	}
	
	
	// check box for region //
	//switch ( _SYSTEM._PS1SYSTEM._CD.Region )
	switch (_SYSTEM._PS1SYSTEM._CDVD.Region)
	{
		case 'A':
			ProgramWindow->Menus->CheckItem ( "North America" );
			break;
			
		case 'E':
			ProgramWindow->Menus->CheckItem ( "Europe" );
			break;
			
		case 'I':
			ProgramWindow->Menus->CheckItem ( "Japan" );
			break;
	}
	
	// check box for audio buffer size //
	switch ( _SYSTEM._PS1SYSTEM._SPU2.NextPlayBuffer_Size )
	{
		case 8192:
			ProgramWindow->Menus->CheckItem ( "8 KB" );
			break;
			
		case 16384:
			ProgramWindow->Menus->CheckItem ( "16 KB" );
			break;
			
		case 32768:
			ProgramWindow->Menus->CheckItem ( "32 KB" );
			break;
			
		case 65536:
			ProgramWindow->Menus->CheckItem ( "64 KB" );
			break;
			
		case 131072:
			ProgramWindow->Menus->CheckItem ( "128 KB" );
			break;
	}
	
	// check box for audio volume //
	switch ( _SYSTEM._PS1SYSTEM._SPU2.GlobalVolume )
	{
		case 0x400:
			ProgramWindow->Menus->CheckItem ( "25%" );
			break;
			
		case 0x1000:
			ProgramWindow->Menus->CheckItem ( "50%" );
			break;
			
		case 0x3000:
			ProgramWindow->Menus->CheckItem ( "75%" );
			break;
			
		case 0x7fff:
			ProgramWindow->Menus->CheckItem ( "100%" );
			break;
	}
	
	// audio filter enable/disable //
	if ( _SYSTEM._PS1SYSTEM._SPU2.AudioFilter_Enabled )
	{
		ProgramWindow->Menus->CheckItem ( "Filter" );
	}
	
	if ( _HPS2X64._SYSTEM._PS1SYSTEM._CPU.bEnableRecompiler )
	{
		if ( _HPS2X64._SYSTEM._PS1SYSTEM._CPU.rs->OptimizeLevel == 1 )
		{
			ProgramWindow->Menus->CheckItem ( "Recompiler: R3000A" );
		}
		else
		{
			ProgramWindow->Menus->CheckItem ( "Recompiler2: R3000A" );
		}
	}
	else
	{
		ProgramWindow->Menus->CheckItem ( "Interpreter: R3000A" );
	}
	
	if ( _HPS2X64._SYSTEM._CPU.bEnableRecompiler )
	{
		if ( _HPS2X64._SYSTEM._CPU.rs->OptimizeLevel == 1 )
		{
			ProgramWindow->Menus->CheckItem ( "Recompiler: R5900" );
		}
		else
		{
			ProgramWindow->Menus->CheckItem ( "Recompiler2: R5900" );
		}
	}
	else
	{
		ProgramWindow->Menus->CheckItem ( "Interpreter: R5900" );
	}
	
	if ( _HPS2X64._SYSTEM._VU0.VU0.bEnableRecompiler )
	{
		ProgramWindow->Menus->CheckItem ( "Recompiler: VU0" );
	}
	else
	{
		ProgramWindow->Menus->CheckItem ( "Interpreter: VU0" );
	}
	
	if ( _HPS2X64._SYSTEM._VU1.VU1.bEnableRecompiler )
	{
		ProgramWindow->Menus->CheckItem ( "Recompiler: VU1" );
	}
	else
	{
		ProgramWindow->Menus->CheckItem ( "Interpreter: VU1" );
	}

	if ( _HPS2X64._SYSTEM._VU1.VU1.ulThreadCount )
	{
		ProgramWindow->Menus->CheckItem ( "VU1: 1 [multi-thread]" );
	}
	else
	{
		ProgramWindow->Menus->CheckItem ( "VU1: 0 [single-thread]" );
	}

	
	//if ( _HPS2X64._SYSTEM._CPU.bEnable_SkipIdleCycles )
	//{
	//	ProgramWindow->Menus->CheckItem ( "Skip Idle Cycles" );
	//}
	

	if ( _HPS2X64._SYSTEM._GPU.ulNumberOfThreads )
	{
		//ProgramWindow->Menus->CheckItem ( "1 (multi-thread)" );
		ProgramWindow->Menus->CheckItem("1 [multi-thread]");
	}
	else
	{
		//ProgramWindow->Menus->CheckItem ( "0 (single-thread)" );
		ProgramWindow->Menus->CheckItem("0 [single-thread]");
	}

	if ( _HPS2X64._SYSTEM._PS1SYSTEM._SPU2.ulNumThreads )
	{
		ProgramWindow->Menus->CheckItem ( "SPU Multi-Thread" );
	}
	
	if ( _HPS2X64._SYSTEM.bEnableVsync )
	{
		ProgramWindow->Menus->CheckItem ( "Enable Vsync" );
	}

#ifdef ALLOW_PS2_HWRENDER
	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		//ProgramWindow->Menus->CheckItem("Renderer: Vulkan: Hardware");
		switch (vulkan_get_gpu_threads())
		{
		case 1:
			ProgramWindow->Menus->CheckItem("Shader Threads: 1");
			break;
		case 2:
			ProgramWindow->Menus->CheckItem("Shader Threads: 2");
			break;
		case 4:
			ProgramWindow->Menus->CheckItem("Shader Threads: 4");
			break;
		case 8:
			ProgramWindow->Menus->CheckItem("Shader Threads: 8");
			break;
		}
	}
	else
	{
		switch (_HPS2X64._SYSTEM._GPU.iCurrentDrawLibrary)
		{
		case Playstation2::GPU::DRAW_LIBRARY_VULKAN:
			ProgramWindow->Menus->CheckItem("Vulkan: Software");
			break;

		default:
			ProgramWindow->Menus->CheckItem("OpenGL: Software");
			break;
		}
	}
#endif

#endif

}



bool hps2x64::CreateOpenGLWindow()
{
	/*
	if (iWindowType == WINDOW_TYPE_VULKAN)
	{
		// unload vulkan
		_HPS2X64._SYSTEM._GPU.UnloadVulkan();

		ProgramWindow->KillWindow();
	}

	if (iWindowType == WINDOW_TYPE_OPENGL)
	{
		cout << "\nhpsx64: ALERT: Window type is already an OPENGL window.";
		return true;
	}

	ProgramWindow->CreateBaseWindow(ProgramWindow_Caption_OpenGL_Loaded, ProgramWindow_Width, ProgramWindow_Height, true);


	cout << "\nAdding menubar";

	////////////////////////////////////////////
	// add menu bar to program window
	WindowClass::MenuBar* m = ProgramWindow->Menus;

	//ProgramWindow->CreateMenuFromJson(jsnMenuBar, "English");
	ProgramWindow->CreateMenuFromYaml(smenu_yaml2, "english");


	cout << "\nShowing menu bar";

	// show the menu bar
	m->Show();

#ifdef VERBOSE_SHOW_DISPLAY_MODES

	ProgramWindow->OutputAllDisplayModes();

#endif

	ProgramWindow->CreateOpenGLContext();


	cout << "\nInitializing open gl for program window";

	/////////////////////////////////////////////////////////
	// enable opengl for the program window
	//ProgramWindow->EnableOpenGL ();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ProgramWindow_Width, ProgramWindow_Height, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	cout << "\nReleasing window from OpenGL";

	// this window is no longer the window we want to draw to
	ProgramWindow->OpenGL_ReleaseWindow();

	cout << "\nEnabling VSync";

	// enable/disable vsync
	ProgramWindow->EnableVSync();
	//ProgramWindow->DisableVSync ();

	// window is now an opengl window
	iWindowType = WINDOW_TYPE_OPENGL;

	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
	_HPS2X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation2::GPU::DRAW_LIBRARY_OPENGL;

	_HPS2X64.Update_CheckMarksOnMenu();
	*/

	return true;
}

bool hps2x64::CreateVulkanWindow()
{
	/*
	if (iWindowType == WINDOW_TYPE_VULKAN)
	{
		cout << "\nhpsx64: ALERT: Window type is already an VULKAN window.";
		return true;
	}

	if (iWindowType == WINDOW_TYPE_OPENGL)
	{
		// kill the current opengl window before making it a vulkan window
		cout << "\nhpsx64: ALERT: Killing the opengl window.";
		ProgramWindow->KillWindow();
	}

	cout << "\nhpsx64: ALERT: Creating the vulkan window.";

	ProgramWindow->CreateBaseWindow(ProgramWindow_Caption_Vulkan_Loaded, ProgramWindow_Width, ProgramWindow_Height, true);

#ifdef VERBOSE_SHOW_DISPLAY_MODES

	ProgramWindow->OutputAllDisplayModes();

#endif


	//vulkan_set_screen_size(ProgramWindow_Width, ProgramWindow_Height);

	// create a vulkan context for the window
	//vulkan_create_context(ProgramWindow->hWnd, ProgramWindow->hInstance);

	// create/re-create swap chain
	//vulkan_create_swap_chain();

	if (_HPS2X64._SYSTEM._GPU.LoadVulkan())
	{
		cout << "\nHPSX64: INFO: *** VULKAN LOADED ***\n";
	}
	else
	{
		cout << "\nHPSX64: ERROR: attempted to load vulkan, but encountered a problem.\n";

		return false;
	}


	cout << "\nAdding menubar";

	////////////////////////////////////////////
	// add menu bar to program window
	WindowClass::MenuBar* m = ProgramWindow->Menus;

	//ProgramWindow->CreateMenuFromJson(jsnMenuBar, "English");
	ProgramWindow->CreateMenuFromYaml(smenu_yaml2, "english");


	cout << "\nShowing menu bar";

	// show the menu bar
	m->Show();


	// window is now a vulkan window
	iWindowType = WINDOW_TYPE_VULKAN;

	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
	_HPS2X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation2::GPU::DRAW_LIBRARY_VULKAN;

	_HPS2X64.Update_CheckMarksOnMenu();
	*/

	return true;
}


void hps2x64::map_callback_functions()
{
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_State", OnClick_File_Load_State);
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_BIOS", OnClick_File_Load_BIOS);
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_GameDisk_PS2", OnClick_File_Load_GameDisk_PS2);
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_GameDisk_PS1", OnClick_File_Load_GameDisk_PS1);
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_AudioDisk", OnClick_File_Load_AudioDisk);
	WindowClass::MenuBar::add_map_function("OnClick_File_Save_State", OnClick_File_Save_State);
	WindowClass::MenuBar::add_map_function("OnClick_File_Reset", OnClick_File_Reset);
	WindowClass::MenuBar::add_map_function("OnClick_File_Run", OnClick_File_Run);
	WindowClass::MenuBar::add_map_function("OnClick_File_Exit", OnClick_File_Exit);

	WindowClass::MenuBar::add_map_function("OnClick_Debug_Break", OnClick_Debug_Break);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_StepInto", OnClick_Debug_StepInto);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_StepPS1_Instr", OnClick_Debug_StepPS1_Instr);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_StepPS2_Instr", OnClick_Debug_StepPS2_Instr);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_OutputCurrentSector", OnClick_Debug_OutputCurrentSector);

	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_All", OnClick_Debug_Show_All);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_FrameBuffer", OnClick_Debug_Show_FrameBuffer);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_R3000A", OnClick_Debug_Show_R3000A);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_Memory", OnClick_Debug_Show_Memory);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_DMA", OnClick_Debug_Show_DMA);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_TIMER", OnClick_Debug_Show_TIMER);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_SPU", OnClick_Debug_Show_SPU);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_CD", OnClick_Debug_Show_CD);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_INTC", OnClick_Debug_Show_INTC);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_SPU0", OnClick_Debug_Show_SPU0);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_SPU1", OnClick_Debug_Show_SPU1);

	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_All", OnClick_Debug_Show_PS2_All);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_FrameBuffer", OnClick_Debug_Show_PS2_FrameBuffer);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_R5900", OnClick_Debug_Show_R5900);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_Memory", OnClick_Debug_Show_PS2_Memory);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_DMA", OnClick_Debug_Show_PS2_DMA);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_TIMER", OnClick_Debug_Show_PS2_TIMER);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_VU0", OnClick_Debug_Show_PS2_VU0);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_VU1", OnClick_Debug_Show_PS2_VU1);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_IPU", OnClick_Debug_Show_PS2_IPU);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_INTC", OnClick_Debug_Show_PS2_INTC);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_Show_PS2_GPU", OnClick_Debug_Show_PS2_GPU);

	WindowClass::MenuBar::add_map_function("OnClick_Controllers0_Configure", OnClick_Controllers0_Configure);
	WindowClass::MenuBar::add_map_function("OnClick_Controllers1_Configure", OnClick_Controllers1_Configure);
	WindowClass::MenuBar::add_map_function("OnClick_Pad1Type_Digital", OnClick_Pad1Type_Digital);
	WindowClass::MenuBar::add_map_function("OnClick_Pad1Type_Analog", OnClick_Pad1Type_Analog);
	WindowClass::MenuBar::add_map_function("OnClick_Pad1Type_DualShock2", OnClick_Pad1Type_DualShock2);
	WindowClass::MenuBar::add_map_function("OnClick_Pad2Type_Digital", OnClick_Pad2Type_Digital);
	WindowClass::MenuBar::add_map_function("OnClick_Pad2Type_Analog", OnClick_Pad2Type_Analog);
	WindowClass::MenuBar::add_map_function("OnClick_Pad2Type_DualShock2", OnClick_Pad2Type_DualShock2);

	WindowClass::MenuBar::add_map_function("OnClick_Pad1Input_None", OnClick_Pad1Input_None);
	WindowClass::MenuBar::add_map_function("OnClick_Pad1Input_Device0", OnClick_Pad1Input_Device0);
	WindowClass::MenuBar::add_map_function("OnClick_Pad1Input_Device1", OnClick_Pad1Input_Device1);
	WindowClass::MenuBar::add_map_function("OnClick_Pad2Input_None", OnClick_Pad2Input_None);
	WindowClass::MenuBar::add_map_function("OnClick_Pad2Input_Device0", OnClick_Pad2Input_Device0);
	WindowClass::MenuBar::add_map_function("OnClick_Pad2Input_Device1", OnClick_Pad2Input_Device1);

	WindowClass::MenuBar::add_map_function("OnClick_Card1_Connect", OnClick_Card1_Connect);
	WindowClass::MenuBar::add_map_function("OnClick_Card1_Disconnect", OnClick_Card1_Disconnect);
	WindowClass::MenuBar::add_map_function("OnClick_Card2_Connect", OnClick_Card2_Connect);
	WindowClass::MenuBar::add_map_function("OnClick_Card2_Disconnect", OnClick_Card2_Disconnect);

	WindowClass::MenuBar::add_map_function("OnClick_Redetect_Pads", OnClick_Redetect_Pads);

	WindowClass::MenuBar::add_map_function("OnClick_Region_Europe", OnClick_Region_Europe);
	WindowClass::MenuBar::add_map_function("OnClick_Region_Japan", OnClick_Region_Japan);
	WindowClass::MenuBar::add_map_function("OnClick_Region_NorthAmerica", OnClick_Region_NorthAmerica);

	WindowClass::MenuBar::add_map_function("OnClick_Audio_Enable", OnClick_Audio_Enable);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Volume_100", OnClick_Audio_Volume_100);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Volume_75", OnClick_Audio_Volume_75);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Volume_50", OnClick_Audio_Volume_50);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Volume_25", OnClick_Audio_Volume_25);

	WindowClass::MenuBar::add_map_function("OnClick_Audio_Buffer_8k", OnClick_Audio_Buffer_8k);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Buffer_16k", OnClick_Audio_Buffer_16k);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Buffer_32k", OnClick_Audio_Buffer_32k);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Buffer_64k", OnClick_Audio_Buffer_64k);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Buffer_1m", OnClick_Audio_Buffer_1m);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_Filter", OnClick_Audio_Filter);
	WindowClass::MenuBar::add_map_function("OnClick_Audio_MultiThread", OnClick_Audio_MultiThread);

	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Software_OpenGL", OnClick_Video_Renderer_Software_OpenGL);
	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Software_Vulkan", OnClick_Video_Renderer_Software_Vulkan);
	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Hardware_Vulkan", OnClick_Video_Renderer_Hardware_Vulkan);
	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Hardware1_Vulkan", OnClick_Video_Renderer_Hardware1_Vulkan);
	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Hardware2_Vulkan", OnClick_Video_Renderer_Hardware2_Vulkan);
	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Hardware4_Vulkan", OnClick_Video_Renderer_Hardware4_Vulkan);
	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Hardware8_Vulkan", OnClick_Video_Renderer_Hardware8_Vulkan);

	WindowClass::MenuBar::add_map_function("OnClick_Video_WindowSizeX1", OnClick_Video_WindowSizeX1);
	WindowClass::MenuBar::add_map_function("OnClick_Video_WindowSizeX15", OnClick_Video_WindowSizeX15);
	WindowClass::MenuBar::add_map_function("OnClick_Video_WindowSizeX2", OnClick_Video_WindowSizeX2);
	WindowClass::MenuBar::add_map_function("OnClick_Video_FullScreen", OnClick_Video_FullScreen);
	WindowClass::MenuBar::add_map_function("OnClick_Video_EnableVsync", OnClick_Video_EnableVsync);

	WindowClass::MenuBar::add_map_function("OnClick_R3000ACPU_Interpreter", OnClick_R3000ACPU_Interpreter);
	WindowClass::MenuBar::add_map_function("OnClick_R3000ACPU_Recompiler", OnClick_R3000ACPU_Recompiler);
	WindowClass::MenuBar::add_map_function("OnClick_R3000ACPU_Recompiler2", OnClick_R3000ACPU_Recompiler2);
	WindowClass::MenuBar::add_map_function("OnClick_R5900CPU_Interpreter", OnClick_R5900CPU_Interpreter);
	WindowClass::MenuBar::add_map_function("OnClick_R5900CPU_Recompiler", OnClick_R5900CPU_Recompiler);
	WindowClass::MenuBar::add_map_function("OnClick_R5900CPU_Recompiler2", OnClick_R5900CPU_Recompiler2);

	WindowClass::MenuBar::add_map_function("OnClick_VU0_Interpreter", OnClick_VU0_Interpreter);
	WindowClass::MenuBar::add_map_function("OnClick_VU0_Recompiler", OnClick_VU0_Recompiler);
	WindowClass::MenuBar::add_map_function("OnClick_VU1_Interpreter", OnClick_VU1_Interpreter);
	WindowClass::MenuBar::add_map_function("OnClick_VU1_Recompiler", OnClick_VU1_Recompiler);
	WindowClass::MenuBar::add_map_function("OnClick_VU1_0Threads", OnClick_VU1_0Threads);
	WindowClass::MenuBar::add_map_function("OnClick_VU1_1Threads", OnClick_VU1_1Threads);

	WindowClass::MenuBar::add_map_function("OnClick_GPU_0Threads", OnClick_GPU_0Threads);
	WindowClass::MenuBar::add_map_function("OnClick_GPU_1Threads", OnClick_GPU_1Threads);


	//for (const auto& pair : WindowClass::mapped_functions) {
	//	std::cout << pair.first << ": " << pair.second << std::endl;
	//}

	//cout << " test2: OnClick_File_Exit:" << (unsigned long long)WindowClass::mapped_functions["OnClick_File_Exit"];
	//cout << " test3: OnClick_File_Exit:" << WindowClass::MenuBar::get_map_function("OnClick_File_Exit");
}



void hps2x64::Initialize ()
{

	u32 xsize, ysize;


	// map functions that will be used by gui interface
	map_callback_functions();


	////////////////////////////////////////////////
	// create program window
	xsize = ProgramWindow_Width;
	ysize = ProgramWindow_Height;

	cout << "\nCreating window";

#ifdef USE_NEW_PROGRAM_WINDOW

	// new window code
	m_prgwindow = std::make_unique<OpenGLWindow>("Hps2x64OpenGLWindow", "hps2x64");
	if (!m_prgwindow->Create(ProgramWindow_Width, ProgramWindow_Height)) {
		std::cout << "Failed to create OpenGL window!" << std::endl;
		//return false;
		return;
	}
	std::cout << "OpenGL window created successfully." << std::endl;

	// *** TODO *** SET PS2 DISPLAY WINDOW
	_SYSTEM._GPU.SetDisplayOutputWindow(ProgramWindow_Width, ProgramWindow_Height, m_prgwindow.get());

	// setup menus //

	SetupMenus();

	// flip screen vertically for now
	m_prgwindow->SetVerticalFlip(true);

	// don't allow the user to resize the window
	m_prgwindow->SetUserResizable(false);

	// make sure the client size is right after the menus get added
	m_prgwindow->SetClientSize(ProgramWindow_Width, ProgramWindow_Height);

	// Show windows
	std::cout << "Showing windows..." << std::endl;
	m_prgwindow->Show();
	// Show keyboard shortcut hints in the title bar so users know how to interact
	// on Linux/Steam Deck where no native menu bar is rendered.
	m_prgwindow->SetCaption("hps2x64  |  F1: Help  F2: Load BIOS  F3: Load Game  F5: Run");
	std::cout << "Windows should now be visible." << std::endl;

#else


	ProgramWindow = new WindowClass::Window ();
	
	CreateOpenGLWindow();

	// set the PS2 display output window too
	_SYSTEM._GPU.SetDisplayOutputWindow(ProgramWindow_Width, ProgramWindow_Height, ProgramWindow);

#endif


	// start system - must do this here rather than in constructor
	_SYSTEM.Start();

	
	// set the timer resolution
	if ( timeBeginPeriod ( 1 ) == TIMERR_NOCANDO )
	{
		cout << "\nhpsx64 ERROR: Problem setting timer period.\n";
	}
	

	// static size of ps2 system object
	std::cout << "\nHPS2X64: INFO: Size of PS2 System Object: " << dec << sizeof(_SYSTEM);

#ifndef EE_ONLY_COMPILE
	// we want the screen to display on the main window for the program when the system encouters start of vertical blank
	// I'll set both the PS1 GPU and PS2 GPU to the same window for now, since only one can display at a time anyways...
	//_SYSTEM._PS1SYSTEM._GPU.SetDisplayOutputWindow ( ProgramWindow_Width, ProgramWindow_Height, ProgramWindow );
#endif

	
	// get executable path
	int len = GetModuleFileName ( NULL, ExePathTemp, c_iExeMaxPathLength );
	ExePathTemp [ len ] = 0;
	
	// remove program name from path
#ifdef LINUX_BUILD
	ExecutablePath = Left ( ExePathTemp, InStrRev ( ExePathTemp, "/" ) + 1 );
#else
	ExecutablePath = Left ( ExePathTemp, InStrRev ( ExePathTemp, "\\" ) + 1 );
#endif
	
	cout << "\nLoading memory cards if available...";
	
	//_SYSTEM._SIO.Load_MemoryCardFile ( ExecutablePath + "card0", 0 );
	//_SYSTEM._SIO.Load_MemoryCardFile ( ExecutablePath + "card1", 1 );
	_SYSTEM._PS1SYSTEM._SIO.Load_PS2MemoryCardFile ( ExecutablePath + "ps2card0", 0 );
	_SYSTEM._PS1SYSTEM._SIO.Load_PS2MemoryCardFile ( ExecutablePath + "ps2card1", 1 );
	
	
	cout << "\nLoading application-level config file...";
	
	// load current configuration settings
	// config settings that are needed:
	// 1. Region
	// 2. Audio - Enable, Volume, Buffer Size, Filter On/Off
	// 3. Peripherals - Pad1/Pad2/PadX keys, Pad1/Pad2/PadX Analog/Digital, Card1/Card2/CardX Connected/Disconnected
	// I like this one... a ".hcfg" file
	LoadConfig ( ExecutablePath + "hps2x64.hcfg" );
	
	
	cout << "\nUpdating check marks";
	
	// update what items are checked or not
	Update_CheckMarksOnMenu ();
	

	// if using hardware rendering, then load vulkan //
	/*
	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// copy frame buffer from hardware to software if hardware rendering is allowed
		//if (_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			if (_HPS2X64._SYSTEM._GPU.LoadVulkan())
			{
				cout << "\nHPSX64: INFO: *** VULKAN LOADED ***\n";
			}
			else
			{
				cout << "\nHPSX64: ERROR: attempted to load vulkan, but encountered a problem.\n";
			}
		}
	}
	*/
	
	if (check_avx2_support()) {
		std::cout << "\n***AVX2 support is available.***\n" << std::endl;
	}
	else {
		std::cout << "\n***AVX2 support is not available.***\n" << std::endl;
	}

	if (check_avx512_support()) {
		std::cout << "\n***AVX512 support is available.***\n" << std::endl;
	}
	else {
		std::cout << "\n***AVX512 support is not available.***\n" << std::endl;
	}


	cout << "\ndone initializing";
	

}


void hps2x64::SetupMenus()
{
	m_prgwindow->CreateMenuBar();

	// File menu
	auto file_menu = m_prgwindow->AddMenu("file", "File");

	m_prgwindow->AddSubmenu("file", "file|load", "Load");
	m_prgwindow->AddMenuItem("file|load", "load|bios", "BIOS", "F2",
		[this]() {
			OnClick_File_Load_BIOS(0);
		});
	m_prgwindow->AddMenuItem("file|load", "load|ps2gamedisk", "PS2 Game Disk", "F3",
		[this]() {
			OnClick_File_Load_GameDisk_PS2(0);
		});
	m_prgwindow->AddMenuItem("file|load", "load|ps1gamedisk", "PS1 Game Disk", "",
		[this]() {
			OnClick_File_Load_GameDisk_PS1(0);
		});
	m_prgwindow->AddMenuItem("file|load", "load|audiodisk", "Audio Disk", "",
		[this]() {
			OnClick_File_Load_AudioDisk(0);
		});
	m_prgwindow->AddMenuItem("file|load", "load|state", "State", "",
		[this]() {
			OnClick_File_Load_State(0);
		});

	m_prgwindow->AddSubmenu("file", "file|save", "Save");
	m_prgwindow->AddMenuItem("file|save", "save|state", "State", "",
		[this]() {
			OnClick_File_Save_State(0);
		});

	m_prgwindow->AddMenuItem("file", "file|run", "Run", "F5",
		[this]() {
			OnClick_File_Run(0);
		});

	m_prgwindow->AddMenuItem("file", "file|reset", "Reset", "F6",
		[this]() {
			OnClick_File_Reset(0);
		});


	m_prgwindow->AddMenuSeparator("file");
	m_prgwindow->AddMenuItem("file", "file|exit", "Exit", "F10",
		[this]() {
			OnClick_File_Exit(0);
		});

	auto debug_menu = m_prgwindow->AddMenu("debug", "Debug");

	m_prgwindow->AddMenuItem("debug", "debug|break", "Break", "",
		[this]() {
			OnClick_Debug_Break(0);
		});
	m_prgwindow->AddMenuItem("debug", "debug|stepinto", "Step Into", "",
		[this]() {
			OnClick_Debug_StepInto(0);
		});

	m_prgwindow->AddSubmenu("debug", "debug|showwindow", "Show Window");

	m_prgwindow->AddSubmenu("debug|showwindow", "showwindow|debugps1", "PS1");
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|framebuffer", "Frame Buffer", "",
		[this]() {
			OnClick_Debug_Show_FrameBuffer(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|r3000a", "R3000A", "",
		[this]() {
			OnClick_Debug_Show_R3000A(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|memory", "Memory", "",
		[this]() {
			OnClick_Debug_Show_Memory(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|dma", "DMA", "",
		[this]() {
			OnClick_Debug_Show_DMA(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|timers", "Timers", "",
		[this]() {
			OnClick_Debug_Show_TIMER(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|spu0", "SPU0", "",
		[this]() {
			OnClick_Debug_Show_SPU0(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|spu1", "SPU1", "",
		[this]() {
			OnClick_Debug_Show_SPU1(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|intc", "INTC", "",
		[this]() {
			OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|gpu", "GPU", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|mdec", "MDEC", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|sio", "SIO", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|pio", "PIO", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|cd", "CD", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|bus", "BUS", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|icache", "ICache", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps1", "debugps1|spu", "SPU", "",
		[this]() {
			OnClick_Debug_Show_SPU(0);
		});


	m_prgwindow->AddSubmenu("debug|showwindow", "showwindow|debugps2", "PS2");
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|framebuffer", "Frame Buffer", "",
		[this]() {
			OnClick_Debug_Show_PS2_FrameBuffer(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|r5900", "R5900", "",
		[this]() {
			OnClick_Debug_Show_R5900(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|memory", "Memory", "",
		[this]() {
			OnClick_Debug_Show_PS2_Memory(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|dma", "DMA", "",
		[this]() {
			OnClick_Debug_Show_PS2_DMA(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|timers", "Timer", "",
		[this]() {
			OnClick_Debug_Show_PS2_TIMER(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|vu0", "VU0", "",
		[this]() {
			OnClick_Debug_Show_PS2_VU0(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|vu1", "VU1", "",
		[this]() {
			OnClick_Debug_Show_PS2_VU1(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|ipu", "IPU", "",
		[this]() {
			OnClick_Debug_Show_PS2_IPU(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|intc", "INTC", "",
		[this]() {
			OnClick_Debug_Show_PS2_INTC(0);
		});
	m_prgwindow->AddMenuItem("showwindow|debugps2", "debugps2|gpu", "GPU", "",
		[this]() {
			OnClick_Debug_Show_PS2_GPU(0);
		});


	m_prgwindow->AddSubmenu("debug", "debug|console", "Debug To Console");
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps1|dma|read", "PS1 | DMA | READ", "",
		[this]() {
			OnClick_Debug_Console_PS1_DMA_READ(0);
		});
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps1|dma|write", "PS1 | DMA | WRITE", "",
		[this]() {
			OnClick_Debug_Console_PS1_DMA_WRITE(0);
		});
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps1|intc|read", "PS1 | INTC | READ", "",
		[this]() {
			OnClick_Debug_Console_PS1_INTC_READ(0);
		});
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps1|intc|write", "PS1 | INTC | WRITE", "",
		[this]() {
			OnClick_Debug_Console_PS1_INTC_WRITE(0);
		});

	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps1|timer|read", "PS1 | TIMER | READ", "",
		[this]() {
			OnClick_Debug_Console_PS1_TIMER_READ(0);
		});
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps1|timer|write", "PS1 | TIMER | WRITE", "",
		[this]() {
			OnClick_Debug_Console_PS1_TIMER_WRITE(0);
		});


	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps2|dma|read", "PS2 | DMA | READ", "",
		[this]() {
			OnClick_Debug_Console_PS2_DMA_READ(0);
		});
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps2|dma|write", "PS2 | DMA | WRITE", "",
		[this]() {
			OnClick_Debug_Console_PS2_DMA_WRITE(0);
		});
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps2|intc|read", "PS2 | INTC | READ", "",
		[this]() {
			OnClick_Debug_Console_PS2_INTC_READ(0);
		});
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps2|intc|write", "PS2 | INTC | WRITE", "",
		[this]() {
			OnClick_Debug_Console_PS2_INTC_WRITE(0);
		});

	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps2|timer|read", "PS2 | TIMER | READ", "",
		[this]() {
			OnClick_Debug_Console_PS2_TIMER_READ(0);
		});
	m_prgwindow->AddMenuItem("debug|console", "debug|console|ps2|timer|write", "PS2 | TIMER | WRITE", "",
		[this]() {
			OnClick_Debug_Console_PS2_TIMER_WRITE(0);
		});


	m_prgwindow->AddMenuItem("debug", "debug|saveconsole", "Save Console", "",
		[this]() {
			OnClick_Debug_SaveConsole(0);
		});


	auto peripherals_menu = m_prgwindow->AddMenu("peripherals", "Peripherals");
	m_prgwindow->AddSubmenu("peripherals", "peripherals|pad1", "Pad1");
	m_prgwindow->AddMenuItem("peripherals|pad1", "pad1|configure", "Configure Joypad 1...", "",
		[this]() {
			OnClick_Controllers0_Configure(0);
		});
	m_prgwindow->AddSubmenu("peripherals|pad1", "pad1|type", "Type");
	m_prgwindow->AddMenuItem("pad1|type", "pad1|digital", "Digital", "",
		[this]() {
			OnClick_Pad1Type_Digital(0);
		});
	m_prgwindow->AddMenuItem("pad1|type", "pad1|analog", "Analog", "",
		[this]() {
			OnClick_Pad1Type_Analog(0);
		});
	m_prgwindow->AddMenuItem("pad1|type", "pad1|dualshock2", "Dual Shock 2", "",
		[this]() {
			OnClick_Pad1Type_DualShock2(0);
		});
	m_prgwindow->AddSubmenu("peripherals|pad1", "pad1|input", "Input");
	m_prgwindow->AddMenuItem("pad1|input", "pad1|none", "None", "",
		[this]() {
			OnClick_Pad1Input_None(0);
		});
	m_prgwindow->AddMenuItem("pad1|input", "pad1|device0", "Device0", "",
		[this]() {
			OnClick_Pad1Input_Device0(0);
		});
	m_prgwindow->AddMenuItem("pad1|input", "pad1|device1", "Device1", "",
		[this]() {
			OnClick_Pad1Input_Device1(0);
		});

	m_prgwindow->AddSubmenu("peripherals", "peripherals|pad2", "Pad2");
	m_prgwindow->AddMenuItem("peripherals|pad2", "pad2|configure", "Configure Joypad 2...", "",
		[this]() {
			OnClick_Controllers1_Configure(0);
		});
	m_prgwindow->AddSubmenu("peripherals|pad2", "pad2|type", "Type");
	m_prgwindow->AddMenuItem("pad2|type", "pad2|digital", "Digital", "",
		[this]() {
			OnClick_Pad2Type_Digital(0);
		});
	m_prgwindow->AddMenuItem("pad2|type", "pad2|analog", "Analog", "",
		[this]() {
			OnClick_Pad2Type_Analog(0);
		});
	m_prgwindow->AddMenuItem("pad2|type", "pad2|dualshock2", "Dual Shock 2", "",
		[this]() {
			OnClick_Pad2Type_DualShock2(0);
		});
	m_prgwindow->AddSubmenu("peripherals|pad2", "pad2|input", "Input");
	m_prgwindow->AddMenuItem("pad2|input", "pad2|none", "None", "",
		[this]() {
			OnClick_Pad2Input_None(0);
		});
	m_prgwindow->AddMenuItem("pad2|input", "pad2|device0", "Device0", "",
		[this]() {
			OnClick_Pad2Input_Device0(0);
		});
	m_prgwindow->AddMenuItem("pad2|input", "pad2|device1", "Device1", "",
		[this]() {
			OnClick_Pad2Input_Device1(0);
		});

	m_prgwindow->AddSubmenu("peripherals", "peripherals|memorycards", "Memory Cards");
	m_prgwindow->AddSubmenu("peripherals|memorycards", "memorycards|card1", "Card 1");
	m_prgwindow->AddMenuItem("memorycards|card1", "card1|connect", "Connect", "",
		[this]() {
			OnClick_Card1_Connect(0);
		});
	m_prgwindow->AddMenuItem("memorycards|card1", "card1|disconnect", "Disconnect", "",
		[this]() {
			OnClick_Card1_Disconnect(0);
		});
	m_prgwindow->AddSubmenu("peripherals|memorycards", "memorycards|card2", "Card 2");
	m_prgwindow->AddMenuItem("memorycards|card2", "card2|connect", "Connect", "",
		[this]() {
			OnClick_Card2_Connect(0);
		});
	m_prgwindow->AddMenuItem("memorycards|card2", "card2|disconnect", "Disconnect", "",
		[this]() {
			OnClick_Card2_Disconnect(0);
		});

	m_prgwindow->AddMenuItem("peripherals", "peripherals|redetectpads", "Re-Detect Joypad(s)", "",
		[this]() {
			OnClick_Redetect_Pads(0);
		});

	auto region_menu = m_prgwindow->AddMenu("region", "Region");
	m_prgwindow->AddMenuItem("region", "region|europe", "Europe", "",
		[this]() {
			OnClick_Region_Europe(0);
		});
	m_prgwindow->AddMenuItem("region", "region|japan", "Japan", "",
		[this]() {
			OnClick_Region_Japan(0);
		});
	m_prgwindow->AddMenuItem("region", "region|america", "North America", "",
		[this]() {
			OnClick_Region_NorthAmerica(0);
		});

	auto audio_menu = m_prgwindow->AddMenu("audio", "Audio");
	m_prgwindow->AddMenuItem("audio", "audio|enable", "Enable", "",
		[this]() {
			OnClick_Audio_Enable(0);
		});
	m_prgwindow->AddSubmenu("audio", "audio|volume", "Volume");
	m_prgwindow->AddMenuItem("audio|volume", "volume|100", "100%", "",
		[this]() {
			OnClick_Audio_Volume_100(0);
		});
	m_prgwindow->AddMenuItem("audio|volume", "volume|75", "75%", "",
		[this]() {
			OnClick_Audio_Volume_75(0);
		});
	m_prgwindow->AddMenuItem("audio|volume", "volume|50", "50%", "",
		[this]() {
			OnClick_Audio_Volume_50(0);
		});
	m_prgwindow->AddMenuItem("audio|volume", "volume|25", "25%", "",
		[this]() {
			OnClick_Audio_Volume_25(0);
		});

	m_prgwindow->AddSubmenu("audio", "audio|buffer", "Buffer Size");
	m_prgwindow->AddMenuItem("audio|buffer", "buffer|8k", "8k", "",
		[this]() {
			OnClick_Audio_Buffer_8k(0);
		});
	m_prgwindow->AddMenuItem("audio|buffer", "buffer|16k", "16k", "",
		[this]() {
			OnClick_Audio_Buffer_16k(0);
		});
	m_prgwindow->AddMenuItem("audio|buffer", "buffer|32k", "32k", "",
		[this]() {
			OnClick_Audio_Buffer_32k(0);
		});
	m_prgwindow->AddMenuItem("audio|buffer", "buffer|64k", "64k", "",
		[this]() {
			OnClick_Audio_Buffer_64k(0);
		});
	m_prgwindow->AddMenuItem("audio|buffer", "buffer|128k", "128k", "",
		[this]() {
			OnClick_Audio_Buffer_1m(0);
		});

	m_prgwindow->AddMenuItem("audio", "audio|filter", "Filter", "",
		[this]() {
			OnClick_Audio_Filter(0);
		});


	auto video_menu = m_prgwindow->AddMenu("video", "Video");

	m_prgwindow->AddSubmenu("video", "video|renderer", "Renderer");
	m_prgwindow->AddSubmenu("video|renderer", "renderer|opengl", "OpenGL");
	m_prgwindow->AddMenuItem("renderer|opengl", "opengl|software", "Software", "",
		[this]() {
			OnClick_Video_Renderer_Software_OpenGL(0);
		});

	/*
	m_prgwindow->AddSubmenu("video|renderer", "renderer|vulkan", "Vulkan");
	m_prgwindow->AddMenuItem("renderer|vulkan", "vulkan|software", "Software", "",
		[this]() {
			OnClick_Video_Renderer_Software_Vulkan(0);
		});
	m_prgwindow->AddMenuItem("renderer|vulkan", "vulkan|hardware", "Hardware", "",
		[this]() {
			OnClick_Video_Renderer_Hardware_Vulkan(0);
		});
	*/

	// scanlines are required on a ps2
	//m_prgwindow->AddSubmenu("video", "video|scanlines", "Scanlines");
	//m_prgwindow->AddMenuItem("video|scanlines", "scanlines|enable", "Enable", "",
	//	[this]() {
	//		OnClick_Video_ScanlinesEnable(0);
	//	});
	//m_prgwindow->AddMenuItem("video|scanlines", "scanlines|disable", "Disable", "",
	//	[this]() {
	//		OnClick_Video_ScanlinesDisable(0);
	//	});

	m_prgwindow->AddSubmenu("video", "video|threads", "Threads");
	m_prgwindow->AddMenuItem("video|threads", "threads|x0", "x0", "",
		[this]() {
			OnClick_GPU_0Threads(0);
		});
	m_prgwindow->AddMenuItem("video|threads", "threads|x1", "x1", "",
		[this]() {
			OnClick_GPU_1Threads(0);
		});

	m_prgwindow->AddSubmenu("video", "video|windowsize", "Window Size");
	m_prgwindow->AddMenuItem("video|windowsize", "windowsize|x1", "x1", "",
		[this]() {
			OnClick_Video_WindowSizeX1(0);
		});
	m_prgwindow->AddMenuItem("video|windowsize", "windowsize|x1.5", "x1.5", "",
		[this]() {
			OnClick_Video_WindowSizeX15(0);
		});
	m_prgwindow->AddMenuItem("video|windowsize", "windowsize|x2", "x2", "",
		[this]() {
			OnClick_Video_WindowSizeX2(0);
		});

	m_prgwindow->AddSubmenu("video", "video|displaymode", "Display Mode");
	m_prgwindow->AddMenuItem("video|displaymode", "displaymode|stretch", "Stretch", "",
		[this]() {
			OnClick_Video_DisplayMode_Stretch(0);
		});
	m_prgwindow->AddMenuItem("video|displaymode", "displaymode|fit", "Fit", "",
		[this]() {
			OnClick_Video_DisplayMode_Fit(0);
		});

	m_prgwindow->AddMenuItem("video", "video|fullscreen", "Full Screen", "F11",
		[this]() {
			OnClick_Video_FullScreen(0);
		});

	auto cpu_menu = m_prgwindow->AddMenu("cpu", "CPU");
	m_prgwindow->AddSubmenu("cpu", "cpu|r3000a", "R3000A");
	m_prgwindow->AddMenuItem("cpu|r3000a", "r3000a|interpreter", "Interpreter", "",
		[this]() {
			OnClick_R3000ACPU_Interpreter(0);
		});
	m_prgwindow->AddMenuItem("cpu|r3000a", "r3000a|recompiler", "Recompiler", "",
		[this]() {
			OnClick_R3000ACPU_Recompiler(0);
		});
	m_prgwindow->AddSubmenu("cpu", "cpu|r5900", "R5900");
	m_prgwindow->AddMenuItem("cpu|r5900", "r5900|interpreter", "Interpreter", "",
		[this]() {
			OnClick_R5900CPU_Interpreter(0);
		});
	m_prgwindow->AddMenuItem("cpu|r5900", "r5900|recompiler", "Recompiler", "",
		[this]() {
			OnClick_R5900CPU_Recompiler(0);
		});
	m_prgwindow->AddSubmenu("cpu", "cpu|vu0", "VU0");
	m_prgwindow->AddMenuItem("cpu|vu0", "vu0|interpreter", "Interpreter", "",
		[this]() {
			OnClick_VU0_Interpreter(0);
		});
	m_prgwindow->AddMenuItem("cpu|vu0", "vu0|recompiler", "Recompiler", "",
		[this]() {
			OnClick_VU0_Recompiler(0);
		});
	m_prgwindow->AddSubmenu("cpu", "cpu|vu1", "VU1");
	m_prgwindow->AddMenuItem("cpu|vu1", "vu1|interpreter", "Interpreter", "",
		[this]() {
			OnClick_VU1_Interpreter(0);
		});
	m_prgwindow->AddMenuItem("cpu|vu1", "vu1|recompiler", "Recompiler", "",
		[this]() {
			OnClick_VU1_Recompiler(0);
		});
}



void hps2x64::RunProgram_Normal()
{
	unsigned long long i, j;

	long xsize, ysize;

	int k;

	// the frame counter is 32-bits
	u32 LastFrameNumber;
	volatile u32* pCurrentFrameNumber;

	bool bRunningTooSlow;

	s64 MilliSecsToWait;

	u64 TicksPerSec, CurrentTimer, TargetTimer;
	s64 TicksLeft;
	double dTicksPerMilliSec;

	u64 ullPerfStart_Timer;
	u64 ullPerfEnd_Timer;
	u64 ullTicksUsedPerSec;
	u32 ulFramesPerSec;


	// get ticks per second for the platform's high-resolution timer
	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&TicksPerSec))
	{
		cout << "\nhpsx64 error: Error returned from call to QueryPerformanceFrequency.\n";
	}

	// calculate the ticks per milli second
	dTicksPerMilliSec = ((double)TicksPerSec) / 1000.0L;

	// get a pointer to the current frame number
	pCurrentFrameNumber = (volatile u32*)&_SYSTEM._GPU.Frame_Count;


	cout << "Running program...\n";

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetCaption(ProgramWindow_Caption);
#else

	ProgramWindow->SetCaption(ProgramWindow_Caption);

#endif

	// this actually needs to loop until a frame is drawn by the core simulator... and then it should return the drawn frame + its size...
	// so the actual platform it is running on can then draw it

	// get the ticks per second for the timer

	// get the start timer value for the run
	if (!QueryPerformanceCounter((LARGE_INTEGER*)&TargetTimer))
	{
		cout << "\nhpsx64: Error returned from QueryPerformanceCounter\n";
	}

	// the target starts equal to the start
	//SystemTimer_Target = SystemTimer_Start;

	cout << "Running program NORMAL...\n";

	// multi-threading testing
	VU::Start_Frame();
	GPU::Start_Frame();
	//SPU2::Start_Thread();

	while (_RunMode.RunNormal)
	{
		//unsigned __int64 i;
		//unsigned int ui;
		//i = __rdtscp(&ui);
		//printf_s("%I64d ticks\n", i);
		//printf_s("TSC_AUX was %x\n", ui);

		// init the ticks used in the next second
		ullTicksUsedPerSec = 0;

		switch (_SYSTEM._GPU.iGraphicsMode)
		{
		case 1:
		case 2:
			// ntsc - about 60 frames per second //

			ulFramesPerSec = 60;
			break;

		case 3:
			// pal - 50 frames per second //

			ulFramesPerSec = 50;
			break;

		default:
			// ??
			ulFramesPerSec = 60;
			break;
		}

		for (j = 0; j < ulFramesPerSec; j++)
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&ullPerfStart_Timer);

			// get the last frame number
			LastFrameNumber = *pCurrentFrameNumber;

			// need to synchronize with graphics
			//GPU::Finish ();

			// multi-threading testing
			//GPU::Start_Frame ();

			// loop until we reach the next frame
			//for ( i = 0; i < CyclesToRunContinuous; i++ )
			while (LastFrameNumber == (*pCurrentFrameNumber))
			{
				// run playstation 1 system in regular mode for one cpu instruction
				_SYSTEM.Run();
			}

			// multi-threading testing
			//GPU::End_Frame ();

			// get the target platform timer value for this frame
			// check if this is ntsc or pal
			// ***TODO*** todo for PS2
			//if ( _SYSTEM._GPU.GPU_CTRL_Read.VIDEO )
			if (_SYSTEM._GPU.iGraphicsMode == GPU::GRAPHICS_MODE_PAL_I)
			{
				// PAL //
				//TargetTimer += ( TicksPerSec / 50 );
				TargetTimer += (((double)TicksPerSec) / GPU::PAL_FramesPerSec);
			}
			else
			{
				// NTSC //
				//TargetTimer += ( TicksPerSec / 60 );
				TargetTimer += (((double)TicksPerSec) / GPU::NTSC_FramesPerSec);
			}

			// process events
			//WindowClass::DoEventsNoWait ();

			// check if we are running slower than target
			if (!QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTimer))
			{
				cout << "\nhps2x64: Error returned from QueryPerformanceCounter\n";
			}

			TicksLeft = TargetTimer - CurrentTimer;

			bRunningTooSlow = false;
			if (TicksLeft < 0)
			{
				// running too slow //
				bRunningTooSlow = true;
			}


			// get the timer value we are at now
			QueryPerformanceCounter((LARGE_INTEGER*)&ullPerfEnd_Timer);
			ullTicksUsedPerSec += ullPerfEnd_Timer - ullPerfStart_Timer;


			// update joysticks once each frame
			Playstation1::SIO::gamepad.Update();


			do
			{
				// active-wait

				// process events
				WindowClass::DoEventsNoWait();
				m_prgwindow->ProcessMessages();

				if (!QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTimer))
				{
					cout << "\nhpsx64: Error returned from QueryPerformanceCounter\n";
				}

				TicksLeft = TargetTimer - CurrentTimer;

				MilliSecsToWait = (u64)(((double)TicksLeft) / dTicksPerMilliSec);

				//if ( MilliSecsToWait <= 0 ) MilliSecsToWait = 1;
				if (MilliSecsToWait < 1) MilliSecsToWait = 0;

#ifdef SPU2_USE_WAVEOUT

			if (SPU2::waveout_driver) {
				HANDLE h = SPU2::waveout_driver->eventHandle();

				DWORD res = MsgWaitForMultipleObjectsEx(
					1, &h,
					MilliSecsToWait,
					QS_ALLINPUT,
					MWMO_ALERTABLE
				);

				if (res == WAIT_OBJECT_0) {
					//cout << "\naudio event";

					SPU2::waveout_driver->onAudioEvent();  // plays queued samples
				}
			} else {
				MsgWaitForMultipleObjectsEx(NULL, NULL, MilliSecsToWait, QS_ALLINPUT, MWMO_ALERTABLE);
			}

#else

				MsgWaitForMultipleObjectsEx(NULL, NULL, MilliSecsToWait, QS_ALLINPUT, MWMO_ALERTABLE);

#endif


				if (!QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTimer))
				{
					cout << "\nhpsx64: Error returned from QueryPerformanceCounter\n";
				}

			} while (CurrentTimer < TargetTimer);


			if (WindowClass::Window::InModalMenuLoop)
			{

				VU::End_Frame();
				GPU::End_Frame();

				SPU2::End_Thread();

				// if menu has been clicked then wait
				WindowClass::Window::WaitForModalMenuLoop();

				VU::Start_Frame();
				GPU::Start_Frame();

				//SPU2::Start_Thread();
			}

			// if menu was clicked, hop out of loop
			if (HandleMenuClick()) break;


			// check if we are running too slow
			if (bRunningTooSlow)
			{
				// set the new timer target to be the current timer
				if (!QueryPerformanceCounter((LARGE_INTEGER*)&TargetTimer))
				{
					cout << "\nhps2x64: Error returned from QueryPerformanceCounter\n";
				}
			}


		}	// end for ( j = 0; j < 60; j++ )


		// update all the debug info windows that are showing
		DebugWindow_Update();

		if (!_RunMode.RunNormal) cout << "\nWaiting for command\n";

		// get the speed as a percentage of full speed
		double dPerf, dPerf2;
		stringstream ss;


		// put program name into program caption
		ss << "hps2x64";


		switch (_SYSTEM._GPU.iGraphicsMode)
		{
		case 1:
			ss << " NTSCP";
			break;

		case 2:
			ss << " NTSCI";
			break;

		case 3:
			ss << " PALI";
			break;

		default:
			ss << " UNKN";
			break;
		}

		// put in caption for statistic
		//ss << " - Speed: " << fixed << setprecision(2);
		ss << " " << fixed << setprecision(2);

		// get the difference in ticks between the time we started and the time we are at now
		//TicksLeft = ullPerfEnd_Timer - ullPerfStart_Timer;

		// if ntsc, 60 frames should take about 1 second
		//dPerf = ( ( dTicksPerMilliSec * 1000L ) / TicksLeft ) * 100L;
		dPerf = ((dTicksPerMilliSec * 1000L) / ullTicksUsedPerSec) * 100L;

		// check if multi-threaded or not
		if (!GPU::ulNumberOfThreads)
		{
			// single-threaded //

			ss << "Thread0 (Main): ";
			ss << dPerf << "%";
		}
		else
		{
			// multi-threaded //

			// also get the speed on the vu1/gpu thread
			TicksLeft = GPU::ullTotalElapsedTime;
			GPU::ullTotalElapsedTime = 0;

			// gpu thread shouldn't take more than a second to render 60 frames
			dPerf2 = ((dTicksPerMilliSec * 1000L) / TicksLeft) * 100L;

			if (!VU::ulThreadCount)
			{
				// gpu only theaded //

				ss << "Thread0: ";
				ss << dPerf << "%";
				ss << " Thread1(GPU): ";
				ss << dPerf2 << "%";
			}
			else
			{
				// vu1+gpu threaded //

				ss << "Thread0 (Main): ";
				ss << dPerf << "%";
				ss << " Thread1 (VU1+GPU): ";
				ss << dPerf2 << "%";
			}

		}

		/*
		if (_SYSTEM._GPU.bEnable_OpenCL)
		{
			ss << " Vulkan: Hardware (Threads:" << vulkan_get_gpu_threads() << " Sync:" << dec << _SYSTEM._GPU.iLastSyncCount << ")";
		}
		else
		*/
		{
			switch (_SYSTEM._GPU.iCurrentDrawLibrary)
			{
			case Playstation2::GPU::DRAW_LIBRARY_VULKAN:
				ss << " Vulkan: Software";
				break;

			default:
				ss << " OpenGL: Software";
				break;
			}

		}

#ifdef USE_NEW_PROGRAM_WINDOW

		m_prgwindow->SetCaption(ss.str().c_str());
#else

		// put the program name and speed statistics in the caption bar for program
		ProgramWindow->SetCaption(ss.str().c_str());

#endif

		//i = __rdtscp(&ui);
		//printf_s("%I64d ticks\n", i);
		//printf_s("TSC_AUX was %x\n", ui);


	}	// end while ( _RunMode.RunNormal )

	// multi-threading testing
	VU::End_Frame();
	GPU::End_Frame();

	SPU2::End_Thread();

}




void hps2x64::RunProgram_Normal_VirtualMachine()
{

	RunProgram_Normal();

}


void hps2x64::RunProgram ()
{
	unsigned long long i, j;
	
	long xsize, ysize;

	int k;
	
	// the frame counter is 32-bits
	u32 LastFrameNumber;
	volatile u32 *pCurrentFrameNumber;
	
	bool bRunningTooSlow;
	
	s64 MilliSecsToWait;
	
	u64 TicksPerSec, CurrentTimer, TargetTimer;
	s64 TicksLeft;
	double dTicksPerMilliSec;
	
	cout << "\nRunning program";


	// LOAD VULKAN //
	// note: actually, don't unconditionally load vulkan like this
	//_SYSTEM._GPU.LoadVulkan();

	
	// get ticks per second for the platform's high-resolution timer
	if ( !QueryPerformanceFrequency ( (LARGE_INTEGER*) &TicksPerSec ) )
	{
		cout << "\nhpsx64 error: Error returned from call to QueryPerformanceFrequency.\n";
	}
	
	// calculate the ticks per milli second
	dTicksPerMilliSec = ( (double) TicksPerSec ) / 1000.0L;
	
	// get a pointer to the current frame number
	pCurrentFrameNumber = (volatile u32*) & _SYSTEM._GPU.Frame_Count;
	
	cout << "\nWaiting for command\n";
	
	// wait for command
	while ( 1 )
	{
		Sleep ( 16 );
		
		// process window-level events (handles close/quit, resize, fullscreen toggle)
		if ( !m_prgwindow->ProcessMessages() )
		{
			OnClick_File_Exit(0);
		}

		// process remaining system events (keyboard shortcuts, etc.)
		WindowClass::DoEvents ();

		HandleMenuClick ();
		
		if ( _RunMode.Exit ) break;

		// check if there is any debugging going on
		// this mode is only required if there are breakpoints set
		if ( _RunMode.RunDebug )
		{
			cout << "Running program in debug mode...\n";
			
#ifdef USE_NEW_PROGRAM_WINDOW

			m_prgwindow->SetCaption("hps2x64");
#else

			ProgramWindow->SetCaption ( "hps2x64" );

#endif
			
			while ( _RunMode.RunDebug )
			{
				
				for ( j = 0; j < 60; j++ )
				{

					// loop until we reach the next frame
					//for ( i = 0; i < CyclesToRunContinuous; i++ )
					while (LastFrameNumber == (*pCurrentFrameNumber))
					{
						// run playstation 1 system in regular mode for at least one cycle
						_SYSTEM.Run ();
						
						// check if any breakpoints were hit
						if ( _SYSTEM._CPU.Breakpoints->Check_IfBreakPointReached () >= 0 ) break;
					}
					
					//cout << "\nSystem is running (debug). " << dec << _SYSTEM._CPU.CycleCount;
					
					// update next to exit loop at
					//_SYSTEM.NextExit_Cycle = _SYSTEM._CPU.CycleCount + CyclesToRunContinuous;
					

					// update joysticks once each frame
					Playstation1::SIO::gamepad.Update();


					// process events
					WindowClass::DoEventsNoWait ();
					m_prgwindow->ProcessMessages();
					
					// if menu has been clicked then wait
					WindowClass::Window::WaitForModalMenuLoop ();
					
					//k = _SYSTEM._CPU.Breakpoints->Check_IfBreakPointReached ();
					if ( _SYSTEM._CPU.Breakpoints->Get_LastBreakPoint () >= 0 )
					{
						cout << "\nbreakpoint hit";
						_RunMode.Value = 0;
						//_SYSTEM._CPU.Breakpoints->Set_LastBreakPoint ( k );
						break;
					}
					
					// if menu was clicked, hop out of loop
					if ( HandleMenuClick () ) break;
				
				}

				// update all the debug info windows that are showing
				DebugWindow_Update ();
				
				if ( !_RunMode.RunDebug )
				{
					cout << "\n_RunMode.Value=" << _RunMode.Value;
					cout << "\nk=" << k;
					cout << "\nWaiting for command\n";
				}
			}
		}

		// run program normally and without debugging
		if ( _RunMode.RunNormal )
		{
			RunProgram_Normal_VirtualMachine();
		}
		
	}
	
	cout << "\nDone running program\n";


	// unload vulkan if it is enabled and hardware rendering is allowed //
	/*
	if (vulkan_is_loaded())
	{
		//vulkan_destroy();
		_HPS2X64._SYSTEM._GPU.UnloadVulkan();
	}
	*/
	
	// Program loop is done here at this point //
	// Closing Program //
	
	// write back memory cards
	//_SYSTEM._SIO.Store_MemoryCardFile ( ExecutablePath + "card0", 0 );
	//_SYSTEM._SIO.Store_MemoryCardFile ( ExecutablePath + "card1", 1 );
	_SYSTEM._PS1SYSTEM._SIO.Store_PS2MemoryCardFile ( ExecutablePath + "ps2card0", 0 );
	_SYSTEM._PS1SYSTEM._SIO.Store_PS2MemoryCardFile ( ExecutablePath + "ps2card1", 1 );
	
	// check if there is an nvm file at all
	if ( _SYSTEM.Last_NVM_Path [ 0 ] )
	{
		// write back to the NVM file ?? //
		
		// write back NVM file ??
		if ( !_SYSTEM._PS1SYSTEM._CDVD.Store_NVMFile ( _SYSTEM.Last_NVM_Path ) )
		{
			cout << "\nhps2x64: ALERT: Problem writing NVM File to PROGRAM directory.\n";
		}
	}
	
	cout << "\nSaving config...";

	// save configuration
	SaveConfig ( ExecutablePath + "hps2x64.hcfg" );
}



void hps2x64::LoadClick ( int i )
{
	MessageBox( NULL, "Clicked load.", "", NULL );
}

void hps2x64::SaveStateClick ( int i )
{
	System::_DebugStatus d;

#ifdef INLINE_DEBUG_MENU
	System::debug << "\r\nSaveStateClick; Previous Debug State: " << System::_SYSTEM->DebugStatus.Value;
#endif
	
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.SaveState = true;
	
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	
#ifdef INLINE_DEBUG_MENU
	System::debug << ";->New Debug State: " << System::_SYSTEM->DebugStatus.Value;
#endif

}


void hps2x64::LoadStateClick ( int i )
{
	System::_DebugStatus d;
	
#ifdef INLINE_DEBUG_MENU
	System::debug << "\r\nLoadStateClick; Previous Debug State: " << System::_SYSTEM->DebugStatus.Value;
#endif

	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.LoadState = true;
	
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	
#ifdef INLINE_DEBUG_MENU
	System::debug << ";->New Debug State: " << System::_SYSTEM->DebugStatus.Value;
#endif

}


void hps2x64::LoadBiosClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.LoadBios = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}


void hps2x64::StartClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.Stop = false;
	d.Value = 0;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}

void hps2x64::StopClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.Stop = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}

void hps2x64::StepInstructionClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.Stop = true;
	d.StepInstruction = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}


void hps2x64::SaveBIOSClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.SaveBIOSToFile = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}

void hps2x64::SaveRAMClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.SaveRAMToFile = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}

void hps2x64::SetBreakPointClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.SetBreakPoint = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}


void hps2x64::SetCycleBreakPointClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.SetCycleBreakPoint = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}

void hps2x64::SetAddressBreakPointClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.SetAddressBreakPoint = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}

void hps2x64::SetValueClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.SetValue = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}

void hps2x64::SetMemoryClick ( int i )
{
	System::_DebugStatus d;
	d.Value = System::_SYSTEM->DebugStatus.Value;
	
	d.SetMemoryStart = true;
	
	//x64ThreadSafe::Utilities::Lock_OR32 ( (long*) &(Cpu::DebugStatus.Value), (long) d.Value );
	//System::_SYSTEM->DebugStatus.Value = d.Value;
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&)System::_SYSTEM->DebugStatus.Value, (long) d.Value );
}




void hps2x64::OnClick_File_Load_State ( int i )
{
	//MenuClicked m;
	//m.File_Load_State = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked File | Load | State\n";
	_HPS2X64.LoadState ();
	
	// set vsync according to the setting
#ifdef USE_NEW_PROGRAM_WINDOW

	if (_HPS2X64._SYSTEM.bEnableVsync)
	{
		//ProgramWindow->EnableVSync();
		m_prgwindow->SetVSync(true);
	}
	else
	{
		//ProgramWindow->DisableVSync();
		m_prgwindow->SetVSync(false);
	}
#else
	if ( _HPS2X64._SYSTEM.bEnableVsync )
	{
		ProgramWindow->EnableVSync ();
	}
	else
	{
		ProgramWindow->DisableVSync ();
	}
#endif
	
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_File_Load_BIOS ( int i )
{
	//MenuClicked m;
	//m.File_Load_BIOS = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked File | Load | BIOS\n";
	_HPS2X64.LoadBIOS ();
	
	cout << "\n->last bios set to: " << sLastBiosPath.c_str();

	_HPS2X64.Update_CheckMarksOnMenu();

	_MenuWasClicked = 1;
}

/*
void hps2x64::OnClick_File_Load_GameDisk_PS2CD ( int i )
{
	MenuClicked m;
	m.File_Load_GameDisk_PS2CD = true;
	x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
}

void hps2x64::OnClick_File_Load_GameDisk_PS2DVD ( int i )
{
	MenuClicked m;
	m.File_Load_GameDisk_PS2DVD = true;
	x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
}
*/

void hps2x64::OnClick_File_Load_GameDisk_PS2 ( int i )
{
	//MenuClicked m;
	//m.File_Load_GameDisk_PS2 = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	string ImagePath;
	bool bDiskOpened;
	
	cout << "\nYou clicked File | Load | Game Disk\n";
	
	// set that PS2 game disk option was clicked for now
	_MenuClick.Value = 0;
	
	if ( _HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen )
	{
		// lid is currently open //
		ImagePath = _HPS2X64.LoadDisk ();
		
		if ( ImagePath != "" )
		{
		
			//bDiskOpened = _SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );
			/*
			if ( _MenuClick.File_Load_GameDisk_PS2DVD )
			{
				// DVD
				// only load 2048 bytes per sector
				bDiskOpened = _SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath, 2048 );
			}
			else
			{
				// must be a CD disk //
				
				// CD
				// load 2352 bytes per sector
				bDiskOpened = _SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );
			}
			*/
		
			bDiskOpened = _HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );

			if ( bDiskOpened )
			{
				
				/*
				if ( _MenuClick.File_Load_GameDisk_PS2CD )
				{
					// PS2 only: need to set the disk type
					_SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PS2CD;
				}
				else if ( _MenuClick.File_Load_GameDisk_PS2DVD )
				{
					// PS2 only: need to set the disk type
					_SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PS2DVD;
				}
				else
				{
					// must be a PS1 disk //
					
					// PS2 only: need to set the disk type
					_SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PSCD;
				}
				*/
				
				if ( _MenuClick.File_Load_GameDisk_PS1 )
				{
					// PS1 Game Disk //
					
					// must be cd
					if ( _HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.bIsDiskCD )
					{
						// Disk image OK //
						_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PSCD;
					}
					else
					{
						// ERROR //
						cout << "\nhps2x64: Error: PS1 game disk is NOT a data CD.";
						return;
					}
				}
				else
				{
					// PS2 Game Disk //
					
					// check if CD or DVD
					if ( _HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.bIsDiskCD )
					{
						// PS2 Game Disk is CD //
						
						_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PS2CD;
					}
					else
					{
						// PS2 Game Disk is DVD //
						
						_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PS2DVD;
					}
				}
				
				// output info for the loaded disk
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.Output_IndexData ();
				
				
				cout << "\nhps2x64 NOTE: Game Disk opened successfully\n";
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.isGameCD = true;
				
				// lid should now be closed since disk is open
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen = false;
				
				// don't want to run any ps1 cd events in ps2 mode, because ps1 devices are not running currently in ps2 mode
				//_HPS2X64._SYSTEM._PS1SYSTEM._CD.Event_LidClose ();
			}
			else
			{
				cout << "\nhpsx64 ERROR: Problem opening disk\n";
			}
		}
		else
		{
			cout << "\nERROR: Unable to open disk image. Either no disk was chosen or other problem.";
		}
	}
	else
	{
		// lid is currently closed //
		
		// open the lid
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen = true;
		
		// close the currently open disk image
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.CloseDiskImage ();
		
		// don't run ps1cd events in ps2 mode for now
		//_HPS2X64._SYSTEM._PS1SYSTEM._CD.Event_LidOpen ();
	}
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_File_Load_GameDisk_PS1 ( int i )
{
	//MenuClicked m;
	//m.File_Load_GameDisk_PS1 = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	string ImagePath;
	bool bDiskOpened;
	
	cout << "\nYou clicked File | Load | Game Disk\n";
	
	// set that PS2 game disk option was clicked for now
	_MenuClick.Value = 0;
	_MenuClick.File_Load_GameDisk_PS1 = 1;
	
	if ( _HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen )
	{
		// lid is currently open //
		ImagePath = _HPS2X64.LoadDisk ();
		
		if ( ImagePath != "" )
		{
		
			//bDiskOpened = _SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );
			/*
			if ( _MenuClick.File_Load_GameDisk_PS2DVD )
			{
				// DVD
				// only load 2048 bytes per sector
				bDiskOpened = _SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath, 2048 );
			}
			else
			{
				// must be a CD disk //
				
				// CD
				// load 2352 bytes per sector
				bDiskOpened = _SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );
			}
			*/
		
			bDiskOpened = _HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );

			if ( bDiskOpened )
			{
				
				/*
				if ( _MenuClick.File_Load_GameDisk_PS2CD )
				{
					// PS2 only: need to set the disk type
					_SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PS2CD;
				}
				else if ( _MenuClick.File_Load_GameDisk_PS2DVD )
				{
					// PS2 only: need to set the disk type
					_SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PS2DVD;
				}
				else
				{
					// must be a PS1 disk //
					
					// PS2 only: need to set the disk type
					_SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PSCD;
				}
				*/
				
				if ( _MenuClick.File_Load_GameDisk_PS1 )
				{
					// PS1 Game Disk //
					
					// must be cd
					if ( _HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.bIsDiskCD )
					{
						// Disk image OK //
						_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PSCD;
					}
					else
					{
						// ERROR //
						cout << "\nhps2x64: Error: PS1 game disk is NOT a data CD.";
						return;
					}
				}
				else
				{
					// PS2 Game Disk //
					
					// check if CD or DVD
					if ( _HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.bIsDiskCD )
					{
						// PS2 Game Disk is CD //
						
						_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PS2CD;
					}
					else
					{
						// PS2 Game Disk is DVD //
						
						_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.CurrentDiskType = CDVD_TYPE_PS2DVD;
					}
				}
				
				// output info for the loaded disk
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.Output_IndexData ();
				
				
				cout << "\nhps2x64 NOTE: Game Disk opened successfully\n";
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.isGameCD = true;
				
				// lid should now be closed since disk is open
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen = false;
				
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.Event_LidClose ();
			}
			else
			{
				cout << "\nhpsx64 ERROR: Problem opening disk\n";
			}
		}
		else
		{
			cout << "\nERROR: Unable to open disk image. Either no disk was chosen or other problem.";
		}
	}
	else
	{
		// lid is currently closed //
		
		// open the lid
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen = true;
		
		// close the currently open disk image
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.CloseDiskImage ();
		
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.Event_LidOpen ();
	}
	
	// set that PS2 game disk option was clicked for now
	_MenuClick.Value = 0;
	
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_File_Load_AudioDisk ( int i )
{
	//MenuClicked m;
	//m.File_Load_AudioDisk = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	string ImagePath;
	bool bDiskOpened;
	
	cout << "\nYou clicked File | Load | Audio Disk\n";
	
	if ( _HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen )
	{
		// lid is currently open //
		ImagePath = _HPS2X64.LoadDisk ();
		
		if ( ImagePath != "" )
		{
			bDiskOpened = _HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );
		
			if ( bDiskOpened )
			{
				cout << "\nhpsx64 NOTE: Audio Disk opened successfully\n";
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.isGameCD = false;
				
				// lid should now be closed since disk is open
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen = false;
				
				_HPS2X64._SYSTEM._PS1SYSTEM._CD.Event_LidClose ();
			}
			else
			{
				cout << "\nhpsx64 ERROR: Problem opening disk\n";
			}
		}
		else
		{
			cout << "\nERROR: Unable to open disk image. Either no disk was chosen or other problem.";
		}
	}
	else
	{
		// lid is currently closed //
		
		// open the lid
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.isLidOpen = true;
		
		// close the currently open disk image
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.cd_image.CloseDiskImage ();
		
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.Event_LidOpen ();
	}
	
	_MenuWasClicked = 1;
}



void hps2x64::OnClick_File_Save_State ( int i )
{
	//MenuClicked m;
	//m.File_Save_State = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked File | Save | State\n";
	
	_HPS2X64.SaveState ();
	
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_File_Reset ( int i )
{
	//MenuClicked m;
	//m.File_Reset = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked File | Reset\n";
	
	// need to call start, not reset
	_HPS2X64._SYSTEM.Start ();
	
	_MenuWasClicked = 1;
}




void hps2x64::OnClick_File_Run ( int i )
{
	//MenuClicked m;
	//m.File_Run = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked File | Run\n";
	_RunMode.Value = 0;
	
	// if there are no breakpoints, then we can run in normal mode
	if ( !_HPS2X64._SYSTEM._CPU.Breakpoints->Count() )
	{
		_RunMode.RunNormal = true;
	}
	else
	{
		_RunMode.RunDebug = true;
	}
	
	// clear the last breakpoint hit
	_HPS2X64._SYSTEM._CPU.Breakpoints->Clear_LastBreakPoint ();
	
	// clear read/write debugging info
	_HPS2X64._SYSTEM._CPU.Last_ReadAddress = 0;
	_HPS2X64._SYSTEM._CPU.Last_WriteAddress = 0;
	_HPS2X64._SYSTEM._CPU.Last_ReadWriteAddress = 0;
	
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_File_Exit ( int i )
{
	//MenuClicked m;
	//m.File_Exit = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked File | Exit\n";
	
	// uuuuuuser chose to exit program
	_RunMode.Value = 0;
	_RunMode.Exit = true;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Break ( int i )
{
	//MenuClicked m;
	//m.Debug_Break = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Debug | Break\n";
	
	// clear the last breakpoint hit if system is running
	if  ( _RunMode.Value != 0 ) _HPS2X64._SYSTEM._CPU.Breakpoints->Clear_LastBreakPoint ();
	
	_RunMode.Value = 0;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_StepInto ( int i )
{
	//MenuClicked m;
	//m.Debug_StepInto = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Debug | Step Into\n";
	
	// step one system cycle
	_HPS2X64.StepCycle ();
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_StepPS1_Instr ( int i )
{
	//MenuClicked m;
	//m.Debug_StepPS1_Instr = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Debug | Step Into\n";
	
	// step a PS1 instruction
	_HPS2X64.StepInstructionPS1 ();
	
	_MenuWasClicked = 1;
}
void hps2x64::OnClick_Debug_StepPS2_Instr ( int i )
{
	//MenuClicked m;
	//m.Debug_StepPS2_Instr = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Debug | Step Into\n";
	
	// step a PS2 instruction
	_HPS2X64.StepInstructionPS2 ();
	
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_Debug_OutputCurrentSector ( int i )
{
	//_HPS2X64._SYSTEM._CD.OutputCurrentSector ();
	//_HPS2X64._SYSTEM.Test ();
	
	/*
	long *pData = (long*) & Playstation1::SIO::DJoy.gameControllerStates[0];
	
	Playstation1::SIO::DJoy.Update( 0 );
	
	cout << "\ngameControllerState: ";
	cout << "\nPOV0=" << hex << Playstation1::SIO::DJoy.gameControllerStates[0].rgdwPOV[0] << " " << dec << Playstation1::SIO::DJoy.gameControllerStates[0].rgdwPOV[0];
	cout << "\nPOV1=" << hex << Playstation1::SIO::DJoy.gameControllerStates[0].rgdwPOV[1] << " " << dec << Playstation1::SIO::DJoy.gameControllerStates[0].rgdwPOV[1];
	cout << "\nPOV2=" << hex << Playstation1::SIO::DJoy.gameControllerStates[0].rgdwPOV[2] << " " << dec << Playstation1::SIO::DJoy.gameControllerStates[0].rgdwPOV[2];
	cout << "\nPOV3=" << hex << Playstation1::SIO::DJoy.gameControllerStates[0].rgdwPOV[3] << " " << dec << Playstation1::SIO::DJoy.gameControllerStates[0].rgdwPOV[3];
	
	for ( int i = 0; i < 32; i++ )
	{
		cout << "\nPOV" << dec << i << "=" << hex << (unsigned long) Playstation1::SIO::DJoy.gameControllerStates[0].rgbButtons[i] << " " << dec << (unsigned long) Playstation1::SIO::DJoy.gameControllerStates[0].rgbButtons[i];
	}
	
	for ( int i = 0; i < 6; i++ )
	{
		cout << "\nAxis#" << dec << i << "=" << hex << *pData << " " << dec << *pData;
		pData++;
	}
	*/

	// output ipu testing data
	cout << "\nIPU-FIFO-IN-SIZE= " << dec << _HPS2X64._SYSTEM._IPU.FifoIn_Size;
	cout << "\nIPU-FIFO-OUT-SIZE= " << dec << _HPS2X64._SYSTEM._IPU.FifoOut_Size;
	cout << "\nIPU-DECODER-FIFO-OUT-SIZE= " << dec << _HPS2X64._SYSTEM._IPU.thedecoder.ipu0_data;
	cout << "\nIPU-COMMAND-WRITE= " << hex << _HPS2X64._SYSTEM._IPU.CMD_Write.Value;
	cout << "\nIPU-COMMAND-READ= " << hex << _HPS2X64._SYSTEM._IPU.CMD_Read.Value;
	cout << "\nIPU-CTRL= " << hex << _HPS2X64._SYSTEM._IPU.Regs.CTRL.Value;
	cout << "\nIPU-BP= " << hex << _HPS2X64._SYSTEM._IPU.Regs.BP.Value;
	cout << "\nIPU-TOP= " << hex << _HPS2X64._SYSTEM._IPU.Regs.TOP.Value;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_PS2_All ( int i )
{
	//MenuClicked m;
	//m.Debug_ShowWindow_PS2_All = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_PS2_FrameBuffer ( int i )
{
	cout << "\nYou clicked Debug | Show PS2 | PS2 FrameBuffer\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|framebuffer";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._GPU.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._GPU.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "PS2 FrameBuffer" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for PS2 FrameBuffer and uncheck item
		cout << "Disabling debug window for PS2 FrameBuffer\n";
		_HPS2X64._SYSTEM._GPU.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "PS2 FrameBuffer" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for PS2 INTC\n";
		_HPS2X64._SYSTEM._GPU.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_R5900 ( int i )
{
	cout << "\nYou clicked Debug | Show Window | R5900\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|r5900";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._CPU.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._CPU.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "R5900" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for R5900\n";
		_HPS2X64._SYSTEM._CPU.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "R5900" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for R5900\n";
		_HPS2X64._SYSTEM._CPU.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_PS2_Memory ( int i )
{
	cout << "\nYou clicked Debug | Show PS2 | PS2 Memory\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|memory";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._BUS.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._BUS.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "PS2 Memory" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for PS2 Memory\n";
		_HPS2X64._SYSTEM._BUS.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "PS2 Memory" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for PS2 Memory\n";
		_HPS2X64._SYSTEM._BUS.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_PS2_DMA ( int i )
{
	cout << "\nYou clicked Debug | Show PS2 | PS2 DMA\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|dma";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._DMA.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._DMA.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "PS2 DMA" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for PS2 DMA\n";
		_HPS2X64._SYSTEM._DMA.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "PS2 DMA" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for PS2 DMA\n";
		_HPS2X64._SYSTEM._DMA.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_PS2_TIMER ( int i )
{
	cout << "\nYou clicked Debug | Show PS2 | PS2 Timers\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|timers";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._TIMERS.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._TIMERS.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "PS2 Timers" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for PS2 Timers\n";
		_HPS2X64._SYSTEM._TIMERS.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "PS2 Timers" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for PS2 Timers\n";
		_HPS2X64._SYSTEM._TIMERS.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_PS2_VU0 ( int i )
{
	cout << "\nYou clicked Debug | Show PS2 | VU0\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|vu0";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._VU0.VU0.DebugWindow_Disable(0);
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._VU0.VU0.DebugWindow_Enable(0);
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "VU0" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for VU0\n";
		_HPS2X64._SYSTEM._VU0.VU0.DebugWindow_Disable ( 0 );
		ProgramWindow->Menus->UnCheckItem ( "VU0" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for VU0\n";
		_HPS2X64._SYSTEM._VU0.VU0.DebugWindow_Enable ( 0 );
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_PS2_VU1 ( int i )
{
	cout << "\nYou clicked Debug | Show PS2 | VU1\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|vu1";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._VU1.VU1.DebugWindow_Disable(1);
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._VU1.VU1.DebugWindow_Enable(1);
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "VU1" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for VU1\n";
		_HPS2X64._SYSTEM._VU1.VU1.DebugWindow_Disable ( 1 );
		ProgramWindow->Menus->UnCheckItem ( "VU1" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for VU1\n";
		_HPS2X64._SYSTEM._VU1.VU1.DebugWindow_Enable ( 1 );
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_PS2_IPU(int i)
{
	cout << "\nYou clicked Debug | Show PS2 | IPU\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|ipu";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._IPU.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._IPU.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if (ProgramWindow->Menus->CheckItem("IPU") == MF_CHECKED)
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for IPU\n";
		_HPS2X64._SYSTEM._IPU.DebugWindow_Disable();
		ProgramWindow->Menus->UnCheckItem("IPU");
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for IPU\n";
		_HPS2X64._SYSTEM._IPU.DebugWindow_Enable();
	}
#endif

	_MenuWasClicked = 1;
}


void hps2x64::OnClick_Debug_Show_PS2_GPU(int i)
{
	cout << "\nYou clicked Debug | Show PS2 | GPU\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|gpu";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._GPU.DebugWindow_Disable2();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._GPU.DebugWindow_Enable2();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if (ProgramWindow->Menus->CheckItem("PS2 GPU") == MF_CHECKED)
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for PS2 GPU\n";
		_HPS2X64._SYSTEM._GPU.DebugWindow_Disable2();
		ProgramWindow->Menus->UnCheckItem("PS2 GPU");
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for PS2 GPU\n";
		_HPS2X64._SYSTEM._GPU.DebugWindow_Enable2();
	}
#endif

	_MenuWasClicked = 1;
}



void hps2x64::OnClick_Debug_Show_PS2_INTC ( int i )
{
	cout << "\nYou clicked Debug | Show PS2 | PS2 INTC\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps2|intc";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._INTC.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._INTC.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "PS2 INTC" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for PS2 INTC\n";
		_HPS2X64._SYSTEM._INTC.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "PS2 INTC" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for PS2 INTC\n";
		_HPS2X64._SYSTEM._INTC.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}





void hps2x64::OnClick_Debug_Show_All ( int i )
{
	//MenuClicked m;
	//m.Debug_ShowWindow_All = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Debug | Show Window | All\n";
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_FrameBuffer ( int i )
{
	//MenuClicked m;
	//m.Debug_ShowWindow_FrameBuffer = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_R3000A ( int i )
{

	cout << "\nYou clicked Debug | Show Window | R3000A\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|r3000a";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._CPU.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._CPU.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "R3000A" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for R3000A\n";
		_HPS2X64._SYSTEM._PS1SYSTEM._CPU.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "R3000A" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for R3000A\n";
		_HPS2X64._SYSTEM._PS1SYSTEM._CPU.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_Memory ( int i )
{
	cout << "\nYou clicked Debug | Show Window | Memory\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|memory";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._BUS.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._BUS.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "Memory" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for Bus\n";
		_HPS2X64._SYSTEM._PS1SYSTEM._BUS.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "Memory" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for Bus\n";
		_HPS2X64._SYSTEM._PS1SYSTEM._BUS.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_DMA ( int i )
{
	cout << "\nYou clicked Debug | Show Window | DMA\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|dma";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._DMA.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._DMA.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "DMA" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS2X64._SYSTEM._PS1SYSTEM._DMA.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "DMA" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS2X64._SYSTEM._PS1SYSTEM._DMA.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_TIMER ( int i )
{
	cout << "\nYou clicked Debug | Show Window | Timers\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|timers";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._TIMERS.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._TIMERS.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "Timers" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS2X64._SYSTEM._PS1SYSTEM._TIMERS.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "Timers" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS2X64._SYSTEM._PS1SYSTEM._TIMERS.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_SPU ( int i )
{
	cout << "\nYou clicked Debug | Show Window | SPU\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|spu";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "SPU" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "SPU" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_CD ( int i )
{
	cout << "\nYou clicked Debug | Show Window | CD\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|cd";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "CD" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "CD" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS2X64._SYSTEM._PS1SYSTEM._CD.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_INTC ( int i )
{
	cout << "\nYou clicked Debug | Show Window | INTC\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|intc";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._INTC.DebugWindow_Disable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._INTC.DebugWindow_Enable();
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "INTC" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS2X64._SYSTEM._PS1SYSTEM._INTC.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "INTC" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS2X64._SYSTEM._PS1SYSTEM._INTC.DebugWindow_Enable ();
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_SPU0 ( int i )
{
	cout << "\nYou clicked Debug | Show Window | SPU0\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|spu0";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.SPU0.DebugWindow_Disable(0);
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.SPU0.DebugWindow_Enable(0);
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "SPU0" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.SPU0.DebugWindow_Disable ( 0 );
		ProgramWindow->Menus->UnCheckItem ( "SPU0" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.SPU0.DebugWindow_Enable ( 0 );
	}
#endif
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Debug_Show_SPU1 ( int i )
{
	cout << "\nYou clicked Debug | Show Window | SPU1\n";

#ifdef USE_NEW_PROGRAM_WINDOW
	static constexpr char* CURRENT_MENU_ITEM = "debugps1|spu1";
	if (m_prgwindow->GetMenuItemChecked(CURRENT_MENU_ITEM))
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.SPU1.DebugWindow_Disable(1);
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, false);
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.SPU1.DebugWindow_Enable(1);
		m_prgwindow->SetMenuItemChecked(CURRENT_MENU_ITEM, true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "SPU1" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.SPU1.DebugWindow_Disable ( 1 );
		ProgramWindow->Menus->UnCheckItem ( "SPU1" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.SPU1.DebugWindow_Enable ( 1 );
	}
#endif
	
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_Debug_Console_PS1_DMA_READ(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps1";
	const std::string DEVICE_NAME = "|dma";
	const std::string DEVICE_ACTION = "|read";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation1::Dma::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Dma::bEnableVerboseDebugRead = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS1_DMA_WRITE(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps1";
	const std::string DEVICE_NAME = "|dma";
	const std::string DEVICE_ACTION = "|write";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation1::Dma::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Dma::bEnableVerboseDebugRead = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS1_INTC_READ(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps1";
	const std::string DEVICE_NAME = "|intc";
	const std::string DEVICE_ACTION = "|read";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation1::Intc::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Intc::bEnableVerboseDebugRead = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS1_INTC_WRITE(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps1";
	const std::string DEVICE_NAME = "|intc";
	const std::string DEVICE_ACTION = "|write";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation1::Intc::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Intc::bEnableVerboseDebugRead = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS1_TIMER_READ(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps1";
	const std::string DEVICE_NAME = "|timer";
	const std::string DEVICE_ACTION = "|read";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation1::Timers::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Timers::bEnableVerboseDebugRead = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS1_TIMER_WRITE(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps1";
	const std::string DEVICE_NAME = "|timer";
	const std::string DEVICE_ACTION = "|write";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation1::Timers::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Timers::bEnableVerboseDebugRead = true;
	}
}


void hps2x64::OnClick_Debug_Console_PS2_DMA_READ(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps2";
	const std::string DEVICE_NAME = "|dma";
	const std::string DEVICE_ACTION = "|read";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation2::Dma::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation2::Dma::bEnableVerboseDebugRead = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS2_DMA_WRITE(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps2";
	const std::string DEVICE_NAME = "|dma";
	const std::string DEVICE_ACTION = "|write";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation2::Dma::bEnableVerboseDebugWrite = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation2::Dma::bEnableVerboseDebugWrite = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS2_INTC_READ(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps2";
	const std::string DEVICE_NAME = "|intc";
	const std::string DEVICE_ACTION = "|read";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation2::Intc::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation2::Intc::bEnableVerboseDebugRead = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS2_INTC_WRITE(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps2";
	const std::string DEVICE_NAME = "|intc";
	const std::string DEVICE_ACTION = "|write";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation2::Intc::bEnableVerboseDebugWrite = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation2::Intc::bEnableVerboseDebugWrite = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS2_TIMER_READ(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps2";
	const std::string DEVICE_NAME = "|timer";
	const std::string DEVICE_ACTION = "|read";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation2::Timers::bEnableVerboseDebugRead = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation2::Timers::bEnableVerboseDebugRead = true;
	}
}

void hps2x64::OnClick_Debug_Console_PS2_TIMER_WRITE(int i)
{
	const std::string DEVICE_PREFIX = "debug|console";
	const std::string DEVICE_SYSTEM = "|ps2";
	const std::string DEVICE_NAME = "|timer";
	const std::string DEVICE_ACTION = "|write";
	const std::string CONTROL_NAME = DEVICE_PREFIX + DEVICE_SYSTEM + DEVICE_NAME + DEVICE_ACTION;

	//std::cout << "concatenated string:" << CONTROL_NAME;

	if (m_prgwindow->GetMenuItemChecked(CONTROL_NAME))
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, false);
		Playstation2::Timers::bEnableVerboseDebugWrite = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation2::Timers::bEnableVerboseDebugWrite = true;
	}
}


void hps2x64::OnClick_Debug_SaveConsole(int i)
{
	CaptureConsoleToFile("console_output.txt");
}


void hps2x64::LoadGamepadConfig(int deviceIndex)
{

	m_gamepadConfig.buttonMappings["X"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_X[deviceIndex];
	m_gamepadConfig.buttonMappings["Circle"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_O[deviceIndex];
	m_gamepadConfig.buttonMappings["Triangle"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Triangle[deviceIndex];
	m_gamepadConfig.buttonMappings["Square"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Square[deviceIndex];
	m_gamepadConfig.buttonMappings["L1"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L1[deviceIndex];  // Left shoulder
	m_gamepadConfig.buttonMappings["R1"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R1[deviceIndex];  // Right shoulder
	m_gamepadConfig.buttonMappings["Select"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Select[deviceIndex];
	m_gamepadConfig.buttonMappings["Start"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Start[deviceIndex];
	m_gamepadConfig.buttonMappings["L3"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L3[deviceIndex];  // Left stick click
	m_gamepadConfig.buttonMappings["R3"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R3[deviceIndex];  // Right stick click
	m_gamepadConfig.buttonMappings["L2"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L2[deviceIndex]; // Left trigger (as button)
	m_gamepadConfig.buttonMappings["R2"] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R2[deviceIndex]; // Right trigger (as button)

	// Set up axis mappings
	m_gamepadConfig.leftStickXAxis = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X[deviceIndex];
	m_gamepadConfig.leftStickYAxis = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y[deviceIndex];
	m_gamepadConfig.rightStickXAxis = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_X[deviceIndex];
	m_gamepadConfig.rightStickYAxis = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y[deviceIndex];
	m_gamepadConfig.leftTriggerAxis = 2;
	m_gamepadConfig.rightTriggerAxis = 5;

	// Default to device 0
	m_gamepadConfig.deviceIndex = deviceIndex;
	m_gamepadConfig.deviceName = "";

}

void hps2x64::ApplyGamepadConfig(int deviceIndex)
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_X[deviceIndex] = m_gamepadConfig.buttonMappings["X"];
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_O[deviceIndex] = m_gamepadConfig.buttonMappings["Circle"];
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Triangle[deviceIndex] = m_gamepadConfig.buttonMappings["Triangle"];
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Square[deviceIndex] = m_gamepadConfig.buttonMappings["Square"];
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L1[deviceIndex] = m_gamepadConfig.buttonMappings["L1"];  // Left shoulder
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R1[deviceIndex] = m_gamepadConfig.buttonMappings["R1"];  // Right shoulder
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Select[deviceIndex] = m_gamepadConfig.buttonMappings["Select"];
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Start[deviceIndex] = m_gamepadConfig.buttonMappings["Start"];
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L3[deviceIndex] = m_gamepadConfig.buttonMappings["L3"];  // Left stick click
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R3[deviceIndex] = m_gamepadConfig.buttonMappings["R3"];  // Right stick click
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L2[deviceIndex] = m_gamepadConfig.buttonMappings["L2"]; // Left trigger (as button)
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R2[deviceIndex] = m_gamepadConfig.buttonMappings["R2"]; // Right trigger (as button)

	// Set up axis mappings
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X[deviceIndex] = m_gamepadConfig.leftStickXAxis;
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y[deviceIndex] = m_gamepadConfig.leftStickYAxis;
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_X[deviceIndex] = m_gamepadConfig.rightStickXAxis;
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y[deviceIndex] = m_gamepadConfig.rightStickYAxis;
	m_gamepadConfig.leftTriggerAxis = 2;
	m_gamepadConfig.rightTriggerAxis = 5;

	// Default to device 0
	m_gamepadConfig.deviceIndex = deviceIndex;
	m_gamepadConfig.deviceName = "";

}


void hps2x64::OnClick_Controllers0_Configure ( int i )
{
	//MenuClicked m;
	//m.Controllers_Configure = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Controllers | Configure...\n";
	
#ifdef USE_OLD_PAD_CONFIG

	Dialog_KeyConfigure::KeyConfigure [ 0 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_X [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 1 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_O [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 2 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Triangle [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 3 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Square [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 4 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R1 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 5 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R2 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 6 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R3 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 7 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L1 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 8 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L2 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 9 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L3 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 10 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Start [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 11 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Select [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 12 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 13 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 14 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_X [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 15 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y [ 0 ];
#else

	LoadGamepadConfig(0);

#endif

	// first make sure that there are joysticks connected
	Playstation1::SIO::gamepad.RefreshDevices();

	// make sure there is a game controller connected before showing dialog
	if (Playstation1::SIO::gamepad.GetDeviceCount())
	{

#ifdef USE_OLD_PAD_CONFIG
		if (Dialog_KeyConfigure::Show_ConfigureKeysDialog(0))
		{
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_X[0] = Dialog_KeyConfigure::KeyConfigure[0];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_O[0] = Dialog_KeyConfigure::KeyConfigure[1];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Triangle[0] = Dialog_KeyConfigure::KeyConfigure[2];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Square[0] = Dialog_KeyConfigure::KeyConfigure[3];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R1[0] = Dialog_KeyConfigure::KeyConfigure[4];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R2[0] = Dialog_KeyConfigure::KeyConfigure[5];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R3[0] = Dialog_KeyConfigure::KeyConfigure[6];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L1[0] = Dialog_KeyConfigure::KeyConfigure[7];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L2[0] = Dialog_KeyConfigure::KeyConfigure[8];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L3[0] = Dialog_KeyConfigure::KeyConfigure[9];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Start[0] = Dialog_KeyConfigure::KeyConfigure[10];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Select[0] = Dialog_KeyConfigure::KeyConfigure[11];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X[0] = Dialog_KeyConfigure::KeyConfigure[12];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y[0] = Dialog_KeyConfigure::KeyConfigure[13];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_X[0] = Dialog_KeyConfigure::KeyConfigure[14];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y[0] = Dialog_KeyConfigure::KeyConfigure[15];
		}
#else

		// disable the program window
		m_prgwindow->SetEnabled(false);

		std::vector<std::string> deviceNames = {
			"Player 1 Controller",
			"Player 2 Controller",
			"Player 3 Controller",
			"Player 4 Controller"
		};

		// Create configuration window using our existing GamepadHandler
		GamepadConfigWindow configWindow(
			"Configure Gamepad",
			m_gamepadConfig,
			[](const GamepadConfigWindow::GamepadConfig& config, bool accepted) {
				std::cout << "Callback received: accepted = " << (accepted ? "TRUE" : "FALSE") << std::endl;
				if (accepted) {
					//std::cout << "Configuration was ACCEPTED" << std::endl;
					//OnGamepadConfigChanged(config);
					m_gamepadConfig = config;
					ApplyGamepadConfig(0);
				}
				else {
					std::cout << "Configuration was CANCELLED" << std::endl;
				}
			},
			Playstation1::SIO::gamepad, // Pass our existing gamepad handler
			0,      // Set which device to configure
			deviceNames       // Custom device names
		);

		// Show the dialog modally
		bool result = configWindow.ShowModal();

		// re-enable the program window
		m_prgwindow->SetEnabled(true);
#endif
	}
	else
	{
		cout << "\n*** hps2x64: *ERROR* no game controller detected!!! ***\n";
	}
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Controllers1_Configure ( int i )
{
	//MenuClicked m;
	//m.Controllers_Configure = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Controllers | Configure...\n";
	
#ifdef USE_OLD_PAD_CONFIG
	Dialog_KeyConfigure::KeyConfigure [ 0 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_X [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 1 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_O [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 2 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Triangle [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 3 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Square [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 4 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R1 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 5 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R2 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 6 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R3 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 7 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L1 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 8 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L2 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 9 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L3 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 10 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Start [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 11 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Select [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 12 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 13 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 14 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_X [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 15 ] = _HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y [ 1 ];
#else

	LoadGamepadConfig(1);
#endif

	// first make sure that there are joysticks connected
	Playstation1::SIO::gamepad.RefreshDevices();

	// make sure there is a game controller connected before showing dialog
	if (Playstation1::SIO::gamepad.GetDeviceCount())
	{
#ifdef USE_OLD_PAD_CONFIG

		if (Dialog_KeyConfigure::Show_ConfigureKeysDialog(1))
		{
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_X[1] = Dialog_KeyConfigure::KeyConfigure[0];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_O[1] = Dialog_KeyConfigure::KeyConfigure[1];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Triangle[1] = Dialog_KeyConfigure::KeyConfigure[2];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Square[1] = Dialog_KeyConfigure::KeyConfigure[3];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R1[1] = Dialog_KeyConfigure::KeyConfigure[4];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R2[1] = Dialog_KeyConfigure::KeyConfigure[5];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_R3[1] = Dialog_KeyConfigure::KeyConfigure[6];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L1[1] = Dialog_KeyConfigure::KeyConfigure[7];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L2[1] = Dialog_KeyConfigure::KeyConfigure[8];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_L3[1] = Dialog_KeyConfigure::KeyConfigure[9];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Start[1] = Dialog_KeyConfigure::KeyConfigure[10];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.Key_Select[1] = Dialog_KeyConfigure::KeyConfigure[11];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X[1] = Dialog_KeyConfigure::KeyConfigure[12];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y[1] = Dialog_KeyConfigure::KeyConfigure[13];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_X[1] = Dialog_KeyConfigure::KeyConfigure[14];
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y[1] = Dialog_KeyConfigure::KeyConfigure[15];
		}
#else

		// disable the program window
		m_prgwindow->SetEnabled(false);

		std::vector<std::string> deviceNames = {
			"Player 1 Controller",
			"Player 2 Controller",
			"Player 3 Controller",
			"Player 4 Controller"
		};

		// Create configuration window using our existing GamepadHandler
		GamepadConfigWindow configWindow(
			"Configure Gamepad",
			m_gamepadConfig,
			[](const GamepadConfigWindow::GamepadConfig& config, bool accepted) {
				std::cout << "Callback received: accepted = " << (accepted ? "TRUE" : "FALSE") << std::endl;
				if (accepted) {
					//std::cout << "Configuration was ACCEPTED" << std::endl;
					//OnGamepadConfigChanged(config);
					m_gamepadConfig = config;
					ApplyGamepadConfig(1);
				}
				else {
					std::cout << "Configuration was CANCELLED" << std::endl;
				}
			},
			Playstation1::SIO::gamepad, // Pass our existing gamepad handler
			1,      // Set which device to configure
			deviceNames       // Custom device names
		);

		// Show the dialog modally
		bool result = configWindow.ShowModal();

		// re-enable the program window
		m_prgwindow->SetEnabled(true);
#endif
	}
	else
	{
		cout << "\n*** hps2x64: *ERROR* no game controller detected!!! ***\n";
	}
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad1Type_Digital ( int i )
{
	//MenuClicked m;
	//m.Pad1Type_Digital = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	// set pad 1 to digital
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 0 ] = 0;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad1Type_Analog ( int i )
{
	//MenuClicked m;
	//m.Pad1Type_Analog = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	// set pad 1 to analog
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 0 ] = 1;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad1Type_DualShock2 ( int i )
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 0 ] = Playstation1::SIO::PADTYPE_DUALSHOCK2;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad1Input_None ( int i )
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 0 ] = -1;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad1Input_Device0 ( int i )
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 0 ] = 0;

	if ( _HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 1 ] == 0 )
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 1 ] = -1;
	}

	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad1Input_Device1 ( int i )
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 0 ] = 1;

	if ( _HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 1 ] == 1 )
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 1 ] = -1;
	}

	_MenuWasClicked = 1;
}




void hps2x64::OnClick_Pad2Type_Digital ( int i )
{
	//MenuClicked m;
	//m.Pad2Type_Digital = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	// set pad 2 to digital
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 1 ] = 0;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad2Type_Analog ( int i )
{
	//MenuClicked m;
	//m.Pad2Type_Analog = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	// set pad 2 to analog
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 1 ] = 1;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad2Type_DualShock2 ( int i )
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 1 ] = Playstation1::SIO::PADTYPE_DUALSHOCK2;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad2Input_None ( int i )
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 1 ] = -1;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad2Input_Device0 ( int i )
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 1 ] = 0;

	if ( _HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 0 ] == 0 )
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 0 ] = -1;
	}

	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Pad2Input_Device1 ( int i )
{
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 1 ] = 1;

	if ( _HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 0 ] == 1 )
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ 0 ] = -1;
	}

	_MenuWasClicked = 1;
}



void hps2x64::OnClick_Card1_Connect ( int i )
{
	//MenuClicked m;
	//m.MemoryCard1_Connected = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	// set memory card 1 to connected
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ] = 0;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Card1_Disconnect ( int i )
{
	//MenuClicked m;
	//m.MemoryCard1_Disconnected = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	// set memory card 1 to disconnected
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ] = 1;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Card2_Connect ( int i )
{
	//MenuClicked m;
	//m.MemoryCard2_Connected = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	// set memory card 2 to connected
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ] = 0;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Card2_Disconnect ( int i )
{
	//MenuClicked m;
	//m.MemoryCard2_Disconnected = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	// set memory card 2 to disconnected
	_HPS2X64._SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ] = 1;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Redetect_Pads ( int i )
{
	Playstation1::SIO::gamepad.RefreshDevices();
	cout << "\nfound # of devices: " << dec << Playstation1::SIO::gamepad.GetDeviceCount();
	
	//_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuWasClicked = 1;
}





void hps2x64::OnClick_Region_Europe ( int i )
{
	//MenuClicked m;
	//m.Region_Europe = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.Region = 'E';
	
	_MenuWasClicked = 1;
	_HPS2X64.Update_CheckMarksOnMenu();
}

void hps2x64::OnClick_Region_Japan ( int i )
{
	//MenuClicked m;
	//m.Region_Japan = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.Region = 'J';
	
	_MenuWasClicked = 1;
	_HPS2X64.Update_CheckMarksOnMenu();
}

void hps2x64::OnClick_Region_NorthAmerica ( int i )
{
	//MenuClicked m;
	//m.Region_NorthAmerica = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._CDVD.Region = 'A';
	
	_MenuWasClicked = 1;
	_HPS2X64.Update_CheckMarksOnMenu();
}

/*
void hps2x64::OnClick_Region_H ( int i )
{
	MenuClicked m;
	m.Region_H = true;
	x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
}

void hps2x64::OnClick_Region_R ( int i )
{
	MenuClicked m;
	m.Region_R = true;
	x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
}

void hps2x64::OnClick_Region_C ( int i )
{
	MenuClicked m;
	m.Region_C = true;
	x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
}

void hps2x64::OnClick_Region_Korea ( int i )
{
	MenuClicked m;
	m.Region_Korea = true;
	x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
}
*/









void hps2x64::OnClick_Audio_Enable ( int i )
{
	//MenuClicked m;
	//m.Audio_Enable = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	if ( _HPS2X64._SYSTEM._PS1SYSTEM._SPU2.AudioOutput_Enabled )
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.AudioOutput_Enabled = false;
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.AudioOutput_Enabled = true;
	}
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Volume_100 ( int i )
{
	//MenuClicked m;
	//m.Audio_Volume_100 = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.GlobalVolume = 0x7fff;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Volume_75 ( int i )
{
	//MenuClicked m;
	//m.Audio_Volume_75 = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.GlobalVolume = 0x3000;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Volume_50 ( int i )
{
	//MenuClicked m;
	//m.Audio_Volume_50 = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.GlobalVolume = 0x1000;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Volume_25 ( int i )
{
	//MenuClicked m;
	//m.Audio_Volume_25 = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.GlobalVolume = 0x400;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Buffer_8k ( int i )
{
	//MenuClicked m;
	//m.Audio_Buffer_8k = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.NextPlayBuffer_Size = 8192;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Buffer_16k ( int i )
{
	//MenuClicked m;
	//m.Audio_Buffer_16k = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.NextPlayBuffer_Size = 16384;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Buffer_32k ( int i )
{
	//MenuClicked m;
	//m.Audio_Buffer_32k = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.NextPlayBuffer_Size = 32768;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Buffer_64k ( int i )
{
	//MenuClicked m;
	//m.Audio_Buffer_64k = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.NextPlayBuffer_Size = 65536;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Buffer_1m ( int i )
{
	//MenuClicked m;
	//m.Audio_Buffer_1m = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.NextPlayBuffer_Size = 131072;
	
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Audio_Filter ( int i )
{
	//MenuClicked m;
	//m.Audio_Filter = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Audio | Filter\n";
	
	if ( _HPS2X64._SYSTEM._PS1SYSTEM._SPU2.AudioFilter_Enabled )
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.AudioFilter_Enabled = false;
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.AudioFilter_Enabled = true;
	}
	
	_MenuWasClicked = 1;
}
void hps2x64::OnClick_Audio_MultiThread ( int i )
{
	//MenuClicked m;
	//m.Audio_Filter = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Audio | Filter\n";
	
	if ( _HPS2X64._SYSTEM._PS1SYSTEM._SPU2.ulNumThreads )
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.ulNumThreads = 0;
	}
	else
	{
		_HPS2X64._SYSTEM._PS1SYSTEM._SPU2.ulNumThreads = 1;
	}
	
	_MenuWasClicked = 1;
}



void hps2x64::OnClick_Video_Renderer_Software_OpenGL(int i)
{
	cout << "\nYou clicked Video | Renderer | Software | OpenGL\n";

	/*
	// if vulkan is loaded, then can't switch back to OpenGL yet
	if (vulkan_is_loaded())
	{
		if (!CreateOpenGLWindow())
		{
			cout << "\nhpsx64: ALERT: Problem switching to an OpenGL window.";
			return;
		}

		// alert
		//cout << "hpsx64: ALERT: Switching from Vulkan->OpenGL is not suppored. Can only switch from OpenGL->Vulkan or Vulkan->Vulkan.";

		// don't do anything
		//return;
	}

	// if previously on hardware renderer, then copy framebuffer
	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// copy frame buffer from hardware to software if hardware rendering is allowed
		if (_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			// copy VRAM from gpu hardware to software VRAM
			_HPS2X64._SYSTEM._GPU.Copy_VRAM_toCPU();
			_HPS2X64._SYSTEM._GPU.Copy_CLUT_toCPU();
			_HPS2X64._SYSTEM._GPU.Copy_VARS_toCPU();

			// unload vulkan when not using it
			_HPS2X64._SYSTEM._GPU.UnloadVulkan();

			//ProgramWindow->KillGLWindow();
			//ProgramWindow->CreateGLWindow(ProgramWindow_Caption, ProgramWindow_Width, ProgramWindow_Height, true, false);

			cout << "\nHPSX64: INFO: *** SWITCHED TO OPENGL SOFTWARE RENDERER ***\n";
		}
	}
	*/

	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
	_HPS2X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation2::GPU::DRAW_LIBRARY_OPENGL;

	_MenuWasClicked = 1;

	_HPS2X64.Update_CheckMarksOnMenu();
}


void hps2x64::OnClick_Video_Renderer_Software_Vulkan(int i)
{
	cout << "\nYou clicked Video | Renderer | Software | Vulkan\n";

	/*
	// if vulkan is not loaded, then load it
	if (!vulkan_is_loaded())
	{
		cout << "\nhpsx64: Vulkan is not loaded.";

		if (!CreateVulkanWindow())
		{
			cout << "\nhpsx64: ALERT: Problem switching to an VULKAN window.";
			cout << "\nhpsx64: ERROR: Problem loading VULKAN.";

			// reverting back to opengl window
			iWindowType = WINDOW_TYPE_VULKAN;
			CreateOpenGLWindow();

			cout << "hpsx64: ALERT: Reverted back to OPENGL window.";

			return;
		}
	}

	cout << "\nhpsx64: Vulkan is loaded now.";

	// check if the commands for running gfx on hardware and displaying to the screen are recorded
	if (!vulkan_is_draw_only_recorded())
	{
		cout << "\nhpsx64: Commands to display to screen are not recorded.";

		// might need to re-record for each frame?
		//vulkan_set_screen_size(ProgramWindow->WindowWidth, ProgramWindow->WindowHeight);
		//vulkan_set_buffer_size(640, 480);
		vulkan_set_screen_size(1, 1);
		vulkan_set_buffer_size(1, 1);
		if (!vulkan_record_draw_only_commands())
		{
			cout << "\nhpsx64: ERROR: Problem recording commands.";
			return;
		}
	}

	cout << "\nhpsx64: Commands to display to screen are recorded now.";

	// if previously on hardware renderer, then copy framebuffer
	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// copy frame buffer from hardware to software if hardware rendering is allowed
		if (_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			// copy VRAM from gpu hardware to software VRAM
			_HPS2X64._SYSTEM._GPU.Copy_VRAM_toCPU();
			_HPS2X64._SYSTEM._GPU.Copy_CLUT_toCPU();
			_HPS2X64._SYSTEM._GPU.Copy_VARS_toCPU();

			// unload vulkan when not using it
			//_HPS2X64._SYSTEM._GPU.UnloadVulkan();

			cout << "\nHPSX64: INFO: *** SWITCHED TO VULKAN SOFTWARE RENDERER ***\n";
		}
	}


	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
	_HPS2X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation2::GPU::DRAW_LIBRARY_VULKAN;
	*/

	_MenuWasClicked = 1;

	_HPS2X64.Update_CheckMarksOnMenu();
}


void hps2x64::OnClick_Video_Renderer_Hardware_Vulkan(int ShaderThreads)
{
	cout << "\nYou clicked Video | Renderer | Hardware | Vulkan\n";

	/*
	// if vulkan is not loaded, then load it
	if (!vulkan_is_loaded())
	{
		cout << "\nhpsx64: Vulkan is not loaded.";

		if (!CreateVulkanWindow())
		{
			cout << "\nhpsx64: ALERT: Problem switching to an VULKAN window.";
			cout << "\nhpsx64: ERROR: Problem loading VULKAN.";

			// reverting back to opengl window
			iWindowType = WINDOW_TYPE_VULKAN;
			CreateOpenGLWindow();

			cout << "hpsx64: ALERT: Reverted back to OPENGL window.";

			return;
		}
	}

	cout << "\nhpsx64: Vulkan is loaded now.";


	// note: should be ok to allow multiple attempts at setting up vulkan
	//if (_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
	{
		// if previously rendering on cpu, copy vram to gpu hardware
		if (!_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
		{
			// get vulkan ready for use
			if (_HPS2X64._SYSTEM._GPU.LoadVulkan())
			{
				// vulkan loaded successfully
				cout << "\nHPSX64: INFO: *** VULKAN LOADED ***\n";

				_HPS2X64._SYSTEM._GPU.Copy_VRAM_toGPU();
				_HPS2X64._SYSTEM._GPU.Copy_CLUT_toGPU();
				_HPS2X64._SYSTEM._GPU.Copy_VARS_toGPU();

			}
			else
			{
				cout << "\nHPSX64: ERROR: problem loading vulkan\n";
			}
		}

	}


	// check if the commands for running gfx on hardware and displaying to the screen are recorded
	//if (!vulkan_is_compute_draw_recorded())
	// check if the number of threads has changed
	//if (vulkan_get_gpu_threads() != ShaderThreads)
	{
		cout << "\nhpsx64: Commands to compute graphics and display them are not recorded.";
		cout << "\nhpsx64: Number of threads has changed. Re-recording compute graphics.";

		// set number of gpu threads
		vulkan_set_gpu_threads(ShaderThreads);

		if (!vulkan_record_default_commands())
		{
			cout << "\nhpsx64: ERROR: Problem recording commands.";
			return;
		}

	}

	cout << "\nhpsx64: Commands to compute graphics and display them are recorded now.";


	// if there was a problem loading vulkan then it should do this
	if (!_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
	{
		cout << "\nhps2x64: ERROR: Unable to use hardware renderer. VULKAN not setup properly. Using software renderer.\n";


		_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
	}


	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = true;
	_HPS2X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation2::GPU::DRAW_LIBRARY_VULKAN;
	*/

	_MenuWasClicked = 1;

	_HPS2X64.Update_CheckMarksOnMenu();
}


void hps2x64::OnClick_Video_Renderer_Hardware1_Vulkan(int i)
{
	OnClick_Video_Renderer_Hardware_Vulkan(1);
}

void hps2x64::OnClick_Video_Renderer_Hardware2_Vulkan(int i)
{
	OnClick_Video_Renderer_Hardware_Vulkan(2);
}

void hps2x64::OnClick_Video_Renderer_Hardware4_Vulkan(int i)
{
	OnClick_Video_Renderer_Hardware_Vulkan(4);
}

void hps2x64::OnClick_Video_Renderer_Hardware8_Vulkan(int i)
{
	OnClick_Video_Renderer_Hardware_Vulkan(8);
}



void hps2x64::OnClick_Video_FullScreen ( int i )
{
	//MenuClicked m;
	//m.Video_FullScreen = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Video | FullScreen\n";

	Playstation2::GPU::MainProgramWindow_Width = (long) ( ((float)ProgramWindow_Width) * 1.0f );
	Playstation2::GPU::MainProgramWindow_Height = (long) ( ((float)ProgramWindow_Height) * 1.0f );
	
#ifdef USE_NEW_PROGRAM_WINDOW

	if (!m_prgwindow->IsFullscreen())
	{
		m_prgwindow->SetClientSize(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);

		// update screen size in vulkan
		//vulkan_set_screen_size(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);
	}

	m_prgwindow->ToggleFullscreen();
#else
	if( ! ProgramWindow->fullscreen )
	{
		ProgramWindow->SetWindowSize ( Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height );

		// update screen size in vulkan
		vulkan_set_screen_size(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);

		if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
		{
			//if (!vulkan_create_swap_chain())
			//{
			//	std::cout << "\nHPS2: ERROR: Problem creating swap chain.\n";
			//	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
			//	_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			//	_HPS2X64._SYSTEM._GPU.UnloadVulkan();
			//}
			if (!vulkan_record_default_commands())
			{
				std::cout << "\nHPS2: ERROR: Problem recording commands for new swap chain.\n";
				_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
				_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
				_HPS2X64._SYSTEM._GPU.UnloadVulkan();
			}
		}
	}
	
	ProgramWindow->ToggleGLFullScreen ();
#endif
	
	_HPS2X64.Update_CheckMarksOnMenu();
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_Video_EnableVsync ( int i )
{
	//MenuClicked m;
	//m.Video_FullScreen = true;
	//x64ThreadSafe::Utilities::Lock_OR64 ( (long long&)_MenuClick.Value, (long long) m.Value );
	cout << "\nYou clicked Video | Enable Vsync\n";

#ifdef USE_NEW_PROGRAM_WINDOW

	if (_HPS2X64._SYSTEM.bEnableVsync)
	{
		_HPS2X64._SYSTEM.bEnableVsync = 0;
		//ProgramWindow->DisableVSync();
		m_prgwindow->SetVSync(false);
	}
	else
	{
		_HPS2X64._SYSTEM.bEnableVsync = 1;
		//ProgramWindow->EnableVSync();
		m_prgwindow->SetVSync(true);
	}
#else
	if ( _HPS2X64._SYSTEM.bEnableVsync )
	{
		_HPS2X64._SYSTEM.bEnableVsync = 0;
		ProgramWindow->DisableVSync ();
	}
	else
	{
		_HPS2X64._SYSTEM.bEnableVsync = 1;
		ProgramWindow->EnableVSync ();
	}
#endif
	
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_Video_WindowSizeX1 ( int i )
{
	Playstation2::GPU::MainProgramWindow_Width = (long) ( ((float)ProgramWindow_Width) * 1.0f );
	Playstation2::GPU::MainProgramWindow_Height = (long) ( ((float)ProgramWindow_Height) * 1.0f );

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetClientSize(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);

	// update screen size in vulkan
	//vulkan_set_screen_size(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);
#else

	ProgramWindow->SetWindowSize ( Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height );

	// update screen size in vulkan
	vulkan_set_screen_size(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);

	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		//if (!vulkan_create_swap_chain())
		//{
		//	std::cout << "\nHPS2: ERROR: Problem creating swap chain.\n";
		//	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
		//	_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
		//	_HPS2X64._SYSTEM._GPU.UnloadVulkan();
		//}
		if (!vulkan_record_default_commands())
		{
			std::cout << "\nHPS2: ERROR: Problem recording commands for new swap chain.\n";
			_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS2X64._SYSTEM._GPU.UnloadVulkan();
		}
	}
#endif

	_HPS2X64.Update_CheckMarksOnMenu();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Video_WindowSizeX15 ( int i )
{
	Playstation2::GPU::MainProgramWindow_Width = (long) ( ((float)ProgramWindow_Width) * 1.5f );
	Playstation2::GPU::MainProgramWindow_Height = (long) ( ((float)ProgramWindow_Height) * 1.5f );

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetClientSize(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);

	// update screen size in vulkan
	//vulkan_set_screen_size(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);
#else

	ProgramWindow->SetWindowSize ( Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height );

	// update screen size in vulkan
	vulkan_set_screen_size(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);

	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		//if (!vulkan_create_swap_chain())
		//{
		//	std::cout << "\nHPS2: ERROR: Problem creating swap chain.\n";
		//	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
		//	_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
		//	_HPS2X64._SYSTEM._GPU.UnloadVulkan();
		//}
		if (!vulkan_record_default_commands())
		{
			std::cout << "\nHPS2: ERROR: Problem recording commands for new swap chain.\n";
			_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS2X64._SYSTEM._GPU.UnloadVulkan();
		}
	}

#endif

	_HPS2X64.Update_CheckMarksOnMenu();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_Video_WindowSizeX2 ( int i )
{
	Playstation2::GPU::MainProgramWindow_Width = (long) ( ((float)ProgramWindow_Width) * 2.0f );
	Playstation2::GPU::MainProgramWindow_Height = (long) ( ((float)ProgramWindow_Height) * 2.0f );

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetClientSize(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);

	// update screen size in vulkan
	//vulkan_set_screen_size(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);
#else

	ProgramWindow->SetWindowSize ( Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height );

	// update screen size in vulkan
	vulkan_set_screen_size(Playstation2::GPU::MainProgramWindow_Width, Playstation2::GPU::MainProgramWindow_Height);

	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		//if (!vulkan_create_swap_chain())
		//{
		//	std::cout << "\nHPS2: ERROR: Problem creating swap chain.\n";
		//	_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
		//	_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
		//	_HPS2X64._SYSTEM._GPU.UnloadVulkan();
		//}
		if (!vulkan_record_default_commands())
		{
			std::cout << "\nHPS2: ERROR: Problem recording commands for new swap chain.\n";
			_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS2X64._SYSTEM._GPU.UnloadVulkan();
		}
	}

#endif

	_HPS2X64.Update_CheckMarksOnMenu();
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_Video_DisplayMode_Stretch(int i)
{
	m_prgwindow->SetDisplayMode(OpenGLWindow::DisplayMode::STRETCH);
	m_prgwindow->SetMenuItemChecked("displaymode|stretch", true);
	m_prgwindow->SetMenuItemChecked("displaymode|fit", false);
}

void hps2x64::OnClick_Video_DisplayMode_Fit(int i)
{
	m_prgwindow->SetDisplayMode(OpenGLWindow::DisplayMode::FIT);
	m_prgwindow->SetMenuItemChecked("displaymode|stretch", false);
	m_prgwindow->SetMenuItemChecked("displaymode|fit", true);
}


void hps2x64::OnClick_R3000ACPU_Interpreter ( int i )
{
	cout << "\nYou clicked CPU | R3000A | Interpreter\n";

	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.bEnableRecompiler = false;
	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.Status.uDisableRecompiler = true;

	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_R3000ACPU_Recompiler ( int i )
{
	cout << "\nYou clicked CPU | R3000A | Recompiler\n";

	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.bEnableRecompiler = true;
	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.Status.uDisableRecompiler = false;

	// need to reset the recompiler
	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.rs->Reset ();
	
	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.rs->OptimizeLevel = 1;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_R3000ACPU_Recompiler2 ( int i )
{
	cout << "\nYou clicked CPU | R3000A | Recompiler\n";

	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.bEnableRecompiler = true;
	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.Status.uDisableRecompiler = false;

	// need to reset the recompiler
	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.rs->Reset ();
	
	_HPS2X64._SYSTEM._PS1SYSTEM._CPU.rs->OptimizeLevel = 2;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_R5900CPU_Interpreter ( int i )
{
	cout << "\nYou clicked CPU | R5900 | Interpreter\n";

	_HPS2X64._SYSTEM._CPU.bEnableRecompiler = false;
	_HPS2X64._SYSTEM._CPU.Status.uDisableRecompiler = true;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_R5900CPU_Recompiler ( int i )
{
	cout << "\nYou clicked CPU | R5900 | Recompiler\n";

	_HPS2X64._SYSTEM._CPU.bEnableRecompiler = true;
	_HPS2X64._SYSTEM._CPU.Status.uDisableRecompiler = false;

	// need to reset the recompiler
	_HPS2X64._SYSTEM._CPU.rs->Reset ();
	
	_HPS2X64._SYSTEM._CPU.rs->OptimizeLevel = 1;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_R5900CPU_Recompiler2 ( int i )
{
	cout << "\nYou clicked CPU | R5900 | Recompiler2\n";

	_HPS2X64._SYSTEM._CPU.bEnableRecompiler = true;
	_HPS2X64._SYSTEM._CPU.Status.uDisableRecompiler = false;

	// need to reset the recompiler
	_HPS2X64._SYSTEM._CPU.rs->Reset ();
	
	_HPS2X64._SYSTEM._CPU.rs->OptimizeLevel = 2;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}


void hps2x64::OnClick_VU0_Interpreter ( int i )
{
	cout << "\nYou clicked CPU | VU0 | Interpreter\n";

	_HPS2X64._SYSTEM._VU0.VU0.bEnableRecompiler = false;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_VU0_Recompiler ( int i )
{
	cout << "\nYou clicked CPU | VU0 | Recompiler\n";

	_HPS2X64._SYSTEM._VU0.VU0.bEnableRecompiler = true;
	
	// need to reset the recompiler
	//_HPS2X64._SYSTEM._VU0.VU0.vrs[0]->Reset ();
	_HPS2X64._SYSTEM._VU0.VU0.bCodeModified [ 0 ] = 1;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_VU1_Interpreter ( int i )
{
	cout << "\nYou clicked CPU | VU1 | Interpreter\n";

	_HPS2X64._SYSTEM._VU1.VU1.bEnableRecompiler = false;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_VU1_Recompiler ( int i )
{
	cout << "\nYou clicked CPU | VU1 | Recompiler\n";

	_HPS2X64._SYSTEM._VU1.VU1.bEnableRecompiler = true;
	
	// need to reset the recompiler
	//_HPS2X64._SYSTEM._VU1.VU1.vrs[1]->Reset ();
	_HPS2X64._SYSTEM._VU1.VU1.bCodeModified [ 1 ] = 1;
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_VU1_0Threads ( int i )
{
	cout << "\nYou clicked GPU | GPU: Threads | 0 threads\n";
	
	_HPS2X64._SYSTEM._VU1.VU1.ulThreadCount = 0;

	// if gpu is multi-threaded, then gpu data is sent to the thread
	if ( _HPS2X64._SYSTEM._GPU.ulNumberOfThreads )
	{
		// multi-threaded GPU //
		_HPS2X64._SYSTEM._GPU.ulThreadedGPU = 1;
	}
	else
	{
		// single-threaded GPU //
		_HPS2X64._SYSTEM._GPU.ulThreadedGPU = 0;
	}
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_VU1_1Threads ( int i )
{
	cout << "\nYou clicked GPU | GPU: Threads | 1 threads\n";
	
	_HPS2X64._SYSTEM._VU1.VU1.ulThreadCount = 1;
	
	// if vu1 is on another thread, then need to thread the gpu data with it
	_HPS2X64._SYSTEM._GPU.ulThreadedGPU = 1;

	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}



#ifdef ENABLE_R5900_IDLE

void hps2x64::OnClick_Cpu_SkipIdleCycles ( int i )
{
	cout << "\nYou clicked CPU | VU1 | Recompiler\n";

	if ( !_HPS2X64._SYSTEM._CPU.bEnable_SkipIdleCycles )
	{
		_HPS2X64._SYSTEM._CPU.bEnable_SkipIdleCycles = true;
		_HPS2X64._SYSTEM._PS1SYSTEM._CPU.bEnable_SkipIdleCycles = true;
	}
	else
	{
		_HPS2X64._SYSTEM._CPU.bEnable_SkipIdleCycles = false;
		_HPS2X64._SYSTEM._PS1SYSTEM._CPU.bEnable_SkipIdleCycles = false;
	}
	
	
	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

#endif

void hps2x64::OnClick_GPU_0Threads ( int i )
{
	cout << "\nYou clicked GPU | GPU: Threads | 0 threads\n";
	
	_HPS2X64._SYSTEM._GPU.ulNumberOfThreads = 0;
	
	// if vu is multi-threaded, then gpu goes on that thread
	if ( _HPS2X64._SYSTEM._VU1.VU1.ulThreadCount )
	{
		// multi-threaded VU //
		_HPS2X64._SYSTEM._GPU.ulThreadedGPU = 1;
	}
	else
	{
		// single-threaded VU //
		_HPS2X64._SYSTEM._GPU.ulThreadedGPU = 0;
	}

	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}

void hps2x64::OnClick_GPU_1Threads ( int i )
{
	cout << "\nYou clicked GPU | GPU: Threads | 1 threads\n";
	
	_HPS2X64._SYSTEM._GPU.ulNumberOfThreads = 1;

	// if gpu is multi-threaaded, then gpu data gets sent to another thread
	_HPS2X64._SYSTEM._GPU.ulThreadedGPU = 1;


	_HPS2X64.Update_CheckMarksOnMenu ();
	_MenuWasClicked = 1;
}




void hps2x64::StepCycle ()
{
	_SYSTEM.Run ();
	
	// clear the last breakpoint hit
	_SYSTEM._CPU.Breakpoints->Clear_LastBreakPoint ();
}

void hps2x64::StepInstructionPS1 ()
{
	// get the current value of PC to check against for PS1
	u32 CheckPC = _SYSTEM._PS1SYSTEM._CPU.PC;
	
	// run until PC Changes
	while ( CheckPC == _SYSTEM._PS1SYSTEM._CPU.PC )
	{
		_SYSTEM.Run ();
	}
	
	// clear the last breakpoint hit
	_SYSTEM._CPU.Breakpoints->Clear_LastBreakPoint ();
}

void hps2x64::StepInstructionPS2 ()
{
	// get the current value of PC to check against for PS2
	u32 CheckPC = _SYSTEM._CPU.PC;
	
	// run until PC Changes
	while ( CheckPC == _SYSTEM._CPU.PC )
	{
		_SYSTEM.Run ();
	}
	
	// clear the last breakpoint hit
	_SYSTEM._CPU.Breakpoints->Clear_LastBreakPoint ();
}


void hps2x64::SaveState ( string FilePath )
{
#ifdef INLINE_DEBUG
	debug << "\r\nEntered function: System::SaveState";
#endif

	static const char* PathToSaveState = "SaveState.hps1";
	
	// make sure cd is not reading asynchronously??
	_SYSTEM._PS1SYSTEM._CD.cd_image.WaitForAllReadsComplete ();

	// save virtual state for R3000A and BUS
	_SYSTEM._PS1SYSTEM._CPU.SaveVirtualState();
	_SYSTEM._PS1SYSTEM._BUS.SaveVirtualState();
	_SYSTEM._BUS.SaveVirtualState();

#ifdef ALLOW_PS2_HWRENDER
	// if previously on hardware renderer, then copy framebuffer
	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// copy frame buffer from hardware to software if hardware rendering is allowed
		if (_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			// copy VRAM from gpu hardware to software VRAM
			_HPS2X64._SYSTEM._GPU.Copy_VRAM_toCPU();
			_HPS2X64._SYSTEM._GPU.Copy_CLUT_toCPU();
			_HPS2X64._SYSTEM._GPU.Copy_VARS_toCPU();
		}
	}
#endif


	////////////////////////////////////////////////////////
	// We need to prompt for the file to save state to
	if ( !FilePath.compare ( "" ) )
	{
#ifdef USE_NEW_PROGRAM_WINDOW

		FilePath = m_prgwindow->ShowSaveFileDialog("Save PS2 State:");
#else
		FilePath = ProgramWindow->ShowFileSaveDialog ();

#endif
	}

	ofstream OutputFile ( FilePath.c_str (), ios::binary );
	
	u32 SizeOfFile;
	
	cout << "Saving state.\n";
	
	if ( !OutputFile )
	{
#ifdef INLINE_DEBUG
	debug << "->Error creating Save State";
#endif

		cout << "Error creating Save State.\n";
		return;
	}


#ifdef INLINE_DEBUG
	debug << "; Creating Save State";
#endif

	// wait for all reads or writes to disk to finish
	//while ( _SYSTEM._CD.cd_image.isReadInProgress );

	// if gpu is running on hardware device, then copy vram and palette from hardware before save
	if ( _HPS2X64._SYSTEM._GPU.bEnable_OpenCL )
	{
		_HPS2X64._SYSTEM._GPU.Copy_VRAM_toCPU ();
		_HPS2X64._SYSTEM._GPU.Copy_CLUT_toCPU();
		_HPS2X64._SYSTEM._GPU.Copy_VARS_toCPU();
	}


	// write entire state into memory
	//OutputFile.write ( (char*) this, sizeof( System ) );
	OutputFile.write ( (char*) &_SYSTEM, sizeof( System ) );
	
	OutputFile.close();
	
	cout << "Done Saving state.\n";
	
#ifdef INLINE_DEBUG
	debug << "->Leaving function: System::SaveState";
#endif
}

void hps2x64::LoadState ( string FilePath )
{
#ifdef INLINE_DEBUG
	debug << "\r\nEntered function: System::LoadState";
#endif

	static const char* PathToSaveState = "SaveState.hps1";

	// make sure cd is not reading asynchronously??
	//_SYSTEM._CD.cd_image.WaitForAllReadsComplete ();
	
	////////////////////////////////////////////////////////
	// We need to prompt for the file to save state to
	if ( !FilePath.compare( "" ) )
	{

#ifdef USE_NEW_PROGRAM_WINDOW

		FilePath = m_prgwindow->ShowOpenFileDialog("Open PS2 State file:");
#else

		FilePath = ProgramWindow->ShowFileOpenDialog ();
#endif
	}

	ifstream InputFile ( FilePath.c_str (), ios::binary );

	cout << "Loading state.\n";
	
	if ( !InputFile )
	{
#ifdef INLINE_DEBUG
	debug << "->Error loading save state";
#endif

		cout << "Error loading save state.\n";
		return;
	}


#ifdef INLINE_DEBUG
	debug << "; Creating Load State";
#endif

	Reset ();

	// read entire state from memory
	//InputFile.read ( (char*) this, sizeof( System ) );
	InputFile.read ( (char*) &_SYSTEM, sizeof( System ) );
	
	InputFile.close();
	
	// re-calibrate timers
	//_SYSTEM._TIMERS.ReCalibrateAll ();
	// refresh objects (set the non-static pointers)
	_SYSTEM.Refresh ();


	//_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = true;
	//_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;

#ifdef ALLOW_PS2_HWRENDER
	// if previously on hardware renderer, then copy framebuffer
	if (_HPS2X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// copy frame buffer from hardware to software if hardware rendering is allowed
		if (_HPS2X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			// copy VRAM from gpu hardware to software VRAM
			_HPS2X64._SYSTEM._GPU.Copy_VRAM_toGPU();
			_HPS2X64._SYSTEM._GPU.Copy_CLUT_toGPU();
			_HPS2X64._SYSTEM._GPU.Copy_VARS_toGPU();
		}
		else
		{
			// the hardware does not allow this particular compute shader for some reason
			_HPS2X64._SYSTEM._GPU.bEnable_OpenCL = false;
		}
	}
#endif


	// save virtual state for R3000A and BUS
	_SYSTEM._PS1SYSTEM._CPU.RestoreVirtualState();
	_SYSTEM._PS1SYSTEM._BUS.RestoreVirtualState();
	_SYSTEM._BUS.RestoreVirtualState();


	cout << "Done Loading state.\n";
	
#ifdef INLINE_DEBUG
	debug << "->Leaving function: System::LoadState";
#endif
}


void hps2x64::LoadBIOS ( string FilePath )
{
	bool bRet;
	string NVMPath;
	
	cout << "Loading BIOS.\n";
	
	////////////////////////////////////////////////////////
	// We need to prompt for the TEST program to run
	if ( !FilePath.compare ( "" ) )
	{
		cout << "Prompting for BIOS file.\n";

#ifdef USE_NEW_PROGRAM_WINDOW

		FilePath = m_prgwindow->ShowOpenFileDialog("Open BIOS file:");
#else

		FilePath = ProgramWindow->ShowFileOpenDialog ();
#endif
	}

	if (!FilePath.compare(""))
	{
		cout << "\nNo file was chosen.\n";
		return;
	}

	cout << "Loading into memory.\n";

	if ( !_SYSTEM.LoadTestProgramIntoBios ( FilePath.c_str() ) )
	{
		// run the test code
		cout << "\nProblem loading test code.\n";

		// clearing last bios loaded into memory
		sLastBiosPath = "";
		
#ifdef INLINE_DEBUG
		debug << "\r\nProblem loading test code.";
#endif

	}
	else
	{
		// code loaded successfully
		cout << "\nCode loaded successfully into BIOS.\n";

		// setting path as the last bios loaded into memory
		sLastBiosPath = FilePath;

		NVMPath = GetPath ( FilePath.c_str () ) + GetFile ( FilePath.c_str () ) + ".nvm";
		NVMPath.copy ( _SYSTEM.Last_NVM_Path, 2048 );
		
		bRet = _SYSTEM._PS1SYSTEM._CDVD.LoadNVMFile ( _SYSTEM.Last_NVM_Path );
		
		if ( bRet )
		{
			// successfully loaded from the program directory //
			cout << "\nhps2x64: SUCCESS: NVM File successfully loaded from BIOS directory.\n";
		}
		else
		{
			cout << "\nhps2x64: ALERT: NVM File not loaded from file system.\n";
		}
	}
	
	cout << "LoadBIOS done.\n";

	//UpdateDebugWindow ();
	
	//DebugStatus.LoadBios = false;
}


string hps2x64::LoadDisk ( string FilePath )
{
	cout << "Loading Disk.\n";
	
	////////////////////////////////////////////////////////
	// We need to prompt for the TEST program to run
	if ( !FilePath.compare ( "" ) )
	{
		cout << "Prompting for DISK file.\n";

#ifdef USE_NEW_PROGRAM_WINDOW

		FilePath = m_prgwindow->ShowOpenFileDialog("Open DISK file:", "CD/DVD Image (cue/bin/ccd/img/iso/chd)\0*.cue;*.bin;*.ccd;*.img;*.chd;*.iso\0");
#else

		FilePath = ProgramWindow->ShowFileOpenDialog ("CD/DVD Image (cue/bin/ccd/img/iso/chd)\0*.cue;*.bin;*.ccd;*.img;*.chd;*.iso\0");
#endif
	}
	
	
	cout << "LoadDisk done.\n";
	

	return FilePath;
}




void hps2x64::LoadConfig ( string ConfigFileName )
{

	// read a JSON file
	std::ifstream i( ConfigFileName );

	if ( !i )
	{
		cout << "\nhps2x64::LoadConfig: Problem loading config file: " << ConfigFileName;
		return;
	}

	nlohmann::json jsonSettings;
	i >> jsonSettings;

	// save the controller settings //

	// pad 1 //

	_SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "DigitalAnalog" ];

	_SYSTEM._PS1SYSTEM._SIO.Key_X [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyX" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_O [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyO" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_Triangle [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyTriangle" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_Square [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeySquare" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_R1 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR1" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_R2 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR2" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_R3 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR3" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_L1 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL1" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_L2 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL2" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_L3 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL3" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_Start [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyStart" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_Select [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeySelect" ];
	_SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyLeftAnalogX" ];
	_SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyLeftAnalogY" ];
	_SYSTEM._PS1SYSTEM._SIO.RightAnalog_X [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyRightAnalogX" ];
	_SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyRightAnalogY" ];

	// pad 2 //

	_SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "DigitalAnalog" ];

	_SYSTEM._PS1SYSTEM._SIO.Key_X [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyX" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_O [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyO" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_Triangle [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyTriangle" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_Square [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeySquare" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_R1 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR1" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_R2 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR2" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_R3 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR3" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_L1 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL1" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_L2 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL2" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_L3 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL3" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_Start [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyStart" ];
	_SYSTEM._PS1SYSTEM._SIO.Key_Select [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeySelect" ];
	_SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyLeftAnalogX" ];
	_SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyLeftAnalogY" ];
	_SYSTEM._PS1SYSTEM._SIO.RightAnalog_X [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyRightAnalogX" ];
	_SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyRightAnalogY" ];

	// memory cards //

	_SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ] = jsonSettings [ "Controllers" ] [ "MemoryCard1" ] [ "Disconnected" ];
	_SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ] = jsonSettings [ "Controllers" ] [ "MemoryCard2" ] [ "Disconnected" ];

	// device settings //

	// CD //

	_SYSTEM._PS1SYSTEM._CD.Region = jsonSettings [ "CD" ] [ "Region" ];

	// SPU //

	_SYSTEM._PS1SYSTEM._SPU.AudioOutput_Enabled = jsonSettings [ "SPU" ] [ "Enable_AudioOutput" ];
	_SYSTEM._PS1SYSTEM._SPU.AudioFilter_Enabled = jsonSettings [ "SPU" ] [ "Enable_Filter" ];
	_SYSTEM._PS1SYSTEM._SPU.NextPlayBuffer_Size = jsonSettings [ "SPU" ] [ "BufferSize" ];
	_SYSTEM._PS1SYSTEM._SPU.GlobalVolume = jsonSettings [ "SPU" ] [ "GlobalVolume" ];

	// Interface //

	sLastBiosPath = jsonSettings["Interface"]["LastBiosPath"];

	// should probably also go ahead and make an attempt to load the last bios file here also if it exists
	if (sLastBiosPath.compare(""))
	{
		LoadBIOS(sLastBiosPath);
	}
}


void hps2x64::SaveConfig ( string ConfigFileName )
{
	// save configuration as json (using nlohmann json)
	nlohmann::json jsonSettings;


	// save the controller settings //

	// pad 1 //

	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "DigitalAnalog" ] = _SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 0 ];

	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyX" ] = _SYSTEM._PS1SYSTEM._SIO.Key_X [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyO" ] = _SYSTEM._PS1SYSTEM._SIO.Key_O [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyTriangle" ] = _SYSTEM._PS1SYSTEM._SIO.Key_Triangle [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeySquare" ] = _SYSTEM._PS1SYSTEM._SIO.Key_Square [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR1" ] = _SYSTEM._PS1SYSTEM._SIO.Key_R1 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR2" ] = _SYSTEM._PS1SYSTEM._SIO.Key_R2 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR3" ] = _SYSTEM._PS1SYSTEM._SIO.Key_R3 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL1" ] = _SYSTEM._PS1SYSTEM._SIO.Key_L1 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL2" ] = _SYSTEM._PS1SYSTEM._SIO.Key_L2 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL3" ] = _SYSTEM._PS1SYSTEM._SIO.Key_L3 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyStart" ] = _SYSTEM._PS1SYSTEM._SIO.Key_Start [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeySelect" ] = _SYSTEM._PS1SYSTEM._SIO.Key_Select [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyLeftAnalogX" ] = _SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyLeftAnalogY" ] = _SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyRightAnalogX" ] = _SYSTEM._PS1SYSTEM._SIO.RightAnalog_X [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyRightAnalogY" ] = _SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y [ 0 ];

	// pad 2 //

	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "DigitalAnalog" ] = _SYSTEM._PS1SYSTEM._SIO.ControlPad_Type [ 1 ];

	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyX" ] = _SYSTEM._PS1SYSTEM._SIO.Key_X [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyO" ] = _SYSTEM._PS1SYSTEM._SIO.Key_O [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyTriangle" ] = _SYSTEM._PS1SYSTEM._SIO.Key_Triangle [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeySquare" ] = _SYSTEM._PS1SYSTEM._SIO.Key_Square [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR1" ] = _SYSTEM._PS1SYSTEM._SIO.Key_R1 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR2" ] = _SYSTEM._PS1SYSTEM._SIO.Key_R2 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR3" ] = _SYSTEM._PS1SYSTEM._SIO.Key_R3 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL1" ] = _SYSTEM._PS1SYSTEM._SIO.Key_L1 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL2" ] = _SYSTEM._PS1SYSTEM._SIO.Key_L2 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL3" ] = _SYSTEM._PS1SYSTEM._SIO.Key_L3 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyStart" ] = _SYSTEM._PS1SYSTEM._SIO.Key_Start [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeySelect" ] = _SYSTEM._PS1SYSTEM._SIO.Key_Select [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyLeftAnalogX" ] = _SYSTEM._PS1SYSTEM._SIO.LeftAnalog_X [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyLeftAnalogY" ] = _SYSTEM._PS1SYSTEM._SIO.LeftAnalog_Y [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyRightAnalogX" ] = _SYSTEM._PS1SYSTEM._SIO.RightAnalog_X [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyRightAnalogY" ] = _SYSTEM._PS1SYSTEM._SIO.RightAnalog_Y [ 1 ];

	// memory cards //

	jsonSettings [ "Controllers" ] [ "MemoryCard1" ] [ "Disconnected" ] = _SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ];
	jsonSettings [ "Controllers" ] [ "MemoryCard2" ] [ "Disconnected" ] = _SYSTEM._PS1SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ];

	// device settings //

	// CD //

	jsonSettings [ "CD" ] [ "Region" ] = _SYSTEM._PS1SYSTEM._CD.Region;

	// SPU //

	jsonSettings [ "SPU" ] [ "Enable_AudioOutput" ] = _SYSTEM._PS1SYSTEM._SPU.AudioOutput_Enabled;
	jsonSettings [ "SPU" ] [ "Enable_Filter" ] = _SYSTEM._PS1SYSTEM._SPU.AudioFilter_Enabled;
	jsonSettings [ "SPU" ] [ "BufferSize" ] = _SYSTEM._PS1SYSTEM._SPU.NextPlayBuffer_Size;
	jsonSettings [ "SPU" ] [ "GlobalVolume" ] = _SYSTEM._PS1SYSTEM._SPU.GlobalVolume;

	// Interface //

	jsonSettings["Interface"]["LastBiosPath"] = sLastBiosPath;


	// write the settings to file as json //

	std::ofstream o( ConfigFileName );
	o << std::setw(4) << jsonSettings << std::endl;	
}



void hps2x64::DebugWindow_Update ()
{
	// can't do anything if they've clicked on the menu
	WindowClass::Window::WaitForModalMenuLoop ();
	
	_SYSTEM._CPU.DebugWindow_Update ();
	_SYSTEM._BUS.DebugWindow_Update ();
	_SYSTEM._DMA.DebugWindow_Update ();
	_SYSTEM._TIMERS.DebugWindow_Update ();
	_SYSTEM._INTC.DebugWindow_Update ();
	_SYSTEM._GPU.DebugWindow_Update ();
	_SYSTEM._VU0.VU0.DebugWindow_Update ( 0 );
	_SYSTEM._VU1.VU1.DebugWindow_Update ( 1 );

	_SYSTEM._IPU.DebugWindow_Update();
	_SYSTEM._GPU.DebugWindow_Update2();

	//_SYSTEM._SPU.DebugWindow_Update ();
	//_SYSTEM._CD.DebugWindow_Update ();

	
#ifndef EE_ONLY_COMPILE
	// update for the ps1 too
	_SYSTEM._PS1SYSTEM._CPU.DebugWindow_Update ();
	_SYSTEM._PS1SYSTEM._BUS.DebugWindow_Update ();
	_SYSTEM._PS1SYSTEM._DMA.DebugWindow_Update ();
	_SYSTEM._PS1SYSTEM._TIMERS.DebugWindow_Update ();
	_SYSTEM._PS1SYSTEM._SPU.DebugWindow_Update ();
	_SYSTEM._PS1SYSTEM._GPU.DebugWindow_Update ();
	_SYSTEM._PS1SYSTEM._CD.DebugWindow_Update ();
	_SYSTEM._PS1SYSTEM._SPU2.SPU0.DebugWindow_Update ( 0 );
	_SYSTEM._PS1SYSTEM._SPU2.SPU1.DebugWindow_Update ( 1 );
#endif

}






WindowClass::Window *Dialog_KeyConfigure::wDialog;

WindowClass::Button *Dialog_KeyConfigure::CmdButtonOk, *Dialog_KeyConfigure::CmdButtonCancel;
WindowClass::Button* Dialog_KeyConfigure::KeyButtons [ c_iDialog_NumberOfButtons ];

WindowClass::Static *Dialog_KeyConfigure::InfoLabel;
WindowClass::Static* Dialog_KeyConfigure::KeyLabels [ c_iDialog_NumberOfButtons ];

u32 Dialog_KeyConfigure::isDialogShowing;
volatile s32 Dialog_KeyConfigure::ButtonClick;

u32 Dialog_KeyConfigure::KeyConfigure [ c_iDialog_NumberOfButtons ];


int Dialog_KeyConfigure::population_count64(unsigned long long w)
{
    w -= (w >> 1) & 0x5555555555555555ULL;
    w = (w & 0x3333333333333333ULL) + ((w >> 2) & 0x3333333333333333ULL);
    w = (w + (w >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return int((w * 0x0101010101010101ULL) >> 56);
}


int Dialog_KeyConfigure::bit_scan_lsb ( unsigned long v )
{
	//unsigned int v;  // find the number of trailing zeros in 32-bit v 
	int r;           // result goes here
	static const int MultiplyDeBruijnBitPosition[32] = 
	{
	  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
	  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};
	r = MultiplyDeBruijnBitPosition[((unsigned long)((v & -v) * 0x077CB531U)) >> 27];
	return r;
}

bool Dialog_KeyConfigure::Show_ConfigureKeysDialog ( int iPadNum )
{
	static constexpr char* Dialog_Caption = "Configure Keys";
	static constexpr int Dialog_Id = 0x6000;
	static constexpr int Dialog_X = 10;
	static constexpr int Dialog_Y = 10;

	static constexpr char* Label1_Caption = "Instructions: Hold down the button on the joypad, and then click the PS button you want to assign it to (while still holding the button down). For analog sticks, hold the stick in that direction (x or y) and then click on the button to assign that axis.";
	static constexpr int Label1_Id = 0x6001;
	static constexpr int Label1_X = 10;
	static constexpr int Label1_Y = 10;
	static constexpr int Label1_Width = 300;
	static constexpr int Label1_Height = 100;
	
	static constexpr int c_iButtonArea_StartId = 0x6100;
	static constexpr int c_iButtonArea_StartX = 10;
	static constexpr int c_iButtonArea_StartY = Label1_Y + Label1_Height + 10;
	static constexpr int c_iButtonArea_ButtonHeight = 20;
	static constexpr int c_iButtonArea_ButtonWidth = 100;
	static constexpr int c_iButtonArea_ButtonPitch = c_iButtonArea_ButtonHeight + 5;

	static constexpr int c_iLabelArea_StartId = 0x6200;
	static constexpr int c_iLabelArea_StartX = c_iButtonArea_StartX + c_iButtonArea_ButtonWidth + 10;
	static constexpr int c_iLabelArea_StartY = c_iButtonArea_StartY;
	static constexpr int c_iLabelArea_LabelHeight = c_iButtonArea_ButtonHeight;
	static constexpr int c_iLabelArea_LabelWidth = 100;
	static constexpr int c_iLabelArea_LabelPitch = c_iLabelArea_LabelHeight + 5;

	
	static constexpr char* CmdButtonOk_Caption = "OK";
	static constexpr int CmdButtonOk_Id = 0x6300;
	static constexpr int CmdButtonOk_X = 10;
	static constexpr int CmdButtonOk_Y = c_iButtonArea_StartY + ( c_iButtonArea_ButtonPitch * c_iDialog_NumberOfButtons ) + 10;
	static constexpr int CmdButtonOk_Width = 50;
	static constexpr int CmdButtonOk_Height = 20;
	
	static constexpr char* CmdButtonCancel_Caption = "Cancel";
	static constexpr int CmdButtonCancel_Id = 0x6400;
	static constexpr int CmdButtonCancel_X = CmdButtonOk_X + CmdButtonOk_Width + 10;
	static constexpr int CmdButtonCancel_Y = CmdButtonOk_Y;
	static constexpr int CmdButtonCancel_Width = 50;
	static constexpr int CmdButtonCancel_Height = 20;
	
	// now set width and height of dialog
	static constexpr int Dialog_Width = Label1_Width + 20;	//c_iLabelArea_StartX + c_iLabelArea_LabelWidth + 10;
	static constexpr int Dialog_Height = CmdButtonOk_Y + CmdButtonOk_Height + 30;
		
	static constexpr char* PS1_Keys [] = { "X", "O", "Triangle", "Square", "R1", "R2", "R3", "L1", "L2", "L3", "Start", "Select", "Left Analog X", "Left Analog Y", "Right Analog X", "Right Analog Y" };
	static constexpr char* Axis_Labels [] = { "Axis X", "Axis Y", "Axis Z", "Axis R", "Axis U", "Axis V" };
	
	bool ret;
	
	int iKeyIdx;
	
	//u32 *Key_X, *Key_O, *Key_Triangle, *Key_Square, *Key_R1, *Key_R2, *Key_R3, *Key_L1, *Key_L2, *Key_L3, *Key_Start, *Key_Select, *LeftAnalogX, *LeftAnalogY, *RightAnalogX, *RightAnalogY;
	//u32* Key_Pointers [ c_iDialog_NumberOfButtons ];

	stringstream ss;
	

	//if ( !isDialogShowing )
	//{
		//x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&) isDialogShowing, true );

		// set the events to use on call back
		//OnClick_Ok = _OnClick_Ok;
		//OnClick_Cancel = _OnClick_Cancel;
		
		// this is the value list that was double clicked on
		// now show a window where the variable can be modified
		// *note* setting the parent to the list-view control
		cout << "\nAllocating dialog";
		wDialog = new WindowClass::Window ();
		
		cout << "\nCreating dialog";


#ifdef USE_NEW_PROGRAM_WINDOW

		wDialog->Create(Dialog_Caption, Dialog_X, Dialog_Y, Dialog_Width, Dialog_Height, WindowClass::Window::DefaultStyle, NULL, hps2x64::m_prgwindow->GetHandle());
#else

		wDialog->Create ( Dialog_Caption, Dialog_X, Dialog_Y, Dialog_Width, Dialog_Height, WindowClass::Window::DefaultStyle, NULL, hps2x64::ProgramWindow->hWnd );
#endif


		wDialog->DisableCloseButton ();
		
		// disable parent window
		cout << "\nDisable parent window";

#ifdef USE_NEW_PROGRAM_WINDOW

		hps2x64::m_prgwindow->SetEnabled(false);
#else

		hps2x64::ProgramWindow->Disable ();

#endif
		
		//cout << "\nCreating static control";
		InfoLabel = new WindowClass::Static ();
		InfoLabel->Create_Text ( wDialog, Label1_X, Label1_Y, Label1_Width, Label1_Height, (char*) Label1_Caption, Label1_Id );
		
		// create the buttons and labels
		cout << "\nAdding buttons and labels.";
		for ( int i = 0; i < c_iDialog_NumberOfButtons; i++ )
		{
			// clear temp string
			//ss.str ( "" );
			
			// get name for label
			/*
			if ( i < 12 )
			{
				// label is for button //
				ss << "Button#" << bit_scan_lsb ( *Key_Pointers [ i ] );
			}
			else
			{
				// label is for analog stick //
				ss << Axis_Labels [ *Key_Pointers [ i ] ];
			}
			*/
			
			// put in a static label for entering a new value
			KeyLabels [ i ] = new WindowClass::Static ();
			KeyLabels [ i ]->Create_Text ( wDialog, c_iLabelArea_StartX, c_iLabelArea_StartY + ( i * c_iLabelArea_LabelPitch ), c_iLabelArea_LabelWidth, c_iLabelArea_LabelHeight, (char*) "test" /*ss.str().c_str()*/, c_iLabelArea_StartId + i );

			// put in a button
			KeyButtons [ i ] = new WindowClass::Button ();
			KeyButtons [ i ]->Create_CmdButton( wDialog, c_iButtonArea_StartX, c_iButtonArea_StartY + ( i * c_iButtonArea_ButtonPitch ), c_iButtonArea_ButtonWidth, c_iButtonArea_ButtonHeight, (char*) PS1_Keys [ i ], c_iButtonArea_StartId + i );
			
			// add event for ok button
			KeyButtons [ i ]->AddEvent ( WM_COMMAND, ConfigureDialog_AnyClick );
		}
		
		// put in an ok button
		CmdButtonOk = new WindowClass::Button ();
		CmdButtonOk->Create_CmdButton( wDialog, CmdButtonOk_X, CmdButtonOk_Y, CmdButtonOk_Width, CmdButtonOk_Height, (char*) CmdButtonOk_Caption, CmdButtonOk_Id );
		
		// add event for ok button
		CmdButtonOk->AddEvent ( WM_COMMAND, ConfigureDialog_AnyClick );
		
		// put in an cancel button
		CmdButtonCancel = new WindowClass::Button ();
		CmdButtonCancel->Create_CmdButton( wDialog, CmdButtonCancel_X, CmdButtonCancel_Y, CmdButtonCancel_Width, CmdButtonCancel_Height, (char*) CmdButtonCancel_Caption, CmdButtonCancel_Id );
		
		// add event for cancel button
		CmdButtonCancel->AddEvent ( WM_COMMAND, ConfigureDialog_AnyClick );
		
		// refresh keys
		Refresh ();
		
		ButtonClick = 0;
		

		if ( _HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ iPadNum ] == -1 )
		{
			_HPS2X64._SYSTEM._PS1SYSTEM._SIO.PortMapping [ iPadNum ] = iPadNum;
		}
		
		while ( ButtonClick != CmdButtonOk_Id && ButtonClick != CmdButtonCancel_Id )
		{
			Sleep ( 10 );
			WindowClass::DoEvents ();
			

			Playstation1::SIO::gamepad.Update();
			

			//if ( ButtonClick != CmdButtonOk_Id && ButtonClick != CmdButtonCancel_Id && ButtonClick != 0 )
			//{
				if ( ( ButtonClick & 0xff00 ) == c_iButtonArea_StartId )
				{
					
					if ( ( ButtonClick & 0xff ) < 12 )
					{
						// get index of key that is pressed
						for ( iKeyIdx = 0; iKeyIdx < 32; iKeyIdx++ )
						{
							if (Playstation1::SIO::gamepad.GetRawDeviceState(iPadNum).rgbButtons[iKeyIdx])
							{
								break;
							}
						}
						
						
						if ( iKeyIdx < 32 )
						{
							KeyConfigure [ ButtonClick & 0xff ] = iKeyIdx;
						}
					}
					else
					{
						
						u32 axis_value [ 6 ];
						u32 max_index;
						
						//long* pData = (long*)&Playstation1::SIO::gamepad.GetRawDeviceState(iPadNum);
						DIJOYSTATE2 oData = Playstation1::SIO::gamepad.GetRawDeviceState(iPadNum);
						long* pData = (long*)&oData;
						
						//cout << "\n";
						for ( int i = 0; i < 6; i++ )
						{
							//cout << hex << Axis_Labels [ i ] << "=" << ((s32*)(&j.joyinfo.dwXpos)) [ i ] << " ";
							if ( pData [ i ] )
							{
								axis_value [ i ] = _Abs ( pData [ i ] - 0x80 );
							}
							else
							{
								axis_value [ i ] = 0;
							}
						}
						
						// check which axis value is greatest
						max_index = -1;
						for ( int i = 0; i < 6; i++ )
						{
							if ( axis_value [ i ] >= 75 )
							{
								max_index = i;
								break;
							}
						}
						
						// store axis
						if ( max_index != -1 ) KeyConfigure [ ButtonClick & 0xff ] = max_index;
						
					}
					
					// clear last button click
					ButtonClick = 0;
					
					Refresh ();
				}
				
				
			//}
		}
		
		ret = false;
		
		if ( ButtonClick == CmdButtonOk_Id )
		{
			ret = true;
		}
				
#ifdef USE_NEW_PROGRAM_WINDOW

		hps2x64::m_prgwindow->SetEnabled(true);
#else

		hps2x64::ProgramWindow->Enable ();
#endif

		delete wDialog;
	//}
	
	return ret;
}


void Dialog_KeyConfigure::Refresh ()
{
	static const char* Axis_Labels [] = { "Axis X", "Axis Y", "Axis Z", "Axis R", "Axis U", "Axis V" };
	
	stringstream ss;
	
	for ( int i = 0; i < c_iDialog_NumberOfButtons; i++ )
	{
		// clear temp string
		ss.str ( "" );
		
		// get name for label
		if ( i < 12 )
		{
			ss << "Button#" << KeyConfigure [ i ];
		}
		else
		{
			// label is for analog stick //
			ss << Axis_Labels [ KeyConfigure [ i ] ];
		}
		
		// put in a static label for entering a new value
		KeyLabels [ i ]->SetText ( (char*) ss.str().c_str() );
	}
}

void Dialog_KeyConfigure::ConfigureDialog_AnyClick ( HWND hCtrl, int idCtrl, unsigned int message, WPARAM wParam, LPARAM lParam )
{
	int i;
	HWND Parent_hWnd;
	
	cout << "\nClicked on a button. idCtrl=" << dec << idCtrl;
	
	x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&) ButtonClick, idCtrl );

	/*
	if ( idCtrl > 6
	
	//cout << "\nClicked the OK button";
	
	// get the handle for the parent window
	Parent_hWnd = WindowClass::Window::GetHandleToParent ( hCtrl );
	
	//cout << "\nParent Window #1=" << (unsigned long) Parent_hWnd;
	
	i = FindInputBoxIndex ( Parent_hWnd );
	
	if ( i < 0 ) return;
	
	ListOfInputBoxes [ i ]->ReturnValue = ListOfInputBoxes [ i ]->editBox1->GetText ();
	if ( ListOfInputBoxes [ i ]->OnClick_Ok ) ListOfInputBoxes [ i ]->OnClick_Ok ( ListOfInputBoxes [ i ]->editBox1->GetText () );
	ListOfInputBoxes [ i ]->KillDialog ();
	*/
}


