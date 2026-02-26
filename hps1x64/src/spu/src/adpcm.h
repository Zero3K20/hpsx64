
// I'll steal stuff from MAME/MESS

/*

    Sony PlayStation SPU (CXD2922BQ/CXD2925Q) emulator
    by pSXAuthor
    MAME adaptation by R. Belmont

*/

//#pragma once
#ifndef __ADPCM_H__
#define __ADPCM_H__

#include <stdint.h>


struct adpcm_packet
{
	unsigned char info,
								flags,
								data[14];
};


struct adpcm_packet_xa
{
	unsigned char data[14];
};


static const int32_t filter_coef[5][2]=
{
	{ 0,0 },
	{ 60,0 },
	{ 115,-52 },
	{ 98,-55 },
	{ 122,-60 },
};


class adpcm_decoder
{
	int32_t l0,l1;

public:
	adpcm_decoder()
	{
		reset();
	}

	adpcm_decoder(const adpcm_decoder &other)
	{
		operator =(other);
	}

	adpcm_decoder &operator =(const adpcm_decoder &other)
	{
		l0=other.l0;
		l1=other.l1;
		return *this;
	}

	void reset()
	{
		l0=l1=0;
	}
	
	static inline int32_t clamp ( int32_t sample ) { return ( ( sample > 0x7fff ) ? 0x7fff : ( ( sample < -0x8000 ) ? -0x8000 : sample ) ); }
	static inline int32_t clamp64 ( int64_t sample ) { return ( ( sample > 0x7fff ) ? 0x7fff : ( ( sample < -0x8000 ) ? -0x8000 : sample ) ); }

	static inline int32_t uclamp ( int32_t sample ) { return ( ( sample > 0x7fff ) ? 0x7fff : ( ( sample < 0 ) ? 0 : sample ) ); }
	static inline int32_t uclamp64 ( int64_t sample ) { return ( ( sample > 0x7fff ) ? 0x7fff : ( ( sample < 0 ) ? 0 : sample ) ); }

	static inline int32_t sign_extend4 ( int32_t value ) { return ( ( value << 28 ) >> 28 ); }
	
	signed short *decode_packet(adpcm_packet *ap, signed short *dp);
	
	// modification from mame - (cd-xa is same, but uses 28 instead of 14 samples)
	short *decode_packet_xa (unsigned char info, adpcm_packet_xa *ap, signed short *dp);
	//short *decode_samples ( int shift, int filter, short *dp, unsigned char *input, int numberofsamples );
	
	int32_t *decode_packet32(adpcm_packet *ap, int32_t *dp);
	
	// modification from mame - (cd-xa is same, but uses 28 instead of 14 samples)
	int32_t *decode_packet_xa32 (unsigned char info, adpcm_packet_xa *ap, int32_t *dp);
};

#endif	// end #ifndef __ADPCM_H__
