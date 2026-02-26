//#pragma once
#ifndef __PS1_GPU_TEMPLATES_H__
#define __PS1_GPU_TEMPLATES_H__



#include "PS1_Gpu.h"


//template<const long SHADED, const long DTD, const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABR>
//static u64 DrawLine_Generic_th(DATA_Write_Format* inputdata, u32 ulThreadNum)

template<const int IDX = 0>
static inline constexpr void init_ps1_array_line_lut()
{
	const int ABE = (IDX >> 2) & 1;
	const int ABR = (ABE) ? ((IDX >> 0) & 3) : 0;
	const int PIXELMASK = ((IDX >> 3) & 1) << 15;
	const int SETPIXELMASK = ((IDX >> 4) & 1) << 15;
	const int DTD = (IDX >> 5) & 1;
	const int SHADED = (IDX >> 6) & 1;
	//ps1_arr_sprite_lut[IDX] = &DrawLine_Generic_th<SHADED,DTD,SETPIXELMASK,PIXELMASK,ABE,ABR>;
	Playstation1::GPU::ps1_arr_line_lut[IDX] = &Playstation1::GPU::DrawLine_Generic_th<SHADED, DTD, PIXELMASK, SETPIXELMASK, ABE, ABR>;

	init_ps1_array_line_lut<IDX + 1>();
}

template<>
inline constexpr void init_ps1_array_line_lut<(1 << 7)>()
{
	return;
}


template<const int IDX = 0>
static inline constexpr void init_ps1_array_sprite_lut()
{
	constexpr const int TEXTURE = (IDX >> 8) & 1;
	constexpr const int TP = (TEXTURE) ? ((IDX >> 0) & 3) : 0;
	constexpr const int TGE = (TEXTURE) ? ((IDX >> 7) & 1) : 1;
	constexpr const int ABE = (IDX >> 4) & 1;
	constexpr const int ABR = (ABE) ? ((IDX >> 2) & 3) : 0;
	constexpr const int PIXELMASK = ((IDX >> 5) & 1) << 15;
	constexpr const int SETPIXELMASK = ((IDX >> 6) & 1) << 15;
	Playstation1::GPU::ps1_arr_sprite_lut[IDX] = &Playstation1::GPU::DrawSprite_Generic_th<TEXTURE, TGE, SETPIXELMASK, PIXELMASK, ABE, ABR, TP>;

	init_ps1_array_sprite_lut<IDX + 1>();
}

template<>
inline constexpr void init_ps1_array_sprite_lut<(1 << 9)>()
{
	return;
}


//template<const long SHADED, const long TEXTURE, const long DTD, const long TGE, const long SETPIXELMASK, const long PIXELMASK, const long ABE, const long ABR, const long TP>
//static u64 DrawTriangle_Generic_th(DATA_Write_Format* inputdata, u32 ulThreadNum)

template<const int OFFSET, const int IDX = 0>
static inline constexpr void init_ps1_array_triangle_lut()
{
	constexpr const int IDX2 = IDX + (OFFSET << 10);
	constexpr const int TEXTURE = (IDX2 >> 9) & 1;
	constexpr const int TGE = (TEXTURE) ? ((IDX2 >> 7) & 1) : 1;
	constexpr const int TP = (TEXTURE) ? ((IDX2 >> 0) & 3) : 0;
	constexpr const int ABE = (IDX2 >> 4) & 1;
	constexpr const int ABR = (ABE) ? ((IDX2 >> 2) & 3) : 0;
	constexpr const int PIXELMASK = ((IDX2 >> 5) & 1) << 15;
	constexpr const int SETPIXELMASK = ((IDX2 >> 6) & 1) << 15;
	constexpr const int SHADED = (IDX2 >> 10) & 1;
	constexpr const int DTD = (SHADED) ? ((IDX2 >> 8) & 1) : 0;
	Playstation1::GPU::ps1_arr_triangle_lut[IDX2] = &Playstation1::GPU::DrawTriangle_Generic_th<SHADED, TEXTURE, DTD, TGE, SETPIXELMASK, PIXELMASK, ABE, ABR, TP>;

	init_ps1_array_triangle_lut<OFFSET, IDX + 1>();
}

template<>
inline constexpr void init_ps1_array_triangle_lut<0, (1 << 10)>()
{
	return;
}
template<>
inline constexpr void init_ps1_array_triangle_lut<1, (1 << 10)>()
{
	return;
}

#endif	// end #ifndef __PS1_GPU_TEMPLATES_H__
