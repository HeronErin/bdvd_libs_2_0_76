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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/vt.h>
#include <sys/kd.h>
#include <errno.h>
#include <pthread.h>

#include <linux/fb.h>

#include <directfb.h>

#include <misc/conf.h>
#include <misc/util.h>

#include <direct/mem.h>
#include <direct/messages.h>
#include <direct/thread.h>
#include <direct/util.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>
#include <core/gfxcard.h>

#include "fbdev.h"
#include "vt.h"

/*
 *  FIXME: the following looks like a bad hack.
 *
 *  SIGUNUSED is no longer unused, but is defined for backwards compatibility.
 *  sparc, mips and alpha signal.h however do not define SIGUNUSED.
 */

#ifdef SIGUNUSED
     #define SIG_SWITCH_FROM  (SIGUNUSED + 10)
     #define SIG_SWITCH_TO    (SIGUNUSED + 11)
#else
     #define SIG_SWITCH_FROM  (31 + 10)
     #define SIG_SWITCH_TO    (31 + 11)
#endif

#ifndef SI_KERNEL
/* glibc 2.1.x doesn't have this in /usr/include/bits/siginfo.h */
     #define SI_KERNEL 0x80
#endif


extern FBDev *dfb_fbdev;

static VirtualTerminal *dfb_vt = NULL;

static DFBResult vt_init_switching();
static int       vt_get_fb( int vt );
static void      vt_set_fb( int vt, int fb );
static void     *vt_thread( DirectThread *thread, void *arg );

DFBResult
dfb_vt_initialize()
{
     DFBResult ret;
     struct vt_stat vs;

     dfb_vt = D_CALLOC( 1, sizeof(VirtualTerminal) );

     setsid();
     dfb_vt->fd0 = open( "/dev/tty0", O_RDONLY );
     if (dfb_vt->fd0 < 0) {
          if (errno == ENOENT) {
               dfb_vt->fd0 = open( "/dev/vc/0", O_RDONLY );
               if (dfb_vt->fd0 < 0) {
                    if (errno == ENOENT) {
                         D_PERROR( "DirectFB/core/vt: Couldn't open "
                                    "neither `/dev/tty0' nor `/dev/vc/0'!\n" );
                    }
                    else {
                         D_PERROR( "DirectFB/core/vt: "
                                    "Error opening `/dev/vc/0'!\n" );
                    }

                    D_FREE( dfb_vt );
                    dfb_vt = NULL;

                    return DFB_INIT;
               }
          }
          else {
               D_PERROR( "DirectFB/core/vt: Error opening `/dev/tty0'!\n");

               D_FREE( dfb_vt );
               dfb_vt = NULL;

               return DFB_INIT;
          }
     }

     if (ioctl( dfb_vt->fd0, VT_GETSTATE, &vs ) < 0) {
          D_PERROR( "DirectFB/core/vt: VT_GETSTATE failed!\n" );
          close( dfb_vt->fd0 );
          D_FREE( dfb_vt );
          dfb_vt = NULL;
          return DFB_INIT;
     }

     dfb_vt->prev = vs.v_active;


     if (!dfb_config->vt_switch) {
          dfb_vt->num = dfb_vt->prev;

          /* move vt to framebuffer */
          dfb_vt->old_fb = vt_get_fb( dfb_vt->num );
          vt_set_fb( dfb_vt->num, -1 );
     }
     else {
          int n;

          n = ioctl( dfb_vt->fd0, VT_OPENQRY, &dfb_vt->num );
          if (n < 0 || dfb_vt->num == -1) {
               D_PERROR( "DirectFB/core/vt: Cannot allocate VT!\n" );
               close( dfb_vt->fd0 );
               D_FREE( dfb_vt );
               dfb_vt = NULL;
               return DFB_INIT;
          }

          /* move vt to framebuffer */
          dfb_vt->old_fb = vt_get_fb( dfb_vt->num );
          vt_set_fb( dfb_vt->num, -1 );

          /* switch to vt */
          while (ioctl( dfb_vt->fd0, VT_ACTIVATE, dfb_vt->num ) < 0) {
               if (errno == EINTR)
                    continue;
               D_PERROR( "DirectFB/core/vt: VT_ACTIVATE failed!\n" );
               close( dfb_vt->fd0 );
               D_FREE( dfb_vt );
               dfb_vt = NULL;
               return DFB_INIT;
          }

          while (ioctl( dfb_vt->fd0, VT_WAITACTIVE, dfb_vt->num ) < 0) {
               if (errno == EINTR)
                    continue;
               D_PERROR( "DirectFB/core/vt: VT_WAITACTIVE failed!\n" );
               close( dfb_vt->fd0 );
               D_FREE( dfb_vt );
               dfb_vt = NULL;
               return DFB_INIT;
          }

          usleep( 40000 );
     }

     ret = vt_init_switching();
     if (ret) {
          if (dfb_config->vt_switch) {
               D_DEBUG( "switching back...\n" );
               ioctl( dfb_vt->fd0, VT_ACTIVATE, dfb_vt->prev );
               ioctl( dfb_vt->fd0, VT_WAITACTIVE, dfb_vt->prev );
               D_DEBUG( "...switched back\n" );
               ioctl( dfb_vt->fd0, VT_DISALLOCATE, dfb_vt->num );
          }

          close( dfb_vt->fd0 );
          D_FREE( dfb_vt );
          dfb_vt = NULL;
          return ret;
     }

     dfb_fbdev->vt = dfb_vt;

     return DFB_OK;
}

DFBResult
dfb_vt_join()
{
     dfb_vt_detach( true );

     return DFB_OK;
}

DFBResult
dfb_vt_shutdown( bool emergency )
{
     const char cursoron_str[] = "\033[?0;0;0c";
     const char blankon_str[] = "\033[9;10]";

     if (!dfb_vt)
          return DFB_OK;

     write( dfb_vt->fd, cursoron_str, sizeof(cursoron_str) );
     write( dfb_vt->fd, blankon_str, sizeof(blankon_str) );

     if (dfb_config->vt_switching) {
          if (ioctl( dfb_vt->fd, VT_SETMODE, &dfb_vt->vt_mode ) < 0)
               D_PERROR( "DirectFB/fbdev/vt: Unable to restore VT mode!!!\n" );

          sigaction( SIGUSR1, &dfb_vt->sig_usr1, NULL );
          sigaction( SIGUSR2, &dfb_vt->sig_usr2, NULL );

          direct_thread_cancel( dfb_vt->thread );
          direct_thread_join( dfb_vt->thread );
          direct_thread_destroy( dfb_vt->thread );

          pthread_mutex_destroy( &dfb_vt->lock );
          pthread_cond_destroy( &dfb_vt->wait );
     }

     if (dfb_config->kd_graphics) {
          if (ioctl( dfb_vt->fd, KDSETMODE, KD_TEXT ) < 0)
               D_PERROR( "DirectFB/Keyboard: KD_TEXT failed!\n" );
     }

     if (dfb_config->vt_switch) {
          D_DEBUG( "switching back...\n" );

          if (ioctl( dfb_vt->fd0, VT_ACTIVATE, dfb_vt->prev ) < 0)
               D_PERROR( "DirectFB/core/vt: VT_ACTIVATE" );

          if (ioctl( dfb_vt->fd0, VT_WAITACTIVE, dfb_vt->prev ) < 0)
               D_PERROR( "DirectFB/core/vt: VT_WAITACTIVE" );

          D_DEBUG( "switched back...\n" );

          usleep( 40000 );

          /* restore con2fbmap */
          vt_set_fb( dfb_vt->num, dfb_vt->old_fb );

          if (close( dfb_vt->fd ) < 0)
               D_PERROR( "DirectFB/core/vt: Unable to "
                          "close file descriptor of allocated VT!\n" );

          if (ioctl( dfb_vt->fd0, VT_DISALLOCATE, dfb_vt->num ) < 0)
               D_PERROR( "DirectFB/core/vt: Unable to disallocate VT!\n" );
     }
     else {
          /* restore con2fbmap */
          vt_set_fb( dfb_vt->num, dfb_vt->old_fb );

          if (close( dfb_vt->fd ) < 0)
               D_PERROR( "DirectFB/core/vt: Unable to "
                          "close file descriptor of current VT!\n" );
     }

     if (close( dfb_vt->fd0 ) < 0)
          D_PERROR( "DirectFB/core/vt: Unable to "
                     "close file descriptor of tty0!\n" );

     D_FREE( dfb_vt );
     dfb_vt = dfb_fbdev->vt = NULL;

     return DFB_OK;
}

DFBResult
dfb_vt_leave( bool emergency )
{
     return DFB_OK;
}

DFBResult
dfb_vt_detach( bool force )
{
     if (dfb_config->vt_switch || force) {
          int            fd;
          struct vt_stat vt_state;

          fd = open( "/dev/tty", O_RDONLY );
          if (fd < 0) {
               if (errno == ENXIO)
                    return DFB_OK;

               D_PERROR( "DirectFB/VT: Opening /dev/tty failed!\n" );
               return errno2result( errno );
          }

          if (ioctl( fd, VT_GETSTATE, &vt_state )) {
               close( fd );
               return DFB_OK;
          }

          if (ioctl( fd, TIOCNOTTY )) {
               D_PERROR( "DirectFB/VT: TIOCNOTTY on /dev/tty failed\n" );
               close( fd );
               return errno2result( errno );
          }

          close( fd );
     }

     return DFB_OK;
}

bool
dfb_vt_switch( int num )
{
     if (!dfb_config->vt_switching)
          return false;

     D_DEBUG( "DirectFB/fbdev/vt: switching to vt %d...\n", num );

     if (ioctl( dfb_vt->fd0, VT_ACTIVATE, num ) < 0)
          D_PERROR( "DirectFB/fbdev/vt: VT_ACTIVATE failed\n" );

     return true;
}

static void *
vt_thread( DirectThread *thread, void *arg )
{
     pthread_mutex_lock( &dfb_vt->lock );

     while (true) {
          direct_thread_testcancel( thread );

          D_DEBUG( "DirectFB/fbdev/vt: %s (%d)\n",
                    __FUNCTION__, dfb_vt->vt_sig);

          switch (dfb_vt->vt_sig) {
               default:
                    D_BUG( "unexpected vt_sig" );
                    /* fall through */

               case -1:
                    pthread_cond_wait( &dfb_vt->wait, &dfb_vt->lock );
                    continue;

               case SIGUSR1:
                    if (ioctl( dfb_vt->fd, VT_RELDISP,
                               dfb_core_suspend( NULL ) == DFB_OK ? 1 : 0 ) < 0)
                         D_PERROR( "DirectFB/fbdev/vt: VT_RELDISP failed\n" );

                    break;

               case SIGUSR2:
                    dfb_core_resume( NULL );

                    if (ioctl( dfb_vt->fd, VT_RELDISP, 2 ) < 0)
                         D_PERROR( "DirectFB/fbdev/vt: VT_RELDISP failed\n" );

                    if (dfb_config->kd_graphics) {
                         if (ioctl( dfb_vt->fd, KDSETMODE, KD_GRAPHICS ) < 0)
                              D_PERROR( "DirectFB/fbdev/vt: KD_GRAPHICS failed!\n" );
                    }

                    break;
          }

          dfb_vt->vt_sig = -1;
     }

     return NULL;
}

static void
vt_switch_handler( int signum )
{
     D_DEBUG( "DirectFB/fbdev/vt: %s (%d)\n", __FUNCTION__, signum);

     pthread_mutex_lock( &dfb_vt->lock );

     dfb_vt->vt_sig = signum;

     pthread_cond_signal( &dfb_vt->wait );

     pthread_mutex_unlock( &dfb_vt->lock );
}

static DFBResult
vt_init_switching()
{
     char buf[32];

     /* FIXME: Opening the device should be moved out of this function. */

     snprintf(buf, 32, "/dev/tty%d", dfb_vt->num);
     dfb_vt->fd = open( buf, O_RDWR );
     if (dfb_vt->fd < 0) {
          if (errno == ENOENT) {
               snprintf(buf, 32, "/dev/vc/%d", dfb_vt->num);
               dfb_vt->fd = open( buf, O_RDWR );
               if (dfb_vt->fd < 0) {
                    if (errno == ENOENT) {
                         D_PERROR( "DirectFB/core/vt: Couldn't open "
                                    "neither `/dev/tty%d' nor `/dev/vc/%d'!\n",
                                    dfb_vt->num, dfb_vt->num );
                    }
                    else {
                         D_PERROR( "DirectFB/core/vt: "
                                    "Error opening `%s'!\n", buf );
                    }

                    return errno2result( errno );
               }
          }
          else {
               D_PERROR( "DirectFB/core/vt: Error opening `%s'!\n", buf );
               return errno2result( errno );
          }
     }

     if (dfb_config->kd_graphics) {
          if (ioctl( dfb_vt->fd, KDSETMODE, KD_GRAPHICS ) < 0) {
               D_PERROR( "DirectFB/fbdev/vt: KD_GRAPHICS failed!\n" );
               close( dfb_vt->fd );
               return DFB_INIT;
          }
     }

     if (dfb_config->vt_switch) {
          ioctl( dfb_vt->fd0, TIOCNOTTY, 0 );
          ioctl( dfb_vt->fd, TIOCSCTTY, 0 );
     }

     if (dfb_config->vt_switching) {
          struct vt_mode vt;
          struct sigaction sig_tty;

          memset( &sig_tty, 0, sizeof( sig_tty ) );
          sig_tty.sa_handler = vt_switch_handler;
          sigemptyset( &sig_tty.sa_mask );

          if (sigaction( SIGUSR1, &sig_tty, &dfb_vt->sig_usr1 ) ||
              sigaction( SIGUSR2, &sig_tty, &dfb_vt->sig_usr2 )) {
               D_PERROR( "DirectFB/fbdev/vt: sigaction failed!\n" );
               close( dfb_vt->fd );
               return DFB_INIT;
          }


          vt.mode   = VT_PROCESS;
          vt.waitv  = 0;
          vt.relsig = SIGUSR1;
          vt.acqsig = SIGUSR2;

          if (ioctl( dfb_vt->fd, VT_SETMODE, &vt ) < 0) {
               D_PERROR( "DirectFB/fbdev/vt: VT_SETMODE failed!\n" );

               sigaction( SIGUSR1, &dfb_vt->sig_usr1, NULL );
               sigaction( SIGUSR2, &dfb_vt->sig_usr2, NULL );

               close( dfb_vt->fd );

               return DFB_INIT;
          }

          direct_util_recursive_pthread_mutex_init( &dfb_vt->lock );

          pthread_cond_init( &dfb_vt->wait, NULL );

          dfb_vt->vt_sig = -1;

          dfb_vt->thread = direct_thread_create( DTT_CRITICAL, vt_thread, NULL, "VT Switcher" );
     }

     return DFB_OK;
}

static int
vt_get_fb( int vt )
{
     struct fb_con2fbmap c2m;

     c2m.console = vt;

     if (ioctl( dfb_fbdev->fd, FBIOGET_CON2FBMAP, &c2m )) {
          D_PERROR( "DirectFB/FBDev/vt: "
                     "FBIOGET_CON2FBMAP failed!\n" );
          return 0;
     }

     return c2m.framebuffer;
}

static void
vt_set_fb( int vt, int fb )
{
     struct fb_con2fbmap c2m;
     struct stat         sbf;

     if (fstat( dfb_fbdev->fd, &sbf )) {
          D_PERROR( "DirectFB/FBDev/vt: Could not fstat fb device!\n" );
          return;
     }

     if (fb >= 0)
          c2m.framebuffer = fb;
     else
          c2m.framebuffer = (sbf.st_rdev & 0xFF) >> 5;

     c2m.console = vt;

     if (ioctl( dfb_fbdev->fd, FBIOPUT_CON2FBMAP, &c2m ) < 0) {
          D_PERROR( "DirectFB/FBDev/vt: "
                     "FBIOPUT_CON2FBMAP failed!\n" );
     }
}

