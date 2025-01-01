
#ifndef __BCMCORE_FCTS_H__
#define __BCMCORE_FCTS_H__

#include <directfb.h>

#include <direct/list.h>
#include <direct/serial.h>

#include <fusion/object.h>
#include <fusion/lock.h>
#include <fusion/reactor.h>

#include <core/coretypes.h>
#include <core/coredefs.h>

#include <core/gfxcard.h>

void dfb_surface_flip_buffers_nonotify( CoreSurface *surface );

void dfb_surface_flip_index_nonotify( DFBSurfaceCapabilities caps, uint32_t * front_index, uint32_t * back_index, uint32_t * idle_index );

DFBResult dfb_surface_create_preallocated_video( CoreDFB *core,
                                                 int width, int height,
                                                 DFBSurfacePixelFormat format,
                                                 DFBSurfaceCapabilities caps, CorePalette *palette,
                                                 SurfaceBuffer * front_buffer, SurfaceBuffer * back_buffer, SurfaceBuffer * idle_buffer,
                                                 CoreSurface **ret_surface );

DFBResult dfb_surface_reformat_preallocated_video( CoreDFB               *core,
                                                   CoreSurface           *surface,
                                                   int                    width,
                                                   int                    height,
                                                   DFBSurfacePixelFormat  format );

DFBResult dfb_surface_reconfig_preallocated_video( CoreSurface           *surface,
                                                   SurfaceBuffer         *front_buffer,
                                                   SurfaceBuffer         *back_buffer,
                                                   SurfaceBuffer         *idle_buffer );

void source2destination_init(BCMCoreLayerHandle layer);

void source2destination_transform(BCMCoreLayerHandle layer, DFBRectangle * source_rect, DFBRectangle * dest_rect);
void destination2source_transform(BCMCoreLayerHandle layer, DFBRectangle * dest_rect, DFBRectangle * source_rect);

void src2display_rect_transform(BCMCoreLayerHandle layer, DFBRectangle * src_rect, DFBRectangle * display_rect);

bdvd_graphics_pixel_format_e
dfb_surface_pixelformat_to_bdvd( DFBSurfacePixelFormat format );

#endif /* __BCMCORE_FCTS_H__ */
