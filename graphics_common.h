/* -*- C++ -*-
 * 
 *  graphics_mmx.h - graphics macros common to graphics_* & AnimationInfo
 *
 *  Copyright (c) 2009 Mion. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifdef BPP16
#define BPP 16
#define RMASK 0xf800
#define GMASK 0x07e0
#define BMASK 0x001f
#define AMASK 0
#else
#define BPP 32
// the mask is the same as the one used in TTF_RenderGlyph_Blended
#define RMASK 0x00ff0000
#define GMASK 0x0000ff00
#define BMASK 0x000000ff
#define AMASK 0xff000000
#endif
#define RGBMASK 0x00ffffff

#ifdef BPP16
#define SET_PIXEL(rgb, alpha) {\
    *dst_buffer = (((rgb)&0xf80000) >> 8) | (((rgb)&0xfc00) >> 5) | (((rgb)&0xf8) >> 3);\
    *alphap++ = (alpha);\
}

#define BLEND_PIXEL(){\
    Uint32 mask2 = (*alphap++ * alpha) >> 11;\
    Uint32 s1 = (*src_buffer | *src_buffer << 16) & 0x07e0f81f;\
    Uint32 d1 = (*dst_buffer | *dst_buffer << 16) & 0x07e0f81f;\
    Uint32 mask1 = (d1 + ((s1-d1) * mask2 >> 5)) & 0x07e0f81f;\
    *dst_buffer = mask1 | mask1 >> 16;\
}

#else
#define SET_PIXEL(rgb, alpha) {\
    *dst_buffer = (rgb);\
    *alphap = (alpha);\
    alphap += 4;\
}

#define BLEND_PIXEL(){\
    Uint32 mask2 = (*alphap * alpha) >> 8;\
    Uint32 mask1 = mask2 ^ 0xff;\
    Uint32 mask_rb = (((*dst_buffer & 0xff00ff) * mask1 +\
                       (*src_buffer & 0xff00ff) * mask2) >> 8) & 0xff00ff;\
    Uint32 mask_g = (((*dst_buffer & 0x00ff00) * mask1 +\
                      (*src_buffer & 0x00ff00) * mask2) >> 8) & 0x00ff00;\
    *dst_buffer = mask_rb | mask_g;\
    alphap += 4;\
}

#define ADDBLEND_PIXEL(){\
    Uint32 mask2 = (*alphap * alpha) >> 8;\
    Uint32 mask_rb = (*dst_buffer & 0xff00ff) +\
                     ((((*src_buffer & 0xff00ff) * mask2) >> 8) & 0xff00ff);\
    mask_rb |= ((mask_rb & 0xff000000) ? 0xff0000 : 0) |\
               ((mask_rb & 0x0000ff00) ? 0x0000ff : 0);\
    Uint32 mask_g = (*dst_buffer & 0x00ff00) +\
                    ((((*src_buffer & 0x00ff00) * mask2) >> 8) & 0x00ff00);\
    mask_g |= ((mask_g & 0xff0000) ? 0xff00 : 0);\
    *dst_buffer = (mask_rb & 0xff00ff) | (mask_g & 0x00ff00);\
    alphap += 4;\
}

#define SUBBLEND_PIXEL(){\
    Uint32 mask2 = (*alphap * alpha) >> 8;\
    Uint32 mask_r = (*dst_buffer & 0xff0000) -\
                    ((((*src_buffer & 0xff0000) * mask2) >> 8) & 0xff0000);\
    mask_r &= ((mask_r & 0xff000000) ? 0 : 0xff0000);\
    Uint32 mask_g = (*dst_buffer & 0x00ff00) -\
                    ((((*src_buffer & 0x00ff00) * mask2) >> 8) & 0x00ff00);\
    mask_g &= ((mask_g & 0xffff0000) ? 0 : 0x00ff00);\
    Uint32 mask_b = (*dst_buffer & 0x0000ff) -\
                    ((((*src_buffer & 0x0000ff) * mask2) >> 8) & 0x0000ff);\
    mask_b &= ((mask_b & 0xffffff00) ? 0 : 0x0000ff);\
    *dst_buffer = (mask_r & 0xff0000) | (mask_g & 0x00ff00) | (mask_b & 0x0000ff);\
    alphap += 4;\
}

#endif


#define BASIC_BLEND(){\
    while(--n > 0) {  \
        BLEND_PIXEL();  \
        ++dst_buffer, ++src_buffer;  \
    } \
}

#define BASIC_ADDBLEND(){\
    while(--n > 0) {  \
        ADDBLEND_PIXEL();  \
        ++dst_buffer, ++src_buffer;  \
    } \
}

#define BASIC_SUBBLEND(){\
    while(--n > 0) {  \
        SUBBLEND_PIXEL();  \
        ++dst_buffer, ++src_buffer;  \
    } \
}


#define MEAN_PIXEL(){\
    int result = ((int)(*src1) + (int)(*src2)) / 2;  \
    (*dst) = result; \
}

#define BASIC_MEAN(){\
    while (--n > 0) {  \
        MEAN_PIXEL();  \
        ++dst; ++src1; ++src2;  \
    }  \
}

#define ADDTO_PIXEL(){\
    int result = (*dst) + (*src);  \
    (*dst) = (result < 255) ? result : 255; \
}

#define BASIC_ADDTO(){\
    while (--n > 0) {  \
        ADDTO_PIXEL();  \
        ++dst, ++src;  \
    }  \
}

#define SUBFROM_PIXEL(){\
    int result = (*dst) - (*src);  \
    (*dst) = (result > 0) ? result : 0;  \
}

#define BASIC_SUBFROM(){\
    while(--n > 0) {  \
        SUBFROM_PIXEL();  \
        ++dst, ++src;  \
    } \
}

