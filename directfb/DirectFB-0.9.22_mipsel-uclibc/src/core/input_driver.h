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

#ifndef __INPUT_DRIVER_H__
#define __INPUT_DRIVER_H__

#include <core/input.h>


static int
driver_get_available();

static void
driver_get_info( InputDriverInfo *info );

static DFBResult
driver_open_device( CoreInputDevice  *device,
                    unsigned int      number,
                    InputDeviceInfo  *info,
                    void            **driver_data );

static DFBResult
driver_get_keymap_entry( CoreInputDevice           *device,
                         void                      *driver_data,
                         DFBInputDeviceKeymapEntry *entry );

static void
driver_close_device( void *driver_data );

static const InputDriverFuncs driver_funcs = {
     GetAvailable:       driver_get_available,
     GetDriverInfo:      driver_get_info,
     OpenDevice:         driver_open_device,
     GetKeymapEntry:     driver_get_keymap_entry,
     CloseDevice:        driver_close_device
};

#define DFB_INPUT_DRIVER(shortname)                                             \
__attribute__((constructor)) void directfb_##shortname();                       \
                                                                                \
void                                                                            \
directfb_##shortname()                                                          \
{                                                                               \
     direct_modules_register( &dfb_input_modules, DFB_INPUT_DRIVER_ABI_VERSION, \
                              #shortname, &driver_funcs );                      \
}

#endif
