/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org>,
              Ville Syrjl <syrjala@sci.fi> and
	      Michel Dnzer <michel@daenzer.net>.

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

#ifndef REG_RADEON_H
#define REG_RADEON_H


#define CRTC_STATUS			0x005c
#define CRTC_VBLANK_SAVE		 0x00000002
#define CRTC_VBLANK_SAVE_CLEAR		 0x00000002

#define RBBM_STATUS			0x0e40
#define	RBBM_FIFOCNT_MASK		 0x0000007f
#define RBBM_ACTIVE			 0x80000000


/******************************************************************************
 *                  GUI Block Memory Mapped Registers                         *
 *                     These registers are FIFOed.                            *
 *****************************************************************************/

#define DST_OFFSET                      0x1404
#define DST_PITCH                       0x1408

#define SRC_Y_X                         0x1434
#define DST_Y_X                         0x1438
#define DST_HEIGHT_WIDTH                0x143c

#define DP_GUI_MASTER_CNTL              0x146c
#define GMC_SRC_PITCH_OFFSET_NODEFAULT	 0x00000001
#define GMC_DST_PITCH_OFFSET_NODEFAULT	 0x00000002
#define GMC_SRC_CLIP_NODEFAULT		 0x00000004
#define GMC_DST_CLIP_NODEFAULT		 0x00000008
#define GMC_BRUSH_SOLIDCOLOR		 0x000000d0
#define GMC_BRUSH_NONE			 0x000000f0
#define GMC_DST_8BPP			 0x00000200
#define GMC_DST_15BPP			 0x00000300
#define GMC_DST_16BPP			 0x00000400
#define GMC_DST_32BPP			 0x00000600
#define GMC_SRC_FG			 0x00001000
#define GMC_SRC_DSTCOLOR		 0x00003000
#define GMC_ROP3_PATCOPY		 0x00f00000
#define GMC_ROP3_SRCCOPY		 0x00cc0000
#define GMC_DP_SRC_RECT			 0x02000000
#define GMC_DST_CLR_CMP_FCN_CLEAR	 0x10000000
#define GMC_WRITE_MASK_DIS		 0x40000000

#define DP_BRUSH_FRGD_CLR               0x147c

#define SRC_OFFSET                      0x15ac
#define SRC_PITCH                       0x15b0

#define CLR_CMP_CNTL                    0x15c0
#define CLR_CMP_CLR_SRC                 0x15c4

#define CLR_CMP_MASK                    0x15cc

#define DST_LINE_START                  0x1600
#define DST_LINE_END                    0x1604

#if 0
#define SC_LEFT                         0x1640
#define SC_RIGHT                        0x1644
#define SC_TOP                          0x1648
#define SC_BOTTOM                       0x164c

#define SRC_SC_RIGHT                    0x1654
#define SRC_SC_BOTTOM                   0x165c
#endif

#define DP_CNTL                         0x16c0
#define DST_X_RIGHT_TO_LEFT		 0x00000000
#define DST_X_LEFT_TO_RIGHT		 0x00000001
#define DST_Y_BOTTOM_TO_TOP		 0x00000000
#define DST_Y_TOP_TO_BOTTOM		 0x00000002
#define DST_X_MAJOR			 0x00000000
#define DST_Y_MAJOR			 0x00000004
#define DST_X_TILE			 0x00000008
#define DST_Y_TILE			 0x00000010
#define DST_LAST_PEL			 0x00000020
#define DST_TRAIL_X_RIGHT_TO_LEFT	 0x00000000
#define DST_TRAIL_X_LEFT_TO_RIGHT	 0x00000040
#define DST_TRAP_FILL_RIGHT_TO_LEFT	 0x00000000
#define DST_TRAP_FILL_LEFT_TO_RIGHT	 0x00000080
#define DST_BRES_SIGN			 0x00000100
#define DST_HOST_BIG_ENDIAN_EN		 0x00000200
#define DST_POLYLINE_NONLAST		 0x00008000
#define DST_RASTER_STALL		 0x00010000
#define DST_POLY_EDGE			 0x00040000

#define SC_TOP_LEFT                     0x16ec
#define SC_BOTTOM_RIGHT                 0x16f0
#define SRC_SC_BOTTOM_RIGHT             0x16f4


/* CONSTANTS */
#define ENGINE_IDLE                     0x0


/* overlay */
#define	OV0_Y_X_START				0x0400
#define	OV0_Y_X_END				0x0404
#define	OV0_PIPELINE_CNTL			0x0408
#define	OV0_EXCLUSIVE_HORZ			0x0408
#	define EXCL_HORZ_START_MASK		0x000000ff
#	define EXCL_HORZ_END_MASK		0x0000ff00
#	define EXCL_HORZ_BACK_PORCH_MASK	0x00ff0000
#	define EXCL_HORZ_EXCLUSIVE_EN		0x80000000
#define	OV0_EXCLUSIVE_VERT			0x040C
#	define EXCL_VERT_START_MASK		0x000003ff
#	define EXCL_VERT_END_MASK		0x03ff0000
#define	OV0_REG_LOAD_CNTL			0x0410
#	define REG_LD_CTL_LOCK			0x00000001L
#	define REG_LD_CTL_VBLANK_DURING_LOCK	0x00000002L
#	define REG_LD_CTL_STALL_GUI_UNTIL_FLIP	0x00000004L
#	define REG_LD_CTL_LOCK_READBACK		0x00000008L
#define	OV0_SCALE_CNTL				0x0420
#	define SCALER_PIX_EXPAND		0x00000001L
#	define SCALER_Y2R_TEMP			0x00000002L
#	define SCALER_HORZ_PICK_NEAREST		0x00000004L
#	define SCALER_VERT_PICK_NEAREST		0x00000008L
#	define SCALER_SIGNED_UV			0x00000010L
#	define SCALER_GAMMA_SEL_MASK		0x00000060L
#	define SCALER_GAMMA_SEL_BRIGHT		0x00000000L
#	define SCALER_GAMMA_SEL_G22		0x00000020L
#	define SCALER_GAMMA_SEL_G18		0x00000040L
#	define SCALER_GAMMA_SEL_G14		0x00000060L
#	define SCALER_COMCORE_SHIFT_UP_ONE	0x00000080L
#	define SCALER_SURFAC_FORMAT		0x00000f00L
#	define SCALER_SOURCE_UNK0		0x00000000L
#	define SCALER_SOURCE_UNK1		0x00000100L
#	define SCALER_SOURCE_UNK2		0x00000200L
#	define SCALER_SOURCE_15BPP		0x00000300L
#	define SCALER_SOURCE_16BPP		0x00000400L
#	define SCALER_SOURCE_32BPP		0x00000600L
#	define SCALER_SOURCE_UNK3		0x00000700L
#	define SCALER_SOURCE_UNK4		0x00000800L
#	define SCALER_SOURCE_YUV9		0x00000900L
#	define SCALER_SOURCE_YUV12		0x00000A00L
#	define SCALER_SOURCE_VYUY422		0x00000B00L
#	define SCALER_SOURCE_YVYU422		0x00000C00L
#	define SCALER_SOURCE_UNK5		0x00000D00L
#	define SCALER_SOURCE_UNK6		0x00000E00L
#	define SCALER_SOURCE_UNK7		0x00000F00L
#	define SCALER_ADAPTIVE_DEINT		0x00001000L
#	define R200_SCALER_TEMPORAL_DEINT	0x00002000L
#	define SCALER_UNKNOWN_FLAG1		0x00004000L
#	define SCALER_SMART_SWITCH		0x00008000L
#	define SCALER_BURST_PER_PLANE		0x007f0000L
#	define SCALER_DOUBLE_BUFFER_REGS	0x01000000L
#	define SCALER_UNKNOWN_FLAG3		0x02000000L
#	define SCALER_UNKNOWN_FLAG4		0x04000000L
#	define SCALER_DIS_LIMIT			0x08000000L
#	define SCALER_PRG_LOAD_START		0x10000000L
#	define SCALER_INT_EMU			0x20000000L
#	define SCALER_ENABLE			0x40000000L
#	define SCALER_SOFT_RESET		0x80000000L
#define	OV0_V_INC				0x0424
#define	OV0_P1_V_ACCUM_INIT			0x0428
#	define OV0_P1_MAX_LN_IN_PER_LN_OUT	0x00000003L
#	define OV0_P1_V_ACCUM_INIT_MASK		0x01ff8000L
#define	OV0_P23_V_ACCUM_INIT			0x042C
#	define OV0_P23_MAX_LN_IN_PER_LN_OUT	0x00000003L
#	define OV0_P23_V_ACCUM_INIT_MASK	0x01ff8000L
#define	OV0_P1_BLANK_LINES_AT_TOP		0x0430
#	define P1_BLNK_LN_AT_TOP_M1_MASK	0x00000fffL
#	define P1_ACTIVE_LINES_M1		0x0fff0000L
#define	OV0_P23_BLANK_LINES_AT_TOP		0x0434
#	define P23_BLNK_LN_AT_TOP_M1_MASK	0x000007ffL
#	define P23_ACTIVE_LINES_M1		0x07ff0000L
#define	OV0_BASE_ADDR				0x043C
#define	OV0_VID_BUF0_BASE_ADRS			0x0440
#	define VIF_BUF0_PITCH_SEL		0x00000001L
#	define VIF_BUF0_TILE_ADRS		0x00000002L
#	define VIF_BUF0_BASE_ADRS_MASK		0xfffffff0L
#	define VIF_BUF0_1ST_LINE_LSBS_MASK	0x48000000L
#define	OV0_VID_BUF1_BASE_ADRS			0x0444
#	define VIF_BUF1_PITCH_SEL		0x00000001L
#	define VIF_BUF1_TILE_ADRS		0x00000002L
#	define VIF_BUF1_BASE_ADRS_MASK		0xfffffff0L
#	define VIF_BUF1_1ST_LINE_LSBS_MASK	0x48000000L
#define	OV0_VID_BUF2_BASE_ADRS			0x0448
#	define VIF_BUF2_PITCH_SEL		0x00000001L
#	define VIF_BUF2_TILE_ADRS		0x00000002L
#	define VIF_BUF2_BASE_ADRS_MASK		0xfffffff0L
#	define VIF_BUF2_1ST_LINE_LSBS_MASK	0x48000000L
#define	OV0_VID_BUF3_BASE_ADRS			0x044C
#	define VIF_BUF3_PITCH_SEL		0x00000001L
#	define VIF_BUF3_TILE_ADRS		0x00000002L
#	define VIF_BUF3_BASE_ADRS_MASK		0xfffffff0L
#	define VIF_BUF3_1ST_LINE_LSBS_MASK	0x48000000L
#define	OV0_VID_BUF4_BASE_ADRS			0x0450
#	define VIF_BUF4_PITCH_SEL		0x00000001L
#	define VIF_BUF4_TILE_ADRS		0x00000002L
#	define VIF_BUF4_BASE_ADRS_MASK		0xfffffff0L
#	define VIF_BUF4_1ST_LINE_LSBS_MASK	0x48000000L
#define	OV0_VID_BUF5_BASE_ADRS			0x0454
#	define VIF_BUF5_PITCH_SEL		0x00000001L
#	define VIF_BUF5_TILE_ADRS		0x00000002L
#	define VIF_BUF5_BASE_ADRS_MASK		0xfffffff0L
#	define VIF_BUF5_1ST_LINE_LSBS_MASK	0x48000000L
#define	OV0_VID_BUF_PITCH0_VALUE		0x0460
#define	OV0_VID_BUF_PITCH1_VALUE		0x0464
#define	OV0_AUTO_FLIP_CNTL			0x0470
#	define OV0_AUTO_FLIP_CNTL_SOFT_BUF_NUM		0x00000007
#	define OV0_AUTO_FLIP_CNTL_SOFT_REPEAT_FIELD	0x00000008
#	define OV0_AUTO_FLIP_CNTL_SOFT_BUF_ODD		0x00000010
#	define OV0_AUTO_FLIP_CNTL_IGNORE_REPEAT_FIELD	0x00000020
#	define OV0_AUTO_FLIP_CNTL_SOFT_EOF_TOGGLE	0x00000040
#	define OV0_AUTO_FLIP_CNTL_VID_PORT_SELECT	0x00000300
#	define OV0_AUTO_FLIP_CNTL_P1_FIRST_LINE_EVEN	0x00010000
#	define OV0_AUTO_FLIP_CNTL_SHIFT_EVEN_DOWN	0x00040000
#	define OV0_AUTO_FLIP_CNTL_SHIFT_ODD_DOWN	0x00080000
#	define OV0_AUTO_FLIP_CNTL_FIELD_POL_SOURCE	0x00800000
#define	OV0_DEINTERLACE_PATTERN			0x0474
#define	OV0_SUBMIT_HISTORY			0x0478
#define	OV0_H_INC				0x0480
#define	OV0_STEP_BY				0x0484
#define	OV0_P1_H_ACCUM_INIT			0x0488
#define	OV0_P23_H_ACCUM_INIT			0x048C
#define	OV0_P1_X_START_END			0x0494
#define	OV0_P2_X_START_END			0x0498
#define	OV0_P3_X_START_END			0x049C
#define	OV0_FILTER_CNTL				0x04A0
#	define FILTER_PROGRAMMABLE_COEF		0x00000000
#	define FILTER_HARD_SCALE_HORZ_Y		0x00000001
#	define FILTER_HARD_SCALE_HORZ_UV	0x00000002
#	define FILTER_HARD_SCALE_VERT_Y		0x00000004
#	define FILTER_HARD_SCALE_VERT_UV	0x00000008
#	define FILTER_HARDCODED_COEF		0x0000000F
#	define FILTER_COEF_MASK			0x0000000F
#define	OV0_FOUR_TAP_COEF_0			0x04B0
#	define OV0_FOUR_TAP_PHASE_0_TAP_0	0x0000000F
#	define OV0_FOUR_TAP_PHASE_0_TAP_1	0x00007F00
#	define OV0_FOUR_TAP_PHASE_0_TAP_2	0x007F0000
#	define OV0_FOUR_TAP_PHASE_0_TAP_3	0x0F000000
#define	OV0_FOUR_TAP_COEF_1			0x04B4
#	define OV0_FOUR_TAP_PHASE_1_5_TAP_0	0x0000000F
#	define OV0_FOUR_TAP_PHASE_1_5_TAP_1	0x00007F00
#	define OV0_FOUR_TAP_PHASE_1_5_TAP_2	0x007F0000
#	define OV0_FOUR_TAP_PHASE_1_5_TAP_3	0x0F000000
#define	OV0_FOUR_TAP_COEF_2			0x04B8
#	define OV0_FOUR_TAP_PHASE_2_6_TAP_0	0x0000000F
#	define OV0_FOUR_TAP_PHASE_2_6_TAP_1	0x00007F00
#	define OV0_FOUR_TAP_PHASE_2_6_TAP_2	0x007F0000
#	define OV0_FOUR_TAP_PHASE_2_6_TAP_3	0x0F000000
#define	OV0_FOUR_TAP_COEF_3			0x04BC
#	define OV0_FOUR_TAP_PHASE_3_7_TAP_0	0x0000000F
#	define OV0_FOUR_TAP_PHASE_3_7_TAP_1	0x00007F00
#	define OV0_FOUR_TAP_PHASE_3_7_TAP_2	0x007F0000
#	define OV0_FOUR_TAP_PHASE_3_7_TAP_3	0x0F000000
#define	OV0_FOUR_TAP_COEF_4			0x04C0
#	define OV0_FOUR_TAP_PHASE_4_TAP_0	0x0000000F
#	define OV0_FOUR_TAP_PHASE_4_TAP_1	0x00007F00
#	define OV0_FOUR_TAP_PHASE_4_TAP_2	0x007F0000
#	define OV0_FOUR_TAP_PHASE_4_TAP_3	0x0F000000
#define	OV0_FLAG_CNTL				0x04DC
#define	OV0_SLICE_CNTL				0x04E0
#	define SLICE_CNTL_DISABLE		0x40000000
#define	OV0_VID_KEY_CLR_LOW			0x04E4
#define	OV0_VID_KEY_CLR_HIGH			0x04E8
#define	OV0_GRPH_KEY_CLR_LOW			0x04EC
#define	OV0_GRPH_KEY_CLR_HIGH			0x04F0
#define	OV0_KEY_CNTL				0x04F4
#	define VIDEO_KEY_FN_MASK		0x00000003L
#	define VIDEO_KEY_FN_FALSE		0x00000000L
#	define VIDEO_KEY_FN_TRUE		0x00000001L
#	define VIDEO_KEY_FN_EQ			0x00000002L
#	define VIDEO_KEY_FN_NE			0x00000003L
#	define GRAPHIC_KEY_FN_MASK		0x00000030L
#	define GRAPHIC_KEY_FN_FALSE		0x00000000L
#	define GRAPHIC_KEY_FN_TRUE		0x00000010L
#	define GRAPHIC_KEY_FN_EQ		0x00000020L
#	define GRAPHIC_KEY_FN_NE		0x00000030L
#	define CMP_MIX_MASK			0x00000100L
#	define CMP_MIX_OR			0x00000000L
#	define CMP_MIX_AND			0x00000100L
#define	OV0_TEST				0x04F8
#	define OV0_SCALER_Y2R_DISABLE		0x00000001L
#	define OV0_SUBPIC_ONLY			0x00000008L
#	define OV0_EXTENSE			0x00000010L
#	define OV0_SWAP_UV			0x00000020L
#define OV0_COL_CONV				0x04FC
#	define OV0_CB_TO_B			0x0000007FL
#	define OV0_CB_TO_G			0x0000FF00L
#	define OV0_CR_TO_G			0x00FF0000L
#	define OV0_CR_TO_R			0x7F000000L
#	define OV0_NEW_COL_CONV			0x80000000L
#define	OV0_LIN_TRANS_A				0x0D20
#define	OV0_LIN_TRANS_B				0x0D24
#define	OV0_LIN_TRANS_C				0x0D28
#define	OV0_LIN_TRANS_D				0x0D2C
#define	OV0_LIN_TRANS_E				0x0D30
#define	OV0_LIN_TRANS_F				0x0D34

#define	DISP_MERGE_CNTL				0x0D60
#define	DISP_TEST_DEBUG_CNTL			0x0D10
#define	DISP_BASE_ADDR				0x023C

#define	VID_BUFFER_CONTROL			0x0900

#define FCP_CNTL				0x0910
#	define FCP_CNTL__PCICLK			0
#	define FCP_CNTL__PCLK			1
#	define FCP_CNTL__PCLKb			2
#	define FCP_CNTL__HREF			3
#	define FCP_CNTL__GND			4
#	define FCP_CNTL__HREFb			5

#define	CAP0_TRIG_CNTL				0x0950

#endif
