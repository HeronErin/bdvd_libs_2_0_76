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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include <pthread.h>

#include <direct/list.h>
#include <fusion/shmalloc.h>

#include <directfb.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>
#include <core/surfacemanager_internal.h>
#include <core/surfacemanager_firstfit.h>
#include <core/system.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>


D_DEBUG_DOMAIN( Core_SM, "Core/SurfaceMgrFirstFit", "DirectFB Surface Manager" );

/*
 * initially there is one big free chunk,
 * chunks are splitted into a free and an occupied chunk if memory is allocated,
 * two chunks are merged to one free chunk if memory is deallocated
 */
struct _Chunk {
     int             magic;

     int             offset;      /* offset in video memory,
                                     is greater or equal to the heap offset */
     int             length;      /* length of this chunk in bytes */

     SurfaceBuffer  *buffer;      /* pointer to surface buffer occupying
                                     this chunk, or NULL if chunk is free */

     int             tolerations; /* number of times this chunk was scanned
                                     occupied, resetted in assure_video */

     Chunk          *prev;
     Chunk          *next;
};

static Chunk* split_chunk( Chunk *c, int length );
static Chunk* free_chunk( SurfaceManager *manager, Chunk *chunk );
static void occupy_chunk( SurfaceManager *manager, Chunk *chunk, SurfaceBuffer *buffer, int length );

SurfaceManager *
dfb_surfacemanager_firstfit_create( unsigned int     offset,
                                    unsigned int     length,
                                    unsigned int     minimum_page_size,
                                    CardLimitations *limits )
{
     Chunk          *chunk;
     SurfaceManager *manager;

#ifdef DEBUG_ALLOCATION
     dfbbcm_show_allocation = getenv( "dfbbcm_show_allocation" ) ? true : false;
#endif

     manager = SHCALLOC( 1, sizeof(SurfaceManager) );
     if (!manager)
          return NULL;

     manager->type = DSMT_SURFACEMANAGER_FIRSTFIT;

     chunk = SHCALLOC( 1, sizeof(Chunk) );
     if (!chunk) {
          SHFREE( manager );
          return NULL;
     }

     chunk->offset = offset;
     chunk->length = length;

     manager->heap_offset      = offset;
     manager->chunks           = chunk;
     manager->length           = length;
     manager->available        = length;
     manager->byteoffset_align = limits->surface_byteoffset_alignment;
     manager->pixelpitch_align = limits->surface_pixelpitch_alignment;
     manager->bytepitch_align  = limits->surface_bytepitch_alignment;
     manager->max_power_of_two_pixelpitch = limits->surface_max_power_of_two_pixelpitch;
     manager->max_power_of_two_bytepitch  = limits->surface_max_power_of_two_bytepitch;
     manager->max_power_of_two_height     = limits->surface_max_power_of_two_height;
#ifdef DEBUG_ALLOCATION
     D_INFO("Firstfit Heap Lenght %d\n", length);
#endif

     fusion_skirmish_init( &manager->lock, "Surface Manager" );

     D_MAGIC_SET( chunk, _Chunk_ );

     D_MAGIC_SET( manager, SurfaceManager );

     return manager;
}

void
dfb_surfacemanager_firstfit_destroy( SurfaceManager *manager )
{
     Chunk *chunk;

     D_ASSERT( manager != NULL );
     D_ASSERT( manager->chunks != NULL );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     D_MAGIC_CLEAR( manager );

     /* Deallocate all chunks. */
     chunk = manager->chunks;
     while (chunk) {
          Chunk *next = chunk->next;

          D_MAGIC_CLEAR( chunk );

          SHFREE( chunk );

          chunk = next;
     }

     /* Destroy manager lock. */
     fusion_skirmish_destroy( &manager->lock );

     /* Deallocate manager struct. */
     SHFREE( manager );
}

DFBResult dfb_surfacemanager_firstfit_suspend( SurfaceManager *manager )
{
     Chunk *c;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     manager->suspended = true;

     c = manager->chunks;
     while (c) {
          if (c->buffer &&
              c->buffer->policy != CSP_VIDEOONLY &&
              c->buffer->video.health == CSH_STORED)
          {
               dfb_surfacemanager_assure_system( manager, c->buffer );
               c->buffer->video.health = CSH_RESTORE;
          }

          c = c->next;
     }

     dfb_surfacemanager_unlock( manager );

     return DFB_OK;
}

DFBResult dfb_surfacemanager_firstfit_adjust_heap_offset( SurfaceManager *manager,
                                                          int             offset )
{
     D_ASSERT( offset >= 0 );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     if (manager->byteoffset_align > 1) {
          offset += manager->byteoffset_align - 1;
          offset -= offset % manager->byteoffset_align;
     }

     if (manager->chunks->buffer == NULL) {
          /* first chunk is free */
          if (offset <= manager->chunks->offset + manager->chunks->length) {
               /* ok, just recalculate offset and length */
               manager->chunks->length = manager->chunks->offset + manager->chunks->length - offset;
               manager->chunks->offset = offset;
          }
          else {
               D_WARN("unable to adjust heap offset");
               /* more space needed than free at the beginning */
               /* TODO: move/destroy instances */
          }
     }
     else {
          D_WARN("unable to adjust heap offset");
          /* very rare case that the first chunk is occupied */
          /* TODO: move/destroy instances */
     }

     manager->heap_offset = offset;

     dfb_surfacemanager_unlock( manager );

     return DFB_OK;
}

void
test_chunks( SurfaceManager  *manager)
{
     Chunk *c;
     Chunk *p;
     unsigned int cum_length = 0;
     bool test_error = false;

     D_ASSERT( manager != NULL );


     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     c = manager->chunks;

     if (c)
     {
          if (c->next)
          {
                p = c;
                c = c->next;

                while (c)
                {
                    if ((p->offset + p->length) != c->offset)
                    {
                        D_INFO("FirstFit allocation error between addr 0x%x size %d and addr 0x%x size %d\n",
                        p->offset,
                        p->length,
                        c->offset,
                        c->length);
                        test_error = true;
                        break;
                    }
                    else
                    {
                        cum_length += p->length;

                    }

                    p = p->next;
                    c = c->next;
                }
                cum_length += p->length;
          }
     }

     if (cum_length != manager->length)
     {
          D_INFO("Cumulative chunk size %d does not match heap size %d\n", cum_length, manager->length);
     }

     dfb_surfacemanager_unlock( manager );
};

void
dfb_surfacemanager_firstfit_enumerate_chunks( SurfaceManager  *manager,
                                              SMChunkCallback  callback,
                                              void            *ctx )
{
     Chunk *c;

     D_ASSERT( manager != NULL );
     D_ASSERT( callback != NULL );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     c = manager->chunks;
     while (c) {

          if (callback( c->buffer, c->offset,
                        c->length, c->tolerations, ctx) == DFENUM_CANCEL)
               break;

          c = c->next;
     }

     test_chunks(manager);

     dfb_surfacemanager_unlock( manager );
}




void
dfb_surfacemanager_firstfit_retrive_surface_info(
    SurfaceManager *manager,
    SurfaceBuffer  *surface,
    int            *offset,
    int            *length,
    int            *tolerations
)
{
     Chunk *c;

     D_ASSERT( manager != NULL );


     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     c = manager->chunks;
     while (c) {
         /* look for the surface */
         if( c->buffer == surface)
         {
             *offset      = c->offset;
             *length      = c->length;
             *tolerations = c->tolerations;
             break;
         }

         c = c->next;
     }

     dfb_surfacemanager_unlock( manager );
}


int align(int num, int alignment)
{
  int align = num & (alignment - 1);
  if (align) num += alignment - align;

  return num;
}

/** public functions NOT locking the surfacemanger theirself,
    to be called between lock/unlock of surfacemanager **/

DFBResult dfb_surfacemanager_firstfit_allocate( SurfaceManager *manager,
                                                SurfaceBuffer  *buffer )
{
     int pitch;
     int length;
     int width;
     int height;
     Chunk *c;

     Chunk *best_free = NULL;
     Chunk *best_occupied = NULL;

     CoreSurface *surface = buffer->surface;


     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (!manager->length || manager->suspended)
          return DFB_NOVIDEOMEMORY;

     width = surface->width;
     height = surface->height;

     /* Align width and height before malloc */
     if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_4)
          width = (width + 3) & ~3;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_8)
          width = (width + 7) & ~7;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_16)
          width = (width + 15) & ~15;

     if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_4)
          height = (height + 3) & ~3;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_8)
          height = (height + 7) & ~7;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_16)
          height = (height + 15) & ~15;

     /* calculate the required length depending on limitations */
     pitch = MAX( width, surface->min_width );

     if (pitch < manager->max_power_of_two_pixelpitch &&
         height < manager->max_power_of_two_height)
          pitch = 1 << direct_log2( pitch );

     if (manager->pixelpitch_align > 1) {
          pitch += manager->pixelpitch_align - 1;
          pitch -= pitch % manager->pixelpitch_align;
     }

     pitch = DFB_BYTES_PER_LINE( buffer->format, pitch );

     if (pitch < manager->max_power_of_two_bytepitch &&
         height < manager->max_power_of_two_height)
          pitch = 1 << direct_log2( pitch );

     if (manager->bytepitch_align > 1) {
          pitch += manager->bytepitch_align - 1;
          pitch -= pitch % manager->bytepitch_align;
     }

     length = DFB_PLANE_MULTIPLY( buffer->format,
                                  MAX( height, surface->min_height ) * pitch );

     if (manager->byteoffset_align > 1) {
          length += manager->byteoffset_align - 1;
          length -= length % manager->byteoffset_align;
     }

     /* Do a pre check before iterating through all chunks. */
     if (length > manager->available/* - manager->heap_offset*/)
          return DFB_NOVIDEOMEMORY;

     buffer->video.pitch = pitch;

     /* examine chunks */
     c = manager->chunks;
     while (c) {
          if (c->length >= length) {
               if (c->buffer) {
                    c->tolerations++;
                    if (c->tolerations > 0xff)
                         c->tolerations = 0xff;

                    if (!c->buffer->video.locked              &&
                        c->buffer->policy <= buffer->policy   &&
                        c->buffer->policy != CSP_VIDEOONLY    &&

                        ((buffer->policy > c->buffer->policy) ||
                         (c->tolerations > manager->min_toleration/8 + 2)))
                    {
                         /* found a nice place to chill */
                         if (!best_occupied  ||
                              best_occupied->length > c->length  ||
                              best_occupied->tolerations < c->tolerations)
                              /* first found or better one? */
                              best_occupied = c;
                    }
               }
               else {
                    /* found a nice place to chill */
                    if (!best_free  ||  best_free->length > c->length)
                         /* first found or better one? */
                         best_free = c;
               }
          }

          c = c->next;
     }

     /* if we found a place */
     if (best_free) {
          occupy_chunk( manager, best_free, buffer, length );

          if (manager->funcs->AllocateSurfaceBuffer) {
               if (manager->funcs->AllocateSurfaceBuffer(manager, buffer) != DFB_OK) {
                    dfb_surfacemanager_firstfit_deallocate( manager, buffer );
                    return DFB_NOVIDEOMEMORY;
               }
          }

          return DFB_OK;
     }

     if (best_occupied) {
          SurfaceBuffer *kicked = best_occupied->buffer;

          D_DEBUG_AT( Core_SM, "Kicking out buffer at %d (%d) with tolerations %d...\n",
                      best_occupied->offset,
                      best_occupied->length, best_occupied->tolerations );

          dfb_surfacemanager_assure_system( manager, kicked );

          kicked->video.health = CSH_INVALID;
          dfb_surface_notify_listeners( kicked->surface, CSNF_VIDEO );

          best_occupied = free_chunk( manager, best_occupied );

          if (kicked->video.access & VAF_HARDWARE_READ) {
               dfb_gfxcard_sync();
               kicked->video.access &= ~(VAF_HARDWARE_READ | VAF_HARDWARE_WRITE);
          }

          occupy_chunk( manager, best_occupied, buffer, length );

          if (manager->funcs->AllocateSurfaceBuffer) {
               if (manager->funcs->AllocateSurfaceBuffer(manager, buffer) != DFB_OK) {
                    dfb_surfacemanager_firstfit_deallocate( manager, buffer );
                    return DFB_NOVIDEOMEMORY;
               }
          }

          return DFB_OK;
     }


     D_DEBUG_AT( Core_SM, "Couldn't allocate enough heap space for video memory surface!\n" );

     /* no luck */
     return DFB_NOVIDEOMEMORY;
}

DFBResult dfb_surfacemanager_firstfit_deallocate( SurfaceManager *manager,
                                                  SurfaceBuffer  *buffer )
{
     int    loops = 0;
     Chunk *chunk = buffer->video.chunk;

     D_ASSERT( buffer->surface );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (manager->funcs->DeallocateSurfaceBuffer) {
         /* Should check DFBResult */
         manager->funcs->DeallocateSurfaceBuffer(manager, buffer);
     }

     if (buffer->video.health == CSH_INVALID)
          return DFB_OK;

     if (buffer->video.access & VAF_SOFTWARE_WRITE) {
          dfb_gfxcard_flush_texture_cache(buffer);
          buffer->video.access &= ~VAF_SOFTWARE_WRITE;
     }

     if (buffer->video.access & VAF_HARDWARE_READ) {
          dfb_gfxcard_sync();
          buffer->video.access &= ~(VAF_HARDWARE_READ | VAF_HARDWARE_WRITE);
     }

     if (buffer->video.access & VAF_HARDWARE_WRITE) {
          dfb_gfxcard_wait_serial( &buffer->video.serial );
          buffer->video.access &= ~VAF_HARDWARE_WRITE;
     }

     buffer->video.health = CSH_INVALID;
     buffer->video.chunk = NULL;

     dfb_surface_notify_listeners( buffer->surface, CSNF_VIDEO );

     while (buffer->video.locked) {
          if (++loops > 1000)
               break;

          sched_yield();
     }

     if (buffer->video.locked)
          D_WARN( "Freeing chunk with a non-zero lock counter" );

     if (chunk)
          free_chunk( manager, chunk );

     return DFB_OK;
}

DFBResult dfb_surfacemanager_firstfit_assure_video( SurfaceManager *manager,
                                                    SurfaceBuffer  *buffer )
{
     DFBResult    ret;
     CoreSurface *surface = buffer->surface;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (manager->suspended)
          return DFB_NOVIDEOMEMORY;

     switch (buffer->video.health) {
          case CSH_STORED:
               if (buffer->video.chunk)
                    buffer->video.chunk->tolerations = 0;

               return DFB_OK;

          case CSH_INVALID:

               ret = dfb_surfacemanager_firstfit_allocate( manager, buffer );
               if (ret == DFB_NOVIDEOMEMORY) {
                    D_WARN( "ran out of video memory" );
#ifdef DEBUG_ALLOCATION
                    dfb_surfacemanager_enumerate_chunks( manager,
                    surfacemanager_chunk_callback,
                    NULL );
#endif
               }
               if (ret)
                    return ret;

               /* FALL THROUGH, because after successful allocation
                  the surface health is CSH_RESTORE */

          case CSH_RESTORE:
               if (buffer->system.health != CSH_STORED)
                    D_BUG( "system/video instances both not stored!" );

               if (buffer->flags & SBF_WRITTEN) {
                    int   i;
                    char *src = buffer->system.addr;
                    char *dst = dfb_system_video_memory_virtual( buffer->video.offset );

                    dfb_surface_video_access_by_software( buffer, DSLF_WRITE );

                    for (i=0; i<surface->height; i++) {
                         direct_memcpy( dst, src,
                                        DFB_BYTES_PER_LINE(buffer->format, surface->width) );
                         src += buffer->system.pitch;
                         dst += buffer->video.pitch;
                    }

                    if (buffer->format == DSPF_YV12 || buffer->format == DSPF_I420) {
                         for (i=0; i<surface->height; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width / 2) );
                              src += buffer->system.pitch / 2;
                              dst += buffer->video.pitch  / 2;
                         }
                    }
                    else if (buffer->format == DSPF_NV12 || buffer->format == DSPF_NV21) {
                         for (i=0; i<surface->height/2; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width) );
                              src += buffer->system.pitch;
                              dst += buffer->video.pitch;
                         }
                    }
                    else if (buffer->format == DSPF_NV16) {
                         for (i=0; i<surface->height; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width) );
                              src += buffer->system.pitch;
                              dst += buffer->video.pitch;
                         }
                    }
               }

               buffer->video.health             = CSH_STORED;

               buffer->video.chunk->tolerations = 0;

               dfb_surface_notify_listeners( surface, CSNF_VIDEO );

               return DFB_OK;

          default:
               break;
     }

     D_BUG( "unknown video instance health" );
     return DFB_BUG;
}


/** internal functions NOT locking the surfacemanager **/

static Chunk* split_chunk( Chunk *c, int length )
{
     Chunk *newchunk;

     D_MAGIC_ASSERT( c, _Chunk_ );

     if (c->length == length)          /* does not need be splitted */
          return c;

     newchunk = (Chunk*) SHCALLOC( 1, sizeof(Chunk) );

     /* calculate offsets and lengths of resulting chunks */
     newchunk->offset = c->offset + c->length - length;
     newchunk->length = length;
     c->length -= newchunk->length;

     /* insert newchunk after chunk c */
     newchunk->prev = c;
     newchunk->next = c->next;
     if (c->next)
          c->next->prev = newchunk;
     c->next = newchunk;

     D_MAGIC_SET( newchunk, _Chunk_ );

     return newchunk;
}

static Chunk*
free_chunk( SurfaceManager *manager, Chunk *chunk )
{
     D_MAGIC_ASSERT( manager, SurfaceManager );
     D_MAGIC_ASSERT( chunk, _Chunk_ );

     if (!chunk->buffer) {
          D_BUG( "freeing free chunk" );
          return chunk;
     }
     else {
#if D_DEBUG_ENABLED
          D_DEBUG_AT( Core_SM,
#else
          D_ALLOCATION( "Core/SurfaceMgrFirstFit -> "
#endif
          "free_chunk: deallocating %d bytes at offset 0x%08X.\n", chunk->length, (unsigned int)chunk->offset );
     }

     if (chunk->buffer->policy == CSP_VIDEOONLY) {
          manager->available += chunk->length;
          D_ALLOCATION( "Core/SurfaceMgrFirstFit -> free_chunk: now %d bytes available\n", manager->available );
     }

     chunk->buffer = NULL;

     manager->min_toleration--;

     if (chunk->prev  &&  !chunk->prev->buffer) {
          Chunk *prev = chunk->prev;

          //D_DEBUG_AT( Core_SM, "  -> merging with previous chunk at %d\n", prev->offset );

          prev->length += chunk->length;

          prev->next = chunk->next;
          if (prev->next)
               prev->next->prev = prev;

          //D_DEBUG_AT( Core_SM, "  -> freeing %p (prev %p, next %p)\n", chunk, chunk->prev, chunk->next);

          D_MAGIC_CLEAR( chunk );

          SHFREE( chunk );
          chunk = prev;
     }

     if (chunk->next  &&  !chunk->next->buffer) {
          Chunk *next = chunk->next;

          //D_DEBUG_AT( Core_SM, "  -> merging with next chunk at %d\n", next->offset );

          chunk->length += next->length;

          chunk->next = next->next;
          if (chunk->next)
               chunk->next->prev = chunk;

          D_MAGIC_CLEAR( next );

          SHFREE( next );
     }

     return chunk;
}

static void
occupy_chunk( SurfaceManager *manager, Chunk *chunk, SurfaceBuffer *buffer, int length )
{
     D_MAGIC_ASSERT( manager, SurfaceManager );
     D_MAGIC_ASSERT( chunk, _Chunk_ );

     if (buffer->policy == CSP_VIDEOONLY) {
          manager->available -= length;
          D_ALLOCATION( "Core/SurfaceMgrFirstFit -> occupy_chunk: now %d bytes available\n", manager->available );
     }

     chunk = split_chunk( chunk, length );

#if D_DEBUG_ENABLED
          D_DEBUG_AT( Core_SM,
#else
          D_ALLOCATION( "Core/SurfaceMgrFirstFit -> "
#endif
          "occupy_chunk: allocating %d bytes at offset 0x%08X.\n", chunk->length, (unsigned int)chunk->offset );

     buffer->video.health = CSH_RESTORE;
     buffer->video.offset = chunk->offset;
     buffer->video.chunk  = chunk;

     chunk->buffer = buffer;

     manager->min_toleration++;
}

