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

#ifndef __DIRECT__SIGNALS_H__
#define __DIRECT__SIGNALS_H__

#include <direct/types.h>


typedef enum {
     DSHR_OK,
     DSHR_REMOVE,
     DSHR_RESUME
} DirectSignalHandlerResult;

typedef DirectSignalHandlerResult (*DirectSignalHandlerFunc)( int   num,
                                                              void *addr,
                                                              void *ctx );


DirectResult direct_signals_initialize();
DirectResult direct_signals_shutdown();

/*
 * Modifies the current thread's signal mask to block everything.
 * Should be called by input threads once to avoid killing themselves
 * in the signal handler by deinitializing all input drivers.
 */
void direct_signals_block_all();


DirectResult direct_signal_handler_add   ( int                       num,
                                           DirectSignalHandlerFunc   func,
                                           void                     *ctx,
                                           DirectSignalHandler     **ret_handler );

DirectResult direct_signal_handler_remove( DirectSignalHandler      *handler );


#endif
