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




#include "hps1x64.h"
#include "WinApiHandler.h"
#include <fstream>
//#include "ConfigFile.h"
#include "StringUtils.h"

#include "guicon.h"

#include "GamepadHandler.hpp"

//#include "vulkan_setup.h"

//#include "yaml.h"

#include "capture_console.h"

using namespace Playstation1;
using namespace Utilities::Strings;
//using namespace Config;



#ifdef _DEBUG_VERSION_

// debug defines go in here

#endif


#define USE_NEW_PROGRAM_WINDOW


#define ENABLE_DIRECT_INPUT


// allow hardware rendering
#define ALLOW_PS1_HWRENDER

// the type of window, either opengl or vulkan
int hps1x64::iWindowType;

GamepadConfigWindow::GamepadConfig hps1x64::m_gamepadConfig;


hps1x64 _HPS1X64;


volatile u32 hps1x64::_MenuClick;
volatile hps1x64::RunMode hps1x64::_RunMode;

WindowClass::Window *hps1x64::ProgramWindow;
std::unique_ptr<OpenGLWindow> hps1x64::m_prgwindow;


// the path for the last bios that was selected
string hps1x64::sLastBiosPath;

string hps1x64::BiosPath;
string hps1x64::ExecutablePath;
char ExePathTemp [ hps1x64::c_iExeMaxPathLength + 1 ];


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{

#if !defined(__GNUC__)
	// don't need this for gcc
	RedirectIOToConsole();
#endif


	// using vulkan
	//char* pData;

	//pData = (char*)vulkan_setup("compute.spv", sizeof(_GPU->VRAM) * 2, sizeof(_GPU->VRAM) * 2, sizeof(inputdata), sizeof(ulPixelInBuffer32), 128 * (1 << 16) * 4);
	//pData = (char*)vulkan_setup("compute.spv", 1024 * 512 * 2, 1024 * 512 * 2, 16 * (1 << 16) * 4, (1 << 20) * 4, 128 * (1 << 16) * 4);
	//pData = (char*)vulkan_setup("C:\\Users\\PCUser\\Desktop\\TheProject\\hpsx64\\compute.spv", 32, 32, 32, 32, 32);


	WindowClass::Register ( hInstance, "testSystem" );

	
	cout << "Initializing program...\n";
		

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


	_HPS1X64.Initialize ();

	// initialize direct input joysticks here for now

	SIO::gamepad.Initialize(hps1x64::m_prgwindow->GetHandle());

	cout << "Starting run of program...\n";
	
	_HPS1X64.RunProgram ();
	
	
	//cin.ignore ();
	
	return 0;
}


hps1x64::hps1x64 ()
{
	cout << "Running hps1x64 constructor...\n";
	
	
	// zero object
	// *** PROBLEM *** this clears out all the defaults for the system
	//memset ( this, 0, sizeof( hps1x64 ) );
}


hps1x64::~hps1x64 ()
{
	cout << "Running hps1x64 destructor...\n";
	
	// end the timer resolution
	if ( timeEndPeriod ( 1 ) == TIMERR_NOCANDO )
	{
		cout << "\nhpsx64 ERROR: Problem ending timer period.\n";
	}
}

void hps1x64::Reset ()
{
	_RunMode.Value = 0;
	
	_SYSTEM.Reset ();
}





// returns 0 if menu was not clicked, returns 1 if menu was clicked
int hps1x64::HandleMenuClick ()
{
	int i;
	int MenuWasClicked = 0;
	
	//if ( _MenuClick.Value )
	if ( _MenuClick )
	{
		cout << "\nA menu item was clicked.\n";

		_MenuClick = 0;
		
		// a menu item was clicked
		MenuWasClicked = 1;
		
		
		// update anything that was checked/unchecked
		Update_CheckMarksOnMenu ();
		
		// clear anything that was clicked
		//x64ThreadSafe::Utilities::Lock_Exchange64 ( (long long&)_MenuClick.Value, 0 );
		
		DebugWindow_Update ();
	}
	
	return MenuWasClicked;
}


void hps1x64::Update_CheckMarksOnMenu ()
{
	// uncheck all first

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetMenuItemChecked("load|bios", false);
	m_prgwindow->SetMenuItemChecked("load|gamedisk", false);
	m_prgwindow->SetMenuItemChecked("load|audiodisk", false);

	m_prgwindow->SetMenuItemChecked("pad1|digital", false);
	m_prgwindow->SetMenuItemChecked("pad1|analog", false);
	m_prgwindow->SetMenuItemChecked("pad1|none", false);
	m_prgwindow->SetMenuItemChecked("pad1|device0", false);
	m_prgwindow->SetMenuItemChecked("pad1|device1", false);

	m_prgwindow->SetMenuItemChecked("pad2|digital", false);
	m_prgwindow->SetMenuItemChecked("pad2|analog", false);
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
	m_prgwindow->SetMenuItemChecked("audio|filter", false);

	m_prgwindow->SetMenuItemChecked("opengl|software", false);
	m_prgwindow->SetMenuItemChecked("vulkan|software", false);
	m_prgwindow->SetMenuItemChecked("vulkan|hardware", false);

	m_prgwindow->SetMenuItemChecked("scanlines|enable", false);
	m_prgwindow->SetMenuItemChecked("scanlines|disable", false);

	m_prgwindow->SetMenuItemChecked("windowsize|x1", false);
	m_prgwindow->SetMenuItemChecked("windowsize|x1.5", false);
	m_prgwindow->SetMenuItemChecked("windowsize|x2", false);

	m_prgwindow->SetMenuItemChecked("displaymode|stretch", false);
	m_prgwindow->SetMenuItemChecked("displaymode|fit", false);

	m_prgwindow->SetMenuItemChecked("r3000a|interpreter", false);
	m_prgwindow->SetMenuItemChecked("r3000a|recompiler", false);

	// check if a bios is loaded into memory or not
	if (sLastBiosPath.compare(""))
	{
		// looks like there is a bios currently loaded into memory //

		m_prgwindow->SetMenuItemChecked("load|bios", true);
	}

	// check box for audio output enable //
	if (_SYSTEM._SPU.AudioOutput_Enabled)
	{
		m_prgwindow->SetMenuItemChecked("audio|enable", true);
	}

	// check box for if disk is loaded and whether data/audio //

	if (!_SYSTEM._CD.isLidOpen)
	{
		switch (_SYSTEM._CD.isGameCD)
		{
		case true:
			m_prgwindow->SetMenuItemChecked("load|gamedisk", true);
			break;

		case false:
			m_prgwindow->SetMenuItemChecked("load|audiodisk", true);
			break;
		}
	}


	switch (_SYSTEM._SIO.ControlPad_Type[0])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("pad1|digital", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("pad1|analog", true);
		break;
	}

	switch (_SYSTEM._SIO.PortMapping[0])
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
	switch (_SYSTEM._SIO.ControlPad_Type[1])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("pad2|digital", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("pad2|analog", true);
		break;
	}

	switch (_SYSTEM._SIO.PortMapping[1])
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
	switch (_SYSTEM._SIO.MemoryCard_ConnectionState[0])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("card1|connect", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("card1|disconnect", true);
		break;
	}

	// do card 2
	switch (_SYSTEM._SIO.MemoryCard_ConnectionState[1])
	{
	case 0:
		m_prgwindow->SetMenuItemChecked("card2|connect", true);
		break;

	case 1:
		m_prgwindow->SetMenuItemChecked("card2|disconnect", true);
		break;
	}


	// check box for region //
	switch (_SYSTEM._CD.Region)
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
	switch (_SYSTEM._SPU.GlobalVolume)
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
	if (_SYSTEM._SPU.AudioFilter_Enabled)
	{
		m_prgwindow->SetMenuItemChecked("audio|filter", true);
	}

	// scanlines enable/disable //
	if (_SYSTEM._GPU.Get_Scanline())
	{
		m_prgwindow->SetMenuItemChecked("scanlines|enable", true);
	}
	else
	{
		m_prgwindow->SetMenuItemChecked("scanlines|disable", true);
	}

	if (m_prgwindow->GetDisplayMode() == OpenGLWindow::DisplayMode::STRETCH)
	{
		m_prgwindow->SetMenuItemChecked("displaymode|stretch", true);

	}
	if (m_prgwindow->GetDisplayMode() == OpenGLWindow::DisplayMode::FIT)
	{
		m_prgwindow->SetMenuItemChecked("displaymode|fit", true);
	}


	// R3000A Interpreter/Recompiler //
	if (_SYSTEM._CPU.bEnableRecompiler)
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
	ProgramWindow->Menus->UnCheckItem("Bios");
	ProgramWindow->Menus->UnCheckItem ( "Insert/Remove Game Disk" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1 Digital" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1 Analog" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1: None" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1: Device0" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 1: Device1" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 2 Digital" );
	ProgramWindow->Menus->UnCheckItem ( "Pad 2 Analog" );
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
	ProgramWindow->Menus->UnCheckItem ( "Enable Scanlines" );
	ProgramWindow->Menus->UnCheckItem ( "Disable Scanlines" );
	ProgramWindow->Menus->UnCheckItem ( "Interpreter: R3000A" );
	ProgramWindow->Menus->UnCheckItem ( "Recompiler: R3000A" );
#ifdef ENABLE_RECOMPILE2
	ProgramWindow->Menus->UnCheckItem ( "Recompiler2: R3000A" );
#endif
	ProgramWindow->Menus->UnCheckItem ( "1 (multi-thread)" );
	ProgramWindow->Menus->UnCheckItem ( "0 (single-thread)" );

	ProgramWindow->Menus->UnCheckItem ( "Renderer: OpenGL: Software" );
	ProgramWindow->Menus->UnCheckItem("Renderer: Vulkan: Software");
	ProgramWindow->Menus->UnCheckItem ( "Renderer: Vulkan: Hardware" );


	// check if a bios is loaded into memory or not
	if (sLastBiosPath.compare(""))
	{
		// looks like there is a bios currently loaded into memory //

		// add a tick mark next to File | Load | Bios
		ProgramWindow->Menus->CheckItem("Bios");
	}

	// check box for audio output enable //
	if ( _SYSTEM._SPU.AudioOutput_Enabled )
	{
		ProgramWindow->Menus->CheckItem ( "Enable" );
	}
	
	// check box for if disk is loaded and whether data/audio //
	
	if ( !_SYSTEM._CD.isLidOpen )
	{
		switch ( _SYSTEM._CD.isGameCD )
		{
			case true:
				ProgramWindow->Menus->CheckItem ( "Insert/Remove Game Disk" );
				break;
			
			case false:
				ProgramWindow->Menus->CheckItem ( "Insert/Remove Audio Disk" );
				break;
		}
	}
	
	// check box for analog/digital pad 1/2 //
	
	// do pad 1
	switch ( _SYSTEM._SIO.ControlPad_Type [ 0 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Pad 1 Digital" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Pad 1 Analog" );
			break;
	}

	switch ( _SYSTEM._SIO.PortMapping [ 0 ] )
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
	switch ( _SYSTEM._SIO.ControlPad_Type [ 1 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Pad 2 Digital" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Pad 2 Analog" );
			break;
	}

	switch ( _SYSTEM._SIO.PortMapping [ 1 ] )
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
	switch ( _SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Connect Card1" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Disconnect Card1" );
			break;
	}
	
	// do card 2
	switch ( _SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ] )
	{
		case 0:
			ProgramWindow->Menus->CheckItem ( "Connect Card2" );
			break;
			
		case 1:
			ProgramWindow->Menus->CheckItem ( "Disconnect Card2" );
			break;
	}
	
	
	// check box for region //
	switch ( _SYSTEM._CD.Region )
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
	switch ( _SYSTEM._SPU.NextPlayBuffer_Size )
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
	switch ( _SYSTEM._SPU.GlobalVolume )
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
	if ( _SYSTEM._SPU.AudioFilter_Enabled )
	{
		ProgramWindow->Menus->CheckItem ( "Filter" );
	}
	
	// scanlines enable/disable //
	if ( _SYSTEM._GPU.Get_Scanline () )
	{
		ProgramWindow->Menus->CheckItem ( "Enable Scanlines" );
	}
	else
	{
		ProgramWindow->Menus->CheckItem ( "Disable Scanlines" );
	}
	
	// R3000A Interpreter/Recompiler //
	if ( _SYSTEM._CPU.bEnableRecompiler )
	{
		if ( _SYSTEM._CPU.rs->OptimizeLevel == 1 )
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
	
	
#ifdef ALLOW_PS1_MULTITHREAD
	if ( _SYSTEM._GPU.ulNumberOfThreads )
	{
		ProgramWindow->Menus->CheckItem ( "1 (multi-thread)" );
	}
	else
	{
		ProgramWindow->Menus->CheckItem ( "0 (single-thread)" );
	}
#endif


#ifdef ALLOW_PS1_HWRENDER
	if ( _SYSTEM._GPU.bEnable_OpenCL )
	{
		ProgramWindow->Menus->CheckItem ( "Renderer: Vulkan: Hardware" );
	}
	else
	{
		switch (_SYSTEM._GPU.iCurrentDrawLibrary)
		{
		case Playstation1::GPU::DRAW_LIBRARY_VULKAN:
			ProgramWindow->Menus->CheckItem("Renderer: Vulkan: Software");
			break;

		default:
			ProgramWindow->Menus->CheckItem("Renderer: OpenGL: Software");
			break;
		}
	}
#endif

#endif

}


bool hps1x64::CreateOpenGLWindow()
{
	/*
	if (iWindowType == WINDOW_TYPE_VULKAN)
	{
		if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
		{
			// copy frame buffer from hardware to software if hardware rendering is allowed
			if (_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
			{
				// copy VRAM from gpu hardware to software VRAM
				_HPS1X64._SYSTEM._GPU.Copy_VRAM_toCPU();
			}
		}


		// unload vulkan
		_HPS1X64._SYSTEM._GPU.UnloadVulkan();


		ProgramWindow->KillWindow();
	}

	if (iWindowType == WINDOW_TYPE_OPENGL)
	{
		cout << "\nhpsx64: ALERT: Window type is already an OPENGL window.";
		return true;
	}

	ProgramWindow->CreateBaseWindow(ProgramWindow_Caption, ProgramWindow_Width, ProgramWindow_Height, true);


	cout << "\nAdding menubar";

	////////////////////////////////////////////
	// add menu bar to program window
	WindowClass::MenuBar* m = ProgramWindow->Menus;

	//ProgramWindow->CreateMenuFromJson(jsnMenuBar, "English");
	ProgramWindow->CreateMenuFromYaml(smenu_yaml, "english");

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

	_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
	_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
	_HPS1X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation1::GPU::DRAW_LIBRARY_OPENGL;

	_HPS1X64.Update_CheckMarksOnMenu();
	*/

	return true;
}

bool hps1x64::CreateVulkanWindow()
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

	ProgramWindow->CreateBaseWindow(ProgramWindow_Caption, ProgramWindow_Width, ProgramWindow_Height, true);

#ifdef VERBOSE_SHOW_DISPLAY_MODES

	ProgramWindow->OutputAllDisplayModes();

#endif


	//vulkan_set_screen_size(ProgramWindow_Width, ProgramWindow_Height);

	// create a vulkan context for the window
	//vulkan_create_context(ProgramWindow->hWnd, ProgramWindow->hInstance);

	// create/re-create swap chain
	//vulkan_create_swap_chain();

	if (_HPS1X64._SYSTEM._GPU.LoadVulkan())
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
	ProgramWindow->CreateMenuFromYaml(smenu_yaml, "english");


	cout << "\nShowing menu bar";

	// show the menu bar
	m->Show();


	// window is now a vulkan window
	iWindowType = WINDOW_TYPE_VULKAN;

	_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
	_HPS1X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation1::GPU::DRAW_LIBRARY_VULKAN;

	_HPS1X64.Update_CheckMarksOnMenu();
	*/

	return true;
}


void hps1x64::map_callback_functions()
{
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_State", OnClick_File_Load_State);
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_BIOS", OnClick_File_Load_BIOS);
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_GameDisk", OnClick_File_Load_GameDisk);
	WindowClass::MenuBar::add_map_function("OnClick_File_Load_AudioDisk", OnClick_File_Load_AudioDisk);
	WindowClass::MenuBar::add_map_function("OnClick_File_Save_State", OnClick_File_Save_State);
	WindowClass::MenuBar::add_map_function("OnClick_File_Reset", OnClick_File_Reset);
	WindowClass::MenuBar::add_map_function("OnClick_File_Run", OnClick_File_Run);
	WindowClass::MenuBar::add_map_function("OnClick_File_Exit", OnClick_File_Exit);

	WindowClass::MenuBar::add_map_function("OnClick_Debug_Break", OnClick_Debug_Break);
	WindowClass::MenuBar::add_map_function("OnClick_Debug_StepInto", OnClick_Debug_StepInto);
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

	WindowClass::MenuBar::add_map_function("OnClick_Controllers0_Configure", OnClick_Controllers0_Configure);
	WindowClass::MenuBar::add_map_function("OnClick_Controllers1_Configure", OnClick_Controllers1_Configure);
	WindowClass::MenuBar::add_map_function("OnClick_Pad1Type_Digital", OnClick_Pad1Type_Digital);
	WindowClass::MenuBar::add_map_function("OnClick_Pad1Type_Analog", OnClick_Pad1Type_Analog);
	WindowClass::MenuBar::add_map_function("OnClick_Pad2Type_Digital", OnClick_Pad2Type_Digital);
	WindowClass::MenuBar::add_map_function("OnClick_Pad2Type_Analog", OnClick_Pad2Type_Analog);

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

	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Software_OpenGL", OnClick_Video_Renderer_Software_OpenGL);
	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Software_Vulkan", OnClick_Video_Renderer_Software_Vulkan);
	WindowClass::MenuBar::add_map_function("OnClick_Video_Renderer_Hardware_Vulkan", OnClick_Video_Renderer_Hardware_Vulkan);
	WindowClass::MenuBar::add_map_function("OnClick_Video_ScanlinesEnable", OnClick_Video_ScanlinesEnable);
	WindowClass::MenuBar::add_map_function("OnClick_Video_ScanlinesDisable", OnClick_Video_ScanlinesDisable);
	WindowClass::MenuBar::add_map_function("OnClick_Video_WindowSizeX1", OnClick_Video_WindowSizeX1);
	WindowClass::MenuBar::add_map_function("OnClick_Video_WindowSizeX15", OnClick_Video_WindowSizeX15);
	WindowClass::MenuBar::add_map_function("OnClick_Video_WindowSizeX2", OnClick_Video_WindowSizeX2);
	WindowClass::MenuBar::add_map_function("OnClick_Video_FullScreen", OnClick_Video_FullScreen);

	WindowClass::MenuBar::add_map_function("OnClick_R3000ACPU_Interpreter", OnClick_R3000ACPU_Interpreter);
	WindowClass::MenuBar::add_map_function("OnClick_R3000ACPU_Recompiler", OnClick_R3000ACPU_Recompiler);
	WindowClass::MenuBar::add_map_function("OnClick_R3000ACPU_Recompiler2", OnClick_R3000ACPU_Recompiler2);

	WindowClass::MenuBar::add_map_function("OnClick_GPU_0Threads", OnClick_GPU_0Threads);
	WindowClass::MenuBar::add_map_function("OnClick_GPU_1Threads", OnClick_GPU_1Threads);
	WindowClass::MenuBar::add_map_function("OnClick_GPU_Software", OnClick_GPU_Software);
	WindowClass::MenuBar::add_map_function("OnClick_GPU_Hardware", OnClick_GPU_Hardware);

	//for (const auto& pair : WindowClass::mapped_functions) {
	//	std::cout << pair.first << ": " << pair.second << std::endl;
	//}

	//cout << " test2: OnClick_File_Exit:" << (unsigned long long)WindowClass::mapped_functions["OnClick_File_Exit"];
	//cout << " test3: OnClick_File_Exit:" << WindowClass::MenuBar::get_map_function("OnClick_File_Exit");
}

int hps1x64::Initialize ()
{

	u32 xsize, ysize;


	// map functions that will be used by gui interface
	map_callback_functions();


	////////////////////////////////////////////////
	// create program window
	xsize = ProgramWindow_Width;
	ysize = ProgramWindow_Height;


#ifdef USE_NEW_PROGRAM_WINDOW

	// new window code
	m_prgwindow = std::make_unique<OpenGLWindow>("Hps1x64OpenGLWindow", "hps1x64");
	if (!m_prgwindow->Create(ProgramWindow_Width, ProgramWindow_Height)) {
		std::cout << "Failed to create OpenGL window!" << std::endl;
		return false;
	}
	std::cout << "OpenGL window created successfully." << std::endl;

	_SYSTEM._GPU.SetDisplayOutputWindow2(ProgramWindow_Width, ProgramWindow_Height, m_prgwindow.get());

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
	std::cout << "Windows should now be visible." << std::endl;

#else

	ProgramWindow = new WindowClass::Window ();

	// init window type
	iWindowType = WINDOW_TYPE_NONE;


	// we want the screen to display on the main window for the program when the system encouters start of vertical blank
	_SYSTEM._GPU.SetDisplayOutputWindow(ProgramWindow_Width, ProgramWindow_Height, ProgramWindow);

	//CreateVulkanWindow();
	CreateOpenGLWindow();

#endif


	// start system - must do this here rather than in constructor
	_SYSTEM.Start();
	
	
	// set the timer resolution
	if ( timeBeginPeriod ( 1 ) == TIMERR_NOCANDO )
	{
		cout << "\nhpsx64 ERROR: Problem setting timer period.\n";
	}




	
	// get executable path
	int len = GetModuleFileName ( NULL, ExePathTemp, c_iExeMaxPathLength );
	ExePathTemp [ len ] = 0;
	
	// remove program name from path
	ExecutablePath = Left ( ExePathTemp, InStrRev ( ExePathTemp, "\\" ) + 1 );

	
	//cout << "\nExecutable Path=" << ExecutablePath.c_str();
	
	cout << "\nLoading memory cards if available...";
	
	_SYSTEM._SIO.Load_MemoryCardFile ( ExecutablePath + "card0", 0 );
	_SYSTEM._SIO.Load_MemoryCardFile ( ExecutablePath + "card1", 1 );
	
	
	cout << "\nLoading application-level config file...";
	
	// load current configuration settings
	// config settings that are needed:
	// 1. Region
	// 2. Audio - Enable, Volume, Buffer Size, Filter On/Off
	// 3. Peripherals - Pad1/Pad2/PadX keys, Pad1/Pad2/PadX Analog/Digital, Card1/Card2/CardX Connected/Disconnected
	// I like this one... a ".hcfg" file
	LoadConfig ( ExecutablePath + "hps1x64.hcfg" );
	
	
	cout << "\nUpdating check marks";
	
	// update what items are checked or not
	Update_CheckMarksOnMenu ();
	
	

	
	cout << "\ndone initializing";
	
	// done
	return 1;
}


void hps1x64::SetupMenus()
{
	m_prgwindow->CreateMenuBar();

	// File menu
	auto file_menu = m_prgwindow->AddMenu("file", "File");

	m_prgwindow->AddSubmenu("file", "file|load", "Load");
	m_prgwindow->AddMenuItem("file|load", "load|bios", "BIOS", "",
		[this]() {
			OnClick_File_Load_BIOS(0);
		});
	m_prgwindow->AddMenuItem("file|load", "load|gamedisk", "Game Disk", "",
		[this]() {
			OnClick_File_Load_GameDisk(0);
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

	m_prgwindow->AddMenuItem("file", "file|run", "Run", "Ctrl+R",
		[this]() {
			OnClick_File_Run(0);
		});

	m_prgwindow->AddMenuItem("file", "file|reset", "Reset", "",
		[this]() {
			OnClick_File_Save_State(0);
		});


	m_prgwindow->AddMenuSeparator("file");
	m_prgwindow->AddMenuItem("file", "file|exit", "Exit", "",
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
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|framebuffer", "Frame Buffer", "",
		[this]() {
			OnClick_Debug_Show_FrameBuffer(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|r3000a", "R3000A", "",
		[this]() {
			OnClick_Debug_Show_R3000A(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|memory", "Memory", "",
		[this]() {
			OnClick_Debug_Show_Memory(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|dma", "DMA", "",
		[this]() {
			OnClick_Debug_Show_DMA(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|timers", "Timers", "",
		[this]() {
			OnClick_Debug_Show_TIMER(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|spu", "SPU", "",
		[this]() {
			OnClick_Debug_Show_SPU(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|intc", "INTC", "",
		[this]() {
			OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|gpu", "GPU", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|mdec", "MDEC", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|sio", "SIO", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|pio", "PIO", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|cd", "CD", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|bus", "BUS", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});
	m_prgwindow->AddMenuItem("debug|showwindow", "debug|icache", "ICache", "",
		[this]() {
			//OnClick_Debug_Show_INTC(0);
		});

	//m_prgwindow->AddMenuItem("debug", "debug|debugwindow", "Show Debug Window", "",
	//	[this]() {
	//		OnClick_Show_DebugWindow(0);
	//	});
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

	m_prgwindow->AddSubmenu("video", "video|scanlines", "Scanlines");
	m_prgwindow->AddMenuItem("video|scanlines", "scanlines|enable", "Enable", "",
		[this]() {
			OnClick_Video_ScanlinesEnable(0);
		});
	m_prgwindow->AddMenuItem("video|scanlines", "scanlines|disable", "Disable", "",
		[this]() {
			OnClick_Video_ScanlinesDisable(0);
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
}


void hps1x64::RunProgram_Normal()
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

	cout << "\nRunning program";

	// get ticks per second for the platform's high-resolution timer
	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&TicksPerSec))
	{
		cout << "\nhpsx64 error: Error returned from call to QueryPerformanceFrequency.\n";
	}

	// calculate the ticks per milli second
	dTicksPerMilliSec = ((double)TicksPerSec) / 1000.0L;

	// get a pointer to the current frame number
	pCurrentFrameNumber = (volatile u32*)&_SYSTEM._GPU.Frame_Count;

	u64 ullPerfStart_Timer;
	u64 ullPerfEnd_Timer;
	u64 ullTicksUsedPerSecond;
	u32 ulFramesPerSec = 60;

	cout << "Running program...\n";

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetCaption("hps1x64");
#else

	ProgramWindow->SetCaption("hps1x64");

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


	while (_RunMode.RunNormal)
	{
		// init ticks used
		ullTicksUsedPerSecond = 0;

		if (_SYSTEM._GPU.GPU_CTRL_Read.VIDEO)
		{
			// pal is 50 frames per second //

			ulFramesPerSec = 50;
		}
		else
		{
			// ntsc is about 60 frames per second //

			ulFramesPerSec = 60;
		}

		// run for one second
		for (j = 0; j < ulFramesPerSec; j++)
		{
			// start timer to get time/ticks used for main program
			QueryPerformanceCounter((LARGE_INTEGER*)&ullPerfStart_Timer);

			// get the last frame number
			LastFrameNumber = *pCurrentFrameNumber;


			// loop until we reach the next frame
			//for ( i = 0; i < CyclesToRunContinuous; i++ )
			while (LastFrameNumber == (*pCurrentFrameNumber))
			{
				// run playstation 1 system in regular mode for one cpu instruction
				_SYSTEM.Run();
			}



			// get the target platform timer value for this frame
			// check if this is ntsc or pal
			if (_SYSTEM._GPU.GPU_CTRL_Read.VIDEO)
			{
				// PAL //

				TargetTimer += (((double)TicksPerSec) / GPU::PAL_FramesPerSec);
			}
			else
			{
				// NTSC //

				TargetTimer += (((double)TicksPerSec) / GPU::NTSC_FramesPerSec);
			}


			// check if we are running slower than target
			if (!QueryPerformanceCounter((LARGE_INTEGER*)&CurrentTimer))
			{
				cout << "\nhps1x64: Error returned from QueryPerformanceCounter\n";
			}

			TicksLeft = TargetTimer - CurrentTimer;

			bRunningTooSlow = false;
			if (TicksLeft < 0)
			{
				// running too slow //
				bRunningTooSlow = true;

			}


			// update the number of time/ticks used to run main program
			QueryPerformanceCounter((LARGE_INTEGER*)&ullPerfEnd_Timer);
			ullTicksUsedPerSecond += ullPerfEnd_Timer - ullPerfStart_Timer;

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

				if (MilliSecsToWait <= 0) MilliSecsToWait = 0;


#ifdef SPU1_USE_WAVEOUT

				HANDLE h = SPU::waveout_driver->eventHandle();

				DWORD res = MsgWaitForMultipleObjectsEx(
					1, &h,
					MilliSecsToWait,
					QS_ALLINPUT,
					MWMO_ALERTABLE
				);

				if (res == WAIT_OBJECT_0) {
					//cout << "\naudio event";

					SPU::waveout_driver->onAudioEvent();  // plays queued samples
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
				// if menu has been clicked then wait
				WindowClass::Window::WaitForModalMenuLoop();
			}

			// if menu was clicked, hop out of loop
			if (HandleMenuClick()) break;


			// check if we are running too slow
			//if ( CurrentTimer > TargetTimer )
			if (bRunningTooSlow)
			{
				// set the new timer target to be the current timer
				if (!QueryPerformanceCounter((LARGE_INTEGER*)&TargetTimer))
				{
					cout << "\nhps1x64: Error returned from QueryPerformanceCounter\n";
				}
			}

		}	// end for ( j = 0; j < ulFramesPerSec; j++ )


		// update all the debug info windows that are showing
		DebugWindow_Update();


		if (!_RunMode.RunNormal) cout << "\nWaiting for command\n";


		// get the speed as a percentage of full speed
		double dPerf;
		stringstream ss;
		//QueryPerformanceCounter ( (LARGE_INTEGER*) &ullPerfEnd_Timer );
		//TicksLeft = ullPerfEnd_Timer - ullPerfStart_Timer;
		//dPerf = ( ( dTicksPerMilliSec * 1000L ) / TicksLeft ) * 100L;
		dPerf = ((dTicksPerMilliSec * 1000L) / ullTicksUsedPerSecond) * 100L;
		if (_SYSTEM._GPU.GPU_CTRL_Read.VIDEO)
		{
			ss << "hps1x64 - Speed(PAL): " << dPerf << "%";
		}
		else
		{
			ss << "hps1x64 - Speed(NTSC): " << dPerf << "%";
		}

		if (_SYSTEM._GPU.bEnable_OpenCL)
		{
			ss << " Vulkan: Hardware";
		}
		else
		{
			if (_SYSTEM._GPU.iCurrentDrawLibrary == GPU::DRAW_LIBRARY_OPENGL)
			{
				ss << " OpenGL: Software";
			}
			else
			{
				ss << " Vulkan: Software";
			}
		}


#ifdef USE_NEW_PROGRAM_WINDOW

		m_prgwindow->SetCaption(ss.str().c_str());
#else

		ProgramWindow->SetCaption(ss.str().c_str());

#endif

	}	// end while (_RunMode.RunNormal)
}



void hps1x64::RunProgram_Normal_VirtualMachine()
{

	RunProgram_Normal();

}


int hps1x64::RunProgram ()
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
		Sleep ( 250 );
		
		// process events
		WindowClass::DoEvents ();

		HandleMenuClick ();
		
		if ( _RunMode.Exit ) break;

		// check if there is any debugging going on
		// this mode is only required if there are breakpoints set
		if ( _RunMode.RunDebug )
		{
			cout << "Running program in debug mode...\n";
			
			
#ifdef USE_NEW_PROGRAM_WINDOW

			m_prgwindow->SetCaption("hps1x64");
#else

			ProgramWindow->SetCaption("hps1x64");
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
					//cout << "\nk=" << k;
					cout << "\nWaiting for command\n";
				}
			}
		}

		// run program normally and without debugging
		if ( _RunMode.RunNormal )
		{

			//RunProgram_Normal();
			RunProgram_Normal_VirtualMachine();

		}	// end if ( _RunMode.RunNormal )
		
	}	// end while ( 1 )
	
	cout << "\nDone running program\n";


	// unload vulkan if it is enabled and hardware rendering is allowed //
	/*
	if (vulkan_is_loaded())
	{
		//vulkan_destroy();
		_SYSTEM._GPU.UnloadVulkan();
	}
	*/


	// write back memory cards
	_SYSTEM._SIO.Store_MemoryCardFile ( ExecutablePath + "card0", 0 );
	_SYSTEM._SIO.Store_MemoryCardFile ( ExecutablePath + "card1", 1 );
	
	cout << "\nSaving config...";

	// save configuration
	SaveConfig ( ExecutablePath + "hps1x64.hcfg" );

	// *TODO* destory all allocations, including vulkan //


	return 1;
}





void hps1x64::OnClick_File_Load_State ( int i )
{
	
	cout << "\nYou clicked File | Load | State\n";
	_HPS1X64.LoadState ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_File_Load_BIOS ( int i )
{
	
	cout << "\nYou clicked File | Load | BIOS\n";
	_HPS1X64.LoadBIOS ();

	cout << "\n->last bios set to: " << sLastBiosPath.c_str();

	_HPS1X64.Update_CheckMarksOnMenu();

	_MenuClick = 1;
}


void hps1x64::OnClick_File_Load_GameDisk ( int i )
{
	
	string ImagePath;
	bool bDiskOpened;
	
	cout << "\nYou clicked File | Load | Game Disk\n";
	
	if ( _HPS1X64._SYSTEM._CD.isLidOpen )
	{
		// lid is currently open //
		ImagePath = _HPS1X64.LoadDisk ();
		
		if ( ImagePath != "" )
		{
			bDiskOpened = _HPS1X64._SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );
		
			// *** testing ***
			/*
			DiskImage::CDImage::_DISKIMAGE->SeekTime ( 29, 17, 0 );
			DiskImage::CDImage::_DISKIMAGE->StartReading ();
			DiskImage::CDImage::_DISKIMAGE->ReadNextSector ();
			cout << hex << "\n\n" << (u32) ((DiskImage::CDImage::Sector::SubQ*)DiskImage::CDImage::_DISKIMAGE->CurrentSubBuffer)->AbsoluteAddress [ 0 ];
			cout << hex << "\n" << (u32) ((DiskImage::CDImage::Sector::SubQ*)DiskImage::CDImage::_DISKIMAGE->CurrentSubBuffer)->AbsoluteAddress [ 1 ];
			cout << hex << "\n" << (u32) ((DiskImage::CDImage::Sector::SubQ*)DiskImage::CDImage::_DISKIMAGE->CurrentSubBuffer)->AbsoluteAddress [ 2 ];
			cout << hex << "\n\n";
			*/

			if ( bDiskOpened )
			{
				cout << "\nhpsx64 NOTE: Game Disk opened successfully\n";
				_HPS1X64._SYSTEM._CD.isGameCD = true;
				
				// lid should now be closed since disk is open
				_HPS1X64._SYSTEM._CD.isLidOpen = false;
				
				_HPS1X64._SYSTEM._CD.Event_LidClose ();
				
				// output info for the loaded disk
				_HPS1X64._SYSTEM._CD.cd_image.Output_IndexData ();
				
				// *** testing *** output some test SubQ data
				/*
				_SYSTEM._CD.cd_image.Output_SubQData ( 0, 2, 0 );
				_SYSTEM._CD.cd_image.Output_SubQData ( 0, 2, 1 );
				_SYSTEM._CD.cd_image.Output_SubQData ( 3, 34, 29 );
				_SYSTEM._CD.cd_image.Output_SubQData ( 3, 34, 30 );
				_SYSTEM._CD.cd_image.Output_SubQData ( 3, 34, 31 );
				_SYSTEM._CD.cd_image.Output_SubQData ( 3, 32, 30 );
				_SYSTEM._CD.cd_image.Output_SubQData ( 3, 32, 31 );
				*/
				/*
				unsigned char AMin = 0, ASec = 0, AFrac = 0;
				cout << "\nTrack for 0,2,1=" << dec << _SYSTEM._CD.cd_image.FindTrack ( 0, 2, 1 );
				cout << "\nTrack for 0,2,0=" << dec << _SYSTEM._CD.cd_image.FindTrack ( 0, 2, 0 );
				cout << "\nTrack for 3,35,7=" << dec << _SYSTEM._CD.cd_image.FindTrack ( 3, 35, 7 );
				cout << "\nTrack for 47,0,0=" << dec << _SYSTEM._CD.cd_image.FindTrack ( 47, 0, 0 );
				_SYSTEM._CD.cd_image.GetTrackStart ( 1, AMin, ASec, AFrac );
				cout << "\nTrack 1 Starts At AMin=" << dec << (u32)AMin << " ASec=" << (u32)ASec << " AFrac=" << (u32)AFrac;
				_SYSTEM._CD.cd_image.GetTrackStart ( 2, AMin, ASec, AFrac );
				cout << "\nTrack 2 Starts At AMin=" << dec << (u32)AMin << " ASec=" << (u32)ASec << " AFrac=" << (u32)AFrac;
				_SYSTEM._CD.cd_image.GetTrackStart ( 3, AMin, ASec, AFrac );
				cout << "\nTrack 3 Starts At AMin=" << dec << (u32)AMin << " ASec=" << (u32)ASec << " AFrac=" << (u32)AFrac;
				_SYSTEM._CD.cd_image.GetTrackStart ( 10, AMin, ASec, AFrac );
				cout << "\nTrack 10 Starts At AMin=" << dec << (u32)AMin << " ASec=" << (u32)ASec << " AFrac=" << (u32)AFrac;
				*/
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
		_HPS1X64._SYSTEM._CD.isLidOpen = true;
		
		// close the currently open disk image
		_HPS1X64._SYSTEM._CD.cd_image.CloseDiskImage ();
		
		_HPS1X64._SYSTEM._CD.Event_LidOpen ();
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_File_Load_AudioDisk ( int i )
{
	string ImagePath;
	bool bDiskOpened;
	
	cout << "\nYou clicked File | Load | Audio Disk\n";
	
	if ( _HPS1X64._SYSTEM._CD.isLidOpen )
	{
		// lid is currently open //
		ImagePath = _HPS1X64.LoadDisk ();
		
		if ( ImagePath != "" )
		{
			bDiskOpened = _HPS1X64._SYSTEM._CD.cd_image.OpenDiskImage ( ImagePath );
		
			if ( bDiskOpened )
			{
				cout << "\nhpsx64 NOTE: Audio Disk opened successfully\n";
				_HPS1X64._SYSTEM._CD.isGameCD = false;
				
				// lid should now be closed since disk is open
				_HPS1X64._SYSTEM._CD.isLidOpen = false;
				
				_HPS1X64._SYSTEM._CD.Event_LidClose ();
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
		_HPS1X64._SYSTEM._CD.isLidOpen = true;
		
		// close the currently open disk image
		_HPS1X64._SYSTEM._CD.cd_image.CloseDiskImage ();
		
		_HPS1X64._SYSTEM._CD.Event_LidOpen ();
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}



void hps1x64::OnClick_File_Save_State ( int i )
{
	cout << "\nYou clicked File | Save | State\n";
	_HPS1X64.SaveState ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_File_Reset ( int i )
{
	
	cout << "\nYou clicked File | Reset\n";
	
	// need to call start, not reset
	_HPS1X64._SYSTEM.Start ();
	
	_MenuClick = 1;
}




void hps1x64::OnClick_File_Run ( int i )
{
	
	cout << "\nYou clicked File | Run\n";
	_HPS1X64._RunMode.Value = 0;
	
	// if there are no breakpoints, then we can run in normal mode
	if ( !_HPS1X64._SYSTEM._CPU.Breakpoints->Count() )
	{
		_HPS1X64._RunMode.RunNormal = true;
	}
	else
	{
		_HPS1X64._RunMode.RunDebug = true;
	}
	
	// clear the last breakpoint hit
	_HPS1X64._SYSTEM._CPU.Breakpoints->Clear_LastBreakPoint ();
	
	// clear read/write debugging info
	_HPS1X64._SYSTEM._CPU.Last_ReadAddress = 0;
	_HPS1X64._SYSTEM._CPU.Last_WriteAddress = 0;
	_HPS1X64._SYSTEM._CPU.Last_ReadWriteAddress = 0;
	
	_MenuClick = 1;
}


void hps1x64::OnClick_File_Exit ( int i )
{
	cout << "\nYou clicked File | Exit\n";
	
	// uuuuuuser chose to exit program
	_RunMode.Value = 0;
	_RunMode.Exit = true;
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Break ( int i )
{
	
	cout << "\nYou clicked Debug | Break\n";
	
	// clear the last breakpoint hit if system is running
	if  ( _RunMode.Value != 0 ) _HPS1X64._SYSTEM._CPU.Breakpoints->Clear_LastBreakPoint ();
	
	_RunMode.Value = 0;
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_StepInto ( int i )
{
	
	cout << "\nYou clicked Debug | Step Into\n";
	_HPS1X64._SYSTEM.Run ();
	
	// clear the last breakpoint hit
	_HPS1X64._SYSTEM._CPU.Breakpoints->Clear_LastBreakPoint ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_OutputCurrentSector ( int i )
{
	
	/*
	long *pData = (long*) & SIO::DJoy.gameControllerStates[0];
	
	SIO::DJoy.Update( 0 );
	
	cout << "\ngameControllerState: ";
	cout << "\nPOV0=" << hex << SIO::DJoy.gameControllerStates[0].rgdwPOV[0] << " " << dec << SIO::DJoy.gameControllerStates[0].rgdwPOV[0];
	cout << "\nPOV1=" << hex << SIO::DJoy.gameControllerStates[0].rgdwPOV[1] << " " << dec << SIO::DJoy.gameControllerStates[0].rgdwPOV[1];
	cout << "\nPOV2=" << hex << SIO::DJoy.gameControllerStates[0].rgdwPOV[2] << " " << dec << SIO::DJoy.gameControllerStates[0].rgdwPOV[2];
	cout << "\nPOV3=" << hex << SIO::DJoy.gameControllerStates[0].rgdwPOV[3] << " " << dec << SIO::DJoy.gameControllerStates[0].rgdwPOV[3];
	
	for ( int i = 0; i < 32; i++ )
	{
		cout << "\nPOV" << dec << i << "=" << hex << (unsigned long) SIO::DJoy.gameControllerStates[0].rgbButtons[i] << " " << dec << (unsigned long) SIO::DJoy.gameControllerStates[0].rgbButtons[i];
	}
	
	for ( int i = 0; i < 6; i++ )
	{
		cout << "\nAxis#" << dec << i << "=" << hex << *pData << " " << dec << *pData;
		pData++;
	}
	*/
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_All ( int i )
{
	cout << "\nYou clicked Debug | Show Window | All\n";
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_FrameBuffer ( int i )
{
	
	cout << "\nYou clicked Debug | Show Window | FrameBuffer\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW
	if (m_prgwindow->GetMenuItemChecked("debug|framebuffer"))
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for GPU\n";

		_HPS1X64._SYSTEM._GPU.DebugWindow_Disable();

		//ProgramWindow->Menus->UnCheckItem("Frame Buffer");
		m_prgwindow->SetMenuItemChecked("debug|framebuffer", false);
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for GPU\n";
		_HPS1X64._SYSTEM._GPU.DebugWindow_Enable();

		//ProgramWindow->Menus->UnCheckItem("Frame Buffer");
		m_prgwindow->SetMenuItemChecked("debug|framebuffer", true);
	}

#else
	if ( ProgramWindow->Menus->CheckItem ( "Frame Buffer" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for GPU\n";
		_HPS1X64._SYSTEM._GPU.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "Frame Buffer" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for GPU\n";
		_HPS1X64._SYSTEM._GPU.DebugWindow_Enable ();
	}
#endif

	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_R3000A ( int i )
{
	
	cout << "\nYou clicked Debug | Show Window | R3000A\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW

	if (m_prgwindow->GetMenuItemChecked("debug|r3000a"))
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for R3000A\n";
		_HPS1X64._SYSTEM._CPU.DebugWindow_Disable();

		//ProgramWindow->Menus->UnCheckItem("R3000A");
		m_prgwindow->SetMenuItemChecked("debug|r3000a", false);
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for R3000A\n";
		_HPS1X64._SYSTEM._CPU.DebugWindow_Enable();

		m_prgwindow->SetMenuItemChecked("debug|r3000a", true);
	}
#else
	if ( ProgramWindow->Menus->CheckItem ( "R3000A" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for R3000A\n";
		_HPS1X64._SYSTEM._CPU.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "R3000A" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for R3000A\n";
		_HPS1X64._SYSTEM._CPU.DebugWindow_Enable ();
	}
#endif
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_Memory ( int i )
{
	
	cout << "\nYou clicked Debug | Show Window | Memory\n";
	
#ifdef USE_NEW_PROGRAM_WINDOW

	if (m_prgwindow->GetMenuItemChecked("debug|memory"))
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for Bus\n";
		_HPS1X64._SYSTEM._BUS.DebugWindow_Disable();

		//ProgramWindow->Menus->UnCheckItem("Memory");
		m_prgwindow->SetMenuItemChecked("debug|memory", false);
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for Bus\n";
		_HPS1X64._SYSTEM._BUS.DebugWindow_Enable();

		m_prgwindow->SetMenuItemChecked("debug|memory", true);
	}
#else
	if ( ProgramWindow->Menus->CheckItem ( "Memory" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		cout << "Disabling debug window for Bus\n";
		_HPS1X64._SYSTEM._BUS.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "Memory" );
	}
	else
	{
		// was not already checked, so enable debugging
		cout << "Enabling debug window for Bus\n";
		_HPS1X64._SYSTEM._BUS.DebugWindow_Enable ();
	}
#endif
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_DMA ( int i )
{
	
	cout << "\nYou clicked Debug | Show Window | DMA\n";

#ifdef USE_NEW_PROGRAM_WINDOW

	if (m_prgwindow->GetMenuItemChecked("debug|dma"))
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._DMA.DebugWindow_Disable();

		//ProgramWindow->Menus->UnCheckItem("DMA");
		m_prgwindow->SetMenuItemChecked("debug|dma", false);
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._DMA.DebugWindow_Enable();

		m_prgwindow->SetMenuItemChecked("debug|dma", true);
	}
#else
	if ( ProgramWindow->Menus->CheckItem ( "DMA" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._DMA.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "DMA" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._DMA.DebugWindow_Enable ();
	}
#endif
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_TIMER ( int i )
{
	cout << "\nYou clicked Debug | Show Window | Timers\n";

#ifdef USE_NEW_PROGRAM_WINDOW

	if (m_prgwindow->GetMenuItemChecked("debug|timers"))
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._TIMERS.DebugWindow_Disable();

		//ProgramWindow->Menus->UnCheckItem("Timers");
		m_prgwindow->SetMenuItemChecked("debug|timers", false);
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._TIMERS.DebugWindow_Enable();

		m_prgwindow->SetMenuItemChecked("debug|timers", true);
	}
#else
	if ( ProgramWindow->Menus->CheckItem ( "Timers" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._TIMERS.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "Timers" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._TIMERS.DebugWindow_Enable ();
	}
#endif
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_SPU ( int i )
{
	cout << "\nYou clicked Debug | Show Window | SPU\n";

#ifdef USE_NEW_PROGRAM_WINDOW

	if (m_prgwindow->GetMenuItemChecked("debug|spu"))
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._SPU.DebugWindow_Disable();

		//ProgramWindow->Menus->UnCheckItem("SPU");
		m_prgwindow->SetMenuItemChecked("debug|spu", false);
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._SPU.DebugWindow_Enable();

		m_prgwindow->SetMenuItemChecked("debug|spu", true);
	}
#else
	if ( ProgramWindow->Menus->CheckItem ( "SPU" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._SPU.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "SPU" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._SPU.DebugWindow_Enable ();
	}
#endif
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_CD ( int i )
{
	cout << "\nYou clicked Debug | Show Window | CD\n";

#ifdef USE_NEW_PROGRAM_WINDOW

	if (m_prgwindow->GetMenuItemChecked("debug|cd"))
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._CD.DebugWindow_Disable();

		//ProgramWindow->Menus->UnCheckItem("CD");
		m_prgwindow->SetMenuItemChecked("debug|cd", false);
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._CD.DebugWindow_Enable();
		
		m_prgwindow->SetMenuItemChecked("debug|cd", true);
	}
#else
	if ( ProgramWindow->Menus->CheckItem ( "CD" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._CD.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "CD" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._CD.DebugWindow_Enable ();
	}
#endif
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Debug_Show_INTC ( int i )
{
	cout << "\nYou clicked Debug | Show Window | INTC\n";

#ifdef USE_NEW_PROGRAM_WINDOW

	if (m_prgwindow->GetMenuItemChecked("debug|intc"))
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._INTC.DebugWindow_Disable();

		//ProgramWindow->Menus->UnCheckItem("INTC");
		m_prgwindow->SetMenuItemChecked("debug|intc", false);
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._INTC.DebugWindow_Enable();

		m_prgwindow->SetMenuItemChecked("debug|intc", true);
	}
#else
	if ( ProgramWindow->Menus->CheckItem ( "INTC" ) == MF_CHECKED )
	{
		// was already checked, so disable debug window for R3000A and uncheck item
		_HPS1X64._SYSTEM._INTC.DebugWindow_Disable ();
		ProgramWindow->Menus->UnCheckItem ( "INTC" );
	}
	else
	{
		// was not already checked, so enable debugging
		_HPS1X64._SYSTEM._INTC.DebugWindow_Enable ();
	}
#endif
	
	_MenuClick = 1;
}


void hps1x64::OnClick_Debug_Console_PS1_DMA_READ(int i)
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

void hps1x64::OnClick_Debug_Console_PS1_DMA_WRITE(int i)
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
		Playstation1::Dma::bEnableVerboseDebugWrite = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Dma::bEnableVerboseDebugWrite = true;
	}
}

void hps1x64::OnClick_Debug_Console_PS1_INTC_READ(int i)
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

void hps1x64::OnClick_Debug_Console_PS1_INTC_WRITE(int i)
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
		Playstation1::Intc::bEnableVerboseDebugWrite = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Intc::bEnableVerboseDebugWrite = true;
	}
}

void hps1x64::OnClick_Debug_Console_PS1_TIMER_READ(int i)
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

void hps1x64::OnClick_Debug_Console_PS1_TIMER_WRITE(int i)
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
		Playstation1::Timers::bEnableVerboseDebugWrite = false;
		cout << "\n*** DEBUG TO CONSOLE END: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
	}
	else
	{
		m_prgwindow->SetMenuItemChecked(CONTROL_NAME, true);
		cout << "\n*** DEBUG TO CONSOLE START: " << DEVICE_SYSTEM << DEVICE_NAME << DEVICE_ACTION;
		Playstation1::Timers::bEnableVerboseDebugWrite = true;
	}
}


void hps1x64::OnClick_Debug_SaveConsole(int i)
{
	CaptureConsoleToFile("console_output.txt");
}



void hps1x64::LoadGamepadConfig(int deviceIndex)
{

	m_gamepadConfig.buttonMappings["X"] = _HPS1X64._SYSTEM._SIO.Key_X[deviceIndex];
	m_gamepadConfig.buttonMappings["Circle"] = _HPS1X64._SYSTEM._SIO.Key_O[deviceIndex];
	m_gamepadConfig.buttonMappings["Triangle"] = _HPS1X64._SYSTEM._SIO.Key_Triangle[deviceIndex];
	m_gamepadConfig.buttonMappings["Square"] = _HPS1X64._SYSTEM._SIO.Key_Square[deviceIndex];
	m_gamepadConfig.buttonMappings["L1"] = _HPS1X64._SYSTEM._SIO.Key_L1[deviceIndex];  // Left shoulder
	m_gamepadConfig.buttonMappings["R1"] = _HPS1X64._SYSTEM._SIO.Key_R1[deviceIndex];  // Right shoulder
	m_gamepadConfig.buttonMappings["Select"] = _HPS1X64._SYSTEM._SIO.Key_Select[deviceIndex];
	m_gamepadConfig.buttonMappings["Start"] = _HPS1X64._SYSTEM._SIO.Key_Start[deviceIndex];
	m_gamepadConfig.buttonMappings["L3"] = _HPS1X64._SYSTEM._SIO.Key_L3[deviceIndex];  // Left stick click
	m_gamepadConfig.buttonMappings["R3"] = _HPS1X64._SYSTEM._SIO.Key_R3[deviceIndex];  // Right stick click
	m_gamepadConfig.buttonMappings["L2"] = _HPS1X64._SYSTEM._SIO.Key_L2[deviceIndex]; // Left trigger (as button)
	m_gamepadConfig.buttonMappings["R2"] = _HPS1X64._SYSTEM._SIO.Key_R2[deviceIndex]; // Right trigger (as button)

	// Set up axis mappings
	m_gamepadConfig.leftStickXAxis = _HPS1X64._SYSTEM._SIO.LeftAnalog_X[deviceIndex];
	m_gamepadConfig.leftStickYAxis = _HPS1X64._SYSTEM._SIO.LeftAnalog_Y[deviceIndex];
	m_gamepadConfig.rightStickXAxis = _HPS1X64._SYSTEM._SIO.RightAnalog_X[deviceIndex];
	m_gamepadConfig.rightStickYAxis = _HPS1X64._SYSTEM._SIO.RightAnalog_Y[deviceIndex];
	m_gamepadConfig.leftTriggerAxis = 2;
	m_gamepadConfig.rightTriggerAxis = 5;

	// Default to device 0
	m_gamepadConfig.deviceIndex = deviceIndex;
	m_gamepadConfig.deviceName = "";

}

void hps1x64::ApplyGamepadConfig(int deviceIndex)
{
	_HPS1X64._SYSTEM._SIO.Key_X[deviceIndex] = m_gamepadConfig.buttonMappings["X"];
	_HPS1X64._SYSTEM._SIO.Key_O[deviceIndex] = m_gamepadConfig.buttonMappings["Circle"];
	_HPS1X64._SYSTEM._SIO.Key_Triangle[deviceIndex] = m_gamepadConfig.buttonMappings["Triangle"];
	_HPS1X64._SYSTEM._SIO.Key_Square[deviceIndex] = m_gamepadConfig.buttonMappings["Square"];
	_HPS1X64._SYSTEM._SIO.Key_L1[deviceIndex] = m_gamepadConfig.buttonMappings["L1"];  // Left shoulder
	_HPS1X64._SYSTEM._SIO.Key_R1[deviceIndex] = m_gamepadConfig.buttonMappings["R1"];  // Right shoulder
	_HPS1X64._SYSTEM._SIO.Key_Select[deviceIndex] = m_gamepadConfig.buttonMappings["Select"];
	_HPS1X64._SYSTEM._SIO.Key_Start[deviceIndex] = m_gamepadConfig.buttonMappings["Start"];
	_HPS1X64._SYSTEM._SIO.Key_L3[deviceIndex] = m_gamepadConfig.buttonMappings["L3"];  // Left stick click
	_HPS1X64._SYSTEM._SIO.Key_R3[deviceIndex] = m_gamepadConfig.buttonMappings["R3"];  // Right stick click
	_HPS1X64._SYSTEM._SIO.Key_L2[deviceIndex] = m_gamepadConfig.buttonMappings["L2"]; // Left trigger (as button)
	_HPS1X64._SYSTEM._SIO.Key_R2[deviceIndex] = m_gamepadConfig.buttonMappings["R2"]; // Right trigger (as button)

	// Set up axis mappings
	_HPS1X64._SYSTEM._SIO.LeftAnalog_X[deviceIndex] = m_gamepadConfig.leftStickXAxis;
	_HPS1X64._SYSTEM._SIO.LeftAnalog_Y[deviceIndex] = m_gamepadConfig.leftStickYAxis;
	_HPS1X64._SYSTEM._SIO.RightAnalog_X[deviceIndex] = m_gamepadConfig.rightStickXAxis;
	_HPS1X64._SYSTEM._SIO.RightAnalog_Y[deviceIndex] = m_gamepadConfig.rightStickYAxis;
	m_gamepadConfig.leftTriggerAxis = 2;
	m_gamepadConfig.rightTriggerAxis = 5;

	// Default to device 0
	m_gamepadConfig.deviceIndex = deviceIndex;
	m_gamepadConfig.deviceName = "";

}


void hps1x64::OnClick_Controllers0_Configure ( int i )
{
	cout << "\nYou clicked Controllers | Configure...\n";
	
#ifdef USE_OLD_PAD_CONFIG

	Dialog_KeyConfigure::KeyConfigure [ 0 ] = _HPS1X64._SYSTEM._SIO.Key_X [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 1 ] = _HPS1X64._SYSTEM._SIO.Key_O [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 2 ] = _HPS1X64._SYSTEM._SIO.Key_Triangle [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 3 ] = _HPS1X64._SYSTEM._SIO.Key_Square [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 4 ] = _HPS1X64._SYSTEM._SIO.Key_R1 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 5 ] = _HPS1X64._SYSTEM._SIO.Key_R2 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 6 ] = _HPS1X64._SYSTEM._SIO.Key_R3 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 7 ] = _HPS1X64._SYSTEM._SIO.Key_L1 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 8 ] = _HPS1X64._SYSTEM._SIO.Key_L2 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 9 ] = _HPS1X64._SYSTEM._SIO.Key_L3 [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 10 ] = _HPS1X64._SYSTEM._SIO.Key_Start [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 11 ] = _HPS1X64._SYSTEM._SIO.Key_Select [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 12 ] = _HPS1X64._SYSTEM._SIO.LeftAnalog_X [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 13 ] = _HPS1X64._SYSTEM._SIO.LeftAnalog_Y [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 14 ] = _HPS1X64._SYSTEM._SIO.RightAnalog_X [ 0 ];
	Dialog_KeyConfigure::KeyConfigure [ 15 ] = _HPS1X64._SYSTEM._SIO.RightAnalog_Y [ 0 ];
#else

	LoadGamepadConfig(0);

#endif
	
	SIO::gamepad.RefreshDevices();

	// make sure there is a game controller connected before showing dialog
	if (SIO::gamepad.GetDeviceCount())
	{
#ifdef USE_OLD_PAD_CONFIG

		if (Dialog_KeyConfigure::Show_ConfigureKeysDialog(0))
		{
			_HPS1X64._SYSTEM._SIO.Key_X[0] = Dialog_KeyConfigure::KeyConfigure[0];
			_HPS1X64._SYSTEM._SIO.Key_O[0] = Dialog_KeyConfigure::KeyConfigure[1];
			_HPS1X64._SYSTEM._SIO.Key_Triangle[0] = Dialog_KeyConfigure::KeyConfigure[2];
			_HPS1X64._SYSTEM._SIO.Key_Square[0] = Dialog_KeyConfigure::KeyConfigure[3];
			_HPS1X64._SYSTEM._SIO.Key_R1[0] = Dialog_KeyConfigure::KeyConfigure[4];
			_HPS1X64._SYSTEM._SIO.Key_R2[0] = Dialog_KeyConfigure::KeyConfigure[5];
			_HPS1X64._SYSTEM._SIO.Key_R3[0] = Dialog_KeyConfigure::KeyConfigure[6];
			_HPS1X64._SYSTEM._SIO.Key_L1[0] = Dialog_KeyConfigure::KeyConfigure[7];
			_HPS1X64._SYSTEM._SIO.Key_L2[0] = Dialog_KeyConfigure::KeyConfigure[8];
			_HPS1X64._SYSTEM._SIO.Key_L3[0] = Dialog_KeyConfigure::KeyConfigure[9];
			_HPS1X64._SYSTEM._SIO.Key_Start[0] = Dialog_KeyConfigure::KeyConfigure[10];
			_HPS1X64._SYSTEM._SIO.Key_Select[0] = Dialog_KeyConfigure::KeyConfigure[11];
			_HPS1X64._SYSTEM._SIO.LeftAnalog_X[0] = Dialog_KeyConfigure::KeyConfigure[12];
			_HPS1X64._SYSTEM._SIO.LeftAnalog_Y[0] = Dialog_KeyConfigure::KeyConfigure[13];
			_HPS1X64._SYSTEM._SIO.RightAnalog_X[0] = Dialog_KeyConfigure::KeyConfigure[14];
			_HPS1X64._SYSTEM._SIO.RightAnalog_Y[0] = Dialog_KeyConfigure::KeyConfigure[15];
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
			SIO::gamepad, // Pass our existing gamepad handler
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
		cout << "\n*** hps1x64: *ERROR* no game controller detected!!! ***\n";
	}
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Controllers1_Configure ( int i )
{
	cout << "\nYou clicked Controllers | Configure...\n";
	
#ifdef USE_OLD_PAD_CONFIG

	Dialog_KeyConfigure::KeyConfigure [ 0 ] = _HPS1X64._SYSTEM._SIO.Key_X [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 1 ] = _HPS1X64._SYSTEM._SIO.Key_O [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 2 ] = _HPS1X64._SYSTEM._SIO.Key_Triangle [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 3 ] = _HPS1X64._SYSTEM._SIO.Key_Square [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 4 ] = _HPS1X64._SYSTEM._SIO.Key_R1 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 5 ] = _HPS1X64._SYSTEM._SIO.Key_R2 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 6 ] = _HPS1X64._SYSTEM._SIO.Key_R3 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 7 ] = _HPS1X64._SYSTEM._SIO.Key_L1 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 8 ] = _HPS1X64._SYSTEM._SIO.Key_L2 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 9 ] = _HPS1X64._SYSTEM._SIO.Key_L3 [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 10 ] = _HPS1X64._SYSTEM._SIO.Key_Start [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 11 ] = _HPS1X64._SYSTEM._SIO.Key_Select [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 12 ] = _HPS1X64._SYSTEM._SIO.LeftAnalog_X [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 13 ] = _HPS1X64._SYSTEM._SIO.LeftAnalog_Y [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 14 ] = _HPS1X64._SYSTEM._SIO.RightAnalog_X [ 1 ];
	Dialog_KeyConfigure::KeyConfigure [ 15 ] = _HPS1X64._SYSTEM._SIO.RightAnalog_Y [ 1 ];
#else

	LoadGamepadConfig(1);
#endif
	
	// first make sure that there are joysticks connected
	SIO::gamepad.RefreshDevices();

	// make sure there is a game controller connected before showing dialog
	if (SIO::gamepad.GetDeviceCount())
	{
#ifdef USE_OLD_PAD_CONFIG
		if (Dialog_KeyConfigure::Show_ConfigureKeysDialog(1))
		{
			_HPS1X64._SYSTEM._SIO.Key_X[1] = Dialog_KeyConfigure::KeyConfigure[0];
			_HPS1X64._SYSTEM._SIO.Key_O[1] = Dialog_KeyConfigure::KeyConfigure[1];
			_HPS1X64._SYSTEM._SIO.Key_Triangle[1] = Dialog_KeyConfigure::KeyConfigure[2];
			_HPS1X64._SYSTEM._SIO.Key_Square[1] = Dialog_KeyConfigure::KeyConfigure[3];
			_HPS1X64._SYSTEM._SIO.Key_R1[1] = Dialog_KeyConfigure::KeyConfigure[4];
			_HPS1X64._SYSTEM._SIO.Key_R2[1] = Dialog_KeyConfigure::KeyConfigure[5];
			_HPS1X64._SYSTEM._SIO.Key_R3[1] = Dialog_KeyConfigure::KeyConfigure[6];
			_HPS1X64._SYSTEM._SIO.Key_L1[1] = Dialog_KeyConfigure::KeyConfigure[7];
			_HPS1X64._SYSTEM._SIO.Key_L2[1] = Dialog_KeyConfigure::KeyConfigure[8];
			_HPS1X64._SYSTEM._SIO.Key_L3[1] = Dialog_KeyConfigure::KeyConfigure[9];
			_HPS1X64._SYSTEM._SIO.Key_Start[1] = Dialog_KeyConfigure::KeyConfigure[10];
			_HPS1X64._SYSTEM._SIO.Key_Select[1] = Dialog_KeyConfigure::KeyConfigure[11];
			_HPS1X64._SYSTEM._SIO.LeftAnalog_X[1] = Dialog_KeyConfigure::KeyConfigure[12];
			_HPS1X64._SYSTEM._SIO.LeftAnalog_Y[1] = Dialog_KeyConfigure::KeyConfigure[13];
			_HPS1X64._SYSTEM._SIO.RightAnalog_X[1] = Dialog_KeyConfigure::KeyConfigure[14];
			_HPS1X64._SYSTEM._SIO.RightAnalog_Y[1] = Dialog_KeyConfigure::KeyConfigure[15];
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
			SIO::gamepad, // Pass our existing gamepad handler
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
		cout << "\n*** hps1x64: *ERROR* no game controller detected!!! ***\n";
	}
	
	_MenuClick = 1;
}



void hps1x64::OnClick_Pad1Type_Digital ( int i )
{
	// set pad 1 to digital
	_HPS1X64._SYSTEM._SIO.ControlPad_Type [ 0 ] = 0;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Pad1Type_Analog ( int i )
{
	// set pad 1 to analog
	_HPS1X64._SYSTEM._SIO.ControlPad_Type [ 0 ] = 1;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Pad1Input_None ( int i )
{
	_HPS1X64._SYSTEM._SIO.PortMapping [ 0 ] = -1;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}
void hps1x64::OnClick_Pad1Input_Device0 ( int i )
{
	_HPS1X64._SYSTEM._SIO.PortMapping [ 0 ] = 0;
	
	if ( _HPS1X64._SYSTEM._SIO.PortMapping [ 1 ] == 0 )
	{
		_HPS1X64._SYSTEM._SIO.PortMapping [ 1 ] = -1;
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}
void hps1x64::OnClick_Pad1Input_Device1 ( int i )
{
	_HPS1X64._SYSTEM._SIO.PortMapping [ 0 ] = 1;
	
	if ( _HPS1X64._SYSTEM._SIO.PortMapping [ 1 ] == 1 )
	{
		_HPS1X64._SYSTEM._SIO.PortMapping [ 1 ] = -1;
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_Pad2Type_Digital ( int i )
{
	// set pad 2 to digital
	_HPS1X64._SYSTEM._SIO.ControlPad_Type [ 1 ] = 0;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Pad2Type_Analog ( int i )
{
	// set pad 2 to analog
	_HPS1X64._SYSTEM._SIO.ControlPad_Type [ 1 ] = 1;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Pad2Input_None ( int i )
{
	_HPS1X64._SYSTEM._SIO.PortMapping [ 1 ] = -1;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}
void hps1x64::OnClick_Pad2Input_Device0 ( int i )
{
	_HPS1X64._SYSTEM._SIO.PortMapping [ 1 ] = 0;
	
	if ( _HPS1X64._SYSTEM._SIO.PortMapping [ 0 ] == 0 )
	{
		_HPS1X64._SYSTEM._SIO.PortMapping [ 0 ] = -1;
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}
void hps1x64::OnClick_Pad2Input_Device1 ( int i )
{
	_HPS1X64._SYSTEM._SIO.PortMapping [ 1 ] = 1;
	
	if ( _HPS1X64._SYSTEM._SIO.PortMapping [ 0 ] == 1 )
	{
		_HPS1X64._SYSTEM._SIO.PortMapping [ 0 ] = -1;
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_Card1_Connect ( int i )
{
	// set memory card 1 to connected
	_HPS1X64._SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ] = 0;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Card1_Disconnect ( int i )
{
	// set memory card 1 to disconnected
	_HPS1X64._SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ] = 1;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_Redetect_Pads ( int i )
{
	// set memory card 1 to disconnected
	SIO::gamepad.RefreshDevices();
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_Card2_Connect ( int i )
{
	// set memory card 2 to connected
	_HPS1X64._SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ] = 0;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Card2_Disconnect ( int i )
{
	// set memory card 2 to disconnected
	_HPS1X64._SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ] = 1;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_Region_Europe ( int i )
{
	_HPS1X64._SYSTEM._CD.Region = 'E';
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Region_Japan ( int i )
{
	_HPS1X64._SYSTEM._CD.Region = 'I';
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Region_NorthAmerica ( int i )
{
	_HPS1X64._SYSTEM._CD.Region = 'A';
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Enable ( int i )
{
	if ( _HPS1X64._SYSTEM._SPU.AudioOutput_Enabled )
	{
		_HPS1X64._SYSTEM._SPU.AudioOutput_Enabled = false;
	}
	else
	{
		_HPS1X64._SYSTEM._SPU.AudioOutput_Enabled = true;
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Volume_100 ( int i )
{
	_HPS1X64._SYSTEM._SPU.GlobalVolume = 0x7fff;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Volume_75 ( int i )
{
	_HPS1X64._SYSTEM._SPU.GlobalVolume = 0x3000;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Volume_50 ( int i )
{
	_HPS1X64._SYSTEM._SPU.GlobalVolume = 0x1000;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Volume_25 ( int i )
{
	_HPS1X64._SYSTEM._SPU.GlobalVolume = 0x400;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Buffer_8k ( int i )
{
	_HPS1X64._SYSTEM._SPU.NextPlayBuffer_Size = 8192;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Buffer_16k ( int i )
{
	_HPS1X64._SYSTEM._SPU.NextPlayBuffer_Size = 16384;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Buffer_32k ( int i )
{
	_HPS1X64._SYSTEM._SPU.NextPlayBuffer_Size = 32768;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Buffer_64k ( int i )
{
	_HPS1X64._SYSTEM._SPU.NextPlayBuffer_Size = 65536;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Buffer_1m ( int i )
{
	_HPS1X64._SYSTEM._SPU.NextPlayBuffer_Size = 131072;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}

void hps1x64::OnClick_Audio_Filter ( int i )
{
	cout << "\nYou clicked Audio | Filter\n";
	
	if ( _HPS1X64._SYSTEM._SPU.AudioFilter_Enabled )
	{
		_HPS1X64._SYSTEM._SPU.AudioFilter_Enabled = false;
	}
	else
	{
		_HPS1X64._SYSTEM._SPU.AudioFilter_Enabled = true;
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
	
	_MenuClick = 1;
}


void hps1x64::OnClick_Video_FullScreen ( int i )
{
	cout << "\nYou clicked Video | FullScreen\n";
	
	GPU::MainProgramWindow_Width = (long) ( ((float)ProgramWindow_Width) * 1.0f );
	GPU::MainProgramWindow_Height = (long) ( ((float)ProgramWindow_Height) * 1.0f );

#ifdef USE_NEW_PROGRAM_WINDOW

	if (!m_prgwindow->IsFullscreen())
	{
		m_prgwindow->SetClientSize(GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height);
	}

	m_prgwindow->ToggleFullscreen();

#else
	if( ! ProgramWindow->fullscreen )
	{
		ProgramWindow->SetWindowSize ( GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height );

		// update screen size in vulkan
		vulkan_set_screen_size(GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height);

		if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
		{
			if (!vulkan_create_swap_chain())
			{
				std::cout << "\nHPS1: ERROR: Problem creating swap chain.\n";
				_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
				_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
				_HPS1X64._SYSTEM._GPU.UnloadVulkan();
			}
			else if (!vulkan_record_default_commands())
			{
				std::cout << "\nHPS1: ERROR: Problem recording commands for new swap chain.\n";
				_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
				_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
				_HPS1X64._SYSTEM._GPU.UnloadVulkan();
			}
		}
	}

	ProgramWindow->ToggleGLFullScreen();
#endif
	
	
	_MenuClick = 1;

	_HPS1X64.Update_CheckMarksOnMenu();
}


void hps1x64::OnClick_Video_DisplayMode_Stretch(int i)
{
	m_prgwindow->SetDisplayMode(OpenGLWindow::DisplayMode::STRETCH);
	m_prgwindow->SetMenuItemChecked("displaymode|stretch", true);
	m_prgwindow->SetMenuItemChecked("displaymode|fit", false);
}

void hps1x64::OnClick_Video_DisplayMode_Fit(int i)
{
	m_prgwindow->SetDisplayMode(OpenGLWindow::DisplayMode::FIT);
	m_prgwindow->SetMenuItemChecked("displaymode|stretch", false);
	m_prgwindow->SetMenuItemChecked("displaymode|fit", true);
}


void hps1x64::OnClick_Video_WindowSizeX1 ( int i )
{
	GPU::MainProgramWindow_Width = (long) ( ((float)ProgramWindow_Width) * 1.0f );
	GPU::MainProgramWindow_Height = (long) ( ((float)ProgramWindow_Height) * 1.0f );

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetClientSize(GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height);
#else

	ProgramWindow->SetWindowSize ( GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height );
#endif

	// update screen size in vulkan
	/*
	vulkan_set_screen_size(GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height);

	if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		if (!vulkan_create_swap_chain())
		{
			std::cout << "\nHPS1: ERROR: Problem creating swap chain.\n";
			_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS1X64._SYSTEM._GPU.UnloadVulkan();
		}
		else if (!vulkan_record_default_commands())
		{
			std::cout << "\nHPS1: ERROR: Problem recording commands for new swap chain.\n";
			_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS1X64._SYSTEM._GPU.UnloadVulkan();
		}
	}
	*/
}

void hps1x64::OnClick_Video_WindowSizeX15 ( int i )
{
	GPU::MainProgramWindow_Width = (long) ( ((float)ProgramWindow_Width) * 1.5f );
	GPU::MainProgramWindow_Height = (long) ( ((float)ProgramWindow_Height) * 1.5f );

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetClientSize(GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height);
#else

	ProgramWindow->SetWindowSize ( GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height );
#endif

	// update screen size in vulkan
	/*
	vulkan_set_screen_size(GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height);

	if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		if (!vulkan_create_swap_chain())
		{
			std::cout << "\nHPS1: ERROR: Problem creating swap chain.\n";
			_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS1X64._SYSTEM._GPU.UnloadVulkan();
		}
		else if (!vulkan_record_default_commands())
		{
			std::cout << "\nHPS1: ERROR: Problem recording commands for new swap chain.\n";
			_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS1X64._SYSTEM._GPU.UnloadVulkan();
		}
	}
	*/

	_MenuClick = 1;

	_HPS1X64.Update_CheckMarksOnMenu();
}

void hps1x64::OnClick_Video_WindowSizeX2 ( int i )
{
	GPU::MainProgramWindow_Width = (long) ( ((float)ProgramWindow_Width) * 2.0f );
	GPU::MainProgramWindow_Height = (long) ( ((float)ProgramWindow_Height) * 2.0f );

#ifdef USE_NEW_PROGRAM_WINDOW

	m_prgwindow->SetClientSize(GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height);
#else

	ProgramWindow->SetWindowSize ( GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height );
#endif

	// update screen size in vulkan
	/*
	vulkan_set_screen_size(GPU::MainProgramWindow_Width, GPU::MainProgramWindow_Height);

	if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		if (!vulkan_create_swap_chain())
		{
			std::cout << "\nHPS1: ERROR: Problem creating swap chain.\n";
			_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS1X64._SYSTEM._GPU.UnloadVulkan();
		}
		else if (!vulkan_record_default_commands())
		{
			std::cout << "\nHPS1: ERROR: Problem recording commands for new swap chain.\n";
			_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
			_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
			_HPS1X64._SYSTEM._GPU.UnloadVulkan();
		}
	}
	*/

	_MenuClick = 1;

	_HPS1X64.Update_CheckMarksOnMenu();
}



void hps1x64::OnClick_Video_Renderer_Software_OpenGL(int i)
{
	cout << "\nYou clicked Video | Renderer | Software | OpenGL\n";

	// if vulkan is loaded, then can't switch back to OpenGL yet
	/*
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
	if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// copy frame buffer from hardware to software if hardware rendering is allowed
		if (_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			// copy VRAM from gpu hardware to software VRAM
			_HPS1X64._SYSTEM._GPU.Copy_VRAM_toCPU();

			// unload vulkan when not using it
			_HPS1X64._SYSTEM._GPU.UnloadVulkan();

			cout << "\nHPSX64: INFO: *** VULKAN UNLOADED ***\n";
		}
	}
	*/

	_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
	_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
	_HPS1X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation1::GPU::DRAW_LIBRARY_OPENGL;

	_MenuClick = 1;

	_HPS1X64.Update_CheckMarksOnMenu();
}


void hps1x64::OnClick_Video_Renderer_Software_Vulkan(int i)
{
	cout << "\nYou clicked Video | Renderer | Software | Vulkan\n";

	/*
	// if vulkan is not loaded, then load it
	if (!vulkan_is_loaded())
	{
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
	if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// copy frame buffer from hardware to software if hardware rendering is allowed
		if (_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			// copy VRAM from gpu hardware to software VRAM
			_HPS1X64._SYSTEM._GPU.Copy_VRAM_toCPU();

		}
	}

	_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
	_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering = false;
	_HPS1X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation1::GPU::DRAW_LIBRARY_VULKAN;
	*/

	_MenuClick = 1;

	_HPS1X64.Update_CheckMarksOnMenu();
}


void hps1x64::OnClick_Video_Renderer_Hardware_Vulkan(int i)
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

	// check if the commands for running gfx on hardware and displaying to the screen are recorded
	if (!vulkan_is_compute_draw_recorded())
	{
		cout << "\nhpsx64: Commands to compute graphics and display them are not recorded.";

		if (!vulkan_record_default_commands())
		{
			cout << "\nhpsx64: ERROR: Problem recording commands.";
			return;
		}

	}

	cout << "\nhpsx64: Commands to compute graphics and display them are recorded now.";


	//if (_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
	{
		// if previously rendering on cpu, copy vram to gpu hardware
		if (!_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
		{
			_HPS1X64._SYSTEM._GPU.Copy_VRAM_toGPU();

		}
	}

	_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = true;
	_HPS1X64._SYSTEM._GPU.iCurrentDrawLibrary = Playstation1::GPU::DRAW_LIBRARY_VULKAN;
	*/

	_MenuClick = 1;

	_HPS1X64.Update_CheckMarksOnMenu();
}



void hps1x64::OnClick_Video_ScanlinesEnable ( int i )
{
	cout << "\nYou clicked Video | Scanlines | Enable\n";
	
	_HPS1X64._SYSTEM._GPU.Set_Scanline ( true );
	
	_MenuClick = 1;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
}

void hps1x64::OnClick_Video_ScanlinesDisable ( int i )
{
	cout << "\nYou clicked Video | Scanlines | Disable\n";
	
	_HPS1X64._SYSTEM._GPU.Set_Scanline ( false );
	
	_MenuClick = 1;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
}

void hps1x64::OnClick_R3000ACPU_Interpreter ( int i )
{
	cout << "\nYou clicked CPU | R3000A | Interpreter\n";

	_HPS1X64._SYSTEM._CPU.bEnableRecompiler = false;
	_HPS1X64._SYSTEM._CPU.Status.uDisableRecompiler = true;
	
	_HPS1X64.Update_CheckMarksOnMenu ();
}

void hps1x64::OnClick_R3000ACPU_Recompiler ( int i )
{
	cout << "\nYou clicked CPU | R3000A | Recompiler\n";

	_HPS1X64._SYSTEM._CPU.bEnableRecompiler = true;
	_HPS1X64._SYSTEM._CPU.Status.uDisableRecompiler = false;

	// need to reset the recompiler
	_HPS1X64._SYSTEM._CPU.rs->Reset ();
	
	_HPS1X64._SYSTEM._CPU.rs->SetOptimizationLevel ( 1 );
	
	_HPS1X64.Update_CheckMarksOnMenu ();
}

void hps1x64::OnClick_R3000ACPU_Recompiler2 ( int i )
{
	cout << "\nYou clicked CPU | R3000A | Recompiler2\n";

	_HPS1X64._SYSTEM._CPU.bEnableRecompiler = true;
	_HPS1X64._SYSTEM._CPU.Status.uDisableRecompiler = false;

	// need to reset the recompiler
	_HPS1X64._SYSTEM._CPU.rs->Reset ();

	_HPS1X64._SYSTEM._CPU.rs->SetOptimizationLevel ( 2 );
	
	_HPS1X64.Update_CheckMarksOnMenu ();
}



void hps1x64::OnClick_GPU_0Threads ( int i )
{
	cout << "\nYou clicked GPU | GPU: Threads | 0 threads\n";
	
	if ( _HPS1X64._SYSTEM._GPU.ulNumberOfThreads )
	{
		_HPS1X64._SYSTEM._GPU.Finish();
		_HPS1X64._SYSTEM._GPU.ulNumberOfThreads = 0;
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
}

void hps1x64::OnClick_GPU_1Threads ( int i )
{
	cout << "\nYou clicked GPU | GPU: Threads | 1 threads\n";
	
	if ( ! _HPS1X64._SYSTEM._GPU.ulNumberOfThreads )
	{
		_HPS1X64._SYSTEM._GPU.Finish();
		_HPS1X64._SYSTEM._GPU.ulNumberOfThreads = 1;
	}
	
	_HPS1X64.Update_CheckMarksOnMenu ();
}


void hps1x64::OnClick_GPU_Software ( int i )
{
	if( _HPS1X64._SYSTEM._GPU.bEnable_OpenCL )
	{
		// copy vram from gpu into cpu ram
		_HPS1X64._SYSTEM._GPU.Copy_VRAM_toCPU();

		// make the change
		_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = 0;
	}

	_HPS1X64.Update_CheckMarksOnMenu ();
}

void hps1x64::OnClick_GPU_Hardware ( int i )
{
	if( !_HPS1X64._SYSTEM._GPU.bEnable_OpenCL )
	{
		// copy vram from cpu ram into gpu
		_HPS1X64._SYSTEM._GPU.Copy_VRAM_toGPU();

		// make the change
		_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = 1;
	}

	_HPS1X64.Update_CheckMarksOnMenu ();
}



void hps1x64::SaveState ( string FilePath )
{
#ifdef INLINE_DEBUG
	debug << "\r\nEntered function: System::SaveState";
#endif

	static const char* PathToSaveState = "SaveState.hps1";
	
	// make sure cd is not reading asynchronously??
	_SYSTEM._CD.cd_image.WaitForAllReadsComplete ();

	// save virtual state for R3000A and BUS
	_SYSTEM._CPU.SaveVirtualState();
	_SYSTEM._BUS.SaveVirtualState();

#ifdef ALLOW_PS1_HWRENDER
	// if previously on hardware renderer, then copy framebuffer
	if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// copy frame buffer from hardware to software if hardware rendering is allowed
		if (_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			// copy VRAM from gpu hardware to software VRAM
			_HPS1X64._SYSTEM._GPU.Copy_VRAM_toCPU();
		}
	}
#endif


	////////////////////////////////////////////////////////
	// We need to prompt for the file to save state to
	if ( !FilePath.compare ( "" ) )
	{
#ifdef USE_NEW_PROGRAM_WINDOW
		FilePath = m_prgwindow->ShowSaveFileDialog("Save PS1 State:");
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
	//while ( _SYSTEM._CD.game_cd_image.isReadInProgress );
	while ( _SYSTEM._CD.cd_image.isReadInProgress );

	// write entire state into memory
	//OutputFile.write ( (char*) this, sizeof( System ) );
	OutputFile.write ( (char*) &_SYSTEM, sizeof( System ) );
	
	OutputFile.close();
	
	cout << "Done Saving state.\n";
	
#ifdef INLINE_DEBUG
	debug << "->Leaving function: System::SaveState";
#endif
}

void hps1x64::LoadState ( string FilePath )
{
#ifdef INLINE_DEBUG
	debug << "\r\nEntered function: System::LoadState";
#endif

	static const char* PathToSaveState = "SaveState.hps1";

	// make sure cd is not reading asynchronously??
	_SYSTEM._CD.cd_image.WaitForAllReadsComplete ();
	
	////////////////////////////////////////////////////////
	// We need to prompt for the file to save state to
	if ( !FilePath.compare( "" ) )
	{
#ifdef USE_NEW_PROGRAM_WINDOW

		FilePath = m_prgwindow->ShowOpenFileDialog("Open PS1 State file:");
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


#ifdef ALLOW_PS1_HWRENDER

	if (_HPS1X64._SYSTEM._GPU.bEnable_OpenCL)
	{
		// if currently rendering on gpu, copy loaded vram to gpu hardware
		if (_HPS1X64._SYSTEM._GPU.bAllowGpuHardwareRendering)
		{
			_HPS1X64._SYSTEM._GPU.Copy_VRAM_toGPU();
		}
		else
		{
			// the hardware does not allow this particular compute shader for some reason
			_HPS1X64._SYSTEM._GPU.bEnable_OpenCL = false;
		}
	}

#endif

	
	// refresh system (reload static pointers)
	_SYSTEM.Refresh ();

	// restore virtual state for R3000A and BUS
	_SYSTEM._CPU.RestoreVirtualState();
	_SYSTEM._BUS.RestoreVirtualState();


	cout << "Done Loading state.\n";


#ifdef INLINE_DEBUG
	debug << "->Leaving function: System::LoadState";
#endif
}


void hps1x64::LoadBIOS ( string FilePath )
{
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
		
		// set the path for the bios
		BiosPath = FilePath;
		
		// setting path as the last bios loaded into memory
		sLastBiosPath = FilePath;

	}
	
	
	cout << "LoadBIOS done.\n";

	//UpdateDebugWindow ();
	
	//DebugStatus.LoadBios = false;
}


string hps1x64::LoadDisk ( string FilePath )
{
	cout << "Loading Disk.\n";
	
	////////////////////////////////////////////////////////
	// We need to prompt for the TEST program to run
	if ( !FilePath.compare ( "" ) )
	{
		cout << "Prompting for DISK file.\n";

#ifdef USE_NEW_PROGRAM_WINDOW

		FilePath = m_prgwindow->ShowOpenFileDialog("Open DISK file:", "CD Image (cue/bin/ccd/img/chd)\0*.cue;*.bin;*.ccd;*.img;*.chd\0");
#else

		FilePath = ProgramWindow->ShowFileOpenDialog ("CD Image (cue/bin/ccd/img/chd)\0*.cue;*.bin;*.ccd;*.img;*.chd\0");
#endif
	}
	
	
	cout << "LoadDisk done.\n";
	

	return FilePath;
}


// create config file object
//Config::File cfg;


void hps1x64::LoadConfig ( string ConfigFileName )
{

	// read a JSON file
	std::ifstream i( ConfigFileName );

	if ( !i )
	{
		cout << "\nhps1x64::LoadConfig: Problem loading config file: " << ConfigFileName;
		return;
	}

	json jsonSettings;
	i >> jsonSettings;

	// save the controller settings //

	// pad 1 //

	_SYSTEM._SIO.ControlPad_Type [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "DigitalAnalog" ];

	_SYSTEM._SIO.Key_X [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyX" ];
	_SYSTEM._SIO.Key_O [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyO" ];
	_SYSTEM._SIO.Key_Triangle [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyTriangle" ];
	_SYSTEM._SIO.Key_Square [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeySquare" ];
	_SYSTEM._SIO.Key_R1 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR1" ];
	_SYSTEM._SIO.Key_R2 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR2" ];
	_SYSTEM._SIO.Key_R3 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR3" ];
	_SYSTEM._SIO.Key_L1 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL1" ];
	_SYSTEM._SIO.Key_L2 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL2" ];
	_SYSTEM._SIO.Key_L3 [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL3" ];
	_SYSTEM._SIO.Key_Start [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyStart" ];
	_SYSTEM._SIO.Key_Select [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeySelect" ];
	_SYSTEM._SIO.LeftAnalog_X [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyLeftAnalogX" ];
	_SYSTEM._SIO.LeftAnalog_Y [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyLeftAnalogY" ];
	_SYSTEM._SIO.RightAnalog_X [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyRightAnalogX" ];
	_SYSTEM._SIO.RightAnalog_Y [ 0 ] = jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyRightAnalogY" ];

	// pad 2 //

	_SYSTEM._SIO.ControlPad_Type [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "DigitalAnalog" ];

	_SYSTEM._SIO.Key_X [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyX" ];
	_SYSTEM._SIO.Key_O [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyO" ];
	_SYSTEM._SIO.Key_Triangle [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyTriangle" ];
	_SYSTEM._SIO.Key_Square [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeySquare" ];
	_SYSTEM._SIO.Key_R1 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR1" ];
	_SYSTEM._SIO.Key_R2 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR2" ];
	_SYSTEM._SIO.Key_R3 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR3" ];
	_SYSTEM._SIO.Key_L1 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL1" ];
	_SYSTEM._SIO.Key_L2 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL2" ];
	_SYSTEM._SIO.Key_L3 [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL3" ];
	_SYSTEM._SIO.Key_Start [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyStart" ];
	_SYSTEM._SIO.Key_Select [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeySelect" ];
	_SYSTEM._SIO.LeftAnalog_X [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyLeftAnalogX" ];
	_SYSTEM._SIO.LeftAnalog_Y [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyLeftAnalogY" ];
	_SYSTEM._SIO.RightAnalog_X [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyRightAnalogX" ];
	_SYSTEM._SIO.RightAnalog_Y [ 1 ] = jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyRightAnalogY" ];

	// memory cards //

	_SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ] = jsonSettings [ "Controllers" ] [ "MemoryCard1" ] [ "Disconnected" ];
	_SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ] = jsonSettings [ "Controllers" ] [ "MemoryCard2" ] [ "Disconnected" ];

	// device settings //

	// CD //

	_SYSTEM._CD.Region = jsonSettings [ "CD" ] [ "Region" ];

	// SPU //

	_SYSTEM._SPU.AudioOutput_Enabled = jsonSettings [ "SPU" ] [ "Enable_AudioOutput" ];
	_SYSTEM._SPU.AudioFilter_Enabled = jsonSettings [ "SPU" ] [ "Enable_Filter" ];
	_SYSTEM._SPU.NextPlayBuffer_Size = jsonSettings [ "SPU" ] [ "BufferSize" ];
	_SYSTEM._SPU.GlobalVolume = jsonSettings [ "SPU" ] [ "GlobalVolume" ];

	// CPU //

	_SYSTEM._CPU.bEnableRecompiler = jsonSettings [ "CPU" ] [ "R3000A" ] [ "EnableRecompiler" ];

	_SYSTEM._CPU.Status.uDisableRecompiler = (!_SYSTEM._CPU.bEnableRecompiler) ? 1 : 0;

	// GPU //

	_SYSTEM._GPU.Set_Scanline (  jsonSettings [ "GPU" ] [ "ScanlineEnable" ] );
	_SYSTEM._GPU.ulNumberOfThreads = jsonSettings [ "GPU" ] [ "Threads" ];

	// Interface //

	sLastBiosPath = jsonSettings["Interface"]["LastBiosPath"];

	// should probably also go ahead and make an attempt to load the last bios file here also if it exists
	if (sLastBiosPath.compare(""))
	{
		LoadBIOS(sLastBiosPath);
	}

}


void hps1x64::SaveConfig ( string ConfigFileName )
{

	// save configuration as json (using nlohmann json)
	json jsonSettings;


	// save the controller settings //

	// pad 1 //

	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "DigitalAnalog" ] = _SYSTEM._SIO.ControlPad_Type [ 0 ];

	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyX" ] = _SYSTEM._SIO.Key_X [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyO" ] = _SYSTEM._SIO.Key_O [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyTriangle" ] = _SYSTEM._SIO.Key_Triangle [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeySquare" ] = _SYSTEM._SIO.Key_Square [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR1" ] = _SYSTEM._SIO.Key_R1 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR2" ] = _SYSTEM._SIO.Key_R2 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyR3" ] = _SYSTEM._SIO.Key_R3 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL1" ] = _SYSTEM._SIO.Key_L1 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL2" ] = _SYSTEM._SIO.Key_L2 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyL3" ] = _SYSTEM._SIO.Key_L3 [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyStart" ] = _SYSTEM._SIO.Key_Start [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeySelect" ] = _SYSTEM._SIO.Key_Select [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyLeftAnalogX" ] = _SYSTEM._SIO.LeftAnalog_X [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyLeftAnalogY" ] = _SYSTEM._SIO.LeftAnalog_Y [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyRightAnalogX" ] = _SYSTEM._SIO.RightAnalog_X [ 0 ];
	jsonSettings [ "Controllers" ] [ "Pad1" ] [ "KeyRightAnalogY" ] = _SYSTEM._SIO.RightAnalog_Y [ 0 ];

	// pad 2 //

	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "DigitalAnalog" ] = _SYSTEM._SIO.ControlPad_Type [ 1 ];

	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyX" ] = _SYSTEM._SIO.Key_X [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyO" ] = _SYSTEM._SIO.Key_O [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyTriangle" ] = _SYSTEM._SIO.Key_Triangle [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeySquare" ] = _SYSTEM._SIO.Key_Square [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR1" ] = _SYSTEM._SIO.Key_R1 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR2" ] = _SYSTEM._SIO.Key_R2 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyR3" ] = _SYSTEM._SIO.Key_R3 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL1" ] = _SYSTEM._SIO.Key_L1 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL2" ] = _SYSTEM._SIO.Key_L2 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyL3" ] = _SYSTEM._SIO.Key_L3 [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyStart" ] = _SYSTEM._SIO.Key_Start [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeySelect" ] = _SYSTEM._SIO.Key_Select [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyLeftAnalogX" ] = _SYSTEM._SIO.LeftAnalog_X [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyLeftAnalogY" ] = _SYSTEM._SIO.LeftAnalog_Y [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyRightAnalogX" ] = _SYSTEM._SIO.RightAnalog_X [ 1 ];
	jsonSettings [ "Controllers" ] [ "Pad2" ] [ "KeyRightAnalogY" ] = _SYSTEM._SIO.RightAnalog_Y [ 1 ];

	// memory cards //

	jsonSettings [ "Controllers" ] [ "MemoryCard1" ] [ "Disconnected" ] = _SYSTEM._SIO.MemoryCard_ConnectionState [ 0 ];
	jsonSettings [ "Controllers" ] [ "MemoryCard2" ] [ "Disconnected" ] = _SYSTEM._SIO.MemoryCard_ConnectionState [ 1 ];

	// device settings //

	// CD //

	jsonSettings [ "CD" ] [ "Region" ] = _SYSTEM._CD.Region;

	// SPU //

	jsonSettings [ "SPU" ] [ "Enable_AudioOutput" ] = _SYSTEM._SPU.AudioOutput_Enabled;
	jsonSettings [ "SPU" ] [ "Enable_Filter" ] = _SYSTEM._SPU.AudioFilter_Enabled;
	jsonSettings [ "SPU" ] [ "BufferSize" ] = _SYSTEM._SPU.NextPlayBuffer_Size;
	jsonSettings [ "SPU" ] [ "GlobalVolume" ] = _SYSTEM._SPU.GlobalVolume;

	// CPU //

	jsonSettings [ "CPU" ] [ "R3000A" ] [ "EnableRecompiler" ] = _SYSTEM._CPU.bEnableRecompiler;

	// GPU //

	jsonSettings [ "GPU" ] [ "ScanlineEnable" ] = _SYSTEM._GPU.Get_Scanline ();
	jsonSettings [ "GPU" ] [ "Threads" ] = _SYSTEM._GPU.ulNumberOfThreads;

	// Interface //

	jsonSettings["Interface"]["LastBiosPath"] = sLastBiosPath;


	// write the settings to file as json //

	std::ofstream o( ConfigFileName );
	o << std::setw(4) << jsonSettings << std::endl;	

}



void hps1x64::DebugWindow_Update ()
{
	// can't do anything if they've clicked on the menu
	WindowClass::Window::WaitForModalMenuLoop ();
	
	_SYSTEM._CPU.DebugWindow_Update ();
	_SYSTEM._BUS.DebugWindow_Update ();
	_SYSTEM._DMA.DebugWindow_Update ();
	_SYSTEM._TIMERS.DebugWindow_Update ();
	_SYSTEM._SPU.DebugWindow_Update ();
	_SYSTEM._GPU.DebugWindow_Update ();
	_SYSTEM._CD.DebugWindow_Update ();
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
	static const char* Dialog_Caption = "Configure Keys";
	static const int Dialog_Id = 0x6000;
	static const int Dialog_X = 10;
	static const int Dialog_Y = 10;

	static const char* Label1_Caption = "Instructions: Hold down the button on the joypad, and then click the PS button you want to assign it to (while still holding the button down). For analog sticks, hold the stick in that direction (x or y) and then click on the button to assign that axis.";
	static const int Label1_Id = 0x6001;
	static const int Label1_X = 10;
	static const int Label1_Y = 10;
	static const int Label1_Width = 300;
	static const int Label1_Height = 100;
	
	static const int c_iButtonArea_StartId = 0x6100;
	static const int c_iButtonArea_StartX = 10;
	static const int c_iButtonArea_StartY = Label1_Y + Label1_Height + 10;
	static const int c_iButtonArea_ButtonHeight = 20;
	static const int c_iButtonArea_ButtonWidth = 100;
	static const int c_iButtonArea_ButtonPitch = c_iButtonArea_ButtonHeight + 5;

	static const int c_iLabelArea_StartId = 0x6200;
	static const int c_iLabelArea_StartX = c_iButtonArea_StartX + c_iButtonArea_ButtonWidth + 10;
	static const int c_iLabelArea_StartY = c_iButtonArea_StartY;
	static const int c_iLabelArea_LabelHeight = c_iButtonArea_ButtonHeight;
	static const int c_iLabelArea_LabelWidth = 100;
	static const int c_iLabelArea_LabelPitch = c_iLabelArea_LabelHeight + 5;

	
	static const char* CmdButtonOk_Caption = "OK";
	static const int CmdButtonOk_Id = 0x6300;
	static const int CmdButtonOk_X = 10;
	static const int CmdButtonOk_Y = c_iButtonArea_StartY + ( c_iButtonArea_ButtonPitch * c_iDialog_NumberOfButtons ) + 10;
	static const int CmdButtonOk_Width = 50;
	static const int CmdButtonOk_Height = 20;
	
	static const char* CmdButtonCancel_Caption = "Cancel";
	static const int CmdButtonCancel_Id = 0x6400;
	static const int CmdButtonCancel_X = CmdButtonOk_X + CmdButtonOk_Width + 10;
	static const int CmdButtonCancel_Y = CmdButtonOk_Y;
	static const int CmdButtonCancel_Width = 50;
	static const int CmdButtonCancel_Height = 20;
	
	// now set width and height of dialog
	static const int Dialog_Width = Label1_Width + 20;	//c_iLabelArea_StartX + c_iLabelArea_LabelWidth + 10;
	static const int Dialog_Height = CmdButtonOk_Y + CmdButtonOk_Height + 30;
		
	static const char* PS1_Keys [] = { "X", "O", "Triangle", "Square", "R1", "R2", "R3", "L1", "L2", "L3", "Start", "Select", "Left Analog X", "Left Analog Y", "Right Analog X", "Right Analog Y" };
	static const char* Axis_Labels [] = { "Axis X", "Axis Y", "Axis Z", "Axis R", "Axis U", "Axis V" };
	
	bool ret = true;
	int iKeyIdx;
	
	//u32 *Key_X, *Key_O, *Key_Triangle, *Key_Square, *Key_R1, *Key_R2, *Key_R3, *Key_L1, *Key_L2, *Key_L3, *Key_Start, *Key_Select, *LeftAnalogX, *LeftAnalogY, *RightAnalogX, *RightAnalogY;
	//u32* Key_Pointers [ c_iDialog_NumberOfButtons ];

	stringstream ss;
	
	/*
#ifndef USE_NEW_PROGRAM_WINDOW
	Joysticks j;
#endif
	

	//if ( !isDialogShowing )
	//{
		//x64ThreadSafe::Utilities::Lock_Exchange32 ( (long&) isDialogShowing, true );

		// set the events to use on call back
		//OnClick_Ok = _OnClick_Ok;
		//OnClick_Cancel = _OnClick_Cancel;
		
		// disable parent window
		cout << "\nDisable parent window";

#ifdef USE_NEW_PROGRAM_WINDOW

		hps1x64::m_prgwindow->SetEnabled(false);
#else

		hps1x64::ProgramWindow->Disable();
#endif


		// this is the value list that was double clicked on
		// now show a window where the variable can be modified
		// *note* setting the parent to the list-view control
		cout << "\nAllocating dialog";
		wDialog = new WindowClass::Window ();
		
		cout << "\nCreating dialog";

#ifdef USE_NEW_PROGRAM_WINDOW

		wDialog->Create(Dialog_Caption, Dialog_X, Dialog_Y, Dialog_Width, Dialog_Height, WindowClass::Window::DefaultStyle, NULL, hps1x64::m_prgwindow->GetHandle());
#else

		wDialog->Create ( Dialog_Caption, Dialog_X, Dialog_Y, Dialog_Width, Dialog_Height, WindowClass::Window::DefaultStyle, NULL, hps1x64::ProgramWindow->hWnd );
#endif


		wDialog->DisableCloseButton ();
		
		
		//cout << "\nCreating static control";
		InfoLabel = new WindowClass::Static ();
		InfoLabel->Create_Text ( wDialog, Label1_X, Label1_Y, Label1_Width, Label1_Height, (char*) Label1_Caption, Label1_Id );
		
		// create the buttons and labels
		cout << "\nAdding buttons and labels.";
		for ( int i = 0; i < c_iDialog_NumberOfButtons; i++ )
		{
			
			// put in a static label for entering a new value
			KeyLabels [ i ] = new WindowClass::Static ();
			KeyLabels [ i ]->Create_Text ( wDialog, c_iLabelArea_StartX, c_iLabelArea_StartY + ( i * c_iLabelArea_LabelPitch ), c_iLabelArea_LabelWidth, c_iLabelArea_LabelHeight, "test" , c_iLabelArea_StartId + i );

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
		

		if ( _HPS1X64._SYSTEM._SIO.PortMapping [ iPadNum ] == -1 )
		{
			_HPS1X64._SYSTEM._SIO.PortMapping [ iPadNum ] = iPadNum;
		}
		
		while ( ButtonClick != CmdButtonOk_Id && ButtonClick != CmdButtonCancel_Id )
		{
			Sleep ( 16 );
			WindowClass::DoEvents ();
			
			SIO::gamepad.Update();
			
			//if ( ButtonClick != CmdButtonOk_Id && ButtonClick != CmdButtonCancel_Id && ButtonClick != 0 )
			//{
				if ( ( ButtonClick & 0xff00 ) == c_iButtonArea_StartId )
				{
					
					if ( ( ButtonClick & 0xff ) < 12 )
					{
						// get index of key that is pressed
						for ( iKeyIdx = 0; iKeyIdx < 32; iKeyIdx++ )
						{
							if (SIO::gamepad.GetRawDeviceState(iPadNum).rgbButtons[iKeyIdx])
							{
								break;
							}
						}
						
						
						if ( iKeyIdx < 32 )
						{
							cout << "key pressed id:" << dec << iKeyIdx;
							KeyConfigure [ ButtonClick & 0xff ] = iKeyIdx;
						}
					}
					else
					{
						
						u32 axis_value [ 6 ];
						u32 max_index;
						
						long* pData = (long*)&SIO::gamepad.GetRawDeviceState(iPadNum);
						
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
		

		delete wDialog;


#ifdef USE_NEW_PROGRAM_WINDOW

		hps1x64::m_prgwindow->SetEnabled(true);
#else
		hps1x64::ProgramWindow->Enable ();
#endif
		*/

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
#ifdef ENABLE_DIRECT_INPUT
			ss << "Button#" << KeyConfigure [ i ];
#else
			// label is for button //
			ss << "Button#" << bit_scan_lsb ( KeyConfigure [ i ] );
#endif
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


