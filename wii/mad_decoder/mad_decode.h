#ifndef __MAD_DECODE_H__
#define __MAD_DECODE_H__

#include "mad.h"
#include "buffer.h"

#include <SDL/begin_code.h>

struct MAD_decoder;

DECLSPEC void          SDLCALL MAD_play(struct MAD_decoder *dec );
DECLSPEC void          SDLCALL MAD_stop(struct MAD_decoder *dec );
DECLSPEC bool          SDLCALL MAD_playing(struct MAD_decoder *dec );
DECLSPEC void          SDLCALL MAD_setvolume(struct MAD_decoder *dec, int volume );
DECLSPEC int           SDLCALL MAD_Decode(struct MAD_decoder *decoder, Uint8 *buffer, size_t bytes, int channels);
DECLSPEC void          SDLCALL MAD_Free(struct MAD_decoder *decoder);

/* Convenience function, create a MAD decoder from filename */
DECLSPEC struct MAD_decoder * SDLCALL MAD_CreateDecoder(const char *fname, size_t bufsize);
/* Create a MAD decoder from a SDL_RWops stream */
DECLSPEC struct MAD_decoder * SDLCALL MAD_CreateDecoder_RW(SDL_RWops *rwIn, size_t buffersize, int freerw);

#include <SDL/close_code.h>

#endif/*__MAD_DECODE_H__*/
