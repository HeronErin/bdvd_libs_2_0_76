/*
 * Ghints.h
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

#ifdef ENABLE_AUTO_GRIDDING_CORE

#ifndef __T2K_GHINTS__
#define __T2K_GHINTS__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

void t2k_ComputeGlobalHints( sfntClass *font, ag_HintHandleType hintHandle, ag_GlobalDataType *gHints, int kanji );
void t2k_ReadGHints( ag_GlobalDataType *gHints, InputStream *in );
void t2k_WriteGHints( ag_GlobalDataType *gHints, OutputStream *out );


#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_GHINTS__ */

#endif /* ENABLE_AUTO_GRIDDING_CORE */




/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/ghints.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: ghints.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.3  1999/09/30 15:11:17  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:56:55  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
