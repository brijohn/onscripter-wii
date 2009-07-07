/***************************************************************************
                             mad_decode.c
      A non-braindead implementation of an MP3 decoder using libmad,
      with SDL_RWops support.
                             -------------------
    begin                : Fri Jun 20 2003
    copyright            : (C) 2003 by Tyler Montbriand
    email                : tsm@accesscomm.ca
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "mad_decode.h"
#include "mad_internal.h"

/* Strings to correspond to MAD_phase_t values. */
const char *PhaseNames[]={"MAD_PHASE_STREAM","MAD_PHASE_FRAME",
                          "MAD_PHASE_CONVERT","MAD_PHASE_DONE",NULL};


/* Sets dec->Phase, and prints a warning message if
   setting dec->Phase to MAD_PHASE_DONE.            */
static void SetPhase(MAD_decoder *dec, MAD_phase_t phase);   

void MAD_play( MAD_decoder *dec )
{
    dec->Playing = true;
}

void MAD_stop( MAD_decoder *dec )
{
    dec->Playing = false;
}

bool MAD_playing( MAD_decoder *dec )
{
    return dec->Playing;
}

void MAD_setvolume( MAD_decoder *dec, int volume )
{
    if ( (volume >= 0) && (volume <= 100) ) {
        dec->Volume = (volume * SDL_MIX_MAXVOLUME) / 100;
    }
}

int MAD_Decode(MAD_decoder *dec, Uint8 *buffer, size_t size,int channels)
{
  int bytes=0;  /* Number of bytes of audio output decoded */
  Uint8 *output;
  /* Sanity checking */        
  if((dec==NULL)||(buffer==NULL)||(size<=0)) return(-1);
 
  if ( !dec->Playing ) return -1;
  
  output = (Uint8 *)malloc(size);

  while(bytes<size) /* Loop until 'size' bytes decoded, or error */  
  switch(dec->Phase)
  {
  case MAD_PHASE_STREAM:  /* Reading raw data from file */
    SetPhase(dec,MAD_PushStream(dec));
    break;
    
  case MAD_PHASE_FRAME:   /* Reading frames from raw data */
    SetPhase(dec,MAD_GetFrame(dec));
    break;
    
  case MAD_PHASE_CONVERT: /* Converting frames to 16-bit PCM data */
    SetPhase(dec,MAD_Convert(dec,(Sint16 *)output,&bytes,size,channels));
    break;
    
  case MAD_PHASE_DONE:  /* Out of data, or fatal error */
    return(-1);
    
  default:  /* Should't happen */
    fprintf(stderr,"Unknown phase %d\n",dec->Phase);
    fprintf(stderr,"Email me at tsm@accesscomm.ca and tell me how you did this\n");
    abort();
    break;
  }
  
  if (bytes < size)
    memset(output + bytes, 0, size - bytes);

  SDL_MixAudio( buffer, output, size, dec->Volume );
  free(output);

  return bytes;
}

void MAD_Free(MAD_decoder *decoder)
{
 if(decoder==NULL) return;

 /* Deinitialize MAD */
 mad_synth_finish(&(decoder->Synth));
 mad_frame_finish(&(decoder->Frame));
 mad_stream_finish(&(decoder->Stream));
 
 if(decoder->buf!=NULL) Buffer_Free(decoder->buf);

 if((decoder->FreeRW)&&(decoder->Source!=NULL))
   SDL_FreeRW(decoder->Source);

 free(decoder);  
}

MAD_decoder *MAD_CreateDecoder(const char *fname, size_t bufsize)
{
 SDL_RWops *source=SDL_RWFromFile(fname,"rb");

 if(source==NULL)
 {
   return(NULL);
 }
 else
 {
   return(MAD_CreateDecoder_RW(source,bufsize,1));
 }
}

MAD_decoder *MAD_CreateDecoder_RW(SDL_RWops *rwIn, size_t buffersize, int freerw)
{
  MAD_decoder *decoder=NULL;

  if(rwIn==NULL) return(NULL);

  decoder=(MAD_decoder *)malloc(sizeof(MAD_decoder));
  if(decoder==NULL) return(NULL);

  memset(decoder,0,sizeof(MAD_decoder));

  decoder->buf=Buffer_Create(buffersize);
  if(decoder->buf==NULL)
  {
    free(decoder);
    return(NULL);
  }
  decoder->Phase=MAD_PHASE_STREAM;
  decoder->Source=rwIn;
  decoder->FreeRW=freerw;
  decoder->FrameCount=0;
  decoder->Playing = 0;
  decoder->Volume = 75;

  /* Initialize MAD */
  mad_stream_init(&(decoder->Stream));
  mad_frame_init(&(decoder->Frame));
  mad_synth_init(&(decoder->Synth));
  mad_timer_reset((&decoder->Timer));

  return(decoder);
}

static void SetPhase(MAD_decoder *dec, MAD_phase_t phase)
{
  if(phase==MAD_PHASE_DONE)
  {
    fprintf(stderr,"Going from %s to %s\n",
      PhaseNames[dec->Phase],PhaseNames[phase]);
  }

  dec->Phase=phase;
}
