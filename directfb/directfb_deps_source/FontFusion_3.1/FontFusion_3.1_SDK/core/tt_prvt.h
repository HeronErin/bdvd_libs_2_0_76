/*
 * TT_PRVT.H
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

/* Private TrueType structures */
#ifndef __T2K_TT_PRVT__
#define __T2K_TT_PRVT__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


/* TrueType\OpenType table tags */

#define tag_FontHeader          0x68656164      /* 'head' */

/* Used by Apple instead of 'head' for bitmap only fonts */
#define tag_BFontHeader         0x62686564      /* 'bhed' */

#define tag_HoriHeader          0x68686561      /* 'hhea' */
#define tag_VertHeader          0x76686561      /* 'vhea' */
#define tag_IndexToLoc          0x6c6f6361      /* 'loca' */
#define tag_MaxProfile          0x6d617870      /* 'maxp' */
#define tag_ControlValue        0x63767420      /* 'cvt ' */
#define tag_MetricValue         0x6d767420      /* 'mvt ' */
#define tag_PreProgram          0x70726570      /* 'prep' */
#define tag_GlyphData           0x676c7966      /* 'glyf' */
#define tag_HorizontalMetrics   0x686d7478      /* 'hmtx' */
#define tag_VerticalMetrics     0x766d7478      /* 'vmtx' */
#define tag_CharToIndexMap      0x636d6170      /* 'cmap' */
#define tag_Kerning             0x6b65726e      /* 'kern' */
#define tag_Gasp                0x67617370      /* 'gasp' */
#define tag_HoriDeviceMetrics   0x68646d78      /* 'hdmx' */
#define tag_Encryption          0x63727970      /* 'cryp' */
#define tag_NamingTable         0x6e616d65      /* 'name' */
#define tag_FontProgram         0x6670676d      /* 'fpgm' */
#define tag_VDMX                0x56444D58      /* 'VDMX' */
#define tag_LTSH                0x4C545348      /* 'LTSH' */
#define tag_fvar                0x66766172      /* 'fvar' */

#define tag_FontVariation       0x66766172      /* 'fvar' */
#define tag_GlyphVariation      0x67766172      /* 'gvar' */
#define tag_CVTVariation        0x63766172      /* 'cvar' */

#define tag_TTCollectionID      0x74746366      /* 'ttcf' */

#define tag_CFF                 0x43464620      /* 'CFF ' */
#define tag_DSIG                0x44534947      /* 'DSIG' */

#define tag_EBLC                0x45424C43      /* 'EBLC' */
#define tag_bloc                0x626c6f63      /* 'bloc' */

#define tag_EBDT                0x45424454      /* 'EBDT' */
#define tag_bdat                0x62646174      /* 'bdat' */

#define tag_EBSC                0x45425343      /* 'EBSC' */

#define tag_post                0x706f7374      /* 'post' */
#define tag_PCLT                0x50434C54      /* 'PCLT' */
#define tag_OS_2                0x4f532f32      /* 'OS/2' */

/* Font Fusion private tags */
#define tag_sloc                0x736c6f63      /* 'sloc' */
#define tag_ffst                0x66667374      /* 'ffst' */
#define tag_ffhm                0x6666686d      /* 'ffhm' */

/* OpenType tags */
#define tag_GPOS                0x47504f53      /* 'GPOS' */
#define tag_GSUB                0x47535542      /* 'GSUB' */


/* T2K\TrueType table structures */

typedef struct {
    uint32   leftRightIndex; /* leftIndex << 16 || rightIndex */
    int16    value; 
} kernPair0Struct;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    uint16 nPairs;
    uint16 searchRange;
    uint16 entrySelector;
    uint16 rangeShift;
    kernPair0Struct *pairs;
} kernSubTable0Data;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    uint16    version;
    int32     length;
    uint16    coverage;
    void     *kernData;
} kernSubTable;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    uint16  version;
    int32   nTables;
    kernSubTable **table; /* kernSubTable *table[] */
} kernClass;


typedef struct {
    uint16 rangeMaxPPEM;
    uint16 rangeGaspBehavior;
} gaspRangeType;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    uint16    version;
    uint16    numRanges;
    gaspRangeType    *gaspRange; /* gaspRange[numRanges] */
} gaspClass;


typedef struct {
    /* public */
    uint32  offset;
    uint32  length;
} CFF_Class;


typedef struct {
    /* public */
    uint32  offset;
    uint32  length;
} DSIGClass;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    uint32  tag;
    uint32  checkSum;
    uint32  offset;
    uint32  length;
} sfnt_DirectoryEntry;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    int32   version;             /* 0x10000 (1.0)                   */
    uint16  numOffsets;          /* number of tables                */
    uint16  searchRange;         /* (max2 <= numOffsets)*16         */
    uint16  entrySelector;       /* log2(max2 <= numOffsets)        */
    uint16  rangeShift;          /* numOffsets*16-searchRange       */
    sfnt_DirectoryEntry **table; /* sfnt_DirectoryEntry : *table[numOffsets] */
} sfnt_OffsetTable;


/* --- */
typedef struct {
    uint16    platformID;
    uint16    specificID;
    uint32    offset;
} sfnt_platformEntry;

typedef struct {
    /* private */
    tsiMemObject *mem;

    int16 version;
    int16 numEncodingTables;

    int32 numGlyphs;

    sfnt_platformEntry **platform;
    uint8 *cmapData;
    int32 length;

    int16  preferedEncodingTable;
    uint16 preferedFormat;

    uint16 selectedPlatformID;
    uint16 selectedPlatformSpecificID;

    uint16 bytesConsumed;

#define NUM_FIGURES 10
    uint16 figIndex[NUM_FIGURES];

    /* public */
} cmapClass;
/* --- */

typedef struct {
    /* private */
    tsiMemObject *mem;
    uint32 version;

    /* public */
    uint32 directoryCount;
    uint32 *tableOffsets;
} ttcfClass;
/* --- */


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    int32       version;             /* for this table, set to 1.0 */
    int32       fontRevision;        /* For Font Manufacturer */
    uint32      checkSumAdjustment;
    uint32      magicNumber;         /* signature, should always be 0x5F0F3CF5  == MAGIC */
    uint16      flags;
    uint16      unitsPerEm;          /* Specifies how many in Font Units we have per EM */

    int32       created_bc;
    int32       created_ad;
    int32       modified_bc;
    int32       modified_ad;

    /** This is the font wide bounding box in ideal space
       (baselines and metrics are NOT worked into these numbers) **/
    int16       xMin;
    int16       yMin;
    int16       xMax;
    int16       yMax;

    uint16      macStyle;               /* Macintosh style word */
    uint16      lowestRecPPEM;          /* lowest recommended pixels per Em */

    /* 0: fully mixed directional glyphs, */
    /* 1: only strongly L->R or T->B glyphs, -1: only strongly R->L or B->T glyphs, */
    /* 2: like 1 but also contains neutrals, -2: like -1 but also contains neutrals */
    int16       fontDirectionHint;

    int16       indexToLocFormat;        /* 0 for short, 1 for long */
    int16       glyphDataFormat;         /* 0 for current format */
} headClass;


typedef struct {
    /* private */
    tsiMemObject *mem;


    /* public */
    int32    version;            /* for this table, set to 1.0 */
    int16    Ascender;
    int16    Descender;
    int16    LineGap;

    uint16   advanceWidthMax;
    int16    minLeftSideBearing;
    int16    minRightSideBearing;
    int16    xMaxExtent;

    int16    caretSlopeRise;
    int16    caretSlopeRun;

    int16    caretOffset;
    int16    reserved2;
    int16    reserved3;
    int16    reserved4;
    int16    reserved5;

    int16    metricDataFormat;
    uint16   numberOfHMetrics;
} hheaClass;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    int32 numGlyphs;        /* == number of glyphs in the fonts */
    int32 numberOfHMetrics; /* number of horizontal metrics */
    int16  *lsb; /* int16 lsb[ numGlyphs ], left side bearing for each glyph in FUnits */
    uint16 *aw;  /* int16 aw[ numGlyphs ], advance width for each glyph in FUnits */
} hmtxClass;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    int32 version;          /* == 1.0 for now */
    int32 N;                /* == number of glyphs in the fonts */
    uint16 defaultWidth;    /* aw = width, if the glyoph not found in the gIndex array     */
    uint16 reserved;        /* reserved    */
    uint16 *gIndex;         /* int16 aw[ N ], advance width for each glyph in FUnits     */
    uint16 *aw;             /* int16 aw[ N ], advance width for each glyph in FUnits     */
} ffhmClass;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    int32   version;                /* for this table, set to 1.0 */
    uint16  numGlyphs;              /* number of glyphs in the font */
    uint16  maxPoints;              /* in an individual glyph */
    uint16  maxContours;            /* in an individual glyph */
    uint16  maxCompositePoints;     /* in an composite glyph */
    uint16  maxCompositeContours;   /* in an composite glyph */
    uint16  maxElements;            /* set to 2, or 1 if no twilightzone points */
    uint16  maxTwilightPoints;      /* max points in element zero */
    uint16  maxStorage;             /* max number of storage locations */
    uint16  maxFunctionDefs;        /* max number of FDEFs in any preprogram */
    uint16  maxInstructionDefs;     /* max number of IDEFs in any preprogram */
    uint16  maxStackElements;       /* max number of stack elements for any individual glyph */
    uint16  maxSizeOfInstructions;  /* max size in bytes for any individual glyph */
    uint16  maxComponentElements;   /* number of glyphs referenced at top level */
    uint16  maxComponentDepth;      /* levels of recursion, 1 for simple components */
} maxpClass;


typedef struct {
    /* private */
    tsiMemObject *mem;

    /* public */
    uint32 *offsets;
    int32   n;
    int16   indexToLocFormat;
} locaClass;


#ifdef ENABLE_T2KS

/* Used by slocClass below. */
typedef struct {
    uint16 gIndex;
    uint16 delta;
    int32  offset;
} T2K_sloc_entry;

/* A class for a private table format. */
typedef struct {
    /* private */
    tsiMemObject *mem;

    uint32 version;
    uint16 num_sloc_entries;
    T2K_sloc_entry *sloc;
    uint32 numCorrections;

    uint32 correctionOffset;
    /* public */

} slocClass;

typedef struct {
    /* private */
    tsiMemObject *mem;
    
    /* public */
    int16   majorVersion;            /* */
    int16   minorVersion;            /* */

    /* properties */
    int16   numAxes;
    int16   minRadius;
    int16   maxRadius;
    int16   gIndexFirstRoman;
    int16   gIndexLastRoman;
} ffstClass;

/* Prototypes */
slocClass *FF_New_slocClass( tsiMemObject *mem, InputStream *in );
void FF_Delete_slocClass( slocClass *t );
uint32 FF_SLOC_MapIndexToOffset( slocClass *t, InputStream *in, int32 gIndex );

#endif /* ENABLE_T2KS */


#ifdef ENABLE_OTF

typedef  unsigned int GlyphID;
typedef  unsigned int Offset;

/* Class Definition table - Format 1 - Class Array */
typedef struct {

    uint16  ClassFormat;    /* = 1 */
    GlyphID StartGlyph;
    uint16  GlyphCount;

    /* uint16 ClassValuesArray[GlyphCount] */
    uint16  *ClassValuesArray;

} ClassDefFormat1_table;

/* Class Range record */
typedef struct {

    GlyphID Start;
    GlyphID End;
    uint16  Class;

} ClassRangeRecord;

/* Class Definition table - Format 2 - Class Ranges */
typedef struct {

    uint16  ClassFormat;    /* = 2 */
    uint16  ClassRangeCount;

    /* ClassRangeRecode ClassRange[ClassRangeCount] */
    ClassRangeRecord *ClassRange;

} ClassDefFormat2_table;

/* Coverage table - Format 1 - Individual Glyph Indices */
typedef struct {

    uint16  CoverageFormat; /* = 1 */
    uint16  GlyphCount;

    /* GlyphID GlyphArray[GlyphCount] */
    GlyphID *GlyphArray;

} CoverageFormat1_table;

/* Coverage range record */
typedef struct {

    GlyphID Start;
    GlyphID End;
    uint16  StartCoverageIndex;

} CoverageRangeRecord;

/* Coverage table - Format 2 - Glyph Ranges */
typedef struct {

    uint16  CoverageFormat; /* = 2 */
    uint16  RangeCount;

    /* CoverageRangeRecord RangeRecord[RangeCount] */
    CoverageRangeRecord *RangeRecord;

} CoverageFormat2_table;

/* Attach Point table */
typedef struct {

    uint16  PointCount;

    /* uint16 PointIndex[PointCount] */
    uint16 *PointIndex;

} AttachPoint_table;

/* Attach List table */
typedef struct {

    Offset  Coverage;
    uint16  GlyphCount;

    /* Offset AttachPoint[GlyphCount] */
    Offset  *AttachPoint;

} AttachList_table;

/* Device table */
typedef struct {

    uint16  StartSize;
    uint16  EndSize;
    uint16  DeltaFormat;    /* 1=2 bit; 2=4 bit; 3=8 bit */

    /* uint16  DeltaValues[] */
    uint16 *DeltaValues;

} Device_table;

/* Caret Value table - Format 1 - Design Units only */
typedef struct {

    uint16  CaretValueFormat;   /* = 1 */
    int16   Coordinate;

} CaretValueFormat1_table;

/* Caret Value table - Format 2 - Contour Points */
typedef struct {

    uint16  CaretValueFormat;   /* = 2 */
    uint16  CaretValuePoint;

} CaretValueFormat2_table;

/* Caret Value table - Format 3 - Design Units & Device table */
typedef struct {

    uint16  CaretValueFormat;   /* = 3 */
    int16   Coordinate;

    Offset  DeviceTable;

} CaretValueFormat3_table;

/* Ligature Glyph table */
typedef struct {

    uint16  CaretCount;

    /* Offset  CarewtValue[CaretCount] */
    Offset *CaretValue;

} LigGlyph_table;

/* Ligature Caret List table */
typedef struct {

    Offset  Coverage;
    uint16  LigGlyphCount;

    /* Offset LigGlyph[LigGlyphCount] */
    Offset *LigGlyph;

} LigCaretList_table;



/* Glyph Definition Header (GDEF) table */
typedef struct {

    int32   Version;    /* initially 0x00010000 */

    Offset  GlyphClassDef;
    Offset  AttachList;
    Offset  LigCaretList;
    Offset  MarkAttachClassDef;

} GDEFHeader_table;

#endif



#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __T2K_TT_PRVT__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: /encentrus/Andover/juju-20060206-1447/ndvd/directfb/directfb_deps_source/FontFusion_3.1/FontFusion_3.1_SDK/core/tt_prvt.h,v 1.1 2006/02/08 17:20:43 xmiville Exp $
 *                                                                           *
 *     $Log: tt_prvt.h,v $
 *     Revision 1.1  2006/02/08 17:20:43  xmiville
 *     Commit of DirectFB changes to 060206-1447 release.
 *
 *     Revision 1.1  2006/01/27 14:18:26  xmiville
 *     *** empty log message ***
 *
 *     Revision 1.16  2004/05/10 14:33:58  Shawn_Flynn
 *     Removed duplicate tag for the "GDEF" table.
 *     
 *     Revision 1.15  2002/05/22 20:04:08  Shawn_Flynn
 *     Modified incorrect typedefs for Truetype table structures.
 *     Added new typedefs for OpenType table structures under ENABLE_OTF.
 *     
 *     Revision 1.14  2002/03/06 16:10:20  Shawn_Flynn
 *     Added definitions for CFF_Class and DSIG_Class.
 *     Added table tags for tag_CFF and tag_DSIG.
 *
 *     Revision 1.13  2001/05/07 20:30:46  shawn
 *     Added element 'uint16 BytesConsumed;' to cmapClass.
 *     Revision 1.12  2001/05/02 17:20:43  reggers
 *     SEAT BELT mode added (Sampo)
 *     Revision 1.11  2001/04/24 21:57:20  reggers
 *     Added GASP table support (Sampo).
 *     Revision 1.10  2001/03/30 15:36:57  shawn
 *     Added numGlyphs, selectedPlatformID and selectedPlatform SpecificID
 *     to the cmapClass.
 *     Modified all member types in headClass and maxpClass to conform
 *     to Truetype specification.
 *
 *     Revision 1.9  2000/11/02 17:21:49  reggers
 *     Treat maxStackElements as unsigned.
 *     Revision 1.8  2000/01/07 19:46:06  reggers
 *     Sampo enhancements for FFS fonts.
 *     Revision 1.7  1999/12/23 22:03:22  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.6  1999/11/04 20:20:41  reggers
 *     Added code for getting fixed/proportional setting, firstCharCode and
 *     lastCharCode.
 *     Revision 1.5  1999/09/30 15:12:40  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.4  1999/08/27 20:08:57  reggers
 *     Latest changes from Sampo
 *     Revision 1.3  1999/07/16 17:52:15  sampo
 *     Sampo work. Drop #8 July 16, 1999
 *     Revision 1.2  1999/05/17 15:58:53  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

