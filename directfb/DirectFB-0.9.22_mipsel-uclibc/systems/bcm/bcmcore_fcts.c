
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

#include <directfb.h>
#include <directfb_strings.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <fusion/shmalloc.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/layers.h>
#include <core/layers_internal.h>
#include <core/palette.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>
#include <core/system.h>
#include <core/windows_internal.h>

#include <gfx/convert.h>
#include <gfx/util.h>
#include <gfx/clip.h>

#include "bcmlayer.h"

void dfb_surface_flip_buffers_nonotify( CoreSurface *surface )
{
    SurfaceBuffer * front;

    if (surface->caps & DSCAPS_TRIPLE) {
        front = surface->front_buffer;
        surface->front_buffer = surface->back_buffer;
        surface->back_buffer = surface->idle_buffer;
        surface->idle_buffer = front;
    } else {
        front = surface->front_buffer;
        surface->front_buffer = surface->back_buffer;
        surface->back_buffer = front;
        /* To avoid problems with buffer deallocation */
        surface->idle_buffer = surface->front_buffer;
    }
}

void dfb_surface_flip_index_nonotify( DFBSurfaceCapabilities caps, uint32_t * front_index, uint32_t * back_index, uint32_t * idle_index )
{
    uint32_t front;

    if (caps & DSCAPS_TRIPLE) {
        front = *front_index;
        *front_index = *back_index;
        *back_index = *idle_index;
        *idle_index = front;
    } else {
        front = *front_index;
        *front_index = *back_index;
        *back_index = front;
        /* To avoid problems with buffer deallocation */
        *idle_index = *front_index;
    }
}

DFBResult
dfb_surface_reformat_preallocated_video(
    CoreDFB *core,
    CoreSurface *surface,
    int width, int height,
    DFBSurfacePixelFormat format )
{
     int old_width, old_height;
     DFBSurfacePixelFormat old_format;

     D_ASSERT( width > 0 );
     D_ASSERT( height > 0 );

     if (width * (long long) height > 4096*4096)
          return DFB_LIMITEXCEEDED;

     if (surface->front_buffer->flags & SBF_FOREIGN_SYSTEM ||
         surface->back_buffer->flags  & SBF_FOREIGN_SYSTEM)
     {
          return DFB_UNSUPPORTED;
     }

     old_width  = surface->width;
     old_height = surface->height;
     old_format = surface->format;

     surface->width  = width;
     surface->height = height;
     surface->format = format;

     if (width      <= surface->min_width &&
         old_width  <= surface->min_width &&
         height     <= surface->min_height &&
         old_height <= surface->min_height &&
         old_format == surface->format)
     {
          dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT );
          return DFB_OK;
     }

     if (surface->caps & DSCAPS_DEPTH) {
        /* Not supported */
        return DFB_UNSUPPORTED;
     }

     if (DFB_PIXELFORMAT_IS_INDEXED( format ) && !surface->palette) {
          DFBResult    ret;
          CorePalette *palette;

          ret = dfb_palette_create( core, 1 << DFB_COLOR_BITS_PER_PIXEL( format ), &palette );
          if (ret) {
               D_DERROR( ret, "Core/Surface: Could not create a palette with %d entries!\n",
                         1 << DFB_COLOR_BITS_PER_PIXEL( format ) );
          }
          else {
               switch (format) {
                    case DSPF_LUT8:
                         dfb_palette_generate_rgb332_map( palette );
                         break;

                    case DSPF_ALUT44:
                         dfb_palette_generate_rgb121_map( palette );
                         break;

                    default:
                         D_WARN( "unknown indexed format" );
               }

               dfb_surface_set_palette( surface, palette );

               dfb_palette_unref( palette );
          }
     }

     dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT | CSNF_SYSTEM | CSNF_VIDEO );

     return DFB_OK;
}

DFBResult
dfb_surface_create_preallocated_video(
    CoreDFB *core,
    int width, int height,
    DFBSurfacePixelFormat format,
    DFBSurfaceCapabilities caps, CorePalette *palette,
    SurfaceBuffer * front_buffer, SurfaceBuffer * back_buffer, SurfaceBuffer * idle_buffer,
    CoreSurface **ret_surface )
{
     DFBResult    ret;
     CoreSurface *surface;

     D_ASSERT( width > 0 );
     D_ASSERT( height > 0 );

     D_DEBUG( "dfb_surface_create_preallocated_video( core %p, size %dx%d, format %d palette %p )\n",
              core, width, height, format, palette );

     if (caps & DSCAPS_DEPTH)
          return DFB_UNSUPPORTED;

     if (width * (long long) height > 4096*4096)
          return DFB_LIMITEXCEEDED;

     surface = dfb_core_create_surface( core );

     ret = dfb_surface_init( core, surface, NULL, width, height, format, caps, palette );
     if (ret) {
          fusion_object_destroy( &surface->object );
          return ret;
     }

     surface->caps |= DSCAPS_VIDEOONLY;

     /* Allocate front buffer. */
     surface->front_buffer = front_buffer;
     if (!surface->front_buffer) {
          fusion_object_destroy( &surface->object );
          return ret;
     }

     /* Allocate back buffer. */
     if (caps & DSCAPS_FLIPPING) {
          surface->back_buffer = back_buffer;
          if (!surface->back_buffer) {
               dfb_surface_destroy_buffer( surface, surface->front_buffer );

               fusion_object_destroy( &surface->object );
               return ret;
          }
     }
     else
          surface->back_buffer = surface->front_buffer;

     /* Allocate extra back buffer. */
     if (caps & DSCAPS_TRIPLE) {
          surface->idle_buffer = idle_buffer;
          if (!surface->idle_buffer) {
               dfb_surface_destroy_buffer( surface, surface->back_buffer );
               dfb_surface_destroy_buffer( surface, surface->front_buffer );

               fusion_object_destroy( &surface->object );
               return ret;
          }
     }
     else
          surface->idle_buffer = surface->front_buffer;

     fusion_object_activate( &surface->object );

     *ret_surface = surface;

     return DFB_OK;
}

DFBResult
dfb_surface_reconfig_preallocated_video(
    CoreSurface       *surface,
    SurfaceBuffer     *front_buffer,
    SurfaceBuffer     *back_buffer,
    SurfaceBuffer     *idle_buffer )
{
     DFBResult      ret = DFB_FAILURE;
     SurfaceBuffer *front;
     SurfaceBuffer *back;
     SurfaceBuffer *idle;
     SurfaceBuffer *new_front = NULL;
     SurfaceBuffer *new_back  = NULL;
     SurfaceBuffer *new_idle  = NULL;

     D_ASSERT( surface != NULL );
     D_ASSERT( front_buffer != NULL );

     dfb_surfacemanager_lock( surface->manager );

     if (surface->caps & DSCAPS_DEPTH) {
        /* Not supported */
        ret = DFB_UNSUPPORTED;
        goto error;
     }

     if (surface->front_buffer || surface->back_buffer || surface->idle_buffer) {
         while ( surface->front_buffer != front_buffer ) {
             dfb_surface_flip_buffers_nonotify( surface );
         }
     }
     front = surface->front_buffer;
     back  = surface->back_buffer;
     idle  = surface->idle_buffer;

     if (front_buffer->policy != CSP_VIDEOONLY) {
        /* Not supported for preallocated_video */
        ret = DFB_UNSUPPORTED;
        goto error;
     }

     if ((front_buffer->flags | back_buffer->flags) & SBF_FOREIGN_SYSTEM) {
        ret = DFB_UNSUPPORTED;
        goto error;
     }

     if (front != front_buffer) {
         new_front = front_buffer;
         if (!new_front) {
             ret = DFB_UNSUPPORTED;
             goto error;
         }
         D_ASSERT(front == NULL);
         front = new_front;
     }

     if (surface->caps & DSCAPS_FLIPPING) {
          D_ASSERT(back_buffer != front_buffer);
          if (back != back_buffer) {
              new_back = back_buffer;
              if (!new_back) {
                   ret = DFB_FAILURE;
                   goto error;
              }
          }
     }
     else {
        if (back != front)
            new_back = front;
     }

     if (surface->caps & DSCAPS_TRIPLE) {
          D_ASSERT(idle_buffer != front_buffer);
          if (idle != idle_buffer) {
              new_idle = idle_buffer;
              if (!new_idle) {
                   ret = DFB_FAILURE;
                   goto error;
              }
          }
     }
     else {
        if (idle != front)
            new_idle = front;
     }

     if (new_front) {
         surface->front_buffer = new_front;
     }

     if (new_back) {
         D_ASSERT(back == NULL);
         surface->back_buffer = new_back;
     }

     if (new_idle) {
         D_ASSERT(idle == NULL);
         surface->idle_buffer = new_idle;
     }

     dfb_surfacemanager_unlock( surface->manager );

     dfb_surface_notify_listeners( surface, CSNF_SIZEFORMAT | CSNF_SYSTEM | CSNF_VIDEO );

     return DFB_OK;

error:

     if (new_idle)
          dfb_surface_destroy_buffer( surface, new_idle );

     if (new_back)
          dfb_surface_destroy_buffer( surface, new_back );

     if (new_front)
          dfb_surface_destroy_buffer( surface, new_front );

     dfb_surfacemanager_unlock( surface->manager );

     return ret;
}

/* Greatest common denominator, Euclid's algorithm, n > 0 & m > 0 */
static inline uint32_t source_destination_gcd(uint32_t n, uint32_t m)
{
    uint32_t r;

    r = n % m;
    while ( r ) {
        n = m;
        m = r;
        r = n % m;
    }

    return m;
}

void source2destination_init(BCMCoreLayerHandle layer)
{

    if (layer->back_rect.w) {
        layer->horizontal_back2front_scalefactor = (double)layer->front_rect.w/(double)layer->back_rect.w;
        layer->horizontal_alignement = layer->back_rect.w/source_destination_gcd(layer->front_rect.w,layer->back_rect.w);
        /* For scaling only even x and width should be passed */
        if (layer->horizontal_alignement % 2) {
            layer->horizontal_alignement *= 2;
        }
    }
    else {
        layer->horizontal_back2front_scalefactor = 0;
        layer->horizontal_alignement = 0;
    }

    if (layer->back_rect.h) {
        layer->vertical_back2front_scalefactor = (double)layer->front_rect.h/(double)layer->back_rect.h;
        layer->vertical_alignement = layer->back_rect.h/source_destination_gcd(layer->front_rect.h,layer->back_rect.h);
    }
    else {
        layer->vertical_back2front_scalefactor = 0;
        layer->vertical_alignement = 0;
    }
}

void source2destination_transform(BCMCoreLayerHandle layer, DFBRectangle * source_rect, DFBRectangle * dest_rect)
{
    DFBRegion clip_region = DFB_REGION_INIT_FROM_RECTANGLE_VALS(0, 0, layer->frontsurface_clone.width, layer->frontsurface_clone.height);

    /* Since we fixed alignement, is this still really needed? Scaling needed even x and width before */
    if (source_rect->x % 2) {
        source_rect->x--;
        source_rect->w++;
    }
    if (source_rect->w % 2) {
        source_rect->w++;
    }

    /* The following calculations are to avoid roundoff errors when
     * scaling source to destination, or doing aspect ration conversion with a region.
     */
    if (layer->horizontal_alignement) {
        source_rect->w += source_rect->x % layer->horizontal_alignement;
        source_rect->x /= layer->horizontal_alignement;
        source_rect->x *= layer->horizontal_alignement;
        if (source_rect->w % layer->horizontal_alignement) {
            source_rect->w /= layer->horizontal_alignement;
            source_rect->w++;
        }
        else {
            source_rect->w /= layer->horizontal_alignement;
        }
        source_rect->w *= layer->horizontal_alignement;
    }
    if (layer->vertical_alignement) {
        source_rect->h += source_rect->y % layer->vertical_alignement;
        source_rect->y /= layer->vertical_alignement;
        source_rect->y *= layer->vertical_alignement;
        if (source_rect->h % layer->vertical_alignement) {
            source_rect->h /= layer->vertical_alignement;
            source_rect->h++;
        }
        else {
            source_rect->h /= layer->vertical_alignement;
        }
        source_rect->h *= layer->vertical_alignement;
    }

    dest_rect->x = layer->front_rect.x + (source_rect->x * layer->horizontal_back2front_scalefactor);
    dest_rect->y = layer->front_rect.y + (source_rect->y * layer->vertical_back2front_scalefactor);
    dest_rect->w = source_rect->w * layer->horizontal_back2front_scalefactor;
    dest_rect->h = source_rect->h * layer->vertical_back2front_scalefactor;

    dfb_clip_stretchblit(&clip_region, source_rect, dest_rect);

    clip_region = DFB_REGION_INIT_FROM_RECTANGLE_VALS(0, 0, layer->backsurface_clone.width, layer->backsurface_clone.height);

    dfb_clip_rectangle(&clip_region, source_rect);
}

void destination2source_transform(BCMCoreLayerHandle layer, DFBRectangle * source_rect, DFBRectangle * dest_rect)
{
    DFBRegion clip_region;

    /* Since we fixed alignement, is this still really needed? Scaling needed even x and width before */
    if (source_rect->x % 2) {
        source_rect->x--;
        source_rect->w++;
    }
    if (source_rect->w % 2) {
        source_rect->w++;
    }

    {
        double horizontal_front2back_scalefactor = (1/layer->horizontal_back2front_scalefactor);
        double vertical_front2back_scalefactor = (1/layer->vertical_back2front_scalefactor);

        dest_rect->x = layer->back_rect.x + (source_rect->x * horizontal_front2back_scalefactor);
        dest_rect->y = layer->back_rect.y + (source_rect->y * vertical_front2back_scalefactor);
        dest_rect->w = source_rect->w * horizontal_front2back_scalefactor;
        dest_rect->h = source_rect->h * vertical_front2back_scalefactor;

        clip_region = DFB_REGION_INIT_FROM_RECTANGLE_VALS(0, 0, layer->backsurface_clone.width, layer->backsurface_clone.height);
        dfb_clip_rectangle(&clip_region, dest_rect);
    }
}

/* convert rectangle coordinates and size from source to display resolution and aspect ratio
   for clear rectangle
 */
void src2display_rect_transform(BCMCoreLayerHandle layer, DFBRectangle * src_rect, DFBRectangle * display_rect)
{
    display_rect->x = layer->front_rect.x + (src_rect->x * layer->horizontal_back2front_scalefactor + 0.5f);
    display_rect->y = layer->front_rect.y + (src_rect->y * layer->vertical_back2front_scalefactor + 0.5f);
    display_rect->w = src_rect->w * layer->horizontal_back2front_scalefactor + 0.5f;
    display_rect->h = src_rect->h * layer->vertical_back2front_scalefactor + 0.5f;
}

bdvd_graphics_pixel_format_e
dfb_surface_pixelformat_to_bdvd( DFBSurfacePixelFormat format )
{
    bdvd_graphics_pixel_format_e returned_format = bdvd_graphics_pixel_format_a8_r8_g8_b8;

    switch (format) {
        case DSPF_YUY2:
            returned_format = bdvd_graphics_pixel_format_cr8_y18_cb8_y08;
        break;
        case DSPF_UYVY:
            returned_format = bdvd_graphics_pixel_format_y18_cr8_y08_cb8;
        break;
        case DSPF_RGB32:
            returned_format = bdvd_graphics_pixel_format_x8_r8_g8_b8;
        break;
        case DSPF_ARGB:
            returned_format = bdvd_graphics_pixel_format_a8_r8_g8_b8;
        break;
        case DSPF_RGB16:
            returned_format = bdvd_graphics_pixel_format_r5_g6_b5;
        break;
        case DSPF_ARGB1555:
            returned_format = bdvd_graphics_pixel_format_a1_r5_g5_b5;
        break;
        case DSPF_ARGB4444:
            returned_format = bdvd_graphics_pixel_format_a4_r4_g4_b4;
        break;
        case DSPF_LUT8:
            returned_format = bdvd_graphics_pixel_format_palette8;
        break;
        default:
            D_ERROR( "dfb_surface_pixelformat_to_bdvd: format is not supported\n" );
    }

    return returned_format;
}

