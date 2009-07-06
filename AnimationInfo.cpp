/* -*- C++ -*-
 * 
 *  AnimationInfo.cpp - General image storage class of ONScripter
 *
 *  Copyright (c) 2001-2008 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
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

#include "AnimationInfo.h"
#include "BaseReader.h"

#include "graphics_common.h"

#if defined(USE_X86_GFX)
#include "graphics_mmx.h"
#include "graphics_sse2.h"
#endif

#if defined(USE_PPC_GFX)
#include "graphics_altivec.h"
#endif

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//Mion: for special graphics routine handling
static unsigned int cpufuncs;


AnimationInfo::AnimationInfo()
{
    is_copy = false;
    
    image_name = NULL;
    image_surface = NULL;
    alpha_buf = NULL;

    duration_list = NULL;
    color_list = NULL;
    file_name = NULL;
    mask_file_name = NULL;

    trans_mode = TRANS_TOPLEFT;
    affine_flag = false;

    reset();
}

AnimationInfo::AnimationInfo(const AnimationInfo &anim)
{
    memcpy(this, &anim, sizeof(AnimationInfo));
    is_copy = true;
}

AnimationInfo::~AnimationInfo()
{
    if (!is_copy) reset();
}

AnimationInfo& AnimationInfo::operator =(const AnimationInfo &anim)
{
    if (this != &anim){
        memcpy(this, &anim, sizeof(AnimationInfo));
        is_copy = true;
    }

    return *this;
}

void AnimationInfo::reset()
{
    remove();

    trans = 256;
    pos.x = pos.y = 0;
    visible = false;
    abs_flag = true;
    scale_x = scale_y = rot = 0;
    blending_mode = BLEND_NORMAL;
    
    font_size_xy[0] = font_size_xy[1] = -1;
    font_pitch = -1;

    mat[0][0] = 1000;
    mat[0][1] = 0;
    mat[1][0] = 0;
    mat[1][1] = 1000;

    layer_no = -1;
}

void AnimationInfo::deleteImageName(){
    if ( image_name ) delete[] image_name;
    image_name = NULL;
}

void AnimationInfo::setImageName( const char *name ){
    deleteImageName();
    image_name = new char[ strlen(name) + 1 ];
    strcpy( image_name, name );
}

void AnimationInfo::deleteSurface(){
    if ( image_surface ) SDL_FreeSurface( image_surface );
    image_surface = NULL;
    if (alpha_buf) delete[] alpha_buf;
    alpha_buf = NULL;
}

void AnimationInfo::remove(){
    deleteImageName();
    deleteSurface();
    removeTag();
}

void AnimationInfo::removeTag(){
    if ( duration_list ){
        delete[] duration_list;
        duration_list = NULL;
    }
    if ( color_list ){
        delete[] color_list;
        color_list = NULL;
    }
    if ( file_name ){
        delete[] file_name;
        file_name = NULL;
    }
    if ( mask_file_name ){
        delete[] mask_file_name;
        mask_file_name = NULL;
    }
    current_cell = 0;
    num_of_cells = 0;
    remaining_time = 0;
    is_animatable = false;
    is_single_line = true;
    is_tight_region = true;
    is_ruby_drawable = false;
    skip_whitespace = true;

    direction = 1;

    color[0] = color[1] = color[2] = 0;
}

// 0 ... restart at the end
// 1 ... stop at the end
// 2 ... reverse at the end
// 3 ... no animation
bool AnimationInfo::proceedAnimation()
{
    bool is_changed = false;
    
    if ( loop_mode != 3 && num_of_cells > 1 ){
        current_cell += direction;
        is_changed = true;
    }

    if ( current_cell < 0 ){ // loop_mode must be 2
        current_cell = 1;
        direction = 1;
    }
    else if ( current_cell >= num_of_cells ){
        if ( loop_mode == 0 ){
            current_cell = 0;
        }
        else if ( loop_mode == 1 ){
            current_cell = num_of_cells - 1;
            is_changed = false;
        }
        else{
            current_cell = num_of_cells - 2;
            direction = -1;
        }
    }

    remaining_time = duration_list[ current_cell ];

    return is_changed;
}

void AnimationInfo::setCell(int cell)
{
    if (cell < 0) cell = 0;
    else if (cell >= num_of_cells) cell = num_of_cells - 1;

    current_cell = cell;
}

int AnimationInfo::doClipping( SDL_Rect *dst, SDL_Rect *clip, SDL_Rect *clipped )
{
    if ( clipped ) clipped->x = clipped->y = 0;

    if ( !dst ||
         dst->x >= clip->x + clip->w || dst->x + dst->w <= clip->x ||
         dst->y >= clip->y + clip->h || dst->y + dst->h <= clip->y )
        return -1;

    if ( dst->x < clip->x ){
        dst->w -= clip->x - dst->x;
        if ( clipped ) clipped->x = clip->x - dst->x;
        dst->x = clip->x;
    }
    if ( clip->x + clip->w < dst->x + dst->w ){
        dst->w = clip->x + clip->w - dst->x;
    }
    
    if ( dst->y < clip->y ){
        dst->h -= clip->y - dst->y;
        if ( clipped ) clipped->y = clip->y - dst->y;
        dst->y = clip->y;
    }
    if ( clip->y + clip->h < dst->y + dst->h ){
        dst->h = clip->y + clip->h - dst->y;
    }
    if ( clipped ){
        clipped->w = dst->w;
        clipped->h = dst->h;
    }

    return 0;
}


void AnimationInfo::blendOnSurface( SDL_Surface *dst_surface, int dst_x, int dst_y,
                                    SDL_Rect &clip, int alpha )
{
    if ( image_surface == NULL ) return;
    
    SDL_Rect dst_rect = {dst_x, dst_y, pos.w, pos.h}, src_rect;
    if ( doClipping( &dst_rect, &clip, &src_rect ) ) return;

    /* ---------------------------------------- */
    
    SDL_LockSurface( dst_surface );
    SDL_LockSurface( image_surface );
    
#ifdef BPP16
    int total_width = image_surface->pitch / 2;
#else
    int total_width = image_surface->pitch / 4;
#endif    
    ONSBuf *src_buffer = (ONSBuf *)image_surface->pixels + total_width * src_rect.y + image_surface->w*current_cell/num_of_cells + src_rect.x;
    ONSBuf *dst_buffer = (ONSBuf *)dst_surface->pixels   + dst_surface->w * dst_rect.y + dst_rect.x;
#ifdef BPP16
    unsigned char *alphap = alpha_buf + image_surface->w * src_rect.y + image_surface->w*current_cell/num_of_cells + src_rect.x;
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    unsigned char *alphap = (unsigned char *)src_buffer + 3;
#else
    unsigned char *alphap = (unsigned char *)src_buffer;
#endif
#endif

#ifndef BPP16
    if (blending_mode == BLEND_NORMAL) {
#endif
        if ((trans_mode == TRANS_COPY) && (alpha == 256)) {
            Uint32* srcmax = (Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h;

            for (int i=dst_rect.h ; i ; --i){
                for (int j=dst_rect.w ; j ; --j, src_buffer++, dst_buffer++){
                    // If we've run out of source area, ignore the remainder.
                    if (src_buffer >= srcmax) goto break2;
                    SET_PIXEL(*src_buffer, 0xff);
                }
                src_buffer += total_width - dst_rect.w;
#ifdef BPP16
                alphap += image_surface->w - dst_rect.w;
#else
                alphap += (image_surface->w - dst_rect.w)*4;
#endif
                dst_buffer += dst_surface->w  - dst_rect.w;
            }
        } else {
            Uint32* srcmax = (Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h;

            for (int i=dst_rect.h ; i ; --i){
#ifdef BPP16
                for (int j=dst_rect.w ; j ; --j, src_buffer++, dst_buffer++){
                    // If we've run out of source area, ignore the remainder.
                    if (src_buffer >= srcmax) goto break2;
                    BLEND_PIXEL();
                }
                src_buffer += total_width - dst_rect.w;
                dst_buffer += dst_surface->w  - dst_rect.w;
                alphap += image_surface->w - dst_rect.w;
#else
                if (src_buffer >= srcmax) goto break2;
                int length = dst_rect.w;
                imageFilterBlend(dst_buffer, src_buffer, alphap, alpha, length);
                src_buffer += total_width;
                dst_buffer += dst_surface->w;
                alphap += (image_surface->w)*4;
#endif
            }
        }
#ifndef BPP16
    } else if (blending_mode == BLEND_ADD) {
        if ((trans_mode == TRANS_COPY) && (alpha == 256)) {
            // "add" the src pix value to the dst
            Uint8* srcmax = (Uint8*) ((Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h);
            Uint8* src_buf = (Uint8*) src_buffer;
            Uint8* dst_buf = (Uint8*) dst_buffer;

            for (int i=dst_rect.h ; i ; --i){
                if (src_buf >= srcmax) goto break2;
                imageFilterAddTo(dst_buf, src_buf, dst_rect.w*4);
                src_buf += total_width * 4;
                dst_buf += dst_surface->w * 4;
            }
        } else {
            // gotta do additive alpha blending
            Uint32* srcmax = (Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h;

            for (int i=dst_rect.h ; i ; --i){
                for (int j=dst_rect.w ; j ; --j, src_buffer++, dst_buffer++){
                    // If we've run out of source area, ignore the remainder.
                    if (src_buffer >= srcmax) goto break2;
                        ADDBLEND_PIXEL();
                    }
                src_buffer += total_width - dst_rect.w;
                alphap += (image_surface->w - dst_rect.w)*4;
                dst_buffer += dst_surface->w  - dst_rect.w;
            }
        }
    } else if (blending_mode == BLEND_SUB) {
        if ((trans_mode == TRANS_COPY) && (alpha == 256)) {
            // "subtract" the src pix value from the dst
            Uint8* srcmax = (Uint8*) ((Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h);
            Uint8* src_buf = (Uint8*) src_buffer;
            Uint8* dst_buf = (Uint8*) dst_buffer;

            for (int i=dst_rect.h ; i ; --i){
                if (src_buf >= srcmax) goto break2;
                imageFilterSubFrom(dst_buf, src_buf, dst_rect.w*4);
                src_buf += total_width * 4;
                dst_buf += dst_surface->w * 4;
            }
        } else {
            // gotta do subtractive alpha blending
            Uint32* srcmax = (Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h;

            for (int i=dst_rect.h ; i ; --i){
                for (int j=dst_rect.w ; j ; --j, src_buffer++, dst_buffer++){
                    // If we've run out of source area, ignore the remainder.
                    if (src_buffer >= srcmax) goto break2;
                        SUBBLEND_PIXEL();
                    }
                src_buffer += total_width - dst_rect.w;
                alphap += (image_surface->w - dst_rect.w)*4;
                dst_buffer += dst_surface->w  - dst_rect.w;
            }
        }
    }
#endif
break2:
    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( dst_surface );
}

//Mion - ogapee2008
void AnimationInfo::blendOnSurface2( SDL_Surface *dst_surface, int dst_x, int dst_y,
                                     SDL_Rect &clip, int alpha )
{
    if ( image_surface == NULL ) return;
    if (scale_x == 0 || scale_y == 0) return;
    
    int i, x, y;

    // project corner point and calculate bounding box
    int min_xy[2]={bounding_rect.x, bounding_rect.y};
    int max_xy[2]={bounding_rect.x+bounding_rect.w-1, 
                   bounding_rect.y+bounding_rect.h-1};

    // clip bounding box
    if (max_xy[0] < clip.x) return;
    if (max_xy[0] >= clip.x+clip.w) max_xy[0] = clip.x+clip.w - 1;
    if (min_xy[0] >= clip.x+clip.w) return;
    if (min_xy[0] < clip.x) min_xy[0] = clip.x;
    if (max_xy[1] < clip.y) return;
    if (max_xy[1] >= clip.y+clip.h) max_xy[1] = clip.y+clip.h - 1;
    if (min_xy[1] >= clip.y+clip.h) return;
    if (min_xy[1] < clip.y) min_xy[1] = clip.y;

    SDL_LockSurface( dst_surface );
    SDL_LockSurface( image_surface );
    
#ifdef BPP16
    int total_width = image_surface->pitch / 2;
#else
    int total_width = image_surface->pitch / 4;
#endif    
    // set pixel by inverse-projection with raster scan
    for (y=min_xy[1] ; y<= max_xy[1] ; y++){
        // calculate the start and end point for each raster scan
        int raster_min = min_xy[0], raster_max = max_xy[0];
        for (i=0 ; i<4 ; i++){
            if (corner_xy[i][1] == corner_xy[(i+1)%4][1]) continue;
            x = (corner_xy[(i+1)%4][0] - corner_xy[i][0])*(y-corner_xy[i][1])/
                (corner_xy[(i+1)%4][1] - corner_xy[i][1]) + corner_xy[i][0];
            if (corner_xy[(i+1)%4][1] - corner_xy[i][1] > 0){
                if (raster_min < x) raster_min = x;
            }
            else{
                if (raster_max > x) raster_max = x;
            }
        }

        ONSBuf *dst_buffer = (ONSBuf *)dst_surface->pixels + dst_surface->w * y + raster_min;

        // inverse-projection
        int x_offset = inv_mat[0][1] * (y-dst_y) / 1000 + pos.w/2;
        int y_offset = inv_mat[1][1] * (y-dst_y) / 1000 + pos.h/2;
        for (x=raster_min-dst_x ; x<=raster_max-dst_x ; x++, dst_buffer++){
            int x2 = inv_mat[0][0] * x / 1000 + x_offset;
            int y2 = inv_mat[1][0] * x / 1000 + y_offset;

            if (x2 < 0 || x2 >= pos.w ||
                y2 < 0 || y2 >= pos.h) continue;

            ONSBuf *src_buffer = (ONSBuf *)image_surface->pixels + total_width * y2 + x2 + pos.w*current_cell;
#ifdef BPP16
            unsigned char *alphap = alpha_buf + image_surface->w * y2 + x2 + pos.w*current_cell;
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            unsigned char *alphap = (unsigned char *)src_buffer + 3;
#else
            unsigned char *alphap = (unsigned char *)src_buffer;
#endif
#endif
#ifndef BPP16
            if (blending_mode == BLEND_NORMAL) {
#endif
                if ((trans_mode == TRANS_COPY) && (alpha == 256)) {
                    SET_PIXEL(*src_buffer, 0xff);
                } else {
                    BLEND_PIXEL();
                }
#ifndef BPP16
            } else if (blending_mode == BLEND_ADD) {
                ADDBLEND_PIXEL();
            } else if (blending_mode == BLEND_SUB) {
                SUBBLEND_PIXEL();
            }
#endif
        }
    }
    
    // unlock surface
    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( dst_surface );
}

// used to draw characters on text_surface
// Alpha = 1 - (1-Da)(1-Sa)
// Color = (DaSaSc + Da(1-Sa)Dc + Sa(1-Da)Sc)/A
void AnimationInfo::blendBySurface( SDL_Surface *surface, int dst_x, int dst_y, SDL_Color &color,
                                    SDL_Rect *clip, bool rotate_flag )
{
    if (image_surface == NULL || surface == NULL) return;
    
    SDL_Rect dst_rect = {dst_x, dst_y, surface->w, surface->h};
    if (rotate_flag){
        dst_rect.w = surface->h;
        dst_rect.h = surface->w;
    }
    SDL_Rect src_rect = {0, 0, 0, 0};
    SDL_Rect clipped_rect;

    /* ---------------------------------------- */
    /* 1st clipping */
    if ( clip ){
        if ( doClipping( &dst_rect, clip, &clipped_rect ) ) return;

        src_rect.x += clipped_rect.x;
        src_rect.y += clipped_rect.y;
    }
    
    /* ---------------------------------------- */
    /* 2nd clipping */
    SDL_Rect clip_rect = {0, 0, image_surface->w, image_surface->h};
    if ( doClipping( &dst_rect, &clip_rect, &clipped_rect ) ) return;
    
    src_rect.x += clipped_rect.x;
    src_rect.y += clipped_rect.y;

    /* ---------------------------------------- */
    
    SDL_LockSurface( surface );
    SDL_LockSurface( image_surface );
    
#ifdef BPP16
    int total_width = image_surface->pitch / 2;
    Uint32 src_color = ((color.r & 0xf8) << 8 |
                        (color.g & 0xfc) << 3 |
                        (color.b & 0xf8) >> 3);
    src_color = (src_color | src_color << 16) & 0x07e0f81f;
#else
    int total_width = image_surface->pitch / 4;
    Uint32 src_color1 = color.r << 16 | color.b;
    Uint32 src_color2 = color.g << 8;
#endif    
    ONSBuf *dst_buffer = (ONSBuf *)image_surface->pixels + total_width * dst_rect.y + image_surface->w*current_cell/num_of_cells + dst_rect.x;
    unsigned char *src_buffer = NULL;
    if (rotate_flag)
        src_buffer = (unsigned char*)surface->pixels + surface->pitch*(surface->h - src_rect.x - 1) + src_rect.y;
    else
        src_buffer = (unsigned char*)surface->pixels + surface->pitch*src_rect.y + src_rect.x;
#ifdef BPP16
    unsigned char *alphap = alpha_buf + image_surface->w * dst_rect.y + image_surface->w*current_cell/num_of_cells + dst_rect.x;
#endif

    Uint32 mask, mask1, mask2, mask_rb;
    
    for (int i=0 ; i<dst_rect.h ; i++){
        for (int j=0 ; j<dst_rect.w ; j++, dst_buffer++){

            mask2 = *src_buffer;

#ifdef BPP16
            Uint32 an_1 = *alphap;
            *alphap = 0xff ^ ((0xff ^ an_1)*(0xff ^ mask2) >> 8);
            mask2 = (mask2 << 5) / *alphap;
            
            Uint32 d1 = (*dst_buffer | *dst_buffer << 16) & 0x07e0f81f;

            mask_rb = (d1 + ((src_color - d1) * mask2 >> 5)) & 0x07e0f81f;
            *dst_buffer = mask_rb | mask_rb >> 16;
            alphap++;
#else
            Uint32 an_1 = *dst_buffer >> 24;
            Uint32 an = 0xff ^ ((0xff ^ an_1)*(0xff ^ mask2) >> 8);
            mask1 = ((0xff ^ mask2)*an_1)>>8;

            mask_rb =  ((*dst_buffer & 0xff00ff) * mask1 + 
                        src_color1 * mask2);
            mask_rb =  (((mask_rb / an) & 0x00ff0000) | 
                        (((mask_rb & 0xffff) / an) & 0xff));
            mask = (((*dst_buffer & 0x00ff00) * mask1 +
                     src_color2 * mask2) / an) & 0x00ff00;
            *dst_buffer = mask_rb | mask | (an << 24);
#endif            
            if (rotate_flag)
                src_buffer -= surface->pitch;
            else
                src_buffer++;
        }
        dst_buffer += total_width - dst_rect.w;
#ifdef BPP16
        alphap += image_surface->w - dst_rect.w;
#endif        
        if (rotate_flag)
            src_buffer = (unsigned char*)surface->pixels + surface->pitch*(surface->h - src_rect.x - 1) + src_rect.y + i + 1;
        else
            src_buffer += surface->pitch  - dst_rect.w;
    }

    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( surface );
}

//Mion - ogapee2008
void AnimationInfo::calcAffineMatrix()
{
    // calculate forward matrix
    // |mat[0][0] mat[0][1]|
    // |mat[1][0] mat[1][1]|
    int cos_i = 1000, sin_i = 0;
    if (rot != 0){
        cos_i = (int)(1000.0 * cos(-M_PI*rot/180));
        sin_i = (int)(1000.0 * sin(-M_PI*rot/180));
    }
    mat[0][0] =  cos_i*scale_x/100;
    mat[0][1] = -sin_i*scale_y/100;
    mat[1][0] =  sin_i*scale_x/100;
    mat[1][1] =  cos_i*scale_y/100;

    // calculate bounding box

    // FIXME (haeleth, 20080323): added initialisers, as otherwise
    // these arrays appear to be being read from before they're
    // assigned any values!  I suspect 0 is not the optimal value to
    // use, as it's probably causing the bounding box to always
    // stretch from (0, 0).  Need to see what this code is actually
    // doing and provide more sane initialisation.
    int min_xy[2] = { 0, 0 }, max_xy[2] = { 0, 0 };
    for (int i=0 ; i<4 ; i++){
        int c_x = (i<2)?(-pos.w/2):(pos.w/2);
        int c_y = ((i+1)&2)?(pos.h/2):(-pos.h/2);
        //Mion: need to make sure corners are in right order (UL,LL,LR,UR)
        if (scale_x < 0) c_x = -c_x;
        if (scale_y < 0) c_y = -c_y;
        corner_xy[i][0] = (mat[0][0] * c_x + mat[0][1] * c_y) / 1000 + pos.x;
        corner_xy[i][1] = (mat[1][0] * c_x + mat[1][1] * c_y) / 1000 + pos.y;

        if (i==0 || min_xy[0] > corner_xy[i][0]) min_xy[0] = corner_xy[i][0];
        if (i==0 || max_xy[0] < corner_xy[i][0]) max_xy[0] = corner_xy[i][0];
        if (i==0 || min_xy[1] > corner_xy[i][1]) min_xy[1] = corner_xy[i][1];
        if (i==0 || max_xy[1] < corner_xy[i][1]) max_xy[1] = corner_xy[i][1];
    }

    bounding_rect.x = min_xy[0];
    bounding_rect.y = min_xy[1];
    bounding_rect.w = max_xy[0]-min_xy[0]+1;
    bounding_rect.h = max_xy[1]-min_xy[1]+1;
    
    // calculate inverse matrix
    int denom = scale_x*scale_y;
    if (denom == 0) return;

    inv_mat[0][0] =  mat[1][1] * 10000 / denom;
    inv_mat[0][1] = -mat[0][1] * 10000 / denom;
    inv_mat[1][0] = -mat[1][0] * 10000 / denom;
    inv_mat[1][1] =  mat[0][0] * 10000 / denom;
}

SDL_Surface *AnimationInfo::allocSurface( int w, int h )
{
    return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, BPP, RMASK, GMASK, BMASK, AMASK);
}

void AnimationInfo::allocImage( int w, int h )
{
    if (!image_surface ||
        image_surface->w != w ||
        image_surface->h != h){
        deleteSurface();

        image_surface = allocSurface( w, h );
#ifdef BPP16
        alpha_buf = new unsigned char[w*h];
#endif        
    }

    abs_flag = true;
    pos.w = w / num_of_cells;
    pos.h = h;
}

void AnimationInfo::copySurface( SDL_Surface *surface, SDL_Rect *src_rect, SDL_Rect *dst_rect )
{
    if (!image_surface || !surface) return;
    
    SDL_Rect _dst_rect = {0, 0};
    if (dst_rect) _dst_rect = *dst_rect;

    SDL_Rect _src_rect = {0, 0, surface->w, surface->h};
    if (src_rect) _src_rect = *src_rect;

    if (_src_rect.x >= surface->w) return;
    if (_src_rect.y >= surface->h) return;
    
    if (_src_rect.x+_src_rect.w >= surface->w)
        _src_rect.w = surface->w - _src_rect.x;
    if (_src_rect.y+_src_rect.h >= surface->h)
        _src_rect.h = surface->h - _src_rect.y;
        
    if (_dst_rect.x+_src_rect.w > image_surface->w)
        _src_rect.w = image_surface->w - _dst_rect.x;
    if (_dst_rect.y+_src_rect.h > image_surface->h)
        _src_rect.h = image_surface->h - _dst_rect.y;
        
    SDL_LockSurface( surface );
    SDL_LockSurface( image_surface );

    int i;
    for (i=0 ; i<_src_rect.h ; i++)
        memcpy( (ONSBuf*)((unsigned char*)image_surface->pixels + image_surface->pitch * (_dst_rect.y+i)) + _dst_rect.x,
                (ONSBuf*)((unsigned char*)surface->pixels + surface->pitch * (_src_rect.y+i)) + _src_rect.x,
                _src_rect.w*sizeof(ONSBuf) );
#if defined(BPP16)
    for (i=0 ; i<_src_rect.h ; i++)
        memset( alpha_buf + image_surface->w * (_dst_rect.y+i) + _dst_rect.x, 0xff, _src_rect.w );
#endif

    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( surface );
}

void AnimationInfo::fill( Uint8 r, Uint8 g, Uint8 b, Uint8 a )
{
    if (!image_surface) return;
    
    SDL_LockSurface( image_surface );
    ONSBuf *dst_buffer = (ONSBuf *)image_surface->pixels;

    Uint32 rgb = (r << 16)|(g << 8)|b;
    unsigned char *alphap = NULL;
#ifdef BPP16
    alphap = alpha_buf;
    int dst_margin = image_surface->w % 2;
#else    
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    alphap = (unsigned char *)dst_buffer + 3;
#else
    alphap = (unsigned char *)dst_buffer;
#endif
    int dst_margin = 0;
#endif

    for (int i=0 ; i<image_surface->h ; i++){
        for (int j=0 ; j<image_surface->w ; j++, dst_buffer++)
            SET_PIXEL(rgb, a);
        dst_buffer += dst_margin;
    }
    SDL_UnlockSurface( image_surface );
}

void AnimationInfo::setupImage( SDL_Surface *surface, SDL_Surface *surface_m, bool has_alpha )
{
    if (surface == NULL) return;
    SDL_LockSurface( surface );
    Uint32 *buffer = (Uint32 *)surface->pixels;

    int w = surface->w;
    int h = surface->h;
    int w2 = w / num_of_cells;
    if (trans_mode == TRANS_ALPHA && !has_alpha)
        w = (w2/2) * num_of_cells;

    allocImage(w, h);
    ONSBuf *dst_buffer = (ONSBuf *)image_surface->pixels;

    unsigned char *alphap = NULL;
#ifdef BPP16
    alphap = alpha_buf;
    int dst_margin = w % 2;
#else    
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    alphap = (unsigned char *)dst_buffer + 3;
#else
    alphap = (unsigned char *)dst_buffer;
#endif
    int dst_margin = 0;
#endif

    SDL_PixelFormat *fmt = surface->format;
    Uint32 ref_color=0;
    if ( trans_mode == TRANS_TOPLEFT ){
        ref_color = *buffer;
    }
    else if ( trans_mode == TRANS_TOPRIGHT ){
        ref_color = *(buffer + surface->w - 1);
    }
    else if ( trans_mode == TRANS_DIRECT ) {
        ref_color = direct_color[0] << fmt->Rshift |
            direct_color[1] << fmt->Gshift |
            direct_color[2] << fmt->Bshift;
    }
    ref_color &= RGBMASK;

    int i, j, c;
    if ( trans_mode == TRANS_ALPHA ){
        if (has_alpha) {
            for (i=0 ; i<h ; i++){
                for (j=0 ; j<w ; j++, buffer++, dst_buffer++)
                    SET_PIXEL(*buffer, *buffer >> 24);
                dst_buffer += dst_margin;
            }
        }
        else {	
	    for (i=0 ; i<h ; i++){
		for (c=0 ; c<num_of_cells ; c++){
		    for (j=0 ; j<w2/2 ; j++, buffer++, dst_buffer++){
			SET_PIXEL(*buffer, (*(buffer+(w2/2)) & 0xff) ^ 0xff);
		    }
		    buffer += (w2 - w2/2);
		}
		buffer += surface->w - w2*num_of_cells;
		dst_buffer += dst_margin;
	    }
	}
    }
    else if ( trans_mode == TRANS_MASK ){
        if (surface_m){
            SDL_LockSurface( surface_m );
            int mw = surface->w;
            int mh = surface->h;

            for (i=0 ; i<h ; i++){
                Uint32 *buffer_m = (Uint32 *)surface_m->pixels + mw*(i%mh);
                for (c=0 ; c<num_of_cells ; c++){
                    for (j=0 ; j<w2 ; j++, buffer++, dst_buffer++){
                        if (surface_m){
                            SET_PIXEL(*buffer, (*(buffer_m + j%mw) & 0xff) ^ 0xff);
                        }
                        else{
                            SET_PIXEL(*buffer, 0xff);
                        }
                    }
                }
                dst_buffer += dst_margin;
            }
            SDL_UnlockSurface( surface_m );
        }
    }
    else if ( trans_mode == TRANS_TOPLEFT ||
              trans_mode == TRANS_TOPRIGHT ||
              trans_mode == TRANS_DIRECT ){
        for (i=0 ; i<h ; i++){
            for (j=0 ; j<w ; j++, buffer++, dst_buffer++){
                if ( (*buffer & RGBMASK) == ref_color ){
                    SET_PIXEL(*buffer, 0x00);
                }
                else{
                    SET_PIXEL(*buffer, 0xff);
                }
            }
            dst_buffer += dst_margin;
        }
    }
    else if ( trans_mode == TRANS_STRING ){
        for (i=0 ; i<h ; i++){
            for (j=0 ; j<w ; j++, buffer++, dst_buffer++)
                SET_PIXEL(*buffer, *buffer >> 24);
            dst_buffer += dst_margin;
        }
    }
    else { // TRANS_COPY
        for (i=0 ; i<h ; i++){
            for (j=0 ; j<w ; j++, buffer++, dst_buffer++)
                SET_PIXEL(*buffer, 0xff);
            dst_buffer += dst_margin;
        }
    }
    
    SDL_UnlockSurface( surface );
}


void AnimationInfo::setCpufuncs(unsigned int func)
{
    cpufuncs = func;
}

unsigned int AnimationInfo::getCpufuncs()
{
    return cpufuncs;
}


void AnimationInfo::imageFilterMean(unsigned char *src1, unsigned char *src2, unsigned char *dst, int length)
{
#if defined(USE_PPC_GFX)
    imageFilterMean_Altivec(src1, src2, dst, length);
#elif defined(USE_X86_GFX)

#ifndef MACOSX
    if (cpufuncs & CPUF_X86_SSE2) {
#endif // !MACOSX

        imageFilterMean_SSE2(src1, src2, dst, length);

#ifndef MACOSX
    } else if (cpufuncs & CPUF_X86_MMX) {

        imageFilterMean_MMX(src1, src2, dst, length);

    }
#endif // !MACOSX

#else // no special gfx handling
    int n = length + 1;
    BASIC_MEAN();
#endif
}

void AnimationInfo::imageFilterAddTo(unsigned char *dst, unsigned char *src, int length)
{
#if defined(USE_PPC_GFX)
    imageFilterAddTo_Altivec(dst, src, length);
#elif defined(USE_X86_GFX)

#ifndef MACOSX
    if (cpufuncs & CPUF_X86_SSE2) {
#endif // !MACOSX

        imageFilterAddTo_SSE2(dst, src, length);

#ifndef MACOSX
    } else if (cpufuncs & CPUF_X86_MMX) {

        imageFilterAddTo_MMX(dst, src, length);

    }
#endif // !MACOSX

#else // no special gfx handling
    int n = length + 1;
    BASIC_ADDTO();
#endif
}

void AnimationInfo::imageFilterSubFrom(unsigned char *dst, unsigned char *src, int length)
{
#if defined(USE_PPC_GFX)
    imageFilterSubFrom_Altivec(dst, src, length);
#elif defined(USE_X86_GFX)

#ifndef MACOSX
    if (cpufuncs & CPUF_X86_SSE2) {
#endif // !MACOSX

        imageFilterSubFrom_SSE2(dst, src, length);

#ifndef MACOSX
    } else if (cpufuncs & CPUF_X86_MMX) {

        imageFilterSubFrom_MMX(dst, src, length);

    }
#endif // !MACOSX

#else // no special gfx handling
    int n = length + 1;
    BASIC_SUBFROM();
#endif
}


void AnimationInfo::imageFilterBlend(Uint32 *dst_buffer, Uint32 *src_buffer, Uint8 *alphap, int alpha, int length)
{
#if defined(USE_X86_GFX)
#ifndef MACOSX
    if (cpufuncs & CPUF_X86_SSE2) {
#endif // !MACOSX

        imageFilterBlend_SSE2(dst_buffer, src_buffer, alphap, alpha, length);

#ifndef MACOSX
    }
#endif // !MACOSX

#else // no special gfx handling
    int n = length + 1;
    BASIC_BLEND();
#endif
}

