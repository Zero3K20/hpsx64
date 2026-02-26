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

//#pragma once
#ifndef __PS1_GPU_H__
#define __PS1_GPU_H__



#include "types.h"
#include "Debug.h"

#include "OpenGLWindow.hpp"

#include "DebugValueList.h"

//#include <GL/glew.h>

#include "WinApiHandler.h"
#include "GNUAsmUtility_x64.h"


//#include "opencl_compute.h"


//#define DRAW_MULTIPLE_PIXELS

//#include "emmintrin.h"

//#define _ENABLE_SSE2_NONTEMPLATE

//#define _ENABLE_SSE2_TRIANGLE_MONO
//#define _ENABLE_SSE2_TRIANGLE_GRADIENT
//#define _ENABLE_SSE2_RECTANGLE_MONO


#ifdef _ENABLE_SSE2

// need to include this file to use SSE2 intrinsics
#include "emmintrin.h"
//#include "smmintrin.h"

#endif


using namespace x64Asm::Utilities;


//#define ENABLE_DRAW_OVERHEAD

#ifndef PS2_COMPILE

// allow opencl code
#define ALLOW_OPENCL_PS1

#endif


// enables templates for PS2 GPU
// this is enabled/disabled via makefile
//#define USE_PS1_GPU_TEMPLATES


#ifdef USE_PS1_GPU_TEMPLATES

#define USE_TEMPLATES_PS1_LINE
#define USE_TEMPLATES_PS1_RECTANGLE
#define USE_TEMPLATES_PS1_TRIANGLE

#endif





namespace Playstation1
{

	class GPU
	{
	
		static Debug::Log debug;
		static GPU *_GPU;
		
		static WindowClass::Window *DisplayOutput_Window;
		static WindowClass::Window *FrameBuffer_DebugWindow;
	
		static OpenGLWindow* m_displaywindow;

	public:
		// GPU input buffer is 64 bytes (16 words)
	
		//////////////////////////
		//	General Parameters	//
		//////////////////////////
		
		// where the registers start at
		static const uint32_t Regs_Start = 0x1f801810;
		
		// where the registers end at
		static const uint32_t Regs_End = 0x1f801814;
	
		// distance between groups of registers
		static const uint32_t Reg_Size = 0x4;
		

		/*
		// need to pack lots of info into the structure for debugging and read/write of hardware registers
		struct HW_Register
		{
			bool ReadOK;
			bool WriteOK;
			bool Unknown;
			char* Name;
			u32 Address;
			u32 SizeInBytes;
			u32* DataPtr;
		};
		HW_Register Registers [ Regs_End - Regs_Start + Reg_Size ];
		*/

		
		// GPU Clock Speed in Hertz
		static const uint32_t c_iClockSpeed = 53222400;
		static constexpr double c_dClockSpeed = 53222400.0L;

		// the number of gpu cycles for every cpu cycle or vice versa
		static constexpr double c_dGPUPerCPU_Cycles = ( 11.0L / 7.0L );
		static constexpr double c_dCPUPerGPU_Cycles = ( 7.0L / 11.0L );
		
		static const u64 c_BIAS = ( 1ULL << 31 );
		static const u64 c_BIAS24 = ( 1ULL << 23 );
		static inline s64 _Round ( s64 FixedPointValue )
		{
			return ( FixedPointValue + c_BIAS );
		}
		
		static inline s64 _Round24 ( s64 FixedPointValue )
		{
			return ( FixedPointValue + c_BIAS24 );
		}
		
		
		
		
		static inline u64 _Abs ( s64 Value )
		{
			return ( ( Value >> 63 ) ^ Value ) - ( Value >> 63 );
		}
		
		static inline u32 _Abs ( s32 Value )
		{
			return ( ( Value >> 31 ) ^ Value ) - ( Value >> 31 );
		}


		// store the draw library currently in use
		enum { DRAW_LIBRARY_NONE = 0, DRAW_LIBRARY_OPENGL, DRAW_LIBRARY_VULKAN };
		static int iCurrentDrawLibrary;

		static bool FlipScreen_Vulkan(void* pSrcBuffer32, int iSrcBufWidth, int iSrcBufHeight, int iDstWinWidth, int iDstWinHeight);
		static bool FlipScreen_OpenGL(void* pSrcBuffer32, int iSrcBufWidth, int iSrcBufHeight, int iDstWinWidth, int iDstWinHeight);
		
		// need to know if device buffer is full
		// note: it appears that the gpu accepts the next command while it is processing the current one
		// note: not sure if it also does this for lines and triangles
		bool bIsBufferFull;
		
		// index for next event
		u32 NextEvent_Idx;
		
		// cycle that the next event will happen at for this device
		u64 NextEvent_Cycle, NextEvent_Cycle_Vsync;

		// the cycle that device is busy until
		u64 BusyUntil_Cycle;

		// used to buffer pixels before drawing to the screen
#ifdef PS2_COMPILE
		static constexpr const int iPixelBuffer_Width = 1;
		static constexpr const int iPixelBuffer_Height = 1;
#else
		static constexpr const int iPixelBuffer_Width = 1024;
		static constexpr const int iPixelBuffer_Height = 1024;	// 512;
#endif
		alignas(32) u32 PixelBuffer [iPixelBuffer_Width * iPixelBuffer_Height];
		
		// size of the main program window
		static u32 MainProgramWindow_Width;
		static u32 MainProgramWindow_Height;
		
		// maximum width/height of a polygon allowed
		static const s32 c_MaxPolygonWidth = 1023;
		static const s32 c_MaxPolygonHeight = 511;

		static u32 Read ( u32 Address );
		static void Write ( u32 Address, u32 Data, u32 Mask );
		
		void DMA_Read ( u32* Data, int ByteReadCount );
		void DMA_Write ( u32* Data, int ByteWriteCount );
		static u32 DMA_WriteBlock ( u32* pMemory, u32 Address, u32 BS );	//( u32* Data, u32 BS );
		static u32 DMA_ReadBlock ( u32* pMemory, u32 Address, u32 BS );
		
		static u64 DMA_ReadyForRead ( void );
		static u64 DMA_ReadyForWrite ( void );
		
		bool LoadVulkan();
		void UnloadVulkan();

		void Start ();

		// returns either vblank interrupt signal, gpu interrupt signal, or no interrupt signal
		void Run ();
		void Reset ();
		
		// need to specify what window to display graphical output to (this should be the main program window)
		// *** TODO *** probably also need to call this whenever the main program window size changes
		void SetDisplayOutputWindow ( u32 width, u32 height, WindowClass::Window* DisplayOutput );
		void SetDisplayOutputWindow2(u32 width, u32 height, OpenGLWindow* DisplayOutput);

		void Draw_Screen ();
		void Draw_FrameBuffer ();

		void Copy_FrameData ();


		void Copy_VRAM_toGPU ();
		void Copy_VRAM_toCPU ();
		
		double GetCycles_SinceLastPixel ();
		double GetCycles_SinceLastHBlank ();
		double GetCycles_SinceLastVBlank ();
		double GetCycles_ToNextPixel ();
		double GetCycles_ToNextHBlank ();
		double GetCycles_ToNextVBlank ();

		double GetCycles_SinceLastPixel ( double dAtCycle );
		double GetCycles_SinceLastHBlank ( double dAtCycle );
		double GetCycles_SinceLastVBlank ( double dAtCycle );
		double GetCycles_ToNextPixel ( double dAtCycle );
		double GetCycles_ToNextHBlank ( double dAtCycle );
		double GetCycles_ToNextVBlank ( double dAtCycle );
		
		double GetCycles_ToNextScanlineStart ();
		double GetCycles_ToNextFieldStart ();
		double GetCycles_SinceLastScanlineStart ();
		double GetCycles_SinceLastFieldStart ();
		
		double GetCycles_ToNextScanlineStart ( double dAtCycle );
		double GetCycles_ToNextFieldStart ( double dAtCycle );
		double GetCycles_SinceLastScanlineStart ( double dAtCycle );
		double GetCycles_SinceLastFieldStart ( double dAtCycle );
		
		
		// gets the total number of scanlines since start of program
		u64 GetScanline_Count ();
		
		// gets the scanline number you are on (for ntsc that would be 0-524, for pal 0-624)
		u64 GetScanline_Number ();
		
		// get cycle# that scanline starts at
		double GetScanline_Start ();
		
		bool isHBlank ();
		bool isVBlank ();
		
		bool isHBlank ( double dAtCycle );
		bool isVBlank ( double dAtCycle );
		
		// get the next event (vblank)
		void GetNextEvent ();
		void GetNextEvent_V ();
		void Update_NextEvent ();
		
		void SetNextEvent ( u64 CycleOffset );
		void SetNextEvent_Cycle ( u64 Cycle );
		void Update_NextEventCycle ();
		
				
		//////////////////////////////////
		//	Device Specific Parameters	//
		//////////////////////////////////
		
		static const u32 GPU_VERSION = 2;
		
		// GPU Data/Command register
		static const uint32_t GPU_DATA = 0x1f801810;

		
		// this is the timer for syncing to 50/60 fps
		// platform dependent, so will only be here temporarily
		u64 VideoTimerStart, VideoTimerStop;
		
		// cycle timings used for drawing (very important on a PS1)
		
		// *** no alpha blending *** //
		static constexpr double dFrameBufferRectangle_02_CyclesPerPixel = 0.5;
		static constexpr double dMoveImage_80_CyclesPerPixel = 1.0;
		
		static constexpr double dMonoTriangle_20_CyclesPerPixel = 0.5;
		static constexpr double dGradientTriangle_30_CyclesPerPixel = 0.6;
		static constexpr double dTextureTriangle4_24_CyclesPerPixel = 0.9;	//3.38688;
		static constexpr double dTextureTriangle8_24_CyclesPerPixel = 1.0;	//6.77376;
		static constexpr double dTextureTriangle16_24_CyclesPerPixel = 1.1;	//11.2896;
		static constexpr double dTextureTriangle4_34_Gradient_CyclesPerPixel = 1.0;	//4.8384;
		static constexpr double dTextureTriangle8_34_Gradient_CyclesPerPixel = 1.1;	//9.6768;
		static constexpr double dTextureTriangle16_34_Gradient_CyclesPerPixel = 1.2;	//18.816;
		static constexpr double dSprite4_64_Cycles = 1.0;
		static constexpr double dSprite8_64_Cycles = 1.1;
		static constexpr double dSprite16_64_Cycles = 1.2;
		static constexpr double dCyclesPerSprite8x8_74_4bit = 64.0;
		static constexpr double dCyclesPerSprite8x8_74_8bit = 64.0;
		static constexpr double dCyclesPerSprite8x8_74_16bit = 64.0;
		static constexpr double dCyclesPerSprite16x16_7c_4bit = 256.0;
		static constexpr double dCyclesPerSprite16x16_7c_8bit = 256.0;
		static constexpr double dCyclesPerSprite16x16_7c_16bit = 256.0;

		// *** alpha blending *** //
		static constexpr double dAlphaBlending_CyclesPerPixel = 0.5;
		
		// *** brightness calculation *** //
		static constexpr double dBrightnessCalculation_CyclesPerPixel = 0.5;
		
		
		// this is the result from a query command sent to the GPU_CTRL register
		u32 GPU_DATA_Read;
		
		union DATA_Write_Format
		{
			// Command | BGR
			struct
			{
				// Color / Shading info
				
				// bits 0-7
				u8 Red;
				
				// bits 8-15
				u8 Green;
				
				// bits 16-23
				u8 Blue;
				
				// the command for the packet
				// bits 24-31
				u8 Command;
			};
			
			// y | x
			struct
			{
				// 16-bit values of y and x in the frame buffer
				// these look like they are signed
				
				// bits 0-10 - x-coordinate
				//s16 x;
				s32 x : 11;
				
				// bits 11-15 - not used
				s32 NotUsed0 : 5;
				
				// bits 16-26 - y-coordinate
				//s16 y;
				s32 y : 11;
				
				// bits 27-31 - not used
				s32 NotUsed1 : 5;
			};
			
			struct
			{
				u8 u;
				u8 v;
				u16 filler11;
			};
			
			// clut | v | u
			struct
			{
				u16 filler13;
				
				struct
				{
					// x-coordinate x/16
					// bits 0-5
					u16 x : 6;
					
					// y-coordinate 0-511
					// bits 6-14
					u16 y : 9;
					
					// bit 15 - Unknown/Unused (should be 0)
					u16 unknown0 : 1;
				};
			} clut;
			
			// h | w
			struct
			{
				u16 w;
				u16 h;
			};
			
			// tpage | v | u
			struct
			{
				// filler for u and v
				u32 filler9 : 16;
				
				// texture page x-coordinate
				// X*64
				// bits 0-3
				u32 tx : 4;

				// texture page y-coordinate
				// 0 - 0; 1 - 256
				// bit 4
				u32 ty : 1;
				
				// Semi-transparency mode
				// 0: 0.5xB+0.5 xF; 1: 1.0xB+1.0 xF; 2: 1.0xB-1.0 xF; 3: 1.0xB+0.25xF
				// bits 5-6
				u32 abr : 2;
				
				// Color mode
				// 0 - 4bit CLUT; 1 - 8bit CLUT; 2 - 15bit direct color; 3 - 15bit direct color
				// bits 7-8
				u32 tp : 2;
				
				// bits 9-10 - Unused
				u32 zero0 : 2;
				
				// bit 11 - same as GP0(E1).bit11 - Texture Disable
				// 0: Normal; 1: Disable if GP1(09h).Bit0=1
				u32 TextureDisable : 1;

				// bits 12-15 - Unused (should be 0)
				u32 Zero0 : 4;
			} tpage;
			
			u32 Value;
				
		};
		
		template<typename T>
		struct t_vector
		{
			T x, y, u, v, r, g, b;
			u32 bgr;
		};


		typedef uint64_t (*ps1_draw_func_template) (DATA_Write_Format* inputdata, uint32_t ulThreadNum);

		//template<const uint32_t TEXTURE, const uint32_t TGE, const uint32_t SETPIXELMASK, const uint32_t PIXELMASK, const uint32_t ABE, const uint32_t ABR, const uint32_t TP>
		// 1 bit + 1 bit + 1 bit + 1 bit + 1 bit + 2 bits + 2 bits = 9 bits
		static ps1_draw_func_template ps1_arr_sprite_lut[1 << 9];
		static ps1_draw_func_template ps1_arr_triangle_lut[1 << 11];
		static ps1_draw_func_template ps1_arr_line_lut[1 << 7];


		// it's actually better to put the x, y values in an array, in case I want to vectorize later
		// but then it takes more effort to sort the coordinates
		static s32 gx [ 4 ], gy [ 4 ];
		static s32 gu [ 4 ], gv [ 4 ];
		static s32 gr [ 4 ], gg [ 4 ], gb [ 4 ];
		static u32 gbgr [ 4 ];
		
		static s32 x, y, w, h;
		static u32 clut_x, clut_y, tpage_tx, tpage_ty, tpage_abr, tpage_tp, command_tge, command_abe, command_abr;
		static s32 u, v;
		u32 sX, sY, dX, dY, iX, iY;
		u32 iCurrentCount, iTotalCount;
		u32 uImageId;
		//static s64 iU, iV;
		
		static u32 NumberOfPixelsDrawn;
		

		// GPU Status/Control register
		static const uint32_t GPU_CTRL = 0x1f801814;

		union GPU_CTRL_Read_Format
		{
			struct
			{
				// bits 0-3
				// Texture page X = tx*64: 0-0;1-64;2-128;3-196;4-...
				u32 TX : 4;

				// bit 4
				// Texture page Y: 0-0;1-256
				u32 TY : 1;

				// bits 5-6
				// Semi-transparent state: 00-0.5xB+0.5xF;01-1.0xB+1.0xF;10-1.0xB-1.0xF;11-1.0xB+0.25xF
				u32 ABR : 2;

				// bits 7-8
				// Texture page color mode: 00-4bit CLUT;01-8bit CLUT; 10-15bit
				u32 TP : 2;

				// bit 9
				// 0-dither off; 1-dither on
				u32 DTD : 1;

				// bit 10
				// 0-Draw to display area prohibited;1-Draw to display area allowed
				// 1 - draw all fields; 0 - only allow draw to display for even fields
				u32 DFE : 1;

				// bit 11
				// 0-off;1-on, apply mask bit to drawn pixels
				u32 MD : 1;

				// bit 12
				// 0-off;1-on, no drawing pixels with mask bit set
				u32 ME : 1;

				// bits 13-15
				//u32 Unknown1 : 3;
				
				// bit 13 - reserved (seems to be always set?)
				u32 Reserved : 1;
				
				// bit 14 - Reverse Flag (0: Normal; 1: Distorted)
				u32 ReverseFlag : 1;
				
				// bit 15 - Texture disable (0: Normal; 1: Disable textures)
				u32 TextureDisable : 1;

				// bits 16-18
				// screen width is: 000-256 pixels;010-320 pixels;100-512 pixels;110-640 pixels;001-368 pixels
				u32 WIDTH : 3;

				// bit 19
				// 0-screen height is 240 pixels; 1-screen height is 480 pixels
				u32 HEIGHT : 1;

				// bit 20
				// 0-NTSC;1-PAL
				u32 VIDEO : 1;

				// bit 21
				// 0- 15 bit direct mode;1- 24-bit direct mode
				u32 ISRGB24 : 1;

				// bit 22
				// 0-Interlace off; 1-Interlace on
				u32 ISINTER : 1;

				// bit 23
				// 0-Display enabled;1-Display disabled
				u32 DEN : 1;

				// bits 24-25
				//u32 Unknown0 : 2;
				
				// bit 24 - Interrupt Request (0: off; 1: on) [GP0(0x1f)/GP1(0x02)]
				u32 IRQ : 1;
				
				// bit 25 - DMA / Data Request
				// When GP1(0x04)=0 -> always zero
				// when GP1(0x04)=1 -> FIFO State (0: full; 1: NOT full)
				// when GP1(0x04)=2 -> Same as bit 28
				// when GP1(0x04)=3 -> same as bit 27
				u32 DataRequest : 1;

				// bit 26
				// Ready to receive CMD word
				// 0: NOT Ready; 1: Ready
				// 0-GPU is busy;1-GPU is idle
				u32 BUSY : 1;

				// bit 27
				// 0-Not ready to send image (packet 0xc0);1-Ready to send image
				u32 IMG : 1;

				// bit 28
				// Ready to receive DMA block
				// 0: NOT Ready; 1: Ready
				// 0-Not ready to receive commands;1-Ready to receive commands
				u32 COM : 1;

				// bit 29-30
				// 00-DMA off, communication through GP0;01-?;10-DMA CPU->GPU;11-DMA GPU->CPU
				u32 DMA : 2;

				// bit 31
				// 0-Drawing even lines in interlace mode;1-Drawing uneven lines in interlace mode
				u32 LCF : 1;
			};
			
			u32 Value;
		};

		// this will hold the gpu status
		GPU_CTRL_Read_Format GPU_CTRL_Read;

		union CTRL_Write_Format
		{
			struct
			{
				u32 Parameter : 24;
				
				u32 Command : 8;
			};
			
			u32 Value;
		};
		
		////////////////////////////
		// CTRL Register Commands //
		////////////////////////////
		
		// resets the GPU, turns off the screen, and sets status to 0x14802000
		// parameter: 0
		static const u32 CTRL_Write_ResetGPU = 0;

		// resets the command buffer
		// parameter: 0
		static const u32 CTRL_Write_ResetBuffer = 1;
		
		// resets the IRQ
		// parameter: 0
		static const u32 CTRL_Write_ResetIRQ = 2;
		
		// turns on/off the display
		// parameter: 0 - display enable; 1 - display disable
		static const u32 CTRL_Write_DisplayEnable = 3;
		
		// sets DMA direction
		// parameter: 0 - dma disabled; 1 - DMA ?; 2 - DMA Cpu to Gpu; 3 - DMA Gpu to Cpu
		static const u32 CTRL_Write_DMASetup = 4;

		// sets top left corner of display area
		// parameter:
		// bit  $00-$09  X (0-1023)
		// bit  $0A-$12  Y (0-512)
		// = Y<<10 + X
		static const u32 CTRL_Write_DisplayArea = 5;
		
		// sets the horizontal display range
		// parameter:
		// bit  $00-$0b   X1 ($1f4-$CDA)
		// bit  $0c-$17   X2
		// = X1+X2<<12
		static const u32 CTRL_Write_HorizontalDisplayRange = 6;
		
		// sets the vertical display range
		// parameter:
		// bit  $00-$09   Y1
		// bit  $0a-$14   Y2
		// = Y1+Y2<<10
		static const u32 CTRL_Write_VerticalDisplayRange = 7;
		
		// sets the display mode
		// parameter:
		// bit  $00-$01  Width 0
		// bit  $02      Height
		// bit  $03      Videomode     See above
		// bit  $04      Isrgb24
		// bit  $05      Isinter
		// bit  $06      Width1
		// bit  $07      Reverseflag
		static const u32 CTRL_Write_DisplayMode = 8;
		
		// returns GPU info
		// parameter:
		// $000000  appears to return Draw area top left?
		// $000001  appears to return Draw area top left?
		// $000002
		// $000003  Draw area top left
		// $000004  Draw area bottom right
		// $000005  Draw offset
		// $000006  appears to return draw offset?
		// $000007  GPU Type, should return 2 for a standard GPU.
		static const u32 CTRL_Write_GPUInfo = 0x10;
		
		
		// other commands are unknown until testing
		
		///////////////////
		// Pixel Formats //
		///////////////////
		
		// BGR format with highest bit being M
		union Pixel_15bit_Format
		{
			struct
			{
				u16 Red : 5;
				u16 Green : 5;
				u16 Blue : 5;

				// bit to mask pixel (1 - masked; 0 - not masked)
				u16 M : 1;
			};
			
			u16 Value;
		};

		union Pixel_24bit_Format
		{
			// 2 pixels encoded using 3 frame buffer pixels
			// G0 | R0 | R1 | B0 | B1 | G1
			struct
			{
				// this format is stored in frame buffer as 2 pixels in a 48 bit space (3 "pixels" of space)
				u8 Green1;
				u8 Blue1;
				u8 Blue0;
				u8 Red1;
				u8 Red0;
				u8 Green0;
			};
			
			struct
			{
				u16 Pixel2;
				u16 Pixel1;
				u16 Pixel0;
			};
		};
		
		union Pixel_8bit_Format
		{
			struct
			{
				// here, there are two indexes stored in one "pixel", but in reversed order
				
				// the index for the pixel on the left in CLUT
				// bits 0-7
				u8 I0;

				// the index for the pixel on the right in CLUT (it is reversed)
				// bits 8-15
				u8 I1;
			};
			
			u16 Value;
		};
		
		union Pixel_4bit_Format
		{
			struct
			{
				// here, there are four indexes stored in one "pixel", but in reversed order
				
				// the index for the leftmost pixel in CLUT (it is reversed)
				u16 I0 : 4;
				
				u16 I1 : 4;
				u16 I2 : 4;
				
				// the index for the rightmost pixel in CLUT (it is reversed)
				u16 I3 : 4;
				
				
			};
			
			u16 Value;
		};


#ifdef PS2_COMPILE
		static constexpr const u32 c_VRAM_Size = 2;
#else
		static constexpr const u32 c_VRAM_Size = 1048576;
#endif
		
		// VRAM is 1MB with a pixel size of 16-bits
		alignas(32) u16 VRAM [ c_VRAM_Size / 2 ];
		
		
		// *** the cycles can change in PS2 mode, so calculate cycle dependent values dynamically *** //
		
		// System data
		static constexpr double SystemBus_CyclesPerSec_PS1Mode = 33868800.0L;
		
		static constexpr double SystemBus_CyclesPerSec_PS2Mode1 = 36864000.0L;
		static constexpr double SystemBus_CyclesPerSec_PS2Mode2 = 37375000.0L;
		
		// testing
		static const u64 DrawOverhead_Cycles = 1;
		
		// Raster data
		static constexpr double NTSC_FramesPerSec = 59.94005994L;
		static constexpr double PAL_FramesPerSec = 50.0L;
		static constexpr double NTSC_FieldsPerSec = ( 59.94005994L / 2.0L );	//NTSC_FieldsPerSec / 2;
		static constexpr double PAL_FieldsPerSec = ( 50.0L / 2.0L );	//PAL_FieldsPerSec / 2;


		// calculate dynamically
		//static constexpr double NTSC_CyclesPerFrame = ( 33868800.0L / ( 59.94005994L / 2.0L ) );
		//static constexpr double PAL_CyclesPerFrame = ( 33868800.0L / ( 50.0L / 2.0L ) );
		//static constexpr double NTSC_FramesPerCycle = 1.0L / ( 33868800.0L / ( 59.94005994L / 2.0L ) );
		//static constexpr double PAL_FramesPerCycle = 1.0L / ( 33868800.0L / ( 50.0L / 2.0L ) );
		double NTSC_CyclesPerFrame;		// = ( 33868800.0L / ( 59.94005994L / 2.0L ) );
		double PAL_CyclesPerFrame;		// = ( 33868800.0L / ( 50.0L / 2.0L ) );
		double NTSC_FramesPerCycle;		// = 1.0L / ( 33868800.0L / ( 59.94005994L / 2.0L ) );
		double PAL_FramesPerCycle;		// = 1.0L / ( 33868800.0L / ( 50.0L / 2.0L ) );
		
		// the EVEN field must be the field that starts at scanline number zero, which means that the even field has 263 scanlines
		// so, for now I just have these mixed up since the EVEN scanlines go from 0-524, making 263 scanlines, and the ODD go from 1-523, making 262
		
		// EVEN scanlines from 0-524 (263 out of 525 total)
		static const u32 NTSC_ScanlinesPerField_Even = 263;
		
		// ODD scanlines from 1-523 (262 out of 525 total)
		static const u32 NTSC_ScanlinesPerField_Odd = 262;
		
		// 525 total scanlines for NTSC
		static const u32 NTSC_ScanlinesPerFrame = 525;	//NTSC_ScanlinesPerField_Odd + NTSC_ScanlinesPerField_Even;
		
		// EVEN scanlines from 0-624 (313 out of 625 total)
		static const u32 PAL_ScanlinesPerField_Even = 313;
		
		// ODD scanlines from 1-623 (312 out of 625 total)
		static const u32 PAL_ScanlinesPerField_Odd = 312;
		
		// 625 total scanlines for PAL
		static const u32 PAL_ScanlinesPerFrame = 625;	//PAL_ScanlinesPerField_Odd + PAL_ScanlinesPerField_Even;

		static constexpr double NTSC_FieldsPerScanline_Even = 1.0L / 263.0L;
		static constexpr double NTSC_FieldsPerScanline_Odd = 1.0L / 262.0L;
		static constexpr double NTSC_FramesPerScanline = 1.0L / 525.0L;
		
		static constexpr double PAL_FieldsPerScanline_Even = 1.0L / 313.0L;
		static constexpr double PAL_FieldsPerScanline_Odd = 1.0L / 312.0L;
		static constexpr double PAL_FramesPerScanline = 1.0L / 625.0L;
		
		
		
		static constexpr double NTSC_ScanlinesPerSec = ( 525.0L * ( 59.94005994L / 2.0L ) );	//NTSC_ScanlinesPerFrame * NTSC_FramesPerSec; //15734.26573425;
		static constexpr double PAL_ScanlinesPerSec = ( 625.0L * ( 50.0L / 2.0L ) );	//PAL_ScanlinesPerFrame * PAL_FramesPerSec; //15625;
		
		
		// calculate dynamically
		//static constexpr double NTSC_CyclesPerScanline = ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );	//SystemBus_CyclesPerSec / NTSC_ScanlinesPerSec; //2152.5504000022;
		//static constexpr double PAL_CyclesPerScanline = ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );	//SystemBus_CyclesPerSec / PAL_ScanlinesPerSec; //2167.6032;
		//static constexpr double NTSC_ScanlinesPerCycle = 1.0L / ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static constexpr double PAL_ScanlinesPerCycle = 1.0L / ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double NTSC_CyclesPerScanline;		// = ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );	//SystemBus_CyclesPerSec / NTSC_ScanlinesPerSec; //2152.5504000022;
		double PAL_CyclesPerScanline;		// = ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );	//SystemBus_CyclesPerSec / PAL_ScanlinesPerSec; //2167.6032;
		double NTSC_ScanlinesPerCycle;		// = 1.0L / ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double PAL_ScanlinesPerCycle;		// = 1.0L / ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		
		
		
		// calculate dynamically
		//static constexpr double NTSC_CyclesPerField_Even = 263.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static constexpr double NTSC_CyclesPerField_Odd = 262.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static constexpr double PAL_CyclesPerField_Even = 313.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		//static constexpr double PAL_CyclesPerField_Odd = 312.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		//static constexpr double NTSC_FieldsPerCycle_Even = 1.0L / ( 263.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		//static constexpr double NTSC_FieldsPerCycle_Odd = 1.0L / ( 262.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		//static constexpr double PAL_FieldsPerCycle_Even = 1.0L / ( 313.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		//static constexpr double PAL_FieldsPerCycle_Odd = 1.0L / ( 312.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		double NTSC_CyclesPerField_Even;	// = 263.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double NTSC_CyclesPerField_Odd;		// = 262.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double PAL_CyclesPerField_Even;		// = 313.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double PAL_CyclesPerField_Odd;		// = 312.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double NTSC_FieldsPerCycle_Even;	// = 1.0L / ( 263.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		double NTSC_FieldsPerCycle_Odd;		// = 1.0L / ( 262.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		double PAL_FieldsPerCycle_Even;		// = 1.0L / ( 313.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		double PAL_FieldsPerCycle_Odd;		// = 1.0L / ( 312.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );



		// calculate dynamically
		//static constexpr double NTSC_DisplayAreaCycles = 240.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static constexpr double PAL_DisplayAreaCycles = 288.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double NTSC_DisplayAreaCycles;	// = 240.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double PAL_DisplayAreaCycles;	// = 288.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );

		
		//static constexpr double NTSC_CyclesPerVBlank_Even = ( 263.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static constexpr double NTSC_CyclesPerVBlank_Odd = ( 262.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static constexpr double PAL_CyclesPerVBlank_Even = ( 313.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		//static constexpr double PAL_CyclesPerVBlank_Odd = ( 312.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		//static constexpr double NTSC_VBlanksPerCycle_Even = 1.0L / ( ( 263.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		//static constexpr double NTSC_VBlanksPerCycle_Odd = 1.0L / ( ( 262.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		//static constexpr double PAL_VBlanksPerCycle_Even = 1.0L / ( ( 313.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		//static constexpr double PAL_VBlanksPerCycle_Odd = 1.0L / ( ( 312.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		double NTSC_CyclesPerVBlank_Even;	// = ( 263.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double NTSC_CyclesPerVBlank_Odd;	// = ( 262.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double PAL_CyclesPerVBlank_Even;	// = ( 313.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double PAL_CyclesPerVBlank_Odd;		// = ( 312.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double NTSC_VBlanksPerCycle_Even;	// = 1.0L / ( ( 263.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		double NTSC_VBlanksPerCycle_Odd;	// = 1.0L / ( ( 262.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		double PAL_VBlanksPerCycle_Even;	// = 1.0L / ( ( 313.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		double PAL_VBlanksPerCycle_Odd;		// = 1.0L / ( ( 312.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );



		// the viewable scanlines (inclusive)
		static const uint32_t PAL_Field0_Viewable_YStart = 23;
		static const uint32_t PAL_Field0_Viewable_YEnd = 310;
		static const uint32_t PAL_Field1_Viewable_YStart = 336;
		static const uint32_t PAL_Field1_Viewable_YEnd = 623;
		
		static const u32 NTSC_VBlank = 240;
		static const u32 PAL_VBlank = 288;
		u32 Display_Width, Display_Height, Raster_Width, Raster_Height;
		u32 Raster_X, Raster_Y, Raster_HBlank;
		u32 Raster_Pixel_INC;
		
		// cycle at which the raster settings last changed at
		u64 RasterChange_StartCycle;
		u64 RasterChange_StartPixelCount;
		u64 RasterChange_StartHBlankCount;
		
		double dCyclesPerFrame, dCyclesPerField0, dCyclesPerField1, dDisplayArea_Cycles, dVBlank0Area_Cycles, dVBlank1Area_Cycles, dHBlankArea_Cycles;
		double dFramesPerCycle, dFieldsPerCycle0, dFieldsPerCycle1; //, dDisplayArea_Cycles, dVBlank0Area_Cycles, dVBlank1Area_Cycles, dHBlankArea_Cycles;
		u64 CyclesPerHBlank, CyclesPerVBlank;

		u64 CyclesPerPixel_INC;
		double dCyclesPerPixel, dCyclesPerScanline;
		double dPixelsPerCycle, dScanlinesPerCycle;
		u64 PixelCycles;
		u32 X_Pixel, Y_Pixel;
		u64 Global_XPixel, Global_YPixel;
		u32 HBlank_X;
		u32 VBlank_Y;
		u32 ScanlinesPerField0, ScanlinesPerField1;
		u32 Raster_XMax;
		u32 Raster_YMax;	// either 525 or 625
		
		
		double dScanlineStart, dNextScanlineStart, dHBlankStart;
		unsigned long long llScanlineStart, llNextScanlineStart, llHBlankStart;
		u32 lScanline, lNextScanline;
		
		
		// enables simulation of scanlines
		long bEnableScanline;
		
		// enable/disable scanline simulation
		inline void Set_Scanline ( long bValue ) { bEnableScanline = bValue; }
		inline long Get_Scanline () { return bEnableScanline; }
		
		
		static const u32 HBlank_X_LUT [ 8 ];
		static const u32 VBlank_Y_LUT [ 2 ];
		static const u32 Raster_XMax_LUT [ 2 ] [ 8 ];
		static const u32 Raster_YMax_LUT [ 2 ];
		u64 CyclesPerPixel_INC_Lookup [ 2 ] [ 8 ];
		double CyclesPerPixel_Lookup [ 2 ] [ 8 ];
		double PixelsPerCycle_Lookup [ 2 ] [ 8 ];
		
		static const int c_iGPUCyclesPerPixel_256 = 10;
		static const int c_iGPUCyclesPerPixel_320 = 8;
		static const int c_iGPUCyclesPerPixel_368 = 7;
		static const int c_iGPUCyclesPerPixel_512 = 5;
		static const int c_iGPUCyclesPerPixel_640 = 4;
		
		static const u32 c_iGPUCyclesPerPixel [];
		
		static constexpr double c_dGPUCyclesPerScanline_NTSC = 53222400.0L / ( 525.0L * 30.0L );
		static constexpr double c_dGPUCyclesPerScanline_PAL = 53222400.0L / ( 625.0L * 30.0L );

		
		static const int c_iVisibleArea_StartX_Cycle = 584; //544;
		static const int c_iVisibleArea_EndX_Cycle = 3192;	//3232;
		static const int c_iVisibleArea_StartY_Pixel_NTSC = 15;
		static const int c_iVisibleArea_EndY_Pixel_NTSC = 257;
		static const int c_iVisibleArea_StartY_Pixel_PAL = 34;
		static const int c_iVisibleArea_EndY_Pixel_PAL = 292;
		
		static const int c_iScreenOutput_MaxWidth = ( c_iVisibleArea_EndX_Cycle >> 2 ) - ( c_iVisibleArea_StartX_Cycle >> 2 );
		static const int c_iScreenOutput_MaxHeight = ( c_iVisibleArea_EndY_Pixel_PAL - c_iVisibleArea_StartY_Pixel_PAL ) << 1;
		
		// raster functions
		void UpdateRaster_VARS ( void );
		
		// returns interrupt data
		//u32 UpdateRaster ( void );
		void UpdateRaster ( void );
		
		// updates LCF
		// note: must have Y_Pixel (CurrentScanline) updated first
		void Update_LCF ();
		
		
		static constexpr double NTSC_CyclesPerPixel_256 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L );
		static constexpr double NTSC_CyclesPerPixel_320 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L );
		static constexpr double NTSC_CyclesPerPixel_368 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L );
		static constexpr double NTSC_CyclesPerPixel_512 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L );
		static constexpr double NTSC_CyclesPerPixel_640 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L );
		static constexpr double PAL_CyclesPerPixel_256 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L );
		static constexpr double PAL_CyclesPerPixel_320 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L );
		static constexpr double PAL_CyclesPerPixel_368 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L );
		static constexpr double PAL_CyclesPerPixel_512 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L );
		static constexpr double PAL_CyclesPerPixel_640 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L );
		
		static constexpr double NTSC_PixelsPerCycle_256 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L );
		static constexpr double NTSC_PixelsPerCycle_320 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L );
		static constexpr double NTSC_PixelsPerCycle_368 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L );
		static constexpr double NTSC_PixelsPerCycle_512 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L );
		static constexpr double NTSC_PixelsPerCycle_640 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L );
		static constexpr double PAL_PixelsPerCycle_256 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L );
		static constexpr double PAL_PixelsPerCycle_320 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L );
		static constexpr double PAL_PixelsPerCycle_368 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L );
		static constexpr double PAL_PixelsPerCycle_512 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L );
		static constexpr double PAL_PixelsPerCycle_640 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L );
		
		
		static const u64 NTSC_CyclesPerPixelINC_256 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_320 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_368 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_512 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_640 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_256 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_320 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_368 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_512 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_640 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L ) ))) + ( 1 << 8 );
		
		
		// the last command written to CTRL register
		u32 CommandReady;
		CTRL_Write_Format NextCommand;
		
		// we need to know if the GPU is busy or not
		s64 BusyCycles;
		
		// we need a "command buffer" for GPU
		// stuff written to DATA register or via dma goes into command buffer
		u32 BufferMode;
		enum { MODE_NORMAL, MODE_IMAGEIN, MODE_IMAGEOUT };
		DATA_Write_Format Buffer [ 16 ];
		u32 BufferSize;
		u32 PixelsLeftToTransfer;
		
		// dither values array
		static const s64 c_iDitherValues [];
		static const s64 c_iDitherZero [];
		static const s64 c_iDitherValues24 [];
		static const s32 c_iDitherValues16 [];
		static const s32 c_iDitherValues4 [];
		static const s16 c_viDitherValues16_add [];
		static const s16 c_viDitherValues16_sub [];

		// draw area data
		
		// upper left hand corner of frame buffer area being drawn to the tv
		u32 ScreenArea_TopLeftX;
		u32 ScreenArea_TopLeftY;
		
		u32 DrawArea_TopLeft, DrawArea_BottomRight, DrawArea_Offset;
		u32 ScreenArea_TopLeft;
		
		// bounding rectangles of the area being drawn to - anything outside of this is not drawn
		u32 DrawArea_TopLeftX;
		u32 DrawArea_TopLeftY;
		u32 DrawArea_BottomRightX;
		u32 DrawArea_BottomRightY;
		
		// also need to maintain the internal coords to return when requested
		u32 iREG_DrawArea_TopLeftX;
		u32 iREG_DrawArea_TopLeftY;
		u32 iREG_DrawArea_BottomRightX;
		u32 iREG_DrawArea_BottomRightY;
		
		// the offset from the upper left hand corner of frame buffer that is being drawn to. Applies to all drawing
		// these are signed and go from -1024 to +1023
		s32 DrawArea_OffsetX;
		s32 DrawArea_OffsetY;
		
		// specifies amount of data being sent to the tv
		u32 DisplayRange_Horizontal;
		u32 DisplayRange_Vertical;
		u32 DisplayRange_X1;
		u32 DisplayRange_X2;
		u32 DisplayRange_Y1;
		u32 DisplayRange_Y2;
		
		// texture window
		u32 TextureWindow;
		u32 TWX, TWY, TWW, TWH;
		u32 TextureWindow_X;
		u32 TextureWindow_Y;
		u32 TextureWindow_Width;
		u32 TextureWindow_Height;
		
		// mask settings
		u32 SetMaskBitWhenDrawing;
		u32 DoNotDrawToMaskAreas;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// the tx and ty in the GPU_CTRL register are actually texture offsets into where the textures start at
		// add these to the texture offsets supplied by the command to get the real offsets in the frame buffer
		
		// also need to count primitives for debugging (step primitive, etc.)
		u32 Primitive_Count;
		
		// also need to count frames for debugging
		u32 Frame_Count;
		
		static const u32 FrameBuffer_Width = 1024;
		static const u32 FrameBuffer_Height = 512;
		static const u32 FrameBuffer_XMask = FrameBuffer_Width - 1;
		static const u32 FrameBuffer_YMask = FrameBuffer_Height - 1;


		//instead of OpenCL, which obviously has issues for them to work out
		//I'll just accelerate graphics using multiple CPU's, which makes more sense anyway
		static const u32 c_iMaxThreads = 2;
		static u32 ulNumberOfThreads;
		static u32 ulNumberOfThreads_Created;
		static Api::Thread* GPUThreads [ c_iMaxThreads ];
		
		// free buffer space required for program to run while multi-threading
		static const u32 c_ulRequiredBuffer = 20000;
		
		// input buffer size must be a power of two
#ifdef ALLOW_OPENCL_PS1
		// this only needs to be large if drawing with compute shader
		static const u64 c_ulInputBuffer_Size = 1 << 16;
#else
		static const u64 c_ulInputBuffer_Size = 1 << 1;
#endif

		static const u64 c_ulInputBuffer_Mask = c_ulInputBuffer_Size - 1;
		static const u64 c_ulInputBuffer_Shift = 4;
		//volatile u32 ulThreads_Idle;
		static volatile u64 ullInputBuffer_Index;
		static volatile u32 ulInputBuffer_Count;
		alignas(32) static u32 inputdata [ ( 1 << c_ulInputBuffer_Shift ) * c_ulInputBuffer_Size ];
		u32 Dummy;

		// size of screen internally
		static constexpr int PS1_SCREEN_X_SIZE = (1024);	//(640);
		static constexpr int PS1_SCREEN_Y_SIZE = (1024);	// (480);

		// the maximum number of entries/commands in command list that gets sent to the hardware gpu compute shader
		static constexpr int PS1_COMMAND_LIST_SIZE = (1 << 16);

		// the maximum number of input pixels in buffer that gets sent to the hardware gpu compute shader
		static constexpr int PS1_PIXEL_LIST_SIZE = (1 << 20);

		// the maximum number of entries in the pre-compute buffer on hardware gpu compute shader
		static constexpr int PS1_PRECOMPUTE_LIST_SIZE = (1 << 16);

		static constexpr int SCRATCH_SPACE_SIZE_BYTES = (16384);
		static constexpr int PS1_GPU_VRAM_SIZE_BYTES = (1024 * 512 * 2);
		static constexpr int PS1_GPU_SVRAM_SIZE_BYTES = (1024 * 512 * 2);

		// size of the input command buffer that gets sent to the hardware gpu compute shader
		static constexpr int PS1_GPU_INPUT_COMMAND_SIZE_BYTES = (PS1_COMMAND_LIST_SIZE * 16 * 4);

		// size of the buffer of input pixels that gets sent to the hardware gpu compute shader
		static constexpr int PS1_GPU_INPUT_PIXEL_SIZE_BYTES = (PS1_PIXEL_LIST_SIZE * 4);

		static constexpr int PS1_GPU_PIXELBUF_SIZE_BYTES = (1024 * 512 * 4);

		// size of the pre-compute/work area on the hardware gpu compute shader
		static constexpr int PS1_GPU_WORK_AREA_SIZE_BYTES = (PS1_PRECOMPUTE_LIST_SIZE * 64 * 4);

		static constexpr int PS1_GPU_STAGING_SIZE_BYTES = (PS1_SCREEN_X_SIZE * PS1_SCREEN_Y_SIZE * 4);

		// pointer to gpu scratch mapped for shader
		static u32* p_uHwScratchData32;

		// pointer into the hardware/opengl copy of the input buffer
		// pointer to mapped input primitive command buffer on gpu for shader
		static u32 *p_uHwInputData32;

		// pointer to gpu memory mapped for shader
		static u32 *p_uHwOutputData32;

		// pointer to mapped pixel/gfx input buffer on gpu for shader
		static u32* p_ulHwPixelInBuffer32;

		// pointer to the hardware pixel output buffer
		static u32* p_ulHwPixelOutBuffer32;

		static volatile u64 ulInputBuffer_WriteIndex;
		static volatile u64 ulInputBuffer_TargetIndex;
		static volatile u64 ulInputBuffer_ReadIndex;


#ifdef ALLOW_OPENCL_PS1

		static const u64 c_ullPixelInBuffer_Size = 1 << 20;
		static const u64 c_ullPixelInBuffer_Mask = c_ullPixelInBuffer_Size - 1;
		static volatile u64 ullPixelInBuffer_WriteIndex;
		static volatile u64 ullPixelInBuffer_TargetIndex;
		static volatile u64 ullPixelInBuffer_ReadIndex;
		alignas(32) static u32 ulPixelInBuffer32 [ c_ullPixelInBuffer_Size ];

#endif


		// current index that gpu opencl gpu is on
		//static volatile u64 ullGPUIndex;
		static volatile u64 ulTBufferIndex;

		// this is true if currently using gpu hardware rendering, false otherwise
		u32 bEnable_OpenCL;

		// if vulkan is setup ok, then can render on gpu hardware using shader renderer
		// note: this needs to be static since it checks at program statup if this is allowed and don't want loading save states to overwrite this
		static u32 bAllowGpuHardwareRendering;


		// for drawing on a separate thread (should be combined with the regular code)
		static void draw_screen_th( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 DrawTriangle_Mono_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 DrawTriangle_Gradient_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 DrawTriangle_Texture_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 DrawTriangle_TextureGradient_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 Draw_Rectangle_60_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 DrawSprite_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static void Draw_Pixel_68_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 Draw_FrameBufferRectangle_02_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 DrawLine_Mono_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 DrawLine_Gradient_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static u64 Transfer_MoveImage_80_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		static void TransferPixelPacketIn_th ( DATA_Write_Format* inputdata, u32 ulThreadNum = 0 );
		
		
		
		
		
		// constructor
		//GPU ();		
		
		// debug info
		static u32* DebugCpuPC;
		

		void ExecuteGPUBuffer ();
		
		void ProcessDataRegWrite ( u32* pData, s32 BS );
		u32 ProcessDataRegRead ();

		void PreTransferPixelPacketOut ();

		u32 TransferPixelPacketIn ( u32* pData, s32 BS );
		u32 TransferPixelPacketOut ();
		
		/////////////////////////////////////////////////////////
		// functions for getting data from command buffer
		inline void GetBGR24 ( DATA_Write_Format Data ) { gbgr [ 0 ] = Data.Value & 0xffffff; }

		inline void GetBGR0_24 ( DATA_Write_Format Data ) { gbgr [ 0 ] = Data.Value & 0xffffff; }
		inline void GetBGR1_24 ( DATA_Write_Format Data ) { gbgr [ 1 ] = Data.Value & 0xffffff; }
		inline void GetBGR2_24 ( DATA_Write_Format Data ) { gbgr [ 2 ] = Data.Value & 0xffffff; }
		inline void GetBGR3_24 ( DATA_Write_Format Data ) { gbgr [ 3 ] = Data.Value & 0xffffff; }

		inline void GetBGR0_8 ( DATA_Write_Format Data ) { gbgr [ 0 ] = Data.Value & 0xffffff; gr [ 0 ] = (u32) Data.Red; gg [ 0 ] = (u32) Data.Green; gb [ 0 ] = (u32) Data.Blue; }
		inline void GetBGR1_8 ( DATA_Write_Format Data ) { gbgr [ 1 ] = Data.Value & 0xffffff; gr [ 1 ] = (u32) Data.Red; gg [ 1 ] = (u32) Data.Green; gb [ 1 ] = (u32) Data.Blue; }
		inline void GetBGR2_8 ( DATA_Write_Format Data ) { gbgr [ 2 ] = Data.Value & 0xffffff; gr [ 2 ] = (u32) Data.Red; gg [ 2 ] = (u32) Data.Green; gb [ 2 ] = (u32) Data.Blue; }
		inline void GetBGR3_8 ( DATA_Write_Format Data ) { gbgr [ 3 ] = Data.Value & 0xffffff; gr [ 3 ] = (u32) Data.Red; gg [ 3 ] = (u32) Data.Green; gb [ 3 ] = (u32) Data.Blue; }
		
		inline void GetBGR ( DATA_Write_Format Data ) { gbgr [ 0 ] = ( ((u32)Data.Red) >> 3 ) | ( ( ((u32)Data.Green) >> 3 ) << 5 ) | ( ( ((u32)Data.Blue) >> 3 ) << 10 ); }
		inline void GetXY ( DATA_Write_Format Data ) { x = Data.x; y = Data.y; }
		inline void GetUV ( DATA_Write_Format Data ) { u = Data.u; v = Data.v; }
		inline void GetBGR0 ( DATA_Write_Format Data ) { gbgr [ 0 ] = ( Data.Red >> 3 ) | ( ( Data.Green >> 3 ) << 5 ) | ( ( Data.Blue >> 3 ) << 10 ); }
		inline void GetXY0 ( DATA_Write_Format Data ) { gx [ 0 ] = Data.x; gy [ 0 ] = Data.y; }
		inline void GetUV0 ( DATA_Write_Format Data ) { gu [ 0 ] = Data.u; gv [ 0 ] = Data.v; }
		inline void GetBGR1 ( DATA_Write_Format Data ) { gbgr [ 1 ] = ( Data.Red >> 3 ) | ( ( Data.Green >> 3 ) << 5 ) | ( ( Data.Blue >> 3 ) << 10 ); }
		inline void GetXY1 ( DATA_Write_Format Data ) { gx [ 1 ] = Data.x; gy [ 1 ] = Data.y; }
		inline void GetUV1 ( DATA_Write_Format Data ) { gu [ 1 ] = Data.u; gv [ 1 ] = Data.v; }
		inline void GetBGR2 ( DATA_Write_Format Data ) { gbgr [ 2 ] = ( Data.Red >> 3 ) | ( ( Data.Green >> 3 ) << 5 ) | ( ( Data.Blue >> 3 ) << 10 ); }
		inline void GetXY2 ( DATA_Write_Format Data ) { gx [ 2 ] = Data.x; gy [ 2 ] = Data.y; }
		inline void GetUV2 ( DATA_Write_Format Data ) { gu [ 2 ] = Data.u; gv [ 2 ] = Data.v; }
		inline void GetBGR3 ( DATA_Write_Format Data ) { gbgr [ 3 ] = ( Data.Red >> 3 ) | ( ( Data.Green >> 3 ) << 5 ) | ( ( Data.Blue >> 3 ) << 10 ); }
		inline void GetXY3 ( DATA_Write_Format Data ) { gx [ 3 ] = Data.x; gy [ 3 ] = Data.y; }
		inline void GetUV3 ( DATA_Write_Format Data ) { gu [ 3 ] = Data.u; gv [ 3 ] = Data.v; }
		inline void GetHW ( DATA_Write_Format Data ) { h = Data.h; w = Data.w; }
		inline void GetCLUT ( DATA_Write_Format Data ) { clut_x = Data.clut.x; clut_y = Data.clut.y; }
		inline void GetTPAGE ( DATA_Write_Format Data ) { tpage_tx = Data.tpage.tx; tpage_ty = Data.tpage.ty; tpage_abr = Data.tpage.abr; tpage_tp = Data.tpage.tp; GPU_CTRL_Read.Value = ( GPU_CTRL_Read.Value & ~0x1ff ) | ( ( Data.Value >> 16 ) & 0x1ff ); }

		////////////////////////////////////////////////////////
		// Functions to process GPU buffer commands
		
		
		void Draw_MonoLine_40 ();
		void Draw_ShadedLine_50 ();
		void Draw_PolyLine_48 ();
		void Draw_GradientLine_50 ();
		void Draw_GradientPolyLine_58 ();
		
		void Draw_Sprite8_74 ();
		void Draw_Sprite16_7c ();
		
		// below is complete

		void Draw_FrameBufferRectangle_02 ();
		
		//void Draw_MonoTriangle_20 ( u32 Coord0, u32 Coord1, u32 Coord2 );
		void Draw_MonoTriangle_20 ( DATA_Write_Format* p_inputdata, u32 ulThreadNum );
		void Draw_MonoRectangle_28 ();
		//void Draw_GradientTriangle_30 ( u32 Coord0, u32 Coord1, u32 Coord2 );
		void Draw_GradientTriangle_30 ( DATA_Write_Format* p_inputdata, u32 ulThreadNum );
		void Draw_GradientRectangle_38 ();
		//void Draw_TextureTriangle_24 ( u32 Coord0, u32 Coord1, u32 Coord2 );
		void Draw_TextureTriangle_24 ( DATA_Write_Format* p_inputdata, u32 ulThreadNum );
		void Draw_TextureRectangle_2c ();
		//void Draw_TextureGradientTriangle_34 ( u32 Coord0, u32 Coord1, u32 Coord2 );
		void Draw_TextureGradientTriangle_34 ( DATA_Write_Format* p_inputdata, u32 ulThreadNum );
		void Draw_TextureGradientRectangle_3c ();
		
		//void Draw_Sprite_64 ();
		void Draw_Sprite_64 ( DATA_Write_Format* p_inputdata, u32 ulThreadNum );
		void Draw_Sprite8x8_74 ();
		void Draw_Sprite16x16_7c ();

		void Draw_Pixel_68 ();
		void Draw_Rectangle_60 ();
		void Draw_Rectangle8x8_70 ();
		void Draw_Rectangle16x16_78 ();
		
		void Transfer_MoveImage_80 ();
		
		
		///////////////////////////////////////////////////////////////////////
		// Function to perform actual drawing of primitives on screen area
		
		void DrawSprite ();
		
		
		void DrawTriangle_Mono ( u32 Coord0, u32 Coord1, u32 Coord2 );
		void DrawTriangle_Gradient ( u32 Coord0, u32 Coord1, u32 Coord2 );
		
		
		void DrawTriangle_Texture ( u32 Coord0, u32 Coord1, u32 Coord2 );
		void DrawTriangle_TextureGradient ( u32 Coord0, u32 Coord1, u32 Coord2 );
		
		
		// sets the portion of frame buffer that is showing on screen
		void Set_ScreenSize ( int _width, int _height );

		
		// inline functions
		
		inline static s32 GetRed16 ( u16 Color ) { return ( ( Color >> 10 ) & 0x1f ); }
		inline static s32 GetGreen16 ( u16 Color ) { return ( ( Color >> 5 ) & 0x1f ); }
		inline static s32 GetBlue16 ( u16 Color ) { return ( Color & 0x1f ); }
		inline static s32 SetRed16 ( u16 Color ) { return ( ( Color & 0x1f ) << 10 ); }
		inline static s32 SetGreen16 ( u16 Color ) { return ( ( Color & 0x1f ) << 5 ); }
		inline static s32 SetBlue16 ( u16 Color ) { return ( Color & 0x1f ); }
		
		inline static s32 GetRed24 ( u32 Color ) { return ( ( Color >> 16 ) & 0xff ); }
		inline static s32 GetGreen24 ( u32 Color ) { return ( ( Color >> 8 ) & 0xff ); }
		inline static s32 GetBlue24 ( u32 Color ) { return ( Color & 0xff ); }
		inline static s32 SetRed24 ( u32 Color ) { return ( ( Color & 0xff ) << 16 ); }
		inline static s32 SetGreen24 ( u32 Color ) { return ( ( Color & 0xff ) << 8 ); }
		inline static s32 SetBlue24 ( u32 Color ) { return ( Color & 0xff ); }
		
		
		inline static u32 Clamp5 ( long n )
		{
			long a = 0x1f;
			a -= n;
			a >>= 31;
			a |= n;
			n >>= 31;
			n = ~n;
			n &= a;
			return n & 0x1f;
		}
		
		// signed clamp to use after an ADD operation
		// CLAMPTO - this is the number of bits to clamp to
		template<typename T, const int CLAMPTO>
		inline static T AddSignedClamp ( T n )
		{
			n &= ~( n >> ( ( sizeof ( T ) * 8 ) - 1 ) );
			return ( n | ( ( n << ( ( sizeof ( T ) * 8 ) - ( CLAMPTO + 1 ) ) ) >> ( ( sizeof ( T ) * 8 ) - 1 ) ) ) & ( ( 1 << CLAMPTO ) - 1 );
		}
		
		// unsigned clamp to use after an ADD operation
		// CLAMPTO - this is the number of bits to clamp to
		template<typename T, const int CLAMPTO>
		inline static T AddUnsignedClamp ( T n )
		{
			//n &= ~( n >> ( ( sizeof ( T ) * 8 ) - 1 ) );
			return ( n | ( ( n << ( ( sizeof ( T ) * 8 ) - ( CLAMPTO + 1 ) ) ) >> ( ( sizeof ( T ) * 8 ) - ( CLAMPTO + 1 ) ) ) ) & ( ( 1 << CLAMPTO ) - 1 );
		}
		
		// signed clamp to use after an ANY (MULTIPLY, etc) operation, but should take longer than the one above
		// CLAMPTO - this is the mask for number of bits to clamp to
		template<typename T, const int CLAMPTO>
		inline static T SignedClamp ( T n )
		{
			long a = ( ( 1 << CLAMPTO ) - 1 );	//0x1f;
			a -= n;
			a >>= ( ( sizeof ( T ) * 8 ) - 1 );	//31;
			a |= n;
			n >>= ( ( sizeof ( T ) * 8 ) - 1 );	//31;
			n = ~n;
			n &= a;
			return n & ( ( 1 << CLAMPTO ) - 1 );	//0x1f;
		}


		// signed clamp to use after an ANY (MULTIPLY, etc) operation, but should take longer than the one above
		// CLAMPTO - this is the mask for number of bits to clamp to
		template<typename T, const int CLAMPTO>
		inline static T UnsignedClamp ( T n )
		{
			long a = ( ( 1 << CLAMPTO ) - 1 );	//0x1f;
			a -= n;
			a >>= ( ( sizeof ( T ) * 8 ) - 1 );	//31;
			n |= a;
			//n >>= ( ( sizeof ( T ) * 8 ) - 1 );	//31;
			//n = ~n;
			//n &= a;
			return n & ( ( 1 << CLAMPTO ) - 1 );	//0x1f;
		}
		
		
		inline static u32 Clamp8 ( long n )
		{
			long a = 0xff;
			a -= n;
			a >>= 31;
			a |= n;
			n >>= 31;
			n = ~n;
			n &= a;
			return n & 0xff;
		}
		
		template<typename T>
		inline static void Swap ( T& Op1, T& Op2 )
		{
			//T Temp = Op1;
			//Op1 = Op2;
			//Op2 = Temp;
			Op1 ^= Op2;
			Op2 ^= Op1;
			Op1 ^= Op2;
		}
		

		inline static u32 Color16To24 ( u16 Color ) { return SetRed24(GetRed16(Color)<<3) | SetGreen24(GetGreen16(Color)<<3) | SetBlue24(GetBlue16(Color)<<3); }
		inline static u16 Color24To16 ( u32 Color ) { return SetRed16(GetRed24(Color)>>3) | SetGreen16(GetGreen24(Color)>>3) | SetBlue16(GetBlue24(Color)>>3); }
		
		inline static u16 MakeRGB16 ( u32 R, u32 G, u32 B ) { return ( ( R & 0x1f ) << 10 ) | ( ( G & 0x1f ) << 5 ) | ( B & 0x1f ); }
		inline static u32 MakeRGB24 ( u32 R, u32 G, u32 B ) { return ( ( R & 0xff ) << 16 ) | ( ( G & 0xff ) << 8 ) | ( B & 0xff ); }
		
		inline static u16 ColorMultiply16 ( u16 Color1, u16 Color2 )
		{
			return SetRed16 ( Clamp5 ( ( GetRed16 ( Color1 ) * GetRed16 ( Color2 ) ) >> 4 ) ) |
					SetGreen16 ( Clamp5 ( ( GetGreen16 ( Color1 ) * GetGreen16 ( Color2 ) ) >> 4 ) ) |
					SetBlue16 ( Clamp5 ( ( GetBlue16 ( Color1 ) * GetBlue16 ( Color2 ) ) >> 4 ) );
		}
		
		inline static u32 ColorMultiply24 ( u32 Color1, u32 Color2 )
		{
			return SetRed24 ( Clamp8 ( ( GetRed24 ( Color1 ) * GetRed24 ( Color2 ) ) >> 7 ) ) |
					SetGreen24 ( Clamp8 ( ( GetGreen24 ( Color1 ) * GetGreen24 ( Color2 ) ) >> 7 ) ) |
					SetBlue24 ( Clamp8 ( ( GetBlue24 ( Color1 ) * GetBlue24 ( Color2 ) ) >> 7 ) );
		}
		
		inline static u16 ColorMultiply1624 ( u64 Color16, u64 Color24 )
		{
			static const u32 c_iBitsPerPixel16 = 5;
			static const u32 c_iRedShift16 = c_iBitsPerPixel16 * 2;
			static const u32 c_iRedMask16 = ( 0x1f << c_iRedShift16 );
			static const u32 c_iGreenShift16 = c_iBitsPerPixel16 * 1;
			static const u32 c_iGreenMask16 = ( 0x1f << c_iGreenShift16 );
			static const u32 c_iBlueShift16 = 0;
			static const u32 c_iBlueMask16 = ( 0x1f << c_iBlueShift16 );
		
			static const u32 c_iBitsPerPixel24 = 8;
			static const u32 c_iRedShift24 = c_iBitsPerPixel24 * 2;
			static const u32 c_iRedMask24 = ( 0xff << ( c_iBitsPerPixel24 * 2 ) );
			static const u32 c_iGreenShift24 = c_iBitsPerPixel24 * 1;
			static const u32 c_iGreenMask24 = ( 0xff << ( c_iBitsPerPixel24 * 1 ) );
			static const u32 c_iBlueShift24 = 0;
			static const u32 c_iBlueMask24 = ( 0xff << ( c_iBitsPerPixel24 * 0 ) );
			
			s64 Red, Green, Blue;
			
			// the multiply should put it in 16.23 fixed point, but need it back in 8.8
			Red = ( ( Color16 & c_iRedMask16 ) * ( Color24 & c_iRedMask24 ) );
			Red |= ( ( Red << ( 64 - ( 16 + 23 ) ) ) >> 63 );
			
			// to get to original position, shift back ( 23 - 8 ) = 15, then shift right 7, for total of 7 + 15 = 22 shift right
			// top bit (38) needs to end up in bit 15, so that would actually shift right by 23
			Red >>= 23;
			
			// the multiply should put it in 16.10 fixed point, but need it back in 8.30
			Green = ( ( (u32) ( Color16 & c_iGreenMask16 ) ) * ( (u32) ( Color24 & c_iGreenMask24 ) ) );
			Green |= ( ( Green << ( 64 - ( 16 + 10 ) ) ) >> 63 );
			
			// top bit (25) needs to end up in bit (10)
			Green >>= 15;
			
			// the multiply should put it in 13.0 fixed point
			Blue = ( ( (u16) ( Color16 & c_iBlueMask16 ) ) * ( (u16) ( Color24 & c_iBlueMask24 ) ) );
			Blue |= ( ( Blue << ( 64 - ( 13 + 0 ) ) ) >> 63 );
			
			// top bit (12) needs to end up in bit 5
			Blue >>= 7;
			
			return ( Red & c_iRedMask16 ) | ( Green & c_iGreenMask16 ) | ( Blue & c_iBlueMask16 );
			
			
			//return SetRed24 ( Clamp8 ( ( GetRed24 ( Color1 ) * GetRed24 ( Color2 ) ) >> 7 ) ) |
			//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( Color1 ) * GetGreen24 ( Color2 ) ) >> 7 ) ) |
			//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( Color1 ) * GetBlue24 ( Color2 ) ) >> 7 ) );
		}


		inline static u16 ColorMultiply241624 ( u64 Color16, u64 Color24 )
		{
			static const u32 c_iBitsPerPixel16 = 5;
			static const u32 c_iRedShift16 = c_iBitsPerPixel16 * 2;
			static const u32 c_iRedMask16 = ( 0x1f << c_iRedShift16 );
			static const u32 c_iGreenShift16 = c_iBitsPerPixel16 * 1;
			static const u32 c_iGreenMask16 = ( 0x1f << c_iGreenShift16 );
			static const u32 c_iBlueShift16 = 0;
			static const u32 c_iBlueMask16 = ( 0x1f << c_iBlueShift16 );
		
			static const u32 c_iBitsPerPixel24 = 8;
			static const u32 c_iRedShift24 = c_iBitsPerPixel24 * 2;
			static const u32 c_iRedMask24 = ( 0xff << ( c_iBitsPerPixel24 * 2 ) );
			static const u32 c_iGreenShift24 = c_iBitsPerPixel24 * 1;
			static const u32 c_iGreenMask24 = ( 0xff << ( c_iBitsPerPixel24 * 1 ) );
			static const u32 c_iBlueShift24 = 0;
			static const u32 c_iBlueMask24 = ( 0xff << ( c_iBitsPerPixel24 * 0 ) );
			
			s64 Red, Green, Blue;
			
			// the multiply should put it in 16.23 fixed point, but need it back in 8.8
			Red = ( ( Color16 & c_iRedMask16 ) * ( Color24 & c_iRedMask24 ) );
			Red |= ( ( Red << ( 64 - ( 16 + 23 ) ) ) >> 63 );
			
			// to get to original position, shift back ( 23 - 8 ) = 15, then shift right 7, for total of 7 + 15 = 22 shift right
			// top bit (38) needs to end up in bit 23, so that would actually shift right by 15
			Red >>= 15;
			
			// the multiply should put it in 16.10 fixed point, but need it back in 8.30
			Green = ( ( Color16 & c_iGreenMask16 ) * ( Color24 & c_iGreenMask24 ) );
			Green |= ( ( Green << ( 64 - ( 16 + 10 ) ) ) >> 63 );
			
			// top bit (25) needs to end up in bit 15, so shift right by 10
			Green >>= 10;
			
			// the multiply should put it in 13.0 fixed point
			Blue = ( ( Color16 & c_iBlueMask16 ) * ( Color24 & c_iBlueMask24 ) );
			Blue |= ( ( Blue << ( 64 - ( 13 + 0 ) ) ) >> 63 );
			
			// top bit (12) needs to end up in bit 7
			Blue >>= 5;
			
			return ( Red & c_iRedMask24 ) | ( Green & c_iGreenMask24 ) | ( Blue & c_iBlueMask24 );
			
			
			//return SetRed24 ( Clamp8 ( ( GetRed24 ( Color1 ) * GetRed24 ( Color2 ) ) >> 7 ) ) |
			//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( Color1 ) * GetGreen24 ( Color2 ) ) >> 7 ) ) |
			//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( Color1 ) * GetBlue24 ( Color2 ) ) >> 7 ) );
		}

		
		inline static u16 SemiTransparency16 ( u16 B, u16 F, u32 abrCode )
		{
			static const u32 ShiftSame = 0;
			static const u32 ShiftHalf = 1;
			static const u32 ShiftQuarter = 2;
			
			static const u32 c_iBitsPerPixel = 5;
			//static const u32 c_iShiftHalf_Mask = ~( ( 1 << 4 ) + ( 1 << 9 ) );
			static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
			//static const u32 c_iShiftQuarter_Mask = ~( ( 3 << 3 ) + ( 3 << 8 ) );
			static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
			//static const u32 c_iClamp_Mask = ( ( 1 << 5 ) + ( 1 << 10 ) + ( 1 << 15 ) );
			static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
			static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
			static const u32 c_iPixelMask = 0x7fff;
			
			u32 Red, Green, Blue;
			
			u32 Color, Actual, Mask;
			
			switch ( abrCode )
			{
				// 0.5xB+0.5 xF
				case 0:
					//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftHalf ) + ( GetRed16( F ) >> ShiftHalf ) ) ) |
					//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftHalf ) + ( GetGreen16( F ) >> ShiftHalf ) ) ) |
					//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftHalf ) + ( GetBlue16( F ) >> ShiftHalf ) ) );
					
					Mask = B & F & c_iLoBitMask;
					Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
					return Color;
					
					break;
				
				// 1.0xB+1.0 xF
				case 1:
					//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftSame ) ) ) |
					//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftSame ) ) ) |
					//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftSame ) ) );
					
					B &= c_iPixelMask;
					F &= c_iPixelMask;
					Actual = B + F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual - Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color |= Mask;
					return Color;
					
					break;
					
				// 1.0xB-1.0 xF
				case 2:
					//Color = SetRed16 ( Clamp5 ( (s32) ( GetRed16 ( B ) >> ShiftSame ) - (s32) ( GetRed16( F ) >> ShiftSame ) ) ) |
					//		SetGreen16 ( Clamp5 ( (s32) ( GetGreen16 ( B ) >> ShiftSame ) - (s32) ( GetGreen16( F ) >> ShiftSame ) ) ) |
					//		SetBlue16 ( Clamp5 ( (s32) ( GetBlue16 ( B ) >> ShiftSame ) - (s32) ( GetBlue16( F ) >> ShiftSame ) ) );
					
					B &= c_iPixelMask;
					F &= c_iPixelMask;
					Actual = B - F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual + Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color &= ~Mask;
					return Color;
					
					break;
					
				// 1.0xB+0.25xF
				case 3:
					//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftQuarter ) ) ) |
					//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftQuarter ) ) ) |
					//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftQuarter ) ) );
					
					B &= c_iPixelMask;
					F = ( F >> 2 ) & c_iShiftQuarter_Mask;
					Actual = B + F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual - Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color |= Mask;
					return Color;
					
					break;
			}
			
			return Color;
		}



		
		inline u32 SemiTransparency24 ( u32 B, u32 F, u32 abrCode )
		{
			static const u32 ShiftSame = 0;
			static const u32 ShiftHalf = 1;
			static const u32 ShiftQuarter = 2;
			
			static const u32 c_iBitsPerPixel = 8;
			//static const u32 c_iShiftHalf_Mask = ~( ( 1 << 4 ) + ( 1 << 9 ) );
			static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
			//static const u32 c_iShiftQuarter_Mask = ~( ( 3 << 3 ) + ( 3 << 8 ) );
			static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
			//static const u32 c_iClamp_Mask = ( ( 1 << 5 ) + ( 1 << 10 ) + ( 1 << 15 ) );
			static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
			static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
			
			u32 Color, Actual, Mask;
			
			//u32 Red, Green, Blue, Color;
			
			switch ( abrCode )
			{
				// 0.5xB+0.5 xF
				case 0:
					//Color = SetRed24 ( Clamp8 ( ( GetRed24 ( B ) >> ShiftHalf ) + ( GetRed24( F ) >> ShiftHalf ) ) ) |
					//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( B ) >> ShiftHalf ) + ( GetGreen24( F ) >> ShiftHalf ) ) ) |
					//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( B ) >> ShiftHalf ) + ( GetBlue24( F ) >> ShiftHalf ) ) );
					
					Mask = B & F & c_iLoBitMask;
					Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
					return Color;
					
					break;
				
				// 1.0xB+1.0 xF
				case 1:
					//Color = SetRed24 ( Clamp8 ( ( GetRed24 ( B ) >> ShiftSame ) + ( GetRed24( F ) >> ShiftSame ) ) ) |
					//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( B ) >> ShiftSame ) + ( GetGreen24( F ) >> ShiftSame ) ) ) |
					//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( B ) >> ShiftSame ) + ( GetBlue24( F ) >> ShiftSame ) ) );
					
					Actual = B + F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual - Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color |= Mask;
					return Color;
					
					break;
					
				// 1.0xB-1.0 xF
				case 2:
					//Color = SetRed24 ( Clamp8 ( (s32) ( GetRed24 ( B ) >> ShiftSame ) - (s32) ( GetRed24( F ) >> ShiftSame ) ) ) |
					//		SetGreen24 ( Clamp8 ( (s32) ( GetGreen24 ( B ) >> ShiftSame ) - (s32) ( GetGreen24( F ) >> ShiftSame ) ) ) |
					//		SetBlue24 ( Clamp8 ( (s32) ( GetBlue24 ( B ) >> ShiftSame ) - (s32) ( GetBlue24( F ) >> ShiftSame ) ) );
					
					Actual = B - F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual + Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color &= ~Mask;
					return Color;
					
					break;
					
				// 1.0xB+0.28xF
				case 3:
					//Color = SetRed24 ( Clamp8 ( ( GetRed24 ( B ) >> ShiftSame ) + ( GetRed24( F ) >> ShiftQuarter ) ) ) |
					//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( B ) >> ShiftSame ) + ( GetGreen24( F ) >> ShiftQuarter ) ) ) |
					//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( B ) >> ShiftSame ) + ( GetBlue24( F ) >> ShiftQuarter ) ) );
					
					F = ( F >> 2 ) & c_iShiftQuarter_Mask;
					Actual = B + F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual - Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color |= Mask;
					return Color;
					
					break;
			}
			
			return Color;
		}

		
		// checks if it is ok to draw to ps1 frame buffer or not by looking at DFE and LCF
		inline bool isDrawOk ()
		{
			if ( GPU_CTRL_Read.DFE || ( !GPU_CTRL_Read.DFE && !GPU_CTRL_Read.LCF ) )
			{
				return true;
			}
			else
			{
				//BusyCycles = 0;
				return false;
				//return true;
			}
		}


		static void sRun () { _GPU->Run (); }
		static void Set_EventCallback ( funcVoid2 UpdateEvent_CB ) { _GPU->NextEvent_Idx = UpdateEvent_CB ( sRun ); };
		
		
		// for interrupt call back
		static funcVoid UpdateInterrupts;
		static void Set_IntCallback ( funcVoid UpdateInt_CB ) { UpdateInterrupts = UpdateInt_CB; };
		

		static const u32 c_InterruptBit = 1;
		static const u32 c_InterruptBit_Vsync = 0;
		
#ifdef PS2_COMPILE
		static const u32 c_InterruptBit_EVsync = 11;
#endif

		//static u32* _Intc_Master;
		static u32* _Intc_Stat;
		static u32* _Intc_Mask;
		static u32* _R3000A_Status_12;
		static u32* _R3000A_Cause_13;
		static u64* _ProcStatus;
		
		inline void ConnectInterrupt ( u32* _IStat, u32* _IMask, u32* _R3000A_Status, u32* _R3000A_Cause, u64* _ProcStat )
		{
			_Intc_Stat = _IStat;
			_Intc_Mask = _IMask;
			//_Intc_Master = _IMaster;
			_R3000A_Cause_13 = _R3000A_Cause;
			_R3000A_Status_12 = _R3000A_Status;
			_ProcStatus = _ProcStat;
		}
		
		
		inline static void SetInterrupt ()
		{
			//*_Intc_Master |= ( 1 << c_InterruptBit );
			*_Intc_Stat |= ( 1 << c_InterruptBit );
			
			UpdateInterrupts ();
			
			/*
			if ( *_Intc_Stat & *_Intc_Mask ) *_R3000A_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
			*/
		}
		
		inline static void ClearInterrupt ()
		{
			//*_Intc_Master &= ~( 1 << c_InterruptBit );
			*_Intc_Stat &= ~( 1 << c_InterruptBit );

			UpdateInterrupts ();

			/*
			if ( ! ( *_Intc_Stat & *_Intc_Mask ) ) *_R3000A_Cause_13 &= ~( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
			*/
		}

		
		
		inline static void SetInterrupt_Vsync ()
		{
			//*_Intc_Master |= ( 1 << c_InterruptBit_Vsync );
			*_Intc_Stat |= ( 1 << c_InterruptBit_Vsync );
			
			UpdateInterrupts ();
			
			/*
			if ( *_Intc_Stat & *_Intc_Mask ) *_R3000A_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
			*/
		}
		
		inline static void ClearInterrupt_Vsync ()
		{
			//*_Intc_Master &= ~( 1 << c_InterruptBit_Vsync );
			*_Intc_Stat &= ~( 1 << c_InterruptBit_Vsync );
			
			UpdateInterrupts ();
			
			/*
			if ( ! ( *_Intc_Stat & *_Intc_Mask ) ) *_R3000A_Cause_13 &= ~( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
			*/
		}


#ifdef PS2_COMPILE
		inline static void SetInterrupt_EVsync ()
		{
			*_Intc_Stat |= ( 1 << c_InterruptBit_EVsync );
			
			UpdateInterrupts ();
			
			/*
			if ( *_Intc_Stat & *_Intc_Mask ) *_R3000A_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
			*/
		}
#endif

		
		static u64* _NextSystemEvent;

		
		////////////////////////////////
		// Debug Info
		static u32* _DebugPC;
		static u64* _DebugCycleCount;
		static u64* _SystemCycleCount;
		static u32 *_NextEventIdx;
		
		static bool DebugWindow_Enabled;
		static void DebugWindow_Enable ();
		static void DebugWindow_Disable ();
		static void DebugWindow_Update ();


template <const uint32_t ABRCODE>		
inline static u16 SemiTransparency16_t ( u16 B, u16 F )
{
	static const u32 ShiftSame = 0;
	static const u32 ShiftHalf = 1;
	static const u32 ShiftQuarter = 2;
	
	static const u32 c_iBitsPerPixel = 5;
	//static const u32 c_iShiftHalf_Mask = ~( ( 1 << 4 ) + ( 1 << 9 ) );
	static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
	//static const u32 c_iShiftQuarter_Mask = ~( ( 3 << 3 ) + ( 3 << 8 ) );
	static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
	//static const u32 c_iClamp_Mask = ( ( 1 << 5 ) + ( 1 << 10 ) + ( 1 << 15 ) );
	static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
	static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
	static const u32 c_iPixelMask = 0x7fff;
	
	//u32 Red, Green, Blue;
	
	u32 Color, Actual, Mask;
	
	switch ( ABRCODE )
	{
		// 0.5xB+0.5 xF
		case 0:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftHalf ) + ( GetRed16( F ) >> ShiftHalf ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftHalf ) + ( GetGreen16( F ) >> ShiftHalf ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftHalf ) + ( GetBlue16( F ) >> ShiftHalf ) ) );
			
			Mask = B & F & c_iLoBitMask;
			Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
			
			break;
		
		// 1.0xB+1.0 xF
		case 1:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftSame ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftSame ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftSame ) ) );
			
			B &= c_iPixelMask;
			F &= c_iPixelMask;
			Actual = B + F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual - Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color |= Mask;
			
			break;
			
		// 1.0xB-1.0 xF
		case 2:
			//Color = SetRed16 ( Clamp5 ( (s32) ( GetRed16 ( B ) >> ShiftSame ) - (s32) ( GetRed16( F ) >> ShiftSame ) ) ) |
			//		SetGreen16 ( Clamp5 ( (s32) ( GetGreen16 ( B ) >> ShiftSame ) - (s32) ( GetGreen16( F ) >> ShiftSame ) ) ) |
			//		SetBlue16 ( Clamp5 ( (s32) ( GetBlue16 ( B ) >> ShiftSame ) - (s32) ( GetBlue16( F ) >> ShiftSame ) ) );
			
			B &= c_iPixelMask;
			F &= c_iPixelMask;
			Actual = B - F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual + Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color &= ~Mask;
			
			break;
			
		// 1.0xB+0.25xF
		case 3:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftQuarter ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftQuarter ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftQuarter ) ) );
			
			B &= c_iPixelMask;
			F = ( F >> 2 ) & c_iShiftQuarter_Mask;
			Actual = B + F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual - Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color |= Mask;
			
			break;
	}
	
	return Color;
}





template<const uint32_t SHADED, const uint32_t TEXTURE, const uint32_t DTD, const uint32_t TGE, const uint32_t SETPIXELMASK, const uint32_t PIXELMASK, const uint32_t ABE, const uint32_t ABR, const uint32_t TP>
static uint64_t DrawTriangle_Generic_th ( DATA_Write_Format* inputdata, uint32_t ulThreadNum )
{
	const int local_id = 0;
	const int group_id = 0;
	const int num_local_threads = 1;
	const int num_global_groups = 1;
	
//#ifdef SINGLE_SCANLINE_MODE
	const int xid = local_id;
	const int yid = 0;
	
	const int xinc = num_local_threads;
	const int yinc = num_global_groups;
	s32 group_yoffset = 0;
//#endif


//inputdata format:
//0: GPU_CTRL_Read
//1: DrawArea_TopLeft
//2: DrawArea_BottomRight
//3: DrawArea_Offset
//4: (TextureWindow)(not used here)
//5: ------------
//6: ------------
//7:GetBGR0_8 ( Buffer [ 0 ] );
//8:GetXY0 ( Buffer [ 1 ] );
//9:GetCLUT ( Buffer [ 2 ] );
//9:GetUV0 ( Buffer [ 2 ] );
//10:GetBGR1_8 ( Buffer [ 3 ] );
//11:GetXY1 ( Buffer [ 4 ] );
//12:GetTPAGE ( Buffer [ 5 ] );
//12:GetUV1 ( Buffer [ 5 ] );
//13:GetBGR2_8 ( Buffer [ 6 ] );
//14:GetXY2 ( Buffer [ 7 ] );
//15:GetUV2 ( Buffer [ 8 ] );

	
	u32 GPU_CTRL_Read;
	//u32 GPU_CTRL_Read_ABR;
	s32 DrawArea_BottomRightX, DrawArea_TopLeftX, DrawArea_BottomRightY, DrawArea_TopLeftY;
	s32 DrawArea_OffsetX, DrawArea_OffsetY;
	u32 Command_ABE;
	u32 Command_TGE;
	u32 GPU_CTRL_Read_DTD;
	
	u16 *ptr;
	
	s32 Temp;
	s32 LeftMostX, RightMostX;
	
	s32 StartX, EndX;
	s32 StartY, EndY;

	s32 DitherValue;

	// new variables
	s32 x0, x1, x2, y0, y1, y2;
	s32 dx_left, dx_right;
	s32 x_left, x_right;
	s32 x_across;
	u32 bgr, bgr_temp;
	s32 Line;
	s32 t0, t1, denominator;

	// more variables for gradient triangle
	s32 dR_left, dG_left, dB_left;
	s32 dR_across, dG_across, dB_across;
	s32 iR, iG, iB;
	s32 R_left, G_left, B_left;
	s32 Roff_left, Goff_left, Boff_left;
	s32 r0, r1, r2, g0, g1, g2, b0, b1, b2;
	//s32 gr [ 3 ], gg [ 3 ], gb [ 3 ];
	
	// variables for texture triangle
	s32 dU_left, dV_left;
	s32 dU_across, dV_across;
	s32 iU, iV;
	s32 U_left, V_left;
	s32 Uoff_left, Voff_left;
	s32 u0, u1, u2, v0, v1, v2;
	s32 gu [ 3 ], gv [ 3 ];
	

	s32 gx [ 3 ], gy [ 3 ], gbgr [ 3 ];
	
	s32 xoff_left, xoff_right;
	
	s32 Red, Green, Blue;
	u32 DestPixel;
	u32 PixelMask, SetPixelMask;

	u32 Coord0, Coord1, Coord2;


	u32 color_add;
	
	u16 *ptr_texture, *ptr_clut;
	u32 clut_xoffset, clut_yoffset;
	u32 clut_x, clut_y, tpage_tx, tpage_ty, tpage_abr, tpage_tp;
	
	u32 TexCoordX, TexCoordY;
	u32 Shift1, Shift2, And1, And2;
	u32 TextureOffset;

	u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	u32 TWX, TWY, TWW, TWH;
	
	u64 NumPixels;
	
	
	// setup vars
	//if ( !local_id )
	//{
		// no bitmaps in opencl ??
		GPU_CTRL_Read = inputdata [ 0 ].Value;
		DrawArea_TopLeftX = inputdata [ 1 ].Value & 0x3ff;
		DrawArea_TopLeftY = ( inputdata [ 1 ].Value >> 10 ) & 0x3ff;
		DrawArea_BottomRightX = inputdata [ 2 ].Value & 0x3ff;
		DrawArea_BottomRightY = ( inputdata [ 2 ].Value >> 10 ) & 0x3ff;
		DrawArea_OffsetX = ( ( (s32) inputdata [ 3 ].Value ) << 21 ) >> 21;
		DrawArea_OffsetY = ( ( (s32) inputdata [ 3 ].Value ) << 10 ) >> 21;
		
		gx [ 0 ] = (s32) ( ( inputdata [ 8 ].x << 5 ) >> 5 );
		gy [ 0 ] = (s32) ( ( inputdata [ 8 ].y << 5 ) >> 5 );
		gx [ 1 ] = (s32) ( ( inputdata [ 11 ].x << 5 ) >> 5 );
		gy [ 1 ] = (s32) ( ( inputdata [ 11 ].y << 5 ) >> 5 );
		gx [ 2 ] = (s32) ( ( inputdata [ 14 ].x << 5 ) >> 5 );
		gy [ 2 ] = (s32) ( ( inputdata [ 14 ].y << 5 ) >> 5 );
		
		Coord0 = 0;
		Coord1 = 1;
		Coord2 = 2;
		
		///////////////////////////////////
		// put top coordinates in x0,y0
		//if ( y1 < y0 )
		if ( gy [ Coord1 ] < gy [ Coord0 ] )
		{
			//Swap ( y0, y1 );
			//Swap ( Coord0, Coord1 );
			Temp = Coord0;
			Coord0 = Coord1;
			Coord1 = Temp;
		}
		
		//if ( y2 < y0 )
		if ( gy [ Coord2 ] < gy [ Coord0 ] )
		{
			//Swap ( y0, y2 );
			//Swap ( Coord0, Coord2 );
			Temp = Coord0;
			Coord0 = Coord2;
			Coord2 = Temp;
		}
		
		///////////////////////////////////////
		// put middle coordinates in x1,y1
		//if ( y2 < y1 )
		if ( gy [ Coord2 ] < gy [ Coord1 ] )
		{
			//Swap ( y1, y2 );
			//Swap ( Coord1, Coord2 );
			Temp = Coord1;
			Coord1 = Coord2;
			Coord2 = Temp;
		}
		
		// get x-values
		x0 = gx [ Coord0 ];
		x1 = gx [ Coord1 ];
		x2 = gx [ Coord2 ];
		
		// get y-values
		y0 = gy [ Coord0 ];
		y1 = gy [ Coord1 ];
		y2 = gy [ Coord2 ];

		//////////////////////////////////////////
		// get coordinates on screen
		x0 += DrawArea_OffsetX;
		y0 += DrawArea_OffsetY;
		x1 += DrawArea_OffsetX;
		y1 += DrawArea_OffsetY;
		x2 += DrawArea_OffsetX;
		y2 += DrawArea_OffsetY;
		
		// get the left/right most x
		LeftMostX = ( ( x0 < x1 ) ? x0 : x1 );
		LeftMostX = ( ( x2 < LeftMostX ) ? x2 : LeftMostX );
		RightMostX = ( ( x0 > x1 ) ? x0 : x1 );
		RightMostX = ( ( x2 > RightMostX ) ? x2 : RightMostX );
		
		// check for some important conditions
		if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
		{
			//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
			return 0;
		}
		
		if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
		{
			//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
			return 0;
		}

		// check if sprite is within draw area
		if ( RightMostX <= ((s32)DrawArea_TopLeftX) || LeftMostX > ((s32)DrawArea_BottomRightX) || y2 <= ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return 0;
		
		// skip drawing if distance between vertices is greater than max allowed by GPU
		if ( ( _Abs( x1 - x0 ) > c_MaxPolygonWidth ) || ( _Abs( x2 - x1 ) > c_MaxPolygonWidth ) || ( y1 - y0 > c_MaxPolygonHeight ) || ( y2 - y1 > c_MaxPolygonHeight ) )
		{
			// skip drawing polygon
			return 0;
		}
		
		/////////////////////////////////////////////////
		// draw top part of triangle
		
		// denominator is negative when x1 is on the left, positive when x1 is on the right
		t0 = y1 - y2;
		t1 = y0 - y2;
		denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );

		NumPixels = _Abs ( denominator ) >> 1;
		if ( ( !ulThreadNum ) && _GPU->ulNumberOfThreads )
		{
			return NumPixels;
		}

		if ( _GPU->bEnable_OpenCL )
		{
			return NumPixels;
		}

		
		if ( SHADED )
		{
		gbgr [ 0 ] = inputdata [ 7 ].Value & 0x00ffffff;
		gbgr [ 1 ] = inputdata [ 10 ].Value & 0x00ffffff;
		gbgr [ 2 ] = inputdata [ 13 ].Value & 0x00ffffff;
		
		// get rgb-values
		r0 = gbgr [ Coord0 ] & 0xff;
		r1 = gbgr [ Coord1 ] & 0xff;
		r2 = gbgr [ Coord2 ] & 0xff;
		g0 = ( gbgr [ Coord0 ] >> 8 ) & 0xff;
		g1 = ( gbgr [ Coord1 ] >> 8 ) & 0xff;
		g2 = ( gbgr [ Coord2 ] >> 8 ) & 0xff;
		b0 = ( gbgr [ Coord0 ] >> 16 ) & 0xff;
		b1 = ( gbgr [ Coord1 ] >> 16 ) & 0xff;
		b2 = ( gbgr [ Coord2 ] >> 16 ) & 0xff;
		}
		else
		{
			bgr = inputdata [ 7 ].Value & 0x00ffffff;
			
			if ( TEXTURE )
			{
				color_add = bgr;
			}
			else
			{
				// convert color to 16-bit (mono-color triangle)
				bgr = ( ( bgr >> 3 ) & 0x1f ) | ( ( bgr >> 6 ) & 0x3e0 ) | ( ( bgr >> 9 ) & 0x7c00 );
			}
		}
		
		
		
		if ( TEXTURE )
		{
		gu [ 0 ] = inputdata [ 9 ].u;
		gu [ 1 ] = inputdata [ 12 ].u;
		gu [ 2 ] = inputdata [ 15 ].u;
		gv [ 0 ] = inputdata [ 9 ].v;
		gv [ 1 ] = inputdata [ 12 ].v;
		gv [ 2 ] = inputdata [ 15 ].v;
		
		// bits 0-5 in upper halfword
		clut_x = ( inputdata [ 9 ].Value >> ( 16 + 0 ) ) & 0x3f;
		clut_y = ( inputdata [ 9 ].Value >> ( 16 + 6 ) ) & 0x1ff;

		TWY = ( inputdata [ 4 ].Value >> 15 ) & 0x1f;
		TWX = ( inputdata [ 4 ].Value >> 10 ) & 0x1f;
		TWH = ( inputdata [ 4 ].Value >> 5 ) & 0x1f;
		TWW = inputdata [ 4 ].Value & 0x1f;
		
		// bits 0-3
		//tpage_tx = GPU_CTRL_Read & 0xf;
		tpage_tx = ( inputdata [ 12 ].Value >> ( 16 + 0 ) ) & 0xf;
		
		// bit 4
		//tpage_ty = ( GPU_CTRL_Read >> 4 ) & 1
		tpage_ty = ( inputdata [ 12 ].Value >> ( 16 + 4 ) ) & 1;

		TWYTWH = ( ( TWY & TWH ) << 3 );
		TWXTWW = ( ( TWX & TWW ) << 3 );
		
		Not_TWH = ~( TWH << 3 );
		Not_TWW = ~( TWW << 3 );

		/////////////////////////////////////////////////////////
		// Get offset into texture page
		TextureOffset = ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 );
		
		clut_xoffset = clut_x << 4;
		
		// get u,v coords
		u0 = gu [ Coord0 ];
		u1 = gu [ Coord1 ];
		u2 = gu [ Coord2 ];
		v0 = gv [ Coord0 ];
		v1 = gv [ Coord1 ];
		v2 = gv [ Coord2 ];
		
		ptr_clut = & ( _GPU->VRAM [ clut_y << 10 ] );
		ptr_texture = & ( _GPU->VRAM [ ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) ] );
		}
		
		//GPU_CTRL_Read_ABR = ( GPU_CTRL_Read >> 5 ) & 3;
		//Command_ABE = inputdata [ 7 ].Command & 2;
		//Command_TGE = inputdata [ 7 ].Command & 1;
		
		//if ( ( bgr & 0x00ffffff ) == 0x00808080 ) Command_TGE = 1;


		
		// DTD is bit 9 in GPU_CTRL_Read
		//GPU_CTRL_Read_DTD = ( GPU_CTRL_Read >> 9 ) & 1;
		
		// ME is bit 12
		//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
		//PixelMask = ( GPU_CTRL_Read & 0x1000 ) << 3;
		
		// MD is bit 11
		//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
		//SetPixelMask = ( GPU_CTRL_Read & 0x0800 ) << 4;
		
		
		// initialize number of pixels drawn
		//NumberOfPixelsDrawn = 0;
		
		
		// bits 5-6
		//GPU_CTRL_Read_ABR = ( GPU_CTRL_Read >> 5 ) & 3;
		//tpage_abr = ( inputdata [ 12 ].Value >> ( 16 + 5 ) ) & 3;
		
		// bits 7-8
		//tpage_tp = ( GPU_CTRL_Read >> 7 ) & 3;
		//tpage_tp = ( inputdata [ 12 ].Value >> ( 16 + 7 ) ) & 3;
		
		//Shift1 = 0;
		//Shift2 = 0;
		//And1 = 0;
		//And2 = 0;

		
		//if ( tpage_tp == 0 )
		//{
		//	And2 = 0xf;
		//	
		//	Shift1 = 2; Shift2 = 2;
		//	And1 = 3; And2 = 0xf;
		//}
		//else if ( tpage_tp == 1 )
		//{
		//	And2 = 0xff;
		//	
		//	Shift1 = 1; Shift2 = 3;
		//	And1 = 1; And2 = 0xff;
		//}
		
		

		// ?? convert to 16-bit ?? (or should leave 24-bit?)
		//bgr = gbgr [ 0 ];
		//bgr = ( ( bgr >> 9 ) & 0x7c00 ) | ( ( bgr >> 6 ) & 0x3e0 ) | ( ( bgr >> 3 ) & 0x1f );
		
		//color_add = bgr;
		
		if ( denominator )
		{
			if ( SHADED )
			{
				dR_across = (long)((((long long)(((r0 - r2) * t0) - ((r1 - r2) * t1))) << 16) / ((long long)denominator));
				dG_across = (long)((((long long)(((g0 - g2) * t0) - ((g1 - g2) * t1))) << 16) / ((long long)denominator));
				dB_across = (long)((((long long)(((b0 - b2) * t0) - ((b1 - b2) * t1))) << 16) / ((long long)denominator));
			
			//dR_across <<= 8;
			//dG_across <<= 8;
			//dB_across <<= 8;
			}
			
			if ( TEXTURE )
			{
				dU_across = (long)((((long long)(((u0 - u2) * t0) - ((u1 - u2) * t1))) << 16) / ((long long)denominator));
				dV_across = (long)((((long long)(((v0 - v2) * t0) - ((v1 - v2) * t1))) << 16) / ((long long)denominator));
			
			//dU_across <<= 8;
			//dV_across <<= 8;
			}
			
		}
		
		
		
		
		//if ( denominator < 0 )
		//{
			// x1 is on the left and x0 is on the right //
			
			////////////////////////////////////
			// get slopes
			
			if ( y1 - y0 )
			{
				/////////////////////////////////////////////
				// init x on the left and right
				x_left = ( x0 << 16 );
				x_right = x_left;
				
				if ( SHADED )
				{
				R_left = ( r0 << 16 );
				G_left = ( g0 << 16 );
				B_left = ( b0 << 16 );
				}

				if ( TEXTURE )
				{
				U_left = ( u0 << 16 );
				V_left = ( v0 << 16 );
				}
				
				if ( denominator < 0 )
				{
					//dx_left = ( ((s64)( x1 - x0 )) * r10 ) >> 32;
					//dx_right = ( ((s64)( x2 - x0 )) * r20 ) >> 32;
					dx_left = ( ( x1 - x0 ) << 16 ) / ( y1 - y0 );
					dx_right = ( ( x2 - x0 ) << 16 ) / ( y2 - y0 );
					
					if ( SHADED )
					{
					dR_left = (( r1 - r0 ) << 16 ) / ( y1 - y0 );
					dG_left = (( g1 - g0 ) << 16 ) / ( y1 - y0 );
					dB_left = (( b1 - b0 ) << 16 ) / ( y1 - y0 );
					}
					
					if ( TEXTURE )
					{
					dU_left = ( (( u1 - u0 ) << 16 ) ) / ( y1 - y0 );
					dV_left = ( (( v1 - v0 ) << 16 ) ) / ( y1 - y0 );
					}
				}
				else
				{
					//dx_right = ( ((s64)( x1 - x0 )) * r10 ) >> 32;
					//dx_left = ( ((s64)( x2 - x0 )) * r20 ) >> 32;
					dx_right = ( ( x1 - x0 ) << 16 ) / ( y1 - y0 );
					dx_left = ( ( x2 - x0 ) << 16 ) / ( y2 - y0 );
					
					if ( SHADED )
					{
					dR_left = (( r2 - r0 ) << 16 ) / ( y2 - y0 );
					dG_left = (( g2 - g0 ) << 16 ) / ( y2 - y0 );
					dB_left = (( b2 - b0 ) << 16 ) / ( y2 - y0 );
					}
					
					if ( TEXTURE )
					{
					dU_left = ( (( u2 - u0 ) << 16 ) ) / ( y2 - y0 );
					dV_left = ( (( v2 - v0 ) << 16 ) ) / ( y2 - y0 );
					}
				}
			}
			else
			{
				if ( denominator < 0 )
				{
					// change x_left and x_right where y1 is on left
					x_left = ( x1 << 16 );
					x_right = ( x0 << 16 );
					
					if ( SHADED )
					{
					R_left = ( r1 << 16 );
					G_left = ( g1 << 16 );
					B_left = ( b1 << 16 );
					}

					if ( TEXTURE )
					{
					U_left = ( u1 << 16 );
					V_left = ( v1 << 16 );
					}
					
					if ( y2 - y1 )
					{
						//dx_left = ( ((s64)( x2 - x1 )) * r21 ) >> 32;
						//dx_right = ( ((s64)( x2 - x0 )) * r20 ) >> 32;
						dx_left = ( ( x2 - x1 ) << 16 ) / ( y2 - y1 );
						dx_right = ( ( x2 - x0 ) << 16 ) / ( y2 - y0 );
						
						if ( SHADED )
						{
						dR_left = (( r2 - r1 ) << 16 ) / ( y2 - y1 );
						dG_left = (( g2 - g1 ) << 16 ) / ( y2 - y1 );
						dB_left = (( b2 - b1 ) << 16 ) / ( y2 - y1 );
						}
						
						if ( TEXTURE )
						{
						dU_left = ( (( u2 - u1 ) << 16 ) ) / ( y2 - y1 );
						dV_left = ( (( v2 - v1 ) << 16 ) ) / ( y2 - y1 );
						}
					}
				}
				else
				{
					x_right = ( x1 << 16 );
					x_left = ( x0 << 16 );
				
					if ( SHADED )
					{
					R_left = ( r0 << 16 );
					G_left = ( g0 << 16 );
					B_left = ( b0 << 16 );
					}
					
					if ( TEXTURE )
					{
					U_left = ( u0 << 16 );
					V_left = ( v0 << 16 );
					}
					
					if ( y2 - y1 )
					{
						//dx_right = ( ((s64)( x2 - x1 )) * r21 ) >> 32;
						//dx_left = ( ((s64)( x2 - x0 )) * r20 ) >> 32;
						dx_right = ( ( x2 - x1 ) << 16 ) / ( y2 - y1 );
						dx_left = ( ( x2 - x0 ) << 16 ) / ( y2 - y0 );
						
						if ( SHADED )
						{
						dR_left = (( r2 - r0 ) << 16 ) / ( y2 - y0 );
						dG_left = (( g2 - g0 ) << 16 ) / ( y2 - y0 );
						dB_left = (( b2 - b0 ) << 16 ) / ( y2 - y0 );
						}
						
						if ( TEXTURE )
						{
						dU_left = ( (( u2 - u0 ) << 16 ) ) / ( y2 - y0 );
						dV_left = ( (( v2 - v0 ) << 16 ) ) / ( y2 - y0 );
						}
					}
				}
			}
		//}
		


	
		////////////////
		// *** TODO *** at this point area of full triangle can be calculated and the rest of the drawing can be put on another thread *** //
		
		
		
		if ( SHADED )
		{
		// r,g,b values are not specified with a fractional part, so there must be an initial fractional part
		R_left |= ( 1 << 15 );
		G_left |= ( 1 << 15 );
		B_left |= ( 1 << 15 );
		}

		if ( TEXTURE )
		{
		U_left |= ( 1 << 15 );
		V_left |= ( 1 << 15 );
		}

		//x_left += 0xffff;
		//x_right -= 1;
		
		StartY = y0;
		EndY = y1;

		if ( StartY < ((s32)DrawArea_TopLeftY) )
		{
			
			if ( EndY < ((s32)DrawArea_TopLeftY) )
			{
				Temp = EndY - StartY;
				StartY = EndY;
			}
			else
			{
				Temp = DrawArea_TopLeftY - StartY;
				StartY = DrawArea_TopLeftY;
			}
			
			x_left += dx_left * Temp;
			x_right += dx_right * Temp;
			
			if ( SHADED )
			{
			R_left += dR_left * Temp;
			G_left += dG_left * Temp;
			B_left += dB_left * Temp;
			}
			
			if ( TEXTURE )
			{
			U_left += dU_left * Temp;
			V_left += dV_left * Temp;
			}
		}
		
		if ( EndY > ((s32)DrawArea_BottomRightY) )
		{
			EndY = DrawArea_BottomRightY + 1;
		}


	//}	// end if ( !local_id )
		
	
	/////////////////////////////////////////////
	// init x on the left and right
	
	


	if ( EndY > StartY )
	{
	
	// in opencl, each worker could be on a different line
	xoff_left = x_left + ( dx_left * (group_yoffset + yid) );
	xoff_right = x_right + ( dx_right * (group_yoffset + yid) );
	
	if ( SHADED )
	{
	Roff_left = R_left + ( dR_left * (group_yoffset + yid) );
	Goff_left = G_left + ( dG_left * (group_yoffset + yid) );
	Boff_left = B_left + ( dB_left * (group_yoffset + yid) );
	}
	
	if ( TEXTURE )
	{
	Uoff_left = U_left + ( dU_left * (group_yoffset + yid) );
	Voff_left = V_left + ( dV_left * (group_yoffset + yid) );
	}
	
	
	//////////////////////////////////////////////
	// draw down to y1
	for ( Line = StartY + group_yoffset + yid; Line < EndY; Line += yinc )
	{
		// left point is included if points are equal
		StartX = ( xoff_left + 0xffffLL ) >> 16;
		EndX = ( xoff_right - 1 ) >> 16;
		
		
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX >= ((s32)DrawArea_TopLeftX) && EndX >= StartX )
		{
			if ( SHADED )
			{
			iR = Roff_left;
			iG = Goff_left;
			iB = Boff_left;
			}
			
			if ( TEXTURE )
			{
			iU = Uoff_left;
			iV = Voff_left;
			}
			
			// get the difference between x_left and StartX
			Temp = ( StartX << 16 ) - xoff_left;
			
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp += ( DrawArea_TopLeftX - StartX ) << 16;
				StartX = DrawArea_TopLeftX;
				
			}
			
			if ( SHADED )
			{
			iR += ( dR_across >> 8 ) * ( Temp >> 8 );
			iG += ( dG_across >> 8 ) * ( Temp >> 8 );
			iB += ( dB_across >> 8 ) * ( Temp >> 8 );
			}
			
			if ( TEXTURE )
			{
			iU += ( dU_across >> 8 ) * ( Temp >> 8 );
			iV += ( dV_across >> 8 ) * ( Temp >> 8 );
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				//EndX = DrawArea_BottomRightX + 1;
				EndX = DrawArea_BottomRightX;
			}
			
			//ptr = & ( _GPU->VRAM [ StartX + xid + ( Line << 10 ) ] );
			//DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			
			
			if ( SHADED )
			{
			iR += ( dR_across * xid );
			iG += ( dG_across * xid );
			iB += ( dB_across * xid );
			}

			if ( TEXTURE )
			{
			iU += ( dU_across * xid );
			iV += ( dV_across * xid );
			}
			
			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
			for ( x_across = StartX + xid; x_across <= EndX; x_across += xinc )
			{
				if ( TEXTURE )
				{
				TexCoordY = (u8) ( ( ( iV >> 16 ) & Not_TWH ) | ( TWYTWH ) );
				//TexCoordY <<= 10;
				switch ( TP )
				{
					case 0:
					case 1:
						TexCoordY <<= 11;
						break;
						
					case 2:
						TexCoordY <<= 10;
						break;
				}

				//TexCoordX = (u8) ( ( iU & ~( TWW << 3 ) ) | ( ( TWX & TWW ) << 3 ) );
				TexCoordX = (u8) ( ( ( iU >> 16 ) & Not_TWW ) | ( TWXTWW ) );

				switch ( TP )
				{
					case 0:
						bgr = ((u8*)ptr_texture) [ ( TexCoordX >> 1 ) + TexCoordY ];
						bgr = ptr_clut [ ( clut_xoffset + ( ( bgr >> ( ( TexCoordX & 1 ) << 2 ) ) & 0xf ) ) & FrameBuffer_XMask ];
						break;
						
					case 1:
						bgr = ((u8*)ptr_texture) [ ( TexCoordX ) + TexCoordY ];
						bgr = ptr_clut [ ( clut_xoffset + bgr ) & FrameBuffer_XMask ];
						break;
						
					case 2:
						bgr = ptr_texture [ ( TexCoordX ) + TexCoordY ];
						break;
				}

				
				}	// end if ( TEXTURE )

				if (bgr || !TEXTURE)
				{

					ptr = &(_GPU->VRAM[(x_across & FrameBuffer_XMask) + ((Line & FrameBuffer_YMask) << 10)]);

					// shade pixel color


					// read pixel from frame buffer if we need to check mask bit
					if (PIXELMASK || ABE)
					{
						DestPixel = *ptr;
					}

					if (SHADED)
					{

						if (DTD)
						{
							DitherValue = c_iDitherValues16[(x_across & 3) + ((Line & 3) << 2)];

							// perform dither
							Red = iR + DitherValue;
							Green = iG + DitherValue;
							Blue = iB + DitherValue;

							if (TEXTURE)
							{
								// perform shift
								Red >>= (16 + 0);
								Green >>= (16 + 0);
								Blue >>= (16 + 0);

								//Red = clamp ( Red, 0, 0x1f );
								//Green = clamp ( Green, 0, 0x1f );
								//Blue = clamp ( Blue, 0, 0x1f );
								Red = Clamp8(Red);
								Green = Clamp8(Green);
								Blue = Clamp8(Blue);
							}
							else
							{
								// perform shift
								Red >>= (16 + 3);
								Green >>= (16 + 3);
								Blue >>= (16 + 3);

								//Red = clamp ( Red, 0, 0x1f );
								//Green = clamp ( Green, 0, 0x1f );
								//Blue = clamp ( Blue, 0, 0x1f );
								Red = Clamp5(Red);
								Green = Clamp5(Green);
								Blue = Clamp5(Blue);
							}
						}
						else
						{
							if (TEXTURE)
							{
								Red = iR >> (16 + 0);
								Green = iG >> (16 + 0);
								Blue = iB >> (16 + 0);
							}
							else
							{
								Red = iR >> (16 + 3);
								Green = iG >> (16 + 3);
								Blue = iB >> (16 + 3);
							}
						}

						if (TEXTURE)
						{
							color_add = (Blue << 16) | (Green << 8) | Red;
						}
						else
						{
							bgr = (Blue << 10) | (Green << 5) | Red;
						}

					}


					bgr_temp = bgr;


					if (TEXTURE && !TGE)
					{
						// brightness calculation
						//bgr_temp = Color24To16 ( ColorMultiply24 ( Color16To24 ( bgr_temp ), color_add ) );
						bgr_temp = ColorMultiply1624(bgr, color_add);
					}

					// semi-transparency
					if (ABE)
					{
						if ((bgr & 0x8000) || !TEXTURE)
						{
							bgr_temp = SemiTransparency16_t<ABR>(DestPixel, bgr_temp);
						}
					}


					// draw pixel if we can draw to mask pixels or mask bit not set
					if (TEXTURE)
					{
						if (!(DestPixel & PIXELMASK)) *ptr = bgr_temp | SETPIXELMASK | (bgr & 0x8000);
					}
					else
					{
						if (!(DestPixel & PIXELMASK)) *ptr = bgr_temp | SETPIXELMASK;
					}

				}
						
				if ( SHADED )
				{
				iR += ( dR_across * xinc );
				iG += ( dG_across * xinc );
				iB += ( dB_across * xinc );
				}
			
				if ( TEXTURE )
				{
				iU += ( dU_across * xinc );
				iV += ( dV_across * xinc );
				}
					
				//ptr += c_iVectorSize;
				//ptr += xinc;
			}
			
		}
		
		
		/////////////////////////////////////
		// update x on left and right
		xoff_left += ( dx_left * yinc );
		xoff_right += ( dx_right * yinc );
		
		if ( SHADED )
		{
		Roff_left += ( dR_left * yinc );
		Goff_left += ( dG_left * yinc );
		Boff_left += ( dB_left * yinc );
		}
		
		if ( TEXTURE )
		{
		Uoff_left += ( dU_left * yinc );
		Voff_left += ( dV_left * yinc );
		}
	}

	} // end if ( EndY > StartY )

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	//if ( !local_id )
	//{
		//////////////////////////////////////////////////////
		// check if y1 is on the left or on the right
		if ( denominator < 0 )
		{
			x_left = ( x1 << 16 );

			x_right = ( x0 << 16 ) + ( dx_right * ( y1 - y0 ) );
			
			if ( SHADED )
			{
			R_left = ( r1 << 16 );
			G_left = ( g1 << 16 );
			B_left = ( b1 << 16 );
			}
			
			if ( TEXTURE )
			{
			U_left = ( u1 << 16 );
			V_left = ( v1 << 16 );
			}
			
			if ( y2 - y1 )
			{
				//dx_left = ( ((s64)( x2 - x1 )) * r21 ) >> 32;
				dx_left = (( x2 - x1 ) << 16 ) / ( y2 - y1 );
				
				if ( SHADED )
				{
				//dR_left = ( ((s64)( r2 - r1 )) * r21 ) >> 24;
				//dG_left = ( ((s64)( g2 - g1 )) * r21 ) >> 24;
				//dB_left = ( ((s64)( b2 - b1 )) * r21 ) >> 24;
				dR_left = (( r2 - r1 ) << 16 ) / ( y2 - y1 );
				dG_left = (( g2 - g1 ) << 16 ) / ( y2 - y1 );
				dB_left = (( b2 - b1 ) << 16 ) / ( y2 - y1 );
				}
				
				if ( TEXTURE )
				{
				dU_left = ( (( u2 - u1 ) << 16 ) ) / ( y2 - y1 );
				dV_left = ( (( v2 - v1 ) << 16 ) ) / ( y2 - y1 );
				}
			}
		}
		else
		{
			x_right = ( x1 << 16 );

			x_left = ( x0 << 16 ) + ( dx_left * ( y1 - y0 ) );
			
			if ( SHADED )
			{
			R_left = ( r0 << 16 ) + ( dR_left * ( y1 - y0 ) );
			G_left = ( g0 << 16 ) + ( dG_left * ( y1 - y0 ) );
			B_left = ( b0 << 16 ) + ( dB_left * ( y1 - y0 ) );
			}
			
			if ( TEXTURE )
			{
			U_left = ( u0 << 16 ) + ( dU_left * ( y1 - y0 ) );
			V_left = ( v0 << 16 ) + ( dV_left * ( y1 - y0 ) );
			}
			
			if ( y2 - y1 )
			{
				//dx_right = ( ((s64)( x2 - x1 )) * r21 ) >> 32;
				dx_right = (( x2 - x1 ) << 16 ) / ( y2 - y1 );
			}
		}


		if ( SHADED )
		{
		R_left += ( 1 << 15 );
		G_left += ( 1 << 15 );
		B_left += ( 1 << 15 );
		}

		if ( TEXTURE )
		{
		U_left += ( 1 << 15 );
		V_left += ( 1 << 15 );
		}

		//x_left += 0xffff;
		//x_right -= 1;
		
		StartY = y1;
		EndY = y2;

		if ( StartY < ((s32)DrawArea_TopLeftY) )
		{
			
			if ( EndY < ((s32)DrawArea_TopLeftY) )
			{
				Temp = EndY - StartY;
				StartY = EndY;
			}
			else
			{
				Temp = DrawArea_TopLeftY - StartY;
				StartY = DrawArea_TopLeftY;
			}
			
			x_left += dx_left * Temp;
			x_right += dx_right * Temp;
			
			if ( SHADED )
			{
			R_left += dR_left * Temp;
			G_left += dG_left * Temp;
			B_left += dB_left * Temp;
			}
			
			if ( TEXTURE )
			{
			U_left += dU_left * Temp;
			V_left += dV_left * Temp;
			}
		}
		
		if ( EndY > ((s32)DrawArea_BottomRightY) )
		{
			EndY = DrawArea_BottomRightY + 1;
		}
		
	//}

	
	if ( EndY > StartY )
	{
	
	// in opencl, each worker could be on a different line
	xoff_left = x_left + ( dx_left * (group_yoffset + yid) );
	xoff_right = x_right + ( dx_right * (group_yoffset + yid) );
	
	if ( SHADED )
	{
	Roff_left = R_left + ( dR_left * (group_yoffset + yid) );
	Goff_left = G_left + ( dG_left * (group_yoffset + yid) );
	Boff_left = B_left + ( dB_left * (group_yoffset + yid) );
	}
	
	if ( TEXTURE )
	{
	Uoff_left = U_left + ( dU_left * (group_yoffset + yid) );
	Voff_left = V_left + ( dV_left * (group_yoffset + yid) );
	}
	
	
	//////////////////////////////////////////////
	// draw down to y1
	for ( Line = StartY + group_yoffset + yid; Line < EndY; Line += yinc )
	{
		// left point is included if points are equal
		StartX = ( xoff_left + 0xffffLL ) >> 16;
		EndX = ( xoff_right - 1 ) >> 16;
		
		
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX >= ((s32)DrawArea_TopLeftX) && EndX >= StartX )
		{
			if ( SHADED )
			{
			iR = Roff_left;
			iG = Goff_left;
			iB = Boff_left;
			}
			
			if ( TEXTURE )
			{
			iU = Uoff_left;
			iV = Voff_left;
			}
			
			// get the difference between x_left and StartX
			Temp = ( StartX << 16 ) - xoff_left;
			
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp += ( DrawArea_TopLeftX - StartX ) << 16;
				StartX = DrawArea_TopLeftX;
				
			}
			
			if ( SHADED )
			{
			iR += ( dR_across >> 8 ) * ( Temp >> 8 );
			iG += ( dG_across >> 8 ) * ( Temp >> 8 );
			iB += ( dB_across >> 8 ) * ( Temp >> 8 );
			}
			
			if ( TEXTURE )
			{
			iU += ( dU_across >> 8 ) * ( Temp >> 8 );
			iV += ( dV_across >> 8 ) * ( Temp >> 8 );
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				//EndX = DrawArea_BottomRightX + 1;
				EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( _GPU->VRAM [ StartX + xid + ( Line << 10 ) ] );
			//DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			
			
			if ( SHADED )
			{
			iR += ( dR_across * xid );
			iG += ( dG_across * xid );
			iB += ( dB_across * xid );
			}

			if ( TEXTURE )
			{
			iU += ( dU_across * xid );
			iV += ( dV_across * xid );
			}
			
			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
			for ( x_across = StartX + xid; x_across <= EndX; x_across += xinc )
			{
				if ( TEXTURE )
				{
				TexCoordY = (u8) ( ( ( iV >> 16 ) & Not_TWH ) | ( TWYTWH ) );
				//TexCoordY <<= 10;
				switch ( TP )
				{
					case 0:
					case 1:
						TexCoordY <<= 11;
						break;
						
					case 2:
						TexCoordY <<= 10;
						break;
				}

				//TexCoordX = (u8) ( ( iU & ~( TWW << 3 ) ) | ( ( TWX & TWW ) << 3 ) );
				TexCoordX = (u8) ( ( ( iU >> 16 ) & Not_TWW ) | ( TWXTWW ) );
				
				switch ( TP )
				{
					case 0:
						bgr = ((u8*)ptr_texture) [ ( TexCoordX >> 1 ) + TexCoordY ];
						bgr = ptr_clut [ ( clut_xoffset + ( ( bgr >> ( ( TexCoordX & 1 ) << 2 ) ) & 0xf ) ) & FrameBuffer_XMask ];
						break;
						
					case 1:
						bgr = ((u8*)ptr_texture) [ ( TexCoordX ) + TexCoordY ];
						bgr = ptr_clut [ ( clut_xoffset + bgr ) & FrameBuffer_XMask ];
						break;
						
					case 2:
						bgr = ptr_texture [ ( TexCoordX ) + TexCoordY ];
						break;
				}
				
				
				/*
				//bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
				bgr = ptr_texture [ ( TexCoordX >> SHIFT1 ) + TexCoordY ];
				
				if ( SHIFT1 )
				{
					//bgr = VRAM [ ( ( ( clut_x << 4 ) + TexelIndex ) & FrameBuffer_XMask ) + ( clut_y << 10 ) ];
					bgr = ptr_clut [ ( clut_xoffset + ( ( bgr >> ( ( TexCoordX & AND1 ) << SHIFT2 ) ) & AND2 ) ) & FrameBuffer_XMask ];
				}
				*/
				
				}	// end if ( TEXTURE )

				if ( bgr || !TEXTURE )
				{
					
					// shade pixel color
					
					
					// read pixel from frame buffer if we need to check mask bit
					if ( PIXELMASK || ABE )
					{
					DestPixel = *ptr;
					}
					
					if ( SHADED )
					{
						
					if ( DTD )
					{
						DitherValue = c_iDitherValues16 [ ( x_across & 3 ) + ( ( Line & 3 ) << 2 ) ];
						
						// perform dither
						Red = iR + DitherValue;
						Green = iG + DitherValue;
						Blue = iB + DitherValue;
						
						if ( TEXTURE )
						{
						// perform shift
						Red >>= ( 16 + 0 );
						Green >>= ( 16 + 0 );
						Blue >>= ( 16 + 0 );
						
						//Red = clamp ( Red, 0, 0x1f );
						//Green = clamp ( Green, 0, 0x1f );
						//Blue = clamp ( Blue, 0, 0x1f );
						Red = Clamp8 ( Red );
						Green = Clamp8 ( Green );
						Blue = Clamp8 ( Blue );
						}
						else
						{
						// perform shift
						Red >>= ( 16 + 3 );
						Green >>= ( 16 + 3 );
						Blue >>= ( 16 + 3 );
						
						//Red = clamp ( Red, 0, 0x1f );
						//Green = clamp ( Green, 0, 0x1f );
						//Blue = clamp ( Blue, 0, 0x1f );
						Red = Clamp5 ( Red );
						Green = Clamp5 ( Green );
						Blue = Clamp5 ( Blue );
						}
					}
					else
					{
						if ( TEXTURE )
						{
						Red = iR >> ( 16 + 0 );
						Green = iG >> ( 16 + 0 );
						Blue = iB >> ( 16 + 0 );
						}
						else
						{
						Red = iR >> ( 16 + 3 );
						Green = iG >> ( 16 + 3 );
						Blue = iB >> ( 16 + 3 );
						}
					}
					
					if ( TEXTURE )
					{
					color_add = ( Blue << 16 ) | ( Green << 8 ) | Red;
					}
					else
					{
					bgr = ( Blue << 10 ) | ( Green << 5 ) | Red;
					}
					
					}
					
					
					bgr_temp = bgr;
					
						
					if ( TEXTURE && !TGE )
					{
					// brightness calculation
					//bgr_temp = Color24To16 ( ColorMultiply24 ( Color16To24 ( bgr_temp ), color_add ) );
					bgr_temp = ColorMultiply1624 ( bgr, color_add );
					}
					
					// semi-transparency
					if ( ABE )
					{
						if ( ( bgr & 0x8000 ) || !TEXTURE )
						{
							bgr_temp = SemiTransparency16_t<ABR> ( DestPixel, bgr_temp );
						}
					}
					

					// draw pixel if we can draw to mask pixels or mask bit not set
					if ( TEXTURE )
					{
						if ( ! ( DestPixel & PIXELMASK ) ) *ptr = bgr_temp | SETPIXELMASK | ( bgr & 0x8000 );
					}
					else
					{
						if ( ! ( DestPixel & PIXELMASK ) ) *ptr = bgr_temp | SETPIXELMASK;
					}
					
				}
						
				if ( SHADED )
				{
				iR += ( dR_across * xinc );
				iG += ( dG_across * xinc );
				iB += ( dB_across * xinc );
				}
			
				if ( TEXTURE )
				{
				iU += ( dU_across * xinc );
				iV += ( dV_across * xinc );
				}
					
				//ptr += c_iVectorSize;
				ptr += xinc;
			}
			
		}
		
		
		/////////////////////////////////////
		// update x on left and right
		xoff_left += ( dx_left * yinc );
		xoff_right += ( dx_right * yinc );
		
		if ( SHADED )
		{
		Roff_left += ( dR_left * yinc );
		Goff_left += ( dG_left * yinc );
		Boff_left += ( dB_left * yinc );
		}
		
		if ( TEXTURE )
		{
		Uoff_left += ( dU_left * yinc );
		Voff_left += ( dV_left * yinc );
		}
	}

	} // end if ( EndY > StartY )

		
	return NumPixels;
}



/*
template<const uint32_t SHADED, const uint32_t TEXTURE, const uint32_t DTD, const uint32_t TGE, const uint32_t TP>
inline static u64 Select_Triangle_Renderer2_t ( DATA_Write_Format* p_inputdata, u32 ulThreadNum )
{
	// SETPIXELMASK, PIXELMASK, ABE, ABR
	u32 GPU_CTRL_Read;
	u32 ABE, ABR;
	u32 PixelMask, SetPixelMask;
	u32 Command_ABE;
	u32 Combine;
	
	GPU_CTRL_Read = p_inputdata [ 0 ].Value;
	Command_ABE = p_inputdata [ 7 ].Command & 2;
	
	PixelMask = ( GPU_CTRL_Read & 0x1000 );
	SetPixelMask = ( GPU_CTRL_Read & 0x0800 );
	
	if ( TEXTURE )
	{
		ABR = ( p_inputdata [ 12 ].Value >> ( 16 + 5 ) ) & 3;
	}
	else
	{
		ABR = ( GPU_CTRL_Read >> 5 ) & 3;
	}
	
	Combine = ( ( PixelMask | SetPixelMask ) >> 8 ) | ( Command_ABE << 1 ) | ( ABR );
	
	switch ( Combine )
	{
		case 0:
		case 1:
		case 2:
		case 3:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0,0,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 4:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0,1,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 5:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0,1,1,TP> ( p_inputdata, ulThreadNum );
			break;
		case 6:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0,1,2,TP> ( p_inputdata, ulThreadNum );
			break;
		case 7:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0,1,3,TP> ( p_inputdata, ulThreadNum );
			break;
			
		case 8:
		case 9:
		case 10:
		case 11:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0,0,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 12:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0,1,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 13:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0,1,1,TP> ( p_inputdata, ulThreadNum );
			break;
		case 14:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0,1,2,TP> ( p_inputdata, ulThreadNum );
			break;
		case 15:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0,1,3,TP> ( p_inputdata, ulThreadNum );
			break;
		
		case 16:
		case 17:
		case 18:
		case 19:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0x8000,0,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 20:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0x8000,1,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 21:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0x8000,1,1,TP> ( p_inputdata, ulThreadNum );
			break;
		case 22:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0x8000,1,2,TP> ( p_inputdata, ulThreadNum );
			break;
		case 23:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0,0x8000,1,3,TP> ( p_inputdata, ulThreadNum );
			break;
			
		case 24:
		case 25:
		case 26:
		case 27:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0x8000,0,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 28:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0x8000,1,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 29:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0x8000,1,1,TP> ( p_inputdata, ulThreadNum );
			break;
		case 30:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0x8000,1,2,TP> ( p_inputdata, ulThreadNum );
			break;
		case 31:
			return DrawTriangle_Generic_th<SHADED,TEXTURE,DTD,TGE,0x8000,0x8000,1,3,TP> ( p_inputdata, ulThreadNum );
			break;
	}
	
	return 0;
}
*/


static u64 Select_Triangle_Renderer_t ( DATA_Write_Format* p_inputdata, u32 ulThreadNum )
{
	//template<const uint32_t SHADED, const uint32_t TEXTURE, const uint32_t DTD, const uint32_t TGE, const uint32_t SETPIXELMASK, const uint32_t PIXELMASK, const uint32_t ABE, const uint32_t ABR, const uint32_t TP>
	//static u64 DrawTriangle_Generic_th(DATA_Write_Format * inputdata, u32 ulThreadNum)
	// SHADED, TEXTURE, DTD, TGE, TP
	u32 GPU_CTRL_Read;
	u32 GPU_CTRL_Read_DTD, Command_TGE;
	u32 PixelMask, SetPixelMask;
	u32 TP;
	u32 SHADED, TEXTURE;
	u32 Combine;
	//u32 GPU_CTRL_Read;
	u32 ABR;
	//u32 PixelMask, SetPixelMask;
	u32 Command_ABE;
	//u32 Combine;

	u64 ullCycles;

	GPU_CTRL_Read = p_inputdata[0].Value;
	Command_ABE = (p_inputdata[7].Command & 2) >> 1;

	PixelMask = (GPU_CTRL_Read & 0x1000) >> 12;
	SetPixelMask = (GPU_CTRL_Read & 0x0800) >> 11;

	TEXTURE = (p_inputdata[7].Command >> 2) & 1;

	if (TEXTURE)
	{
		ABR = (p_inputdata[12].Value >> (16 + 5)) & 3;
	}
	else
	{
		ABR = (GPU_CTRL_Read >> 5) & 3;
	}

	GPU_CTRL_Read = p_inputdata [ 0 ].Value;
	
	GPU_CTRL_Read_DTD = ( GPU_CTRL_Read >> 9 ) & 1;
	Command_TGE = p_inputdata [ 7 ].Command & 1;

	TEXTURE = ( p_inputdata [ 7 ].Command >> 2 ) & 1;
	SHADED = ( p_inputdata [ 7 ].Command >> 4 ) & 1;
	
	TP = ( p_inputdata [ 12 ].Value >> ( 16 + 7 ) ) & 3;

	// todo: add additional optimizations (same colors in shaded or shaded texture, etc.)

	Combine = (TP) | (ABR << 2) | (Command_ABE << 4) | (PixelMask << 5) | (SetPixelMask << 6) | (Command_TGE << 7) | (GPU_CTRL_Read_DTD << 8) | (TEXTURE << 9) | (SHADED << 10);

	ullCycles = ps1_arr_triangle_lut[Combine](p_inputdata, 0);

	return ullCycles;


	//PixelMask = ( GPU_CTRL_Read & 0x1000 );
	//SetPixelMask = ( GPU_CTRL_Read & 0x0800 );
	
	//if ( !SHADED ) GPU_CTRL_Read_DTD = 0;
	//if ( !TEXTURE ) Command_TGE = 1;
	
	

	/*
	//Combine = Command_TGE | ( GPU_CTRL_Read_DTD << 1 ) | ( ( PixelMask | SetPixelMask ) >> 9 );
	Combine = ( SHADED << 5 ) | ( GPU_CTRL_Read_DTD << 4 ) | ( TEXTURE << 3 ) | ( Command_TGE << 2 ) | ( TP );
	
	switch ( Combine )
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
			return Select_Triangle_Renderer2_t<0,0,0,1,2> ( p_inputdata, ulThreadNum );
			break;
		case 8:
		case 24:
			// TP = 0
			return Select_Triangle_Renderer2_t<0,1,0,0,0> ( p_inputdata, ulThreadNum );
			break;
		case 9:
		case 25:
			// TP = 1
			return Select_Triangle_Renderer2_t<0,1,0,0,1> ( p_inputdata, ulThreadNum );
			break;
		case 10:
		case 11:
		
		case 26:
		case 27:
			// TP = 2
			return Select_Triangle_Renderer2_t<0,1,0,0,2> ( p_inputdata, ulThreadNum );
			break;
		case 12:
		case 28:
			return Select_Triangle_Renderer2_t<0,1,0,1,0> ( p_inputdata, ulThreadNum );
			break;
		case 13:
		case 29:
			return Select_Triangle_Renderer2_t<0,1,0,1,1> ( p_inputdata, ulThreadNum );
			break;
		case 14:
		case 15:
		
		case 30:
		case 31:
			return Select_Triangle_Renderer2_t<0,1,0,1,2> ( p_inputdata, ulThreadNum );
			break;
			
		case 0 + 32:
		case 1 + 32:
		case 2 + 32:
		case 3 + 32:
		case 4 + 32:
		case 5 + 32:
		case 6 + 32:
		case 7 + 32:
			return Select_Triangle_Renderer2_t<1,0,0,1,2> ( p_inputdata, ulThreadNum );
			break;
		
		case 16 + 32:
		case 17 + 32:
		case 18 + 32:
		case 19 + 32:
		case 20 + 32:
		case 21 + 32:
		case 22 + 32:
		case 23 + 32:
			return Select_Triangle_Renderer2_t<1,0,1,1,2> ( p_inputdata, ulThreadNum );
			break;
		case 8 + 32:
			return Select_Triangle_Renderer2_t<1,1,0,0,0> ( p_inputdata, ulThreadNum );
			break;
		case 24 + 32:
			return Select_Triangle_Renderer2_t<1,1,1,0,0> ( p_inputdata, ulThreadNum );
			break;
		case 9 + 32:
			return Select_Triangle_Renderer2_t<1,1,0,0,1> ( p_inputdata, ulThreadNum );
			break;
		case 25 + 32:
			return Select_Triangle_Renderer2_t<1,1,1,0,1> ( p_inputdata, ulThreadNum );
			break;
		case 10 + 32:
		case 11 + 32:
			return Select_Triangle_Renderer2_t<1,1,0,0,2> ( p_inputdata, ulThreadNum );
			break;
		
		case 26 + 32:
		case 27 + 32:
			return Select_Triangle_Renderer2_t<1,1,1,0,2> ( p_inputdata, ulThreadNum );
			break;
		case 12 + 32:
			return Select_Triangle_Renderer2_t<1,1,0,1,0> ( p_inputdata, ulThreadNum );
			break;
		case 28 + 32:
			return Select_Triangle_Renderer2_t<1,1,1,1,0> ( p_inputdata, ulThreadNum );
			break;
		case 13 + 32:
			return Select_Triangle_Renderer2_t<1,1,0,1,1> ( p_inputdata, ulThreadNum );
			break;
		case 29 + 32:
			return Select_Triangle_Renderer2_t<1,1,1,1,1> ( p_inputdata, ulThreadNum );
			break;
		case 14 + 32:
		case 15 + 32:
			return Select_Triangle_Renderer2_t<1,1,0,1,2> ( p_inputdata, ulThreadNum );
			break;
		
		case 30 + 32:
		case 31 + 32:
			return Select_Triangle_Renderer2_t<1,1,1,1,2> ( p_inputdata, ulThreadNum );
			break;
			
	}

	return 0;
	*/
}


template<const uint32_t TEXTURE, const uint32_t TGE, const uint32_t SETPIXELMASK, const uint32_t PIXELMASK, const uint32_t ABE, const uint32_t ABR, const uint32_t TP>
static uint64_t DrawSprite_Generic_th ( DATA_Write_Format* inputdata, uint32_t ulThreadNum )
{
	const int local_id = 0;
	const int group_id = 0;
	const int num_local_threads = 1;
	const int num_global_groups = 1;
	
//#ifdef SINGLE_SCANLINE_MODE
	const int xid = local_id;
	const int yid = 0;
	
	const int xinc = num_local_threads;
	const int yinc = num_global_groups;
	s32 group_yoffset = 0;
//#endif


//inputdata format:
//0: GPU_CTRL_Read
//1: DrawArea_BottomRightX
//2: DrawArea_TopLeftX
//3: DrawArea_BottomRightY
//4: DrawArea_TopLeftY
//5: DrawArea_OffsetX
//6: DrawArea_OffsetY
//7: TextureWindow
//-------------------------
//0: GPU_CTRL_Read
//1: DrawArea_TopLeft
//2: DrawArea_BottomRight
//3: DrawArea_Offset
//4: TextureWindow
//5: ------------
//6: ------------
//7: GetBGR24 ( Buffer [ 0 ] );
//8: GetXY ( Buffer [ 1 ] );
//9: GetCLUT ( Buffer [ 2 ] );
//9: GetUV ( Buffer [ 2 ] );
//10: GetHW ( Buffer [ 3 ] );


	// notes: looks like sprite size is same as specified by w/h

	//u32 Pixel,

	u32 GPU_CTRL_Read, GPU_CTRL_Read_ABR;
	s32 DrawArea_BottomRightX, DrawArea_TopLeftX, DrawArea_BottomRightY, DrawArea_TopLeftY;
	s32 DrawArea_OffsetX, DrawArea_OffsetY;
	u32 Command_ABE;
	u32 Command_TGE;

	
	u32 TexelIndex;
	
	
	u16 *ptr;
	s32 StartX, EndX, StartY, EndY;
	s32 x, y, w, h;
	
	//s32 group_yoffset;
	
	u32 Temp;
	
	// new variables
	s32 x0, x1, y0, y1;
	s32 u0, v0;
	s32 u, v;
	u32 bgr, bgr_temp;
	s32 iU, iV;
	s32 x_across;
	s32 Line;
	
	u32 DestPixel;
	u32 PixelMask, SetPixelMask;

	
	u32 color_add;
	
	u16 *ptr_texture, *ptr_clut;
	u32 clut_xoffset, clut_yoffset;
	u32 clut_x, clut_y, tpage_tx, tpage_ty, tpage_abr, tpage_tp, command_tge, command_abe, command_abr;
	
	u32 TexCoordX, TexCoordY;
	u32 Shift1, Shift2, And1, And2;
	u32 TextureOffset;

	u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	u32 TWX, TWY, TWW, TWH;
	
	u32 NumPixels;
	

	// set variables
	//if ( !local_id )
	//{
		GPU_CTRL_Read = inputdata [ 0 ].Value;
		DrawArea_TopLeftX = inputdata [ 1 ].Value & 0x3ff;
		DrawArea_TopLeftY = ( inputdata [ 1 ].Value >> 10 ) & 0x3ff;
		DrawArea_BottomRightX = inputdata [ 2 ].Value & 0x3ff;
		DrawArea_BottomRightY = ( inputdata [ 2 ].Value >> 10 ) & 0x3ff;
		DrawArea_OffsetX = ( ( (s32) inputdata [ 3 ].Value ) << 21 ) >> 21;
		DrawArea_OffsetY = ( ( (s32) inputdata [ 3 ].Value ) << 10 ) >> 21;
		
		x = inputdata [ 8 ].x;
		y = inputdata [ 8 ].y;
		
		// x and y are actually 11 bits
		x = ( x << ( 5 + 16 ) ) >> ( 5 + 16 );
		y = ( y << ( 5 + 16 ) ) >> ( 5 + 16 );
		
		w = inputdata [ 10 ].w;
		h = inputdata [ 10 ].h;

		// get top left corner of sprite and bottom right corner of sprite
		x0 = x + DrawArea_OffsetX;
		y0 = y + DrawArea_OffsetY;
		x1 = x0 + w - 1;
		y1 = y0 + h - 1;

		
		// check for some important conditions
		if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
		{
			//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
			return 0;
		}
		
		if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
		{
			//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
			return 0;
		}
		
		// check if sprite is within draw area
		if ( x1 < ((s32)DrawArea_TopLeftX) || x0 > ((s32)DrawArea_BottomRightX) || y1 < ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return 0;

		if ( TEXTURE )
		{
		u = inputdata [ 9 ].u;
		v = inputdata [ 9 ].v;
		
		// get texture coords
		u0 = u;
		v0 = v;
		}
		
		StartX = x0;
		EndX = x1;
		StartY = y0;
		EndY = y1;

		if ( StartY < ((s32)DrawArea_TopLeftY) )
		{
			if ( TEXTURE )
			{
			v0 += ( DrawArea_TopLeftY - StartY );
			}
			
			StartY = DrawArea_TopLeftY;
		}
		
		if ( EndY > ((s32)DrawArea_BottomRightY) )
		{
			EndY = DrawArea_BottomRightY;
		}
		
		if ( StartX < ((s32)DrawArea_TopLeftX) )
		{
			if ( TEXTURE )
			{
			u0 += ( DrawArea_TopLeftX - StartX );
			}
			
			StartX = DrawArea_TopLeftX;
		}
		
		if ( EndX > ((s32)DrawArea_BottomRightX) )
		{
			EndX = DrawArea_BottomRightX;
		}
		
		NumPixels = ( EndX - StartX + 1 ) * ( EndY - StartY + 1 );
		if ( ( !ulThreadNum ) && _GPU->ulNumberOfThreads )
		{
			return NumPixels;
		}
		
		if ( _GPU->bEnable_OpenCL )
		{
			return NumPixels;
		}

		
		// set bgr64
		//bgr64 = gbgr [ 0 ];
		//bgr64 |= ( bgr64 << 16 );
		//bgr64 |= ( bgr64 << 32 );
		bgr = inputdata [ 7 ].Value & 0x00ffffff;
		//bgr = ( ( bgr >> 9 ) & 0x7c00 ) | ( ( bgr >> 6 ) & 0x3e0 ) | ( ( bgr >> 3 ) & 0x1f );


		
		//Command_ABE = inputdata [ 7 ].Command & 2;
		
		// ME is bit 12
		//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
		//PixelMask = ( GPU_CTRL_Read & 0x1000 ) << 3;
		
		// MD is bit 11
		//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
		//SetPixelMask = ( GPU_CTRL_Read & 0x0800 ) << 4;
		
		// bits 5-6
		//GPU_CTRL_Read_ABR = ( GPU_CTRL_Read >> 5 ) & 3;
		
		if ( TEXTURE )
		{
		//Command_TGE = inputdata [ 7 ].Command & 1;
		//if ( ( bgr & 0x00ffffff ) == 0x00808080 ) Command_TGE = 1;

		// bits 0-5 in upper halfword
		clut_x = ( inputdata [ 9 ].Value >> ( 16 + 0 ) ) & 0x3f;
		clut_y = ( inputdata [ 9 ].Value >> ( 16 + 6 ) ) & 0x1ff;
	
		TWY = ( inputdata [ 4 ].Value >> 15 ) & 0x1f;
		TWX = ( inputdata [ 4 ].Value >> 10 ) & 0x1f;
		TWH = ( inputdata [ 4 ].Value >> 5 ) & 0x1f;
		TWW = inputdata [ 4 ].Value & 0x1f;

		// bits 0-3
		tpage_tx = GPU_CTRL_Read & 0xf;
		
		// bit 4
		tpage_ty = ( GPU_CTRL_Read >> 4 ) & 1;
		
		// bits 7-8
		//tpage_tp = ( GPU_CTRL_Read >> 7 ) & 3;
		


		TWYTWH = ( ( TWY & TWH ) << 3 );
		TWXTWW = ( ( TWX & TWW ) << 3 );
		
		Not_TWH = ~( TWH << 3 );
		Not_TWW = ~( TWW << 3 );

		/////////////////////////////////////////////////////////
		// Get offset into texture page
		TextureOffset = ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 );
		
		//////////////////////////////////////////////////////
		// Get offset into color lookup table
		//u32 ClutOffset = ( clut_x << 4 ) + ( clut_y << 10 );
		
		clut_xoffset = clut_x << 4;
		
		
		color_add = bgr;
		
		ptr_clut = & ( _GPU->VRAM [ clut_y << 10 ] );
		ptr_texture = & ( _GPU->VRAM [ ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) ] );

		
		//iV = v0;
		iV = v0 + group_yoffset + yid;
		}
		else
		{
			// convert 24-bit color to 15-bit
			bgr = ( ( bgr >> 3 ) & 0x1f ) | ( ( bgr >> 6 ) & 0x3e0 ) | ( ( bgr >> 9 ) & 0x7c00 );
		}
		
		// initialize number of pixels drawn
		//NumberOfPixelsDrawn = 0;
		
	//}


	// initialize number of pixels drawn
	//NumberOfPixelsDrawn = 0;
	
	
	
	//NumberOfPixelsDrawn = ( EndX - StartX + 1 ) * ( EndY - StartY + 1 );
		


	//for ( Line = StartY; Line <= EndY; Line++ )
		for (Line = StartY + group_yoffset + yid; Line <= EndY; Line += yinc)
		{
			if (TEXTURE)
			{
				// need to start texture coord from left again
				//iU = u0;
				iU = u0 + xid;

				TexCoordY = (u8)((iV & Not_TWH) | (TWYTWH));

				switch (TP)
				{
				case 0:
				case 1:
					TexCoordY <<= 11;
					break;

				case 2:
					TexCoordY <<= 10;
					break;
				}
			}

			//ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			//ptr = &(_GPU->VRAM[StartX + xid + (Line << 10)]);


			// draw horizontal line
			//for ( x_across = StartX; x_across <= EndX; x_across += xinc )
			for (x_across = StartX + xid; x_across <= EndX; x_across += xinc)
			{
				if (TEXTURE)
				{
					//TexCoordX = (u8) ( ( iU & ~( TWW << 3 ) ) | ( ( TWX & TWW ) << 3 ) );
					TexCoordX = (u8)((iU & Not_TWW) | (TWXTWW));

					switch (TP)
					{
					case 0:
						bgr = ((u8*)ptr_texture)[(TexCoordX >> 1) + TexCoordY];
						bgr = ptr_clut[(clut_xoffset + ((bgr >> ((TexCoordX & 1) << 2)) & 0xf)) & FrameBuffer_XMask];
						break;

					case 1:
						bgr = ((u8*)ptr_texture)[(TexCoordX)+TexCoordY];
						bgr = ptr_clut[(clut_xoffset + bgr) & FrameBuffer_XMask];
						break;

					case 2:
						bgr = ptr_texture[(TexCoordX)+TexCoordY];
						break;
					}

				}	// end if ( TEXTURE )


				if (bgr || !TEXTURE)
				{
					// get ptr into vram
					ptr = &(_GPU->VRAM[(x_across & FrameBuffer_XMask) + ((Line & FrameBuffer_YMask) << 10)]);

					if (PIXELMASK || ABE)
					{
						// read pixel from frame buffer if we need to check mask bit
						DestPixel = *ptr;	//VRAM [ x_across + ( Line << 10 ) ];
					}

					bgr_temp = bgr;


					if (TEXTURE && !TGE)
					{
						// brightness calculation
						//bgr_temp = Color24To16 ( ColorMultiply24 ( Color16To24 ( bgr_temp ), color_add ) );
						bgr_temp = ColorMultiply1624(bgr_temp, color_add);
					}


					// semi-transparency
					if (ABE)
					{
						if ((bgr & 0x8000) || !TEXTURE)
						{
							//bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read_ABR );
							bgr_temp = SemiTransparency16_t<ABR>(DestPixel, bgr_temp);
						}
					}

					// check if we should set mask bit when drawing
					//bgr_temp |= SETPIXELMASK | ( bgr & 0x8000 );

					// make sure pointer into VRAM is within the boundaries of vram
					//if (ptr < std::end(_GPU->VRAM))
					{

						if (TEXTURE)
						{
							// draw pixel if we can draw to mask pixels or mask bit not set
							if (!(DestPixel & PIXELMASK)) *ptr = bgr_temp | SETPIXELMASK | (bgr & 0x8000);
						}
						else
						{
							if (!(DestPixel & PIXELMASK)) *ptr = bgr_temp | SETPIXELMASK;
						}

					}	// end if (ptr < std::end(_GPU->VRAM))
				}

				if (TEXTURE)
				{
					/////////////////////////////////////////////////////////
					// interpolate texture coords across
					//iU += c_iVectorSize;
					iU += xinc;
				}

				// update pointer for pixel out
				//ptr += c_iVectorSize;
				//ptr += xinc;

			}

			if (TEXTURE)
			{
				/////////////////////////////////////////////////////////
				// interpolate texture coords down
				//iV++;	//+= dV_left;
				iV += yinc;
			}
		}
	
	return NumPixels;
}


/*
template<const uint32_t TEXTURE, const uint32_t TGE, const uint32_t TP>
static u64 Select_Sprite_Renderer2_t ( DATA_Write_Format* p_inputdata, u32 ulThreadNum )
{
	u32 GPU_CTRL_Read;
	u32 PixelMask, SetPixelMask;
	u32 ABR;
	//u32 TP;
	u32 Command_ABE;
	u32 Combine;
	
	Command_ABE = p_inputdata [ 7 ].Command & 2;
	
	GPU_CTRL_Read = p_inputdata [ 0 ].Value;
	
	PixelMask = ( GPU_CTRL_Read & 0x1000 );
	SetPixelMask = ( GPU_CTRL_Read & 0x0800 );
	
	// bits 5-6
	ABR = ( GPU_CTRL_Read >> 5 ) & 3;
	
	
	Combine = ( ( PixelMask | SetPixelMask ) >> 8 ) | ( Command_ABE << 1 ) | ABR;
	
	switch ( Combine )
	{
		case 0:
		case 1:
		case 2:
		case 3:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0,0,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 4:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0,1,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 5:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0,1,1,TP> ( p_inputdata, ulThreadNum );
			break;
		case 6:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0,1,2,TP> ( p_inputdata, ulThreadNum );
			break;
		case 7:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0,1,3,TP> ( p_inputdata, ulThreadNum );
			break;
		case 8:
		case 9:
		case 10:
		case 11:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0,0,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 12:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0,1,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 13:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0,1,1,TP> ( p_inputdata, ulThreadNum );
			break;
		case 14:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0,1,2,TP> ( p_inputdata, ulThreadNum );
			break;
		case 15:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0,1,3,TP> ( p_inputdata, ulThreadNum );
			break;
		case 16:
		case 17:
		case 18:
		case 19:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0x8000,0,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 20:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0x8000,1,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 21:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0x8000,1,1,TP> ( p_inputdata, ulThreadNum );
			break;
		case 22:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0x8000,1,2,TP> ( p_inputdata, ulThreadNum );
			break;
		case 23:
			return DrawSprite_Generic_th<TEXTURE,TGE,0,0x8000,1,3,TP> ( p_inputdata, ulThreadNum );
			break;
		case 24:
		case 25:
		case 26:
		case 27:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0x8000,0,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 28:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0x8000,1,0,TP> ( p_inputdata, ulThreadNum );
			break;
		case 29:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0x8000,1,1,TP> ( p_inputdata, ulThreadNum );
			break;
		case 30:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0x8000,1,2,TP> ( p_inputdata, ulThreadNum );
			break;
		case 31:
			return DrawSprite_Generic_th<TEXTURE,TGE,0x8000,0x8000,1,3,TP> ( p_inputdata, ulThreadNum );
			break;
	}

	return 0;
}
*/


static u64 Select_Sprite_Renderer_t ( DATA_Write_Format* p_inputdata, u32 ulThreadNum )
{
	//template<const uint32_t TEXTURE, const uint32_t TGE, const uint32_t SETPIXELMASK, const uint32_t PIXELMASK, const uint32_t ABE, const uint32_t ABR, const uint32_t TP>
	//static u64 DrawSprite_Generic_th(DATA_Write_Format * inputdata, u32 ulThreadNum)

	u32 GPU_CTRL_Read;
	u32 Command_TGE;
	u32 PixelMask, SetPixelMask;
	u32 TEXTURE;
	u32 TP;
	
	u32 Combine;

	u32 ABR;
	u32 Command_ABE;

	u64 ullCycles;

	Command_ABE = ( p_inputdata[7].Command & 2 ) >> 1;

	GPU_CTRL_Read = p_inputdata[0].Value;

	PixelMask = (GPU_CTRL_Read & 0x1000) >> 12;
	SetPixelMask = (GPU_CTRL_Read & 0x0800) >> 11;

	// bits 5-6
	ABR = (GPU_CTRL_Read >> 5) & 3;
	
	Command_TGE = p_inputdata [ 7 ].Command & 1;
	
	TEXTURE = ( p_inputdata [ 7 ].Command >> 2 ) & 1;
	
	
	// bits 7-8
	TP = ( GPU_CTRL_Read >> 7 ) & 3;

	// todo: add additional optimizations (same colors in shaded or shaded texture, etc.)

	Combine = (TP) | (ABR << 2) | (Command_ABE << 4) | (PixelMask << 5) | (SetPixelMask << 6) | (Command_TGE << 7) | (TEXTURE << 8);

	ullCycles = ps1_arr_sprite_lut[Combine](p_inputdata, 0);

	return ullCycles;

	
	//if ( !TEXTURE ) Command_TGE = 1;
	
	/*
	Combine = ( TEXTURE << 3 ) | ( Command_TGE << 2 ) | TP;
	
	switch ( Combine )
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			return Select_Sprite_Renderer2_t<0,1,2> ( p_inputdata, ulThreadNum );
			break;
		case 8:
			return Select_Sprite_Renderer2_t<1,0,0> ( p_inputdata, ulThreadNum );
			break;
		case 9:
			return Select_Sprite_Renderer2_t<1,0,1> ( p_inputdata, ulThreadNum );
			break;
		case 10:
		case 11:
			return Select_Sprite_Renderer2_t<1,0,2> ( p_inputdata, ulThreadNum );
			break;
		case 12:
			return Select_Sprite_Renderer2_t<1,1,0> ( p_inputdata, ulThreadNum );
			break;
		case 13:
			return Select_Sprite_Renderer2_t<1,1,1> ( p_inputdata, ulThreadNum );
			break;
		case 14:
		case 15:
			return Select_Sprite_Renderer2_t<1,1,2> ( p_inputdata, ulThreadNum );
			break;
	}

	return 0;
	*/
}



template<const uint32_t SHADED, const uint32_t DTD, const uint32_t PIXELMASK, const uint32_t SETPIXELMASK, const uint32_t ABE, const uint32_t ABR>
static uint64_t DrawLine_Generic_th ( DATA_Write_Format* inputdata, uint32_t ulThreadNum )
{
	const int local_id = 0;
	const int group_id = 0;
	const int num_local_threads = 1;
	const int num_global_groups = 1;
	
//#ifdef SINGLE_SCANLINE_MODE
	const int xid = 0;
	const int yid = 0;
	
	const int xinc = num_local_threads;
	const int yinc = num_global_groups;
	int group_yoffset = group_id;
//#endif

//inputdata format:
//0: GPU_CTRL_Read
//1: DrawArea_TopLeft
//2: DrawArea_BottomRight
//3: DrawArea_Offset
//4: (TextureWindow)(not used here)
//5: ------------
//6: ------------
//7: GetBGR24 ( Buffer [ 0 ] );
//8: GetXY0 ( Buffer [ 1 ] );
//9: GetXY1 ( Buffer [ 2 ] );
//10: GetXY2 ( Buffer [ 3 ] );



	u32 GPU_CTRL_Read, GPU_CTRL_Read_ABR;
	s32 DrawArea_BottomRightX, DrawArea_TopLeftX, DrawArea_BottomRightY, DrawArea_TopLeftY;
	s32 DrawArea_OffsetX, DrawArea_OffsetY;
	u32 GPU_CTRL_Read_DTD;
	s32 DitherValue;
	
	s32 Temp;
	s32 LeftMostX, RightMostX;
	
	
	// the y starts and ends at the same place, but the x is different for each line
	s32 StartY, EndY;
	
	
	//s64 r10, r20, r21;
	
	// new variables
	s32 x0, x1, y0, y1;
	s32 dx_left, dx_right;
	u32 bgr;
	s32 t0, t1, denominator;

	s32 iR, iG, iB;
	//s32 R_left, G_left, B_left;
	//s32 Roff_left, Goff_left, Boff_left;
	s32 r0, r1, r2, g0, g1, g2, b0, b1, b2;
	s32 Red, Blue, Green;
	s32 dr, dg, db;
	
	u32 Coord0, Coord1, Coord2;
	
	u32 PixelMask, SetPixelMask;
	
	s32 gx [ 3 ], gy [ 3 ], gbgr [ 3 ];
	u32 Command_ABE;
	
	s32 x_left, x_right;
	
	//s32 group_yoffset;
	
	
	s32 StartX, EndX;
	s32 x_across;
	u32 xoff, yoff;
	s32 xoff_left, xoff_right;
	u32 DestPixel;
	u32 bgr_temp;
	s32 Line;
	u16 *ptr;
	
	s32 x_distance, y_distance, line_length, incdec;
	s32 y_left, dy_left, yoff_left;
	
	s32 ix, iy, dx, dy;
	
	u32 NumPixels;
	
	// setup vars
	//if ( !local_id )
	//{
		
		// no bitmaps in opencl ??
		GPU_CTRL_Read = inputdata [ 0 ].Value;
		DrawArea_TopLeftX = inputdata [ 1 ].Value & 0x3ff;
		DrawArea_TopLeftY = ( inputdata [ 1 ].Value >> 10 ) & 0x3ff;
		DrawArea_BottomRightX = inputdata [ 2 ].Value & 0x3ff;
		DrawArea_BottomRightY = ( inputdata [ 2 ].Value >> 10 ) & 0x3ff;
		DrawArea_OffsetX = ( ( (s32) inputdata [ 3 ].Value ) << 21 ) >> 21;
		DrawArea_OffsetY = ( ( (s32) inputdata [ 3 ].Value ) << 10 ) >> 21;

		gx [ 0 ] = (s32) ( ( inputdata [ 8 ].x << 5 ) >> 5 );
		gy [ 0 ] = (s32) ( ( inputdata [ 8 ].y << 5 ) >> 5 );
		gx [ 1 ] = (s32) ( ( inputdata [ 10 ].x << 5 ) >> 5 );
		gy [ 1 ] = (s32) ( ( inputdata [ 10 ].y << 5 ) >> 5 );
		//gx [ 2 ] = (s32) ( ( inputdata [ 10 ].x << 5 ) >> 5 );
		//gy [ 2 ] = (s32) ( ( inputdata [ 10 ].y << 5 ) >> 5 );
		
		Coord0 = 0;
		Coord1 = 1;
		//Coord2 = 2;

		///////////////////////////////////
		// put top coordinates in x0,y0
		//if ( y1 < y0 )
		if ( gy [ Coord1 ] < gy [ Coord0 ] )
		{
			//Swap ( y0, y1 );
			//Swap ( Coord0, Coord1 );
			Temp = Coord0;
			Coord0 = Coord1;
			Coord1 = Temp;
		}
		
		// get x-values
		x0 = gx [ Coord0 ];
		x1 = gx [ Coord1 ];
		
		// get y-values
		y0 = gy [ Coord0 ];
		y1 = gy [ Coord1 ];

		//////////////////////////////////////////
		// get coordinates on screen
		x0 += DrawArea_OffsetX;
		y0 += DrawArea_OffsetY;
		x1 += DrawArea_OffsetX;
		y1 += DrawArea_OffsetY;
		
		// get the left/right most x
		LeftMostX = ( ( x0 < x1 ) ? x0 : x1 );
		//LeftMostX = ( ( x2 < LeftMostX ) ? x2 : LeftMostX );
		RightMostX = ( ( x0 > x1 ) ? x0 : x1 );
		//RightMostX = ( ( x2 > RightMostX ) ? x2 : RightMostX );

		// check for some important conditions
		if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
		{
			//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
			return 0;
		}
		
		if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
		{
			//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
			return 0;
		}

		// check if sprite is within draw area
		if ( RightMostX < ((s32)DrawArea_TopLeftX) || LeftMostX > ((s32)DrawArea_BottomRightX) || y1 < ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return 0;
		
		// skip drawing if distance between vertices is greater than max allowed by GPU
		if ( ( _Abs( x1 - x0 ) > c_MaxPolygonWidth ) || ( y1 - y0 > c_MaxPolygonHeight ) )
		{
			// skip drawing polygon
			return 0;
		}
		
		x_distance = _Abs( x1 - x0 );
		y_distance = _Abs( y1 - y0 );

		if ( x_distance > y_distance )
		{
			NumPixels = x_distance;
			
			if ( LeftMostX < ((s32)DrawArea_TopLeftX) )
			{
				NumPixels -= ( DrawArea_TopLeftX - LeftMostX );
			}
			
			if ( RightMostX > ((s32)DrawArea_BottomRightX) )
			{
				NumPixels -= ( RightMostX - DrawArea_BottomRightX );
			}
		}
		else
		{
			NumPixels = y_distance;
			
			if ( y0 < ((s32)DrawArea_TopLeftY) )
			{
				NumPixels -= ( DrawArea_TopLeftY - y0 );
			}
			
			if ( y1 > ((s32)DrawArea_BottomRightY) )
			{
				NumPixels -= ( y1 - DrawArea_BottomRightY );
			}
		}
		
		if ( ( !ulThreadNum ) && _GPU->ulNumberOfThreads )
		{
			return NumPixels;
		}

		if ( _GPU->bEnable_OpenCL )
		{
			return NumPixels;
		}

		
		gbgr [ 0 ] = inputdata [ 7 ].Value & 0x00ffffff;
		
		//GPU_CTRL_Read_ABR = ( GPU_CTRL_Read >> 5 ) & 3;
		//Command_ABE = inputdata [ 7 ].Command & 2;
		
		
		// ME is bit 12
		//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
		//PixelMask = ( GPU_CTRL_Read & 0x1000 ) << 3;
		
		// MD is bit 11
		//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
		//SetPixelMask = ( GPU_CTRL_Read & 0x0800 ) << 4;
		
		
		// initialize number of pixels drawn
		//NumberOfPixelsDrawn = 0;
		
		
		
		if ( SHADED )
		{
		// DTD is bit 9 in GPU_CTRL_Read
		//GPU_CTRL_Read_DTD = ( GPU_CTRL_Read >> 9 ) & 1;

		gbgr [ 1 ] = inputdata [ 9 ].Value & 0x00ffffff;
		
		// get rgb-values
		r0 = gbgr [ Coord0 ] & 0xff;
		r1 = gbgr [ Coord1 ] & 0xff;

		g0 = ( gbgr [ Coord0 ] >> 8 ) & 0xff;
		g1 = ( gbgr [ Coord1 ] >> 8 ) & 0xff;

		b0 = ( gbgr [ Coord0 ] >> 16 ) & 0xff;
		b1 = ( gbgr [ Coord1 ] >> 16 ) & 0xff;
		
		iR = ( r0 << 16 ) + 0x8000;
		iG = ( g0 << 16 ) + 0x8000;
		iB = ( b0 << 16 ) + 0x8000;
		
		}
		else
		{
		// get color(s)
		bgr = gbgr [ 0 ];
		
		// ?? convert to 16-bit ?? (or should leave 24-bit?)
		//bgr = ( ( bgr & ( 0xf8 << 0 ) ) >> 3 ) | ( ( bgr & ( 0xf8 << 8 ) ) >> 6 ) | ( ( bgr & ( 0xf8 << 16 ) ) >> 9 );
		bgr = ( ( bgr >> 9 ) & 0x7c00 ) | ( ( bgr >> 6 ) & 0x3e0 ) | ( ( bgr >> 3 ) & 0x1f );
		}

		
		
		
		/////////////////////////////////////////////////
		// draw top part of triangle
		
		// denominator is negative when x1 is on the left, positive when x1 is on the right
		//t0 = y1 - y2;
		//t1 = y0 - y2;
		//denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );

		// get reciprocals
		// *** todo ***
		//if ( y1 - y0 ) r10 = ( 1LL << 48 ) / ((s64)( y1 - y0 ));
		//if ( y2 - y0 ) r20 = ( 1LL << 48 ) / ((s64)( y2 - y0 ));
		//if ( y2 - y1 ) r21 = ( 1LL << 48 ) / ((s64)( y2 - y1 ));
		
		///////////////////////////////////////////
		// start at y0
		//Line = y0;
		
	
	StartX = x0;
	EndX = x1;
	StartY = y0;
	EndY = y1;
	
	// check if line is horizontal
	if ( x_distance > y_distance )
	{
		
		// get the largest length
		line_length = x_distance;
		
		//if ( denominator < 0 )
		//{
			// x1 is on the left and x0 is on the right //
			
			////////////////////////////////////
			// get slopes
			
		//ix = x0;
		iy = ( y0 << 16 ) + 0x8000;
		//x_right = x_left;
		
		
		//if ( y1 - y0 )
		if ( line_length )
		{
			/////////////////////////////////////////////
			// init x on the left and right
			
			//dx_left = ( ( x1 - x0 ) << 16 ) / ( ( y1 - y0 ) + 1 );
			//dx = ( ( x1 - x0 ) << 16 ) / line_length;
			dy = ( ( y1 - y0 ) << 16 ) / line_length;
			
			if ( SHADED )
			{
			dr = ( ( r1 - r0 ) << 16 ) / line_length;
			dg = ( ( g1 - g0 ) << 16 ) / line_length;
			db = ( ( b1 - b0 ) << 16 ) / line_length;
			}
		}

		
		// check if line is going left or right
		if ( x1 > x0 )
		{
			// line is going to the right
			incdec = 1;
			
			// clip against edge of screen
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
				
				iy += dy * Temp;
				
				if ( SHADED )
				{
				iR += dr * Temp;
				iG += dg * Temp;
				iB += db * Temp;
				}
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
			}
		}
		else
		{
			// line is going to the left from the right
			incdec = -1;
			
			// clip against edge of screen
			if ( StartX > ((s32)DrawArea_BottomRightX) )
			{
				Temp = StartX - DrawArea_BottomRightX;
				StartX = DrawArea_BottomRightX;
				
				iy += dy * Temp;
				
				if ( SHADED )
				{
				iR += dr * Temp;
				iG += dg * Temp;
				iB += db * Temp;
				}
			}
			
			if ( EndX < ((s32)DrawArea_TopLeftX) )
			{
				EndX = DrawArea_TopLeftX - 1;
			}
		}
		
		
		if ( dy <= 0 )
		{
	
			if ( ( iy >> 16 ) < ((s32)DrawArea_TopLeftY) )
			{
				return NumPixels;
			}
			//else
			//{
			//	// line is veering onto screen
			//	
			//	// get y value it hits screen at
			//	ix = ( ( ( y0 << 16 ) + 0x8000 ) - ( ((s32)DrawArea_TopLeftY) << 16 ) ) / ( dy >> 8 );
			//	ix -= ( x0 << 8 ) + 0xff;
			//	
			//}
			
			if ( EndY < ((s32)DrawArea_TopLeftY) )
			{
				// line is going down, so End Y would
				EndY = DrawArea_TopLeftY - 1;
			}
		}
		
		if ( dy >= 0 )
		{
			if ( ( iy >> 16 ) > ((s32)DrawArea_BottomRightY) )
			{
				// line is veering off screen
				return NumPixels;
			}
			
			if ( EndY > ((s32)DrawArea_BottomRightY) )
			{
				// line is going down, so End Y would
				EndY = DrawArea_BottomRightY + 1;
			}
		}
		
		
		////////////////
		// *** TODO *** at this point area of full triangle can be calculated and the rest of the drawing can be put on another thread *** //
		
		// draw the line horizontally
		for ( ix = StartX; ix != EndX; ix += incdec )
		{
			Line = iy >> 16;
			
			if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
			{
				if ( SHADED )
				{
				if ( DTD )
				{
					//bgr = ( _Round( iR ) >> 32 ) | ( ( _Round( iG ) >> 32 ) << 8 ) | ( ( _Round( iB ) >> 32 ) << 16 );
					//bgr = ( _Round( iR ) >> 35 ) | ( ( _Round( iG ) >> 35 ) << 5 ) | ( ( _Round( iB ) >> 35 ) << 10 );
					//DitherValue = DitherLine [ x_across & 0x3 ];
					DitherValue = c_iDitherValues16 [ ( ix & 3 ) + ( ( Line & 3 ) << 2 ) ];
					
					// perform dither
					//Red = iR + DitherValue;
					//Green = iG + DitherValue;
					//Blue = iB + DitherValue;
					Red = iR + DitherValue;
					Green = iG + DitherValue;
					Blue = iB + DitherValue;
					
					//Red = Clamp5 ( ( iR + DitherValue ) >> 27 );
					//Green = Clamp5 ( ( iG + DitherValue ) >> 27 );
					//Blue = Clamp5 ( ( iB + DitherValue ) >> 27 );
					
					// perform shift
					Red >>= ( 16 + 3 );
					Green >>= ( 16 + 3 );
					Blue >>= ( 16 + 3 );
					
					//Red = clamp ( Red, 0, 0x1f );
					//Green = clamp ( Green, 0, 0x1f );
					//Blue = clamp ( Blue, 0, 0x1f );
					Red = Clamp5 ( Red );
					Green = Clamp5 ( Green );
					Blue = Clamp5 ( Blue );
				}
				else
				{
					Red = iR >> ( 16 + 3 );
					Green = iG >> ( 16 + 3 );
					Blue = iB >> ( 16 + 3 );
				}
					
					
				bgr = ( Blue << 10 ) | ( Green << 5 ) | Red;
				}
				
				
				
				//ptr = & ( _GPU->VRAM [ ix + ( Line << 10 ) ] );
				ptr = &(_GPU->VRAM[(ix & FrameBuffer_XMask) + ((Line & FrameBuffer_YMask) << 10)]);

				if ( ABE || PIXELMASK )
				{
				// read pixel from frame buffer if we need to check mask bit
				DestPixel = *ptr;
				}
				
				
				bgr_temp = bgr;
				
	
				// semi-transparency
				//if ( Command_ABE )
				if ( ABE )
				{
					//bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read_ABR );
					bgr_temp = SemiTransparency16_t<ABR> ( DestPixel, bgr_temp );
				}
				

				// draw pixel if we can draw to mask pixels or mask bit not set
				//if ( ! ( DestPixel & PixelMask ) ) *ptr = ( bgr_temp | SetPixelMask );
				if ( ! ( DestPixel & PIXELMASK ) ) *ptr = ( bgr_temp | SETPIXELMASK );
			}
			
			iy += dy;
			
			if ( SHADED )
			{
			iR += dr;
			iG += dg;
			iB += db;
			}
		}
		
	}
	else
	{
		// line is vertical //

		// get the largest length
		line_length = y_distance;
		
		//if ( denominator < 0 )
		//{
			// x1 is on the left and x0 is on the right //
			
			////////////////////////////////////
			// get slopes
			
		ix = ( x0 << 16 ) + 0x8000;
		//iy = y0;
		//x_right = x_left;
		
		//if ( y1 - y0 )
		if ( line_length )
		{
			/////////////////////////////////////////////
			// init x on the left and right
			
			//dx_left = ( ( x1 - x0 ) << 16 ) / ( ( y1 - y0 ) + 1 );
			dx = ( ( x1 - x0 ) << 16 ) / line_length;
			//dy = ( ( y1 - y0 ) << 16 ) / line_length;,
			
			if ( SHADED )
			{
			dr = ( ( r1 - r0 ) << 16 ) / line_length;
			dg = ( ( g1 - g0 ) << 16 ) / line_length;
			db = ( ( b1 - b0 ) << 16 ) / line_length;
			}
		}
		
		
		
		// check if line is going up or down
		if ( y1 > y0 )
		{
			// line is going to the down
			incdec = 1;
			
			// clip against edge of screen
			if ( StartY < ((s32)DrawArea_TopLeftY) )
			{
				Temp = DrawArea_TopLeftY - StartY;
				StartY = DrawArea_TopLeftY;
				
				ix += dx * Temp;
				if ( SHADED )
				{
				iR += dr * Temp;
				iG += dg * Temp;
				iB += db * Temp;
				}
			}
			
			if ( EndY > ((s32)DrawArea_BottomRightY) )
			{
				EndY = DrawArea_BottomRightY + 1;
			}
		}
		else
		{
			// line is going to the left from the up
			incdec = -1;
			
			// clip against edge of screen
			if ( StartY > ((s32)DrawArea_BottomRightY) )
			{
				Temp = StartY - DrawArea_BottomRightY;
				StartY = DrawArea_BottomRightY;
				
				ix += dx * Temp;
				if ( SHADED )
				{
				iR += dr * Temp;
				iG += dg * Temp;
				iB += db * Temp;
				}
			}
			
			if ( EndY < ((s32)DrawArea_TopLeftY) )
			{
				EndY = DrawArea_TopLeftY - 1;
			}
		}
	
		if ( dx <= 0 )
		{
			if ( ( ix >> 16 ) < ((s32)DrawArea_TopLeftX) )
			{
				// line is veering off screen
				return NumPixels;
			}
			
			if ( EndX < ((s32)DrawArea_TopLeftX) )
			{
				EndX = DrawArea_TopLeftX - 1;
			}
		}
		
		if ( dx >= 0 )
		{
			if ( ( ix >> 16 ) > ((s32)DrawArea_BottomRightX) )
			{
				// line is veering off screen
				return NumPixels;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
			}
		}
		

	//}	// end if ( !local_id )
	
	
		// draw the line vertically
		for ( iy = StartY; iy != EndY; iy += incdec )
		{
			Line = ix >> 16;
			
			if ( Line >= ((s32)DrawArea_TopLeftX) && Line <= ((s32)DrawArea_BottomRightX) )
			{
				if ( SHADED )
				{
				if ( DTD )
				{
					//DitherValue = DitherLine [ x_across & 0x3 ];
					DitherValue = c_iDitherValues16 [ ( Line & 3 ) + ( ( iy & 3 ) << 2 ) ];
					
					// perform dither
					//Red = iR + DitherValue;
					//Green = iG + DitherValue;
					//Blue = iB + DitherValue;
					Red = iR + DitherValue;
					Green = iG + DitherValue;
					Blue = iB + DitherValue;
					
					//Red = Clamp5 ( ( iR + DitherValue ) >> 27 );
					//Green = Clamp5 ( ( iG + DitherValue ) >> 27 );
					//Blue = Clamp5 ( ( iB + DitherValue ) >> 27 );
					
					// perform shift
					Red >>= ( 16 + 3 );
					Green >>= ( 16 + 3 );
					Blue >>= ( 16 + 3 );
					
					//Red = clamp ( Red, 0, 0x1f );
					//Green = clamp ( Green, 0, 0x1f );
					//Blue = clamp ( Blue, 0, 0x1f );
					Red = Clamp5 ( Red );
					Green = Clamp5 ( Green );
					Blue = Clamp5 ( Blue );
				}
				else
				{
					Red = iR >> ( 16 + 3 );
					Green = iG >> ( 16 + 3 );
					Blue = iB >> ( 16 + 3 );
				}
					
					
				bgr = ( Blue << 10 ) | ( Green << 5 ) | Red;
				}
				
				
				//ptr = & ( _GPU->VRAM [ Line + ( iy << 10 ) ] );
				ptr = &(_GPU->VRAM[(Line & FrameBuffer_XMask) + ((iy & FrameBuffer_YMask) << 10)]);

				if ( ABE || PIXELMASK )
				{
				// read pixel from frame buffer if we need to check mask bit
				DestPixel = *ptr;
				}
				
				
				bgr_temp = bgr;
				
	
				// semi-transparency
				//if ( Command_ABE )
				if ( ABE )
				{
					//bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read_ABR );
					bgr_temp = SemiTransparency16_t<ABR> ( DestPixel, bgr_temp );
				}
				
				// check if we should set mask bit when drawing
				//bgr_temp |= SetPixelMask;

				// draw pixel if we can draw to mask pixels or mask bit not set
				//if ( ! ( DestPixel & PixelMask ) ) *ptr = ( bgr_temp | SetPixelMask );
				if ( ! ( DestPixel & PIXELMASK ) ) *ptr = ( bgr_temp | SETPIXELMASK );
			}
			
			ix += dx;
			
			if ( SHADED )
			{
			iR += dr;
			iG += dg;
			iB += db;
			}
		}


	}
	
	return NumPixels;
}






static u64 Select_Line_Renderer_t ( DATA_Write_Format* p_inputdata, u32 ulThreadNum )
{
	//template<const uint32_t SHADED, const uint32_t DTD, const uint32_t PIXELMASK, const uint32_t SETPIXELMASK, const uint32_t ABE, const uint32_t ABR>
	//static u64 DrawLine_Generic_th(DATA_Write_Format* inputdata, u32 ulThreadNum)

	u32 GPU_CTRL_Read;
	u32 GPU_CTRL_Read_DTD;
	u32 PixelMask, SetPixelMask;
	u32 Command_ABE;
	u32 SHADED;
	u32 Combine;

	u32 ABR;

	u64 ullCycles;

	GPU_CTRL_Read = p_inputdata[0].Value;

	Command_ABE = ( p_inputdata[7].Command & 2 ) >> 1;
	ABR = (GPU_CTRL_Read >> 5) & 3;

	SHADED = ( p_inputdata [ 7 ].Command >> 4 ) & 1;
	
	GPU_CTRL_Read_DTD = ( GPU_CTRL_Read >> 9 ) & 1;
	
	PixelMask = ( GPU_CTRL_Read & 0x1000 ) >> 12;
	SetPixelMask = ( GPU_CTRL_Read & 0x0800 ) >> 11;

	Combine = ABR | (Command_ABE << 2) | (SetPixelMask << 3) | (PixelMask << 4) | (GPU_CTRL_Read_DTD << 5) | (SHADED << 6);

	ullCycles = ps1_arr_line_lut[Combine](p_inputdata, 0);

	return ullCycles;
	
	
	//if ( !SHADED ) GPU_CTRL_Read_DTD = 0;
	
	// todo: add additional optimizations (same colors in shaded or shaded texture, etc.)
	
	/*
	Combine = GPU_CTRL_Read_DTD | ( SHADED << 1 ) | ( ( PixelMask | SetPixelMask ) >> 9 );
	
	switch ( Combine )
	{
		case 0:
		case 1:
			return Select_Line_Renderer2_t<0,0,0,0> ( p_inputdata, ulThreadNum );
			break;
		case 2:
			return Select_Line_Renderer2_t<1,0,0,0> ( p_inputdata, ulThreadNum );
			break;
		case 3:
			return Select_Line_Renderer2_t<1,1,0,0> ( p_inputdata, ulThreadNum );
			break;
		case 4:
		case 5:
			return Select_Line_Renderer2_t<0,0,0x8000,0> ( p_inputdata, ulThreadNum );
			break;
		case 6:
			return Select_Line_Renderer2_t<1,0,0x8000,0> ( p_inputdata, ulThreadNum );
			break;
		case 7:
			return Select_Line_Renderer2_t<1,1,0x8000,0> ( p_inputdata, ulThreadNum );
			break;
		case 8:
		case 9:
			return Select_Line_Renderer2_t<0,0,0,0x8000> ( p_inputdata, ulThreadNum );
			break;
		case 10:
			return Select_Line_Renderer2_t<1,0,0,0x8000> ( p_inputdata, ulThreadNum );
			break;
		case 11:
			return Select_Line_Renderer2_t<1,1,0,0x8000> ( p_inputdata, ulThreadNum );
			break;
		case 12:
		case 13:
			return Select_Line_Renderer2_t<0,0,0x8000,0x8000> ( p_inputdata, ulThreadNum );
			break;
		case 14:
			return Select_Line_Renderer2_t<1,0,0x8000,0x8000> ( p_inputdata, ulThreadNum );
			break;
		case 15:
			return Select_Line_Renderer2_t<1,1,0x8000,0x8000> ( p_inputdata, ulThreadNum );
			break;
	}

	return 0;
	*/
}

	//static void FlushToHardware ( u64 ullReadIdx, u64 ullWriteIdx );
	static void FlushToHardware ( u64 ullReadIdx, u64 ullWriteIdx, u64 ullReadPixelIdx, u64 ullWritePixelIdx );

	static void Flush ();
	static void Finish ();

static void Select_KillGpuThreads_t ( u32* p_inputbuffer, u32 ulThreadNum )
{
	if ( _GPU->ulNumberOfThreads )
	{
		// set command
		//p_inputbuffer [ 15 ] = 7;
		p_inputbuffer [ 7 ] = 5 << 24;

		ulInputBuffer_WriteIndex++;
	}
}




	};


};

#endif	// end #ifndef __PS1_GPU_H__
