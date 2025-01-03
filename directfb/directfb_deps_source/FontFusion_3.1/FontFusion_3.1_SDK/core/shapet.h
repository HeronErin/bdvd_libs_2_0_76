/*
 * Shapet.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#ifndef __T2K_SHAPET__
#define __T2K_SHAPET__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#include "tt_prvt.h"

#ifdef ALGORITHMIC_STYLES
/*
 * multiplier == 1.0 means do nothing
 */
void tsi_SHAPET_BOLD_GLYPH( GlyphClass *glyph, tsiMemObject *mem, int16 UPEM, F16Dot16 params[] );
void tsi_SHAPET_BOLD_METRICS( hmtxClass *hmtx, tsiMemObject *mem, int16 UPEM, F16Dot16 params[] );

#endif /* ALGORITHMIC_STYLES */

typedef struct
{
	/* private */
	tsiMemObject *mem;
	/* public */
} SHAPETClass;


#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_SHAPET__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/shapet.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: shapet.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.6  2003/01/07 18:25:50  Shawn_Flynn
 *     dtypED.
 *     
 *     Revision 1.5  1999/10/19 12:14:51  jfatal
 *     Typo, forgot to change one letter in the include file name to lower case.
 *     Revision 1.4  1999/10/18 17:00:48  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:11:42  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:57:39  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
