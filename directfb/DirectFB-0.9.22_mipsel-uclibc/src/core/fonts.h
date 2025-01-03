/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __FONTS_H__
#define __FONTS_H__

#include <pthread.h>

#include <fusion/lock.h>

#include <directfb.h>

#include <core/coretypes.h>

#include <core/state.h>

/*
 * glyph struct
 */
typedef struct {
     CoreSurface *surface;              /* contains bitmap of glyph              */
     int          start;                /* x offset of glyph in surface          */
     int          width;                /* width of the glyphs bitmap            */
     int          height;               /* height of the glyphs bitmap           */
     int          left;                 /* x offset of the glyph                 */
     int          top;                  /* y offset of the glyph                 */
     int          advance_x;            /* placement of next glyph on the x axis */
     int          advance_y;            /* placement of next glyph on the y axis */
} CoreGlyphData;

/*
 * font struct
 */

struct _CoreFont {
     int                      magic;

     DFBSurfacePixelFormat    pixel_format;
     DFBSurfaceCapabilities   surface_caps;
     DFBFontLayout            layout;   /* the layout of the font on the screen */

     CardState                state;    /* the state used to blit glyphs    */

     CoreDFB         *core;

     CoreSurface    **surfaces;         /* contain bitmaps of loaded glyphs */
     int              rows;
     int              next_x;

     DirectTree      *glyph_infos;      /* infos about loaded glyphs        */

     int              height;           /* font height                      */
     int              width;            /* font width                       */
     int              line_height;      /* line height from one baseline to 
                                           the next.                        */


     int              ascender;         /* a positive value, the distance
                                           from the baseline to the top     */
     int              descender;        /* a negative value, the distance
                                           from the baseline to the bottom  */
     int              maxadvance;       /* width of largest character       */

     pthread_mutex_t  lock;             /* lock during access to the font   */

     void            *impl_data;        /* a pointer used by the impl.      */

     DFBResult   (* RenderGlyph)  ( CoreFont *thiz, unichar glyph,
                                    CoreGlyphData *info, CoreSurface *surface );
     DFBResult   (* GetKerning)   ( CoreFont *thiz,
                                    unichar prev, unichar current,
                                    int *kern_x, int *kern_y );
};

/*
 * allocates and initializes a new font structure
 */
CoreFont *dfb_font_create( CoreDFB *core );

/*
 * destroy all data in the CoreFont struct
 */
void dfb_font_destroy( CoreFont *font );

void dfb_font_drop_destination( CoreFont *font, CoreSurface *surface );

/*
 * lock the font before accessing it
 */
static inline void
dfb_font_lock( CoreFont *font )
{
     D_MAGIC_ASSERT( font, CoreFont );

     pthread_mutex_lock( &font->lock );

     dfb_state_lock( &font->state );
}

/*
 * unlock the font after access
 */
static inline void
dfb_font_unlock( CoreFont *font )
{
     D_MAGIC_ASSERT( font, CoreFont );

     dfb_state_unlock( &font->state );

     pthread_mutex_unlock( &font->lock );
}

/*
 * loads glyph data from font
 */
DFBResult dfb_font_get_glyph_data( CoreFont        *font,
                                   unichar          glyph,
                                   CoreGlyphData  **glyph_data );

#endif
