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
#ifndef __PS2_GPU_TEMPLATES_H__
#define __PS2_GPU_TEMPLATES_H__

#include "PS2_Gpu.h"


#ifdef USE_PS2_GPU_TEMPLATES


#ifdef _ENABLE_AVX512_GPU

template<const int IDX = 0>
static inline constexpr void init_ps2_array_line_lutx16()
{
	// current line drawing template
	//template<const long SHADED, const long DTHE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>
	//static u64 Render_Generic_Line_t(u64 * p_inputbuffer, u32 ulThreadNum)

	constexpr const int FBPSM = ((IDX >> 2) & 7);
	constexpr const int ZBPSM = ((IDX >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;

	constexpr const int SHADED = ((IDX >> 8) & 1);
	constexpr const int DTHE = ((SHADED) ? ((IDX >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));

#ifdef USE_TEMPLATES_PS2_LINE

	Playstation2::GPU::ps2_arr_line_lut[IDX] = &Playstation2::GPU::Render_Generic_Line_tx16<SHADED, DTHE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_line_lutx16<IDX + 1>();
}

template<>
static inline constexpr void init_ps2_array_line_lutx16<(1 << 9)>()
{
	return;
}

template<const int IDX = 0>
static inline constexpr void init_ps2_array_sprite_lutx16()
{
	// current sprite drawing template
	//template<const long TME, const long FGE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>

	constexpr const int FBPSM = ((IDX >> 2) & 7);
	constexpr const int ZBPSM = ((IDX >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;

	constexpr const int TME = ((IDX >> 8) & 1);
	constexpr const int FGE = ((TME) ? ((IDX >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));

#ifdef USE_TEMPLATES_PS2_RECTANGLE

	Playstation2::GPU::ps2_arr_sprite_lut[IDX] = &Playstation2::GPU::Render_Generic_Rectangle_tx16<TME, FGE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_sprite_lutx16<IDX + 1>();
}

template<>
static inline constexpr void init_ps2_array_sprite_lutx16<(1 << 9)>()
{
	return;
}


template<const int OFFSET, const int IDX = 0>
static inline constexpr void init_ps2_array_triangle_lutx16()
{
	// current triangle drawing template
	//template<const long SHADED, const long TME, const long FST, const long FGE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>
	//static u64 Render_Generic_Triangle_t(u64 * p_inputbuffer, u32 ulThreadNum)

	constexpr const int IDX2 = IDX + (OFFSET << 10);

	constexpr const int FBPSM = ((IDX2 >> 2) & 7);
	constexpr const int ZBPSM = ((IDX2 >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX2 >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;

	constexpr const int SHADED = ((IDX2 >> 10) & 1);
	constexpr const int TME = ((IDX2 >> 9) & 1);
	constexpr const int FST = ((TME) ? ((IDX2 >> 8) & 1) : 0);
	constexpr const int FGE = ((TME) ? ((IDX2 >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX2 >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));

#ifdef USE_TEMPLATES_PS2_TRIANGLE

	Playstation2::GPU::ps2_arr_triangle_lut[IDX2] = &Playstation2::GPU::Render_Generic_Triangle_tx16<SHADED, TME, FST, FGE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_triangle_lutx16<OFFSET, IDX + 1>();
}

template<>
static inline constexpr void init_ps2_array_triangle_lutx16<0, (1 << 10)>()
{
	return;
}
template<>
static inline constexpr void init_ps2_array_triangle_lutx16<1, (1 << 10)>()
{
	return;
}


#endif	// end #ifdef _ENABLE_AVX512_GPU


#ifdef _ENABLE_AVX2_GPU

template<const int IDX = 0>
static inline constexpr void init_ps2_array_line_lutx8()
{
	// current line drawing template
	//template<const long SHADED, const long DTHE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>
	//static u64 Render_Generic_Line_t(u64 * p_inputbuffer, u32 ulThreadNum)

	constexpr const int FBPSM = ((IDX >> 2) & 7);
	constexpr const int ZBPSM = ((IDX >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;

	constexpr const int SHADED = ((IDX >> 8) & 1);
	constexpr const int DTHE = ((SHADED) ? ((IDX >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));

#ifdef USE_TEMPLATES_PS2_LINE

	Playstation2::GPU::ps2_arr_line_lut[IDX] = &Playstation2::GPU::Render_Generic_Line_tx8<SHADED, DTHE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_line_lutx8<IDX + 1>();
}

template<>
static inline constexpr void init_ps2_array_line_lutx8<(1 << 9)>()
{
	return;
}

template<const int IDX = 0>
static inline constexpr void init_ps2_array_sprite_lutx8()
{
	// current sprite drawing template
	//template<const long TME, const long FGE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>

	constexpr const int FBPSM = ((IDX >> 2) & 7);
	constexpr const int ZBPSM = ((IDX >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;

	constexpr const int TME = ((IDX >> 8) & 1);
	constexpr const int FGE = ((TME) ? ((IDX >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));

#ifdef USE_TEMPLATES_PS2_RECTANGLE

	Playstation2::GPU::ps2_arr_sprite_lut[IDX] = &Playstation2::GPU::Render_Generic_Rectangle_tx8<TME, FGE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_sprite_lutx8<IDX + 1>();
}

template<>
static inline constexpr void init_ps2_array_sprite_lutx8<(1 << 9)>()
{
	return;
}


template<const int OFFSET, const int IDX = 0>
static inline constexpr void init_ps2_array_triangle_lutx8()
{
	// current triangle drawing template
	//template<const long SHADED, const long TME, const long FST, const long FGE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>
	//static u64 Render_Generic_Triangle_t(u64 * p_inputbuffer, u32 ulThreadNum)

	constexpr const int IDX2 = IDX + (OFFSET << 10);

	constexpr const int FBPSM = ((IDX2 >> 2) & 7);
	constexpr const int ZBPSM = ((IDX2 >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX2 >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;

	constexpr const int SHADED = ((IDX2 >> 10) & 1);
	constexpr const int TME = ((IDX2 >> 9) & 1);
	constexpr const int FST = ((TME) ? ((IDX2 >> 8) & 1) : 0);
	constexpr const int FGE = ((TME) ? ((IDX2 >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX2 >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));

#ifdef USE_TEMPLATES_PS2_TRIANGLE

	Playstation2::GPU::ps2_arr_triangle_lut[IDX2] = &Playstation2::GPU::Render_Generic_Triangle_tx8<SHADED, TME, FST, FGE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_triangle_lutx8<OFFSET, IDX + 1>();
}

template<>
static inline constexpr void init_ps2_array_triangle_lutx8<0, (1 << 10)>()
{
	return;
}
template<>
static inline constexpr void init_ps2_array_triangle_lutx8<1, (1 << 10)>()
{
	return;
}

#endif	// end #ifdef _ENABLE_AVX2_GPU


//template<const int IDX = 0>
template<const int OFFSET, const int IDX = 0>
static inline constexpr void init_ps2_array_line_lutx4()
{
	// current line drawing template
	//template<const long SHADED, const long DTHE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>
	//static u64 Render_Generic_Line_t(u64 * p_inputbuffer, u32 ulThreadNum)

	constexpr const int IDX2 = IDX + (OFFSET << 10);

	constexpr const int FBPSM = ((IDX2 >> 2) & 7);
	constexpr const int ZBPSM = ((IDX2 >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX2 >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;
	//constexpr const int ZFRAME2 = 1;

	constexpr const int SHADED = ((IDX2 >> 8) & 1);
	constexpr const int DTHE = ((SHADED) ? ((IDX2 >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX2 >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));

#ifdef USE_TEMPLATES_PS2_LINE

	// should be DTHE=1,FGE=1,ABE=1,ATST=1,DATE=1,ZTST=1,ZMSK=1,FBPSM=1,ZBPSM=1,TME=1,TPSM=1,CPSM=1,FILTER=1,FST=1,SHADED=1,AA1=1
	Playstation2::GPU::ps2_arr_line_lut[IDX2] = &Playstation2::GPU::Render_Generic_Line_t<SHADED, DTHE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_line_lutx4<OFFSET, IDX + 1>();
}

template<> static inline constexpr void init_ps2_array_line_lutx4<0,(1 << 9)>(){}


//template<const int IDX = 0>
template<const int OFFSET, const int IDX = 0>
static inline constexpr void init_ps2_array_sprite_lutx4()
{
	// current sprite drawing template
	//template<const long TME, const long FGE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>

	constexpr const int IDX2 = IDX + (OFFSET << 10);

	constexpr const int FBPSM = ((IDX2 >> 2) & 7);
	constexpr const int ZBPSM = ((IDX2 >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX2 >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;
	//constexpr const int ZFRAME2 = 1;

	constexpr const int TME = ((IDX2 >> 8) & 1);
	constexpr const int FGE = ((TME) ? ((IDX2 >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX2 >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));

#ifdef USE_TEMPLATES_PS2_RECTANGLE

	// should be DTHE=1,FGE=1,ABE=1,ATST=1,DATE=1,ZTST=1,ZMSK=1,FBPSM=1,ZBPSM=1,TME=1,TPSM=1,CPSM=1,TFX/TCC=1,FILTER=1
	Playstation2::GPU::ps2_arr_sprite_lut[IDX2] = &Playstation2::GPU::Render_Generic_Rectangle_t<TME, FGE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_sprite_lutx4<OFFSET, IDX + 1>();
}

template<> static inline constexpr void init_ps2_array_sprite_lutx4<0,(1 << 9)>(){}


template<const int OFFSET, const int IDX = 0>
static inline constexpr void init_ps2_array_triangle_lutx4()
{
	// current triangle drawing template
	//template<const long SHADED, const long TME, const long FST, const long FGE, const long ABE, const long ZMSK, const long FBPSM, const long ZBPSM>
	//static u64 Render_Generic_Triangle_t(u64 * p_inputbuffer, u32 ulThreadNum)

	constexpr const int IDX2 = IDX + (OFFSET << 10);

	constexpr const int FBPSM = ((IDX2 >> 2) & 7);
	constexpr const int ZBPSM = ((IDX2 >> 0) & 3);

	// make sure the frame buffer and z-buffer have the correct combination
	constexpr const int ZFRAME = ((((FBPSM & 0x3) == 2) && ((ZBPSM & 0x3) == 2)) || (((FBPSM & 0x3) != 2) && ((ZBPSM & 0x3) != 2))) ? 1 : 0;

	constexpr const int ZMSK = ((IDX2 >> 5) & 1);
	//constexpr const int ZFRAME2 = ZFRAME | ZMSK;
	//constexpr const int ZFRAME2 = ZFRAME & (ZMSK - 1);

	constexpr const int SHADED = ((IDX2 >> 10) & 1);
	constexpr const int TME = ((IDX2 >> 9) & 1);
	constexpr const int FST = ((TME) ? ((IDX2 >> 8) & 1) : 0);
	constexpr const int FGE = ((TME) ? ((IDX2 >> 7) & 1) : 0);
	constexpr const int ABE = ((IDX2 >> 6) & 1);

	constexpr const int FBPSM2 = ((((FBPSM & 3) == 3) ? 0xa : (FBPSM & 3)) | ((FBPSM > 3) ? 0x30 : 0));
	constexpr const int ZBPSM2 = (((ZBPSM & 3) == 3) ? 0xa : (ZBPSM & 3));


#ifdef USE_TEMPLATES_PS2_TRIANGLE

	// 11 - bits
	// shaded - 1 bit
	// tme - 1 bit
	// fst - 1 bit
	// fge - 1 bit
	// abe - 1 bit
	// zmsk - 1 bit
	// fbpsm - 3 bits
	// zbpsm - 2 bits

	// fbpsm - 1 bit
	// zbpsm - 1 bit

	// ztst - 1 bit
	// texpsm - 3 bits - just use lut+shift+mask w/avx2+ - could use 2 bits (32/16/32clut/16clut)
	// texfunc/tcc - 3 bits - could use 1 bit for on/off
	// aa1 - 1 bit
	// mipmap - 1 bit
	// filter - 1 bit
	Playstation2::GPU::ps2_arr_triangle_lut[IDX2] = &Playstation2::GPU::Render_Generic_Triangle_t<SHADED, TME, FST, FGE, ABE, ZMSK, FBPSM2, ZBPSM2>;

#endif

	init_ps2_array_triangle_lutx4<OFFSET, IDX + 1>();
}

template<> static inline constexpr void init_ps2_array_triangle_lutx4<0, (1 << 10)>(){}
template<> static inline constexpr void init_ps2_array_triangle_lutx4<1, (1 << 10)>(){}




static void setup_x4_renderer()
{
	// set to 4 for x4 and 8 for x8 renderer
	Playstation2::GPU::iDrawVectorSize = 4;

	// init lookups for software x4 renderer
	// shading disabled, aa1 disabled
	init_ps2_array_line_lutx4<0>();

	// shading disabled, aa1 disabled
	init_ps2_array_sprite_lutx4<0>();

	init_ps2_array_triangle_lutx4<0>();
	init_ps2_array_triangle_lutx4<1>();

}

#ifdef _ENABLE_AVX2

static void setup_x8_renderer()
{
	// set to 4 for x4 and 8 for x8 renderer
	Playstation2::GPU::iDrawVectorSize = 8;

	// init lookups for software x4 renderer
	init_ps2_array_line_lutx8<>();

	init_ps2_array_sprite_lutx8<>();

	init_ps2_array_triangle_lutx8<0>();
	init_ps2_array_triangle_lutx8<1>();
}

#endif

#ifdef _ENABLE_AVX512

static void setup_x16_renderer()
{
	Playstation2::GPU::iDrawVectorSize = 16;

	// init lookups for software x16 renderer
	init_ps2_array_line_lutx16<>();

	init_ps2_array_sprite_lutx16<>();

	init_ps2_array_triangle_lutx16<0>();
	init_ps2_array_triangle_lutx16<1>();
}

#endif

#endif	// end #ifdef USE_PS2_GPU_TEMPLATES

#endif	// end __PS2_GPU_TEMPLATES_H__
