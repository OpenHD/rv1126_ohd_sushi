//
// Created by consti10 on 25.05.21.
//

#ifndef MEDIASEVER_IMX415_REGS_ROCKCHIP_H
#define MEDIASEVER_IMX415_REGS_ROCKCHIP_H

// just like nvidia does, move the register stuff into a .h file to make the rest more easy to read
// these are all definitions that were in the original rockchip driver

// clarification on notation (s):
// registers for this sensor can have different size(s). For example, a sensor register can spawn over 8 bit or 32 bit.
// Therefore, the following notation is used:
// "Register name" (only) 8 bit register (8 bit)
// "Register name" _L and _H : low and high bit regs for this register (16 bit)
// "Register name" _L , _M and _H : low, middle and high bit regs for this register (32 bit)

// TODO: One could probably do this more nicely, for example storing the lowest address & size in bytes instead of doing all this _L,_M and _H crap.
// this would make it all less error-prone, too.
// but this is the way rockchip did it in the original driver, so I went with it.

// so we have the "regval struct"
#include "imx415_write_regs.h"

#define IMX415_VMAX_L 0x3024
#define IMX415_VMAX_M 0x3025
#define IMX415_VMAX_H 0x3026
#define IMX415_HMAX_L 0x3028
#define IMX415_HMAX_H 0x3029
//
#define IMX415_SHR0_L 0x3050
#define IMX415_SHR0_M 0x3051
#define IMX415_SHR0_H 0x3052

#define IMX415_XMSTA 0x3002


// each of them is 2 byte though
#define IMX415_TCLKPOST 0x4018
#define IMX415_TCLKPREPARE 0x401A
#define IMX415_TCLKTRAIL 0x401C
#define IMX415_TCLKZERO_L 0x401E
#define IMX415_TCLKZERO_H 0x401F
#define IMX415_THSPREPARE 0x4020
#define IMX415_THSZERO 0x4022
// This is one of the few where we need to write 2 bytes
#define IMX415_THSZERO_L 0x4022
#define IMX415_THSZERO_H 0x4023
#define IMX415_THSTRAIL 0x4024
#define IMX415_THSEXIT 0x4026
#define IMX415_TLPX 0x4028

// regarding cropping:
#define IMX415_WINMODE 0x301C
// 2 byte
#define IMX415_PIX_HST_L 0x3040
#define IMX415_PIX_HST_H 0x3041
#define IMX415_PIX_HWIDTH_L 0x3042
#define IMX415_PIX_HWIDTH_H 0x3043
#define IMX415_PIX_VST_L 0x3044
#define IMX415_PIX_VST_H 0x3045
#define IMX415_PIX_VWIDTH_L 0x3046
#define IMX415_PIX_VWIDTH_H 0x3047

// regarding "INCK"
#define IMX415_BCWAIT_TIME_L 0x3008
#define IMX415_BCWAIT_TIME_H 0x3009
#define IMX415_CPWAIT_TIME_L 0x300A
#define IMX415_CPWAIT_TIME_H 0x300B
#define IMX415_SYS_MODE 0x3033 // weird is there something wrong in the spec sheet ? also referred to as 0x034
#define IMX415_INCKSEL1 0x3115
#define IMX415_INCKSEL2 0x3116
#define IMX415_INCKSEL3_L 0x3118
#define IMX415_INCKSEL3_H 0x3119
#define IMX415_INCKSEL4_L 0x311A
#define IMX415_INCKSEL4_H 0x311B
#define IMX415_INCKSEL5 0x311E
#define IMX415_TXCLKESC_FREQ_L 0x4004
#define IMX415_TXCLKESC_FREQ_H 0x4005
#define IMX415_INCKSEL6 0x400C
#define IMX415_INCKSEL7 0x4074

//other stuff - pretty sure sensor is in "master" mode
#define IMX415_XMASTER 0x3003
#define IMX415_XMSTA 0x3002

// rest that was not yet defined
#define IMX415_HADD 0x3020
#define IMX415_VADD 0x3021
#define IMX415_ADDMODE 0x3022
#define IMX415_ADBIT 0x3031
#define IMX415_MDBIT 0x3032
#define IMX415_DIG_CLP_VSTAET 0x30D9
#define IMX415_DIG_VLP_VNUM 0x30DA

// bit conversions
// low and high (2x8 bit) forming 16bit number
// low == smaller reg number of both
#define IMX415_FETCH_16BIT_H(VAL)	(((VAL) >> 8) & 0x07)
#define IMX415_FETCH_16BIT_L(VAL)	((VAL) & 0xFF)

#define IMX415_FETCH_24BIT_H(VAL)		(((VAL) >> 16) & 0x0F)
#define IMX415_FETCH_24BIT_M(VAL)		(((VAL) >> 8) & 0xFF)
#define IMX415_FETCH_24BIT_L(VAL)		((VAL) & 0xFF)

#define IMX415_EFFECTIVE_PIXEL_W 3864
#define IMX415_EFFECTIVE_PIXEL_H 2192
#define IMX415_RECOMMENDED_RECORDING_W 3840
#define IMX415_RECOMMENDED_RECORDING_H 2160



//For 891Mbps & 37.125 | For 1485 & 37.125
// 07Fh                | 07Fh
// 05Bh                | 05Bh
// 5h                  | 8h               SYS_MODE
// 00h                 | 00h
// 24h                 | 24h
// 0C0h                | 0A0              INCKSEL3
// 0E0h                | 0E0h
// 24h                 | 24h
// 0948h               | 0948h
// 0h                  | 1h               INCKSEL6
// 1h                  | 0h               INCKSEL7


//3864-1920 = 1944 | 1944/2 = 972
//2192-1080 = 1112 | 1112/2 = 556

// 4k sensor cropped down to 1080p
static __maybe_unused const struct regval imx415_linear_10bit_3864x2192_891M_regs_cropping[] = {
        {IMX415_VMAX_L, 0xCA}, //maybe same
        {IMX415_VMAX_M, 0x08}, //maybe same
        {IMX415_HMAX_L,IMX415_FETCH_16BIT_L(0x44C)},
        {IMX415_HMAX_H,IMX415_FETCH_16BIT_H(0x44C)},
        {0x302C, 0x00}, //cannot find
        {0x302D, 0x00}, //cannot find
        {IMX415_SYS_MODE, 0x05},
        {IMX415_SHR0_L, 0x08},
        {IMX415_SHR0_M, 0x00},
        {0x3054, 0x19}, //cannot find in spec, but is IMX415_SF1_EXPO_REG_L in rockchip
        {0x3058, 0x3E}, //cannot find in spec, but is IMX415_SF2_EXPO_REG_L in rockchip
        {0x3060, 0x25}, //cannot find in spec, but is IMX415_RHS1_REG_L     in rockchip
        {0x3064, 0x4a}, //maybe same          ,but is IMX415_RHS2_REG_L     in rockchip
        {0x30CF, 0x00}, //cannot find
        {IMX415_INCKSEL3_L, 0xC0},
        {0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)
        {IMX415_INCKSEL6, 0x00},

        {IMX415_TCLKPOST, 0x7F},   //here applies the 0x00xx workaround
        {IMX415_TCLKPREPARE, 0x37},//here applies the 0x00xx workaround
        {IMX415_TCLKTRAIL, 0x37},  //here applies the 0x00xx workaround
        {IMX415_TCLKZERO_L, 0xF7}, //why the heck is this the only one of all where the higher bits need to be set to 0 argh
        {IMX415_TCLKZERO_H, 0x00}, // -- " --
        {IMX415_THSPREPARE, 0x3F}, //here applies the 0x00xx workaround
        {IMX415_THSZERO, 0x6F},    //here applies the 0x00xx workaround
        {IMX415_THSTRAIL, 0x3F},   //here applies the 0x00xx workaround
        {IMX415_THSEXIT, 0x5F},    //here applies the 0x00xx workaround
        {IMX415_TLPX, 0x2F},       //here applies the 0x00xx workaround
        {IMX415_INCKSEL7, 0x01},
        // added for testing Consti10:
        {IMX415_WINMODE,0x04}, //WINMODE //0: All-pixel mode, Horizontal/Vertical 2/2-line binning 4: Window cropping mode
        {IMX415_PIX_HST_L,IMX415_FETCH_16BIT_L(972)}, //PIX_HST Effective pixel Start position (Horizontal direction) | Default in spec: 0x000
        {IMX415_PIX_HST_H,IMX415_FETCH_16BIT_H(972)}, //""
        {IMX415_PIX_HWIDTH_L,IMX415_FETCH_16BIT_L(1920)}, //PIX_HWIDTH Effective pixel Cropping width (Horizontal direction) | Default in spec: 0x0F18==3864
        {IMX415_PIX_HWIDTH_H,IMX415_FETCH_16BIT_H(1920)},  //""
        {IMX415_PIX_VST_L,IMX415_FETCH_16BIT_L(556*2)}, //PIX_VST Effective pixel Star position (Vertical direction) Designated in V units ( Line×2 ) | Default in spec: 0x000
        {IMX415_PIX_VST_H,IMX415_FETCH_16BIT_H(556*2)}, //""
        {IMX415_PIX_VWIDTH_L,IMX415_FETCH_16BIT_L(1080*2)}, //PIX_VWIDTH Effective pixel Cropping width (Vertical direction) Designated in V units ( Line×2 ) | Default in spec: 0x1120==4384
        {IMX415_PIX_VWIDTH_H,IMX415_FETCH_16BIT_H(1080*2)}, //""
        /*{IMX415_WINMODE,0x04}, //WINMODE //0: All-pixel mode, Horizontal/Vertical 2/2-line binning 4: Window cropping mode
        {IMX415_PIX_HST_L,IMX415_FETCH_16BIT_L(0)}, //PIX_HST Effective pixel Start position (Horizontal direction) | Default in spec: 0x000
        {IMX415_PIX_HST_H,IMX415_FETCH_16BIT_H(0)}, //""
        {IMX415_PIX_HWIDTH_L,IMX415_FETCH_16BIT_L(3864)}, //PIX_HWIDTH Effective pixel Cropping width (Horizontal direction) | Default in spec: 0x0F18==3864
        {IMX415_PIX_HWIDTH_H,IMX415_FETCH_16BIT_H(3864)},  //""
        {IMX415_PIX_VST_L,IMX415_FETCH_16BIT_L(0)}, //PIX_VST Effective pixel Star position (Vertical direction) Designated in V units ( Line×2 ) | Default in spec: 0x000
        {IMX415_PIX_VST_H,IMX415_FETCH_16BIT_H(0)}, //""
        {IMX415_PIX_VWIDTH_L,IMX415_FETCH_16BIT_L(4384)}, //PIX_VWIDTH Effective pixel Cropping width (Vertical direction) Designated in V units ( Line×2 ) | Default in spec: 0x1120==4384
        {IMX415_PIX_VWIDTH_H,IMX415_FETCH_16BIT_H(4384)}, //"" */
        // added for testing Consti10 end

        {REG_NULL, 0x00},
};

//             | For 891Mbps & 37.125 | For 1485 & 37.125          | For 1782 & 37.125
//BCWAIT_TIME  | 07Fh                 | 07Fh                       | 07Fh
//CPWAIT_TIME  | 05Bh                 | 05Bh                       | 05Bh
//SYS_MODE     | 5h                   | 8h               SYS_MODE  | 4h
//INCKSEL1     | 00h                  | 00h                        | 00h
//INCKSEL2     | 24h                  | 24h                        | 24h
//INCKSEL3     | 0C0h                 | 0A0              INCKSEL3  | 0C0h
//INCKSEL4     | 0E0h                 | 0E0h                       | 0E0h
//INCKSEL5     | 24h                  | 24h                        | 24h
//TXCLKESC_FREQ| 0948h                | 0948h                      | 0948h
//INCKSEL6     | 0h                   | 1h               INCKSEL6  | 1h
//INCKSEL7     | 1h                   | 0h               INCKSEL7  | 0h

// 4k but in 1782Mbps mode (max 60 fps) - YEAH, works
static __maybe_unused const struct regval imx415_linear_10bit_3864x2192_1782_regs[] = {
        {IMX415_VMAX_L, 0xCA}, //maybe same
        {IMX415_VMAX_M, 0x08}, //maybe same
        {IMX415_HMAX_L,IMX415_FETCH_16BIT_L(0x226)},
        {IMX415_HMAX_H,IMX415_FETCH_16BIT_H(0x226)},
        {0x302C, 0x00}, //cannot find
        {0x302D, 0x00}, //cannot find
        {IMX415_SYS_MODE, 0x04},
        {IMX415_SHR0_L, 0x08},
        {IMX415_SHR0_M, 0x00},
        {0x3054, 0x19}, //cannot find in spec, but is IMX415_SF1_EXPO_REG_L in rockchip
        {0x3058, 0x3E}, //cannot find in spec, but is IMX415_SF2_EXPO_REG_L in rockchip
        {0x3060, 0x25}, //cannot find in spec, but is IMX415_RHS1_REG_L     in rockchip
        {0x3064, 0x4a}, //maybe same          ,but is IMX415_RHS2_REG_L     in rockchip
        {0x30CF, 0x00}, //cannot find
        {IMX415_INCKSEL3_L, 0xC0},
        {0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)
        {IMX415_INCKSEL6, 0x01}, //changed

        {IMX415_TCLKPOST, 0xB7},   //here applies the 0x00xx workaround
        {IMX415_TCLKPREPARE, 0x67},//here applies the 0x00xx workaround
        {IMX415_TCLKTRAIL, 0x6F},  //here applies the 0x00xx workaround
        {IMX415_TCLKZERO_L, 0xDF}, //why the heck is this the only one of all where the higher bits need to be set to 0 argh
        {IMX415_TCLKZERO_H, 0x01}, // -- " --
        {IMX415_THSPREPARE, 0x6F}, //here applies the 0x00xx workaround
        {IMX415_THSZERO, 0xCF},    //here applies the 0x00xx workaround
        {IMX415_THSTRAIL, 0x6F},   //here applies the 0x00xx workaround
        {IMX415_THSEXIT, 0xB7},    //here applies the 0x00xx workaround
        {IMX415_TLPX, 0x5F},       //here applies the 0x00xx workaround
        {IMX415_INCKSEL7, 0x00}, //changed
        // added for testing Consti10:

        // added for testing Consti10 end

        {REG_NULL, 0x00},
};

// 0xCA

// 4k but in 2376 mode - yeah works. Allows up to 4k90fps
static __maybe_unused const struct regval imx415_linear_10bit_3864x2192_2376_regs[] = {
        {IMX415_VMAX_L, IMX415_FETCH_24BIT_L(0x08ca)}, //maybe same
        {IMX415_VMAX_M, IMX415_FETCH_24BIT_M(0x08ca)}, //maybe same
        {IMX415_HMAX_L,IMX415_FETCH_16BIT_L(0x16E)},
        {IMX415_HMAX_H,IMX415_FETCH_16BIT_H(0x16E)},
        {0x302C, 0x00}, //cannot find
        {0x302D, 0x00}, //cannot find
        {IMX415_SYS_MODE, 0x0},
        {IMX415_SHR0_L, 0x08}, //?
        {IMX415_SHR0_M, 0x00}, //?
        {0x3054, 0x19}, //cannot find in spec, but is IMX415_SF1_EXPO_REG_L in rockchip
        {0x3058, 0x3E}, //cannot find in spec, but is IMX415_SF2_EXPO_REG_L in rockchip
        {0x3060, 0x25}, //cannot find in spec, but is IMX415_RHS1_REG_L     in rockchip
        {0x3064, 0x4a}, //maybe same          ,but is IMX415_RHS2_REG_L     in rockchip
        {0x30CF, 0x00}, //cannot find
        //{IMX415_INCKSEL3_L, 0xC0},
        {IMX415_INCKSEL3_L, IMX415_FETCH_16BIT_L(0x100)},
        {IMX415_INCKSEL3_H, IMX415_FETCH_16BIT_H(0x100)},
        {0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)
        {IMX415_INCKSEL6, 0x01}, //changed

        {IMX415_TCLKPOST, 0xE7},   //here applies the 0x00xx workaround
        {IMX415_TCLKPREPARE, 0x8F},//here applies the 0x00xx workaround
        {IMX415_TCLKTRAIL, 0x8F},  //here applies the 0x00xx workaround
        {IMX415_TCLKZERO_L, IMX415_FETCH_16BIT_L(0x027F)}, //why the heck is this the only one of all where the higher bits need to be set to 0 argh
        {IMX415_TCLKZERO_H, IMX415_FETCH_16BIT_H(0x027F)}, // -- " --
        {IMX415_THSPREPARE, 0x97}, //here applies the 0x00xx workaround
        //{IMX415_THSZERO, 0xCF},    //here applies the 0x00xx workaround
        // this is the only one where we need to write 2 bytes for thszero
        {IMX415_THSZERO_L,IMX415_FETCH_16BIT_L(0x010F)},
        {IMX415_THSZERO_H,IMX415_FETCH_16BIT_H(0x010F)},
        {IMX415_THSTRAIL, 0x97},   //here applies the 0x00xx workaround
        {IMX415_THSEXIT, 0xF7},    //here applies the 0x00xx workaround
        {IMX415_TLPX, 0x7F},       //here applies the 0x00xx workaround
        {IMX415_INCKSEL7, 0x00}, //changed
        // added for testing Consti10:

        // added for testing Consti10 end

        {REG_NULL, 0x00},
};

// 4k with 2x2 binning == 1080p
// works, up to 90 fps
static __maybe_unused const struct regval imx415_linear_10bit_3864x2192_2376_regs_binning[] = {
        {IMX415_VMAX_L, IMX415_FETCH_24BIT_L(0x08ca)}, //maybe same
        {IMX415_VMAX_M, IMX415_FETCH_24BIT_M(0x08ca)}, //maybe same
        {IMX415_HMAX_L,IMX415_FETCH_16BIT_L(0x16E)}, //0x16E / 2 =  366 / 2 = 183 | 300==0x12C | 244
        {IMX415_HMAX_H,IMX415_FETCH_16BIT_H(0x16E)},
        // hm just write different vmax
        //{IMX415_HMAX_L,IMX415_FETCH_16BIT_L(0xF4)},
        //{IMX415_HMAX_H,IMX415_FETCH_16BIT_H(0xF4)},
        //
        {0x302C, 0x00}, //cannot find
        {0x302D, 0x00}, //cannot find
        {IMX415_SYS_MODE, 0x0},
        {IMX415_SHR0_L, 0x08}, //?
        {IMX415_SHR0_M, 0x00}, //?
        {0x3054, 0x19}, //cannot find in spec, but is IMX415_SF1_EXPO_REG_L in rockchip
        {0x3058, 0x3E}, //cannot find in spec, but is IMX415_SF2_EXPO_REG_L in rockchip
        {0x3060, 0x25}, //cannot find in spec, but is IMX415_RHS1_REG_L     in rockchip
        {0x3064, 0x4a}, //maybe same          ,but is IMX415_RHS2_REG_L     in rockchip
        {0x30CF, 0x00}, //cannot find
        {IMX415_INCKSEL3_L, IMX415_FETCH_16BIT_L(0x100)},
        {IMX415_INCKSEL3_H, IMX415_FETCH_16BIT_H(0x100)},
        {0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)
        {IMX415_INCKSEL6, 0x01}, //changed

        {IMX415_TCLKPOST, 0xE7},   //here applies the 0x00xx workaround
        {IMX415_TCLKPREPARE, 0x8F},//here applies the 0x00xx workaround
        {IMX415_TCLKTRAIL, 0x8F},  //here applies the 0x00xx workaround
        {IMX415_TCLKZERO_L, IMX415_FETCH_16BIT_L(0x027F)}, //why the heck is this the only one of all where the higher bits need to be set to 0 argh
        {IMX415_TCLKZERO_H, IMX415_FETCH_16BIT_H(0x027F)}, // -- " --
        {IMX415_THSPREPARE, 0x97}, //here applies the 0x00xx workaround
        // this is the only one where we need to write 2 bytes for thszero
        {IMX415_THSZERO_L,IMX415_FETCH_16BIT_L(0x010F)},
        {IMX415_THSZERO_H,IMX415_FETCH_16BIT_H(0x010F)},
        {IMX415_THSTRAIL, 0x97},   //here applies the 0x00xx workaround
        {IMX415_THSEXIT, 0xF7},    //here applies the 0x00xx workaround
        {IMX415_TLPX, 0x7F},       //here applies the 0x00xx workaround
        {IMX415_INCKSEL7, 0x00}, //changed
        // added for testing Consti10:
        {0x301C,0x00}, //WINMODE //0: All-pixel mode, Horizontal/Vertical 2/2-line binning 4: Window cropping mode
        {0x3020,0x01}, //HADD //0h: All-pixel mode 1h: Horizontal 2 binning
        {0x3021,0x01}, //VADD //0h: All-pixel mode 1h: Vertical 2 binning
        {0x3022,0x01}, //ADDMODE //0h: All-pixel mode 1h: Horizontal/Vertical 2/2-line binning
        //
        // to resolve:
        {0x3031,0x00}, //ADBIT //set by global to 0 , 0=10bit 1=12bit
        {0x3032,0x00}, //MDBIT //set by global to 0
        //
        {0x30D9,0x02}, //DIG_CLP_VSTAET ? 0x02=binning 0x06=All-pixel scan mode , default 0x06
        {0x30DA,0x01}, //DIG_CLP_VNUM ? 0x01=binning 0x02=all-pixel scan mode, default 0x02
        // added for testing Consti10 end

        {REG_NULL, 0x00},
};

// crop to 1080p
// doesn't work yet, but at least gives something resembling an image
static __maybe_unused const struct regval imx415_linear_10bit_3864x2192_2376_regs_cropping[] = {
        {IMX415_VMAX_L, IMX415_FETCH_24BIT_L(0x08ca)}, //maybe same
        {IMX415_VMAX_M, IMX415_FETCH_24BIT_M(0x08ca)}, //maybe same
        {IMX415_HMAX_L,IMX415_FETCH_16BIT_L(0x16E)}, //0x16E / 2 =  366 / 2 = 183 | 300==0x12C | 244
        {IMX415_HMAX_H,IMX415_FETCH_16BIT_H(0x16E)},
        //
        {0x302C, 0x00}, //cannot find
        {0x302D, 0x00}, //cannot find
        {IMX415_SYS_MODE, 0x0},
        {IMX415_SHR0_L, 0x08}, //?
        {IMX415_SHR0_M, 0x00}, //?
        {0x3054, 0x19}, //cannot find in spec, but is IMX415_SF1_EXPO_REG_L in rockchip
        {0x3058, 0x3E}, //cannot find in spec, but is IMX415_SF2_EXPO_REG_L in rockchip
        {0x3060, 0x25}, //cannot find in spec, but is IMX415_RHS1_REG_L     in rockchip
        {0x3064, 0x4a}, //maybe same          ,but is IMX415_RHS2_REG_L     in rockchip
        {0x30CF, 0x00}, //cannot find
        //{IMX415_INCKSEL3_L, 0xC0},
        {IMX415_INCKSEL3_L, IMX415_FETCH_16BIT_L(0x100)},
        {IMX415_INCKSEL3_H, IMX415_FETCH_16BIT_H(0x100)},
        {0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)
        {IMX415_INCKSEL6, 0x01}, //changed

        {IMX415_TCLKPOST, 0xE7},   //here applies the 0x00xx workaround
        {IMX415_TCLKPREPARE, 0x8F},//here applies the 0x00xx workaround
        {IMX415_TCLKTRAIL, 0x8F},  //here applies the 0x00xx workaround
        {IMX415_TCLKZERO_L, IMX415_FETCH_16BIT_L(0x027F)}, //why the heck is this the only one of all where the higher bits need to be set to 0 argh
        {IMX415_TCLKZERO_H, IMX415_FETCH_16BIT_H(0x027F)}, // -- " --
        {IMX415_THSPREPARE, 0x97}, //here applies the 0x00xx workaround
        // this is the only one where we need to write 2 bytes for thszero
        {IMX415_THSZERO_L,IMX415_FETCH_16BIT_L(0x010F)},
        {IMX415_THSZERO_H,IMX415_FETCH_16BIT_H(0x010F)},
        {IMX415_THSTRAIL, 0x97},   //here applies the 0x00xx workaround
        {IMX415_THSEXIT, 0xF7},    //here applies the 0x00xx workaround
        {IMX415_TLPX, 0x7F},       //here applies the 0x00xx workaround
        {IMX415_INCKSEL7, 0x00}, //changed
        // added for testing Consti10:
        // 1920 x 1080
        // H: 3840-1920= 1920 | 1920/2=960
        // V:2160-1080 = 1080 | 1080/2=540
        // In horizontal direction:
        // In vertical direction:
        // horizontal offset = 12+960 | HST = same
        // vertical offset = 12+540   | VST = x*2
        // horizontal length of selected area= 1920 | but output should be 12+1920+12 | HWIDTH=same
        // vertical length of selected area = 1080+13+3                               | VWIDTH = x*2
        /*{IMX415_WINMODE,0x04}, //WINMODE //0: All-pixel mode, Horizontal/Vertical 2/2-line binning 4: Window cropping mode
        {IMX415_PIX_HST_L,IMX415_FETCH_16BIT_L(12+960)}, //PIX_HST Effective pixel Start position (Horizontal direction) | Default in spec: 0x000
        {IMX415_PIX_HST_H,IMX415_FETCH_16BIT_H(12+960)}, //""
        {IMX415_PIX_HWIDTH_L,IMX415_FETCH_16BIT_L(1920)}, //PIX_HWIDTH Effective pixel Cropping width (Horizontal direction) | Default in spec: 0x0F18==3864
        {IMX415_PIX_HWIDTH_H,IMX415_FETCH_16BIT_H(1920)},  //""
        {IMX415_PIX_VST_L,IMX415_FETCH_16BIT_L((12+540)*2)}, //PIX_VST Effective pixel Star position (Vertical direction) Designated in V units ( Line×2 ) | Default in spec: 0x000
        {IMX415_PIX_VST_H,IMX415_FETCH_16BIT_H((12+540)*2)}, //""
        {IMX415_PIX_VWIDTH_L,IMX415_FETCH_16BIT_L((13+1080+3)*2)}, //PIX_VWIDTH Effective pixel Cropping width (Vertical direction) Designated in V units ( Line×2 ) | Default in spec: 0x1120==4384
        {IMX415_PIX_VWIDTH_H,IMX415_FETCH_16BIT_H((13+1080+3)*2)},*/
        {IMX415_WINMODE,0x04}, //WINMODE //0: All-pixel mode, Horizontal/Vertical 2/2-line binning 4: Window cropping mode
        {IMX415_PIX_HST_L,IMX415_FETCH_16BIT_L(1920)}, //PIX_HST Effective pixel Start position (Horizontal direction) | Default in spec: 0x000
        {IMX415_PIX_HST_H,IMX415_FETCH_16BIT_H(1920)}, //""
        {IMX415_PIX_HWIDTH_L,IMX415_FETCH_16BIT_L(12+1920+12)}, //PIX_HWIDTH Effective pixel Cropping width (Horizontal direction) | Default in spec: 0x0F18==3864
        {IMX415_PIX_HWIDTH_H,IMX415_FETCH_16BIT_H(12+1920+12)},  //""
        {IMX415_PIX_VST_L,IMX415_FETCH_16BIT_L((1080)*2)}, //PIX_VST Effective pixel Star position (Vertical direction) Designated in V units ( Line×2 ) | Default in spec: 0x000
        {IMX415_PIX_VST_H,IMX415_FETCH_16BIT_H((1080)*2)}, //""
        {IMX415_PIX_VWIDTH_L,IMX415_FETCH_16BIT_L((1+12+8+1080+8+2+1)*2)}, //PIX_VWIDTH Effective pixel Cropping width (Vertical direction) Designated in V units ( Line×2 ) | Default in spec: 0x1120==4384
        {IMX415_PIX_VWIDTH_H,IMX415_FETCH_16BIT_H((1+12+8+1080+8+2+1)*2)}, //ehm did I mess up something here ?!
        //
        //{0x30D9,0x02}, //DIG_CLP_VSTAET ? 0x02=binning 0x06=All-pixel scan mode , default 0x06
        //{0x30DA,0x01}, //DIG_CLP_VNUM ? 0x01=binning 0x02=all-pixel scan mode, default 0x02
        // added for testing Consti10 end

        {REG_NULL, 0x00},
};

//5216

// the "image" schematics
//    res     | Horizontal   | Vertical
// 3840x2160  | 12+12=24     | 1+12+8+8+2+1= 32
// 1920x1080  | 6+6+12=24    | 1+6+4+4+1+1=  17

// PIX_VWIDTH
//V TTL (1farame line length or VMAX) ≥ (PIX_VWIDTH / 2) + 46
// (4384 /2) +46 = 2238

//Also: In all pixel mode,, 4 lane, 891 mbpp
// VMAX: 0x8CA = 2250
// and rockchip used .vts_def = 0x08ca
// HMAX: 44Ch = 1100
// and rockchip uses .hts_def = 0x044c * IMX415_4LANES * 2

// Whereas in Horizontal/Vertical 2/2-line binning mode, 4 lane:
// VMAX: 0x8CA (SAME!)
// HMAX:44Ch (SAME!)

// Frame rate on Window cropping mode
// Frame rate [frame/s] = 1 / (V TTL × (1H period))
/// 1H period (unit: [s]) : Set "1H period" or more in the table of "Operating mode" before cropping mode.
// 1/2250*14.9*1e-6= 6.62222222222
// 1/4384*(14.9*10^6)=3398.72262774

// no matter if i use -f 90 or -f 120, the lowest vblank value always is
// 1143 | 1080+(1+6+4+4+1+1)=1097

// "The frame rate can be calculated from the pixel clock, image width and height and horizontal and vertical blanking."
// -> that's what I got from rockchip :)
// from rpi:https://www.raspberrypi.org/forums/viewtopic.php?t=300105
// frame rate = pixel_rate / ((width+hblank) * (height+vblank))
// which makes sense ;)
// the only hard thing is the "pixel rate" - still not completely sure about that ;)
// pixel_rate = (u32)link_freq_items[mode->mipi_freq_idx] / mode->bpp * 2 * IMX415_4LANES; <- rockchip
// the weird thing is the "*2" part - for some reason rockchip declares the pixel rate as half of what the imx415 spec sheet would have,
// then multiplies by 2, giving the right one - e.g. 891*2=1782

/*static __maybe_unused int calculateFrameRate(int pixel_rate,int hts_def,int vts_def){
    return pixel_rate*1000 / (hts_def*vts_def);
}
// 1425.6 / (550*2250)
static_assert(calculateFrameRate((891*2 *4 * 2 / 10),0x226 * 4 * 2,0x08ca)==0);
//(891*2 *4 * 2 / 10) / ((550 * 4 * 2)*(2250)) * 1000 * 100 = 14.4 (ms)
// 1000 / ((891*2 *4 * 2 / 10) / ((550 * 4 * 2)*(2250)) * 1000 * 100) = 69.4444444444 (fps)

// 1000 / ((891*2 *4 * 2 / 10) / ((365 * 4 * 2)*(2238)) * 1000 * 100) =

// 458*4*2 = 3664

//X (891000000*2 *4 * 2 / 10) / ((550 * 4 * 2)*(2250))
//X (891000000*2 *4 * 2 / 10) / ((458 * 4 * 2)*(2238))

static __maybe_unused int calculateFrameRate(int pixel_rate,int image_width,int image_height,int hts_def,int vts_def){
    //const int v_blank=hts_def - image_width;
    //const int h_blank=vts_def - image_height;
    //return 0;
    return pixel_rate / (hts_def*vts_def);
}*/

//IMX219 spec sheet:
// Frame_Rate[frame/s]= 1 / (Time_per_line[sec]*(Frame length)
// Time_Per_line[sec] = Line_length_pck[pix] / (2*Pix_clock_Freq[Mhz]

//1H period (unit: [µs]) : Fix 1H time in a mode before cropping and calculate it by the value of "Number of INCK in
//1H" in the table of "Operating Mode" and "List of Operation Modes and Output Rates".

// 1 ÷ (35,6 ÷ 10^6 × 1125)
// where 35.6== 1H period (microseconds) and 1125==V (lines)
// X = 1H period[clock] * INCK[mhz]

//1H period = 365 | INCK = 74.25 Mhz | 1V period=2238
// 1 / ((365 / 74.25) / 10^6*2238)=90,895736164

//0x16E=366 | 0x08CA = 2250
// 1 / ((366 / 37.125) / 10^6 * 2250) = 45,081967213
// 1 / ((366 / 74,25) / 10^6 * 2250) = 90,163934426
// 1 / ((366 / 37.125) / 10^6 * 2286) = 44,372014974
// 1 / ((366 / 37.125) / 10^6 * 1222) = 83,006895441
// 1 / ((244 / 37.125) / 10^6 * 2250) = 67,62295082
// 1 / ((244 / 37.125) / 10^6 * 1222) = 124,510343162
// 1 / ((365 / 74,25) / 10^6 * 2250) = 90,410958904
// 1 / ((365 / 74,25) / 10^6 * 2238) = 90,895736164
// 1 / ((365 / 74,25) / 10^6 * (1097*2)) = 92,718622395
// 1 / ((365 / 74,25) / 10^6 * 1222) = 166,468623187
// 1 / ((365 / 74,25) / 10^6 * 1715) = 118,614960661
// 244==0xF4
// 1222

// Spec sheet 4k 90 fps
// 0x16E = 366
// 366 / 37,125 = 9,858585859
// 366 / 74.25 = 4.92929292929
// 1 / (5.0 / 10^6 * 2250)=88,888888889
// 1 / (4.92929292929/ 10^6 * 2250)=90.1639344263

// 0x4400=17408 0x0898=2200
//(1920+(6+6+12)) ÷ 4 ÷ 2=243

//0x0898
//0x898
//0x208
// e-con systems spec sheet:
//1296x732 is cropped to 1280x720 on the application end to skip the dummy bytes.
//1296x732 is obtained from the sensor through center cropping and binning. 1296x732 will
//be referred to as 1280x720 throughout the document.
//1296x732 * 2 = 2592 * 1464

// def values of the cropping regs:
// PIX_HWIDTH=0x0F18  == 3864 | PIX_VWIDTH=0x1120 == 4384  *0.5=2192
// -> ergo:

// 4k pixels:
// Horizontal: 12+3840+12      = 3864
// Vertical: 1+12+8+2160+8+2+1 = 2192

// 2x2 binning pixels:
// Horizontal: 6+1920+6+12     = 1944
// Vertical: 1+6+4+1080+4+1+1  = 1097

// 1080p cropping:
// HorizontalW: 12+1920+12          = 1944
// VerticalH  : 1+12+8+1080+8+2+1   = 1112
// offset Horizontal: 3864 - 1944 = 1920
// offset Vertical  : 2192 - 1112 = 1080


// pixel_rate = 1188000000 / 10 * 2 * 4 = 950400000

// 1 / ((366 / 74,25) / 10^6 * 6948) = 29,198165293

//from some other sheet:
//HMAX | VMAX | WINPH | WINPV | WINWH | WINWV
//4400d| 520d | 640d  | 300d  | 656d  | 496d

// from imx317:
//Frame rate [frame/s] = (72 x 10^6 ) / {HMAX register value × VMAX register value × (SVR register value + 1) }
// For example, for readout mode 3/4:
// (72*10^6) / (493*4451*(0+1) )=32.8116434
// (72*10^6) / (260*2219*(0+1) )=124.796339
// using this equation for imx415:
// (72*10^6) / (366*2250*(0+1) )= 87.431694 ??
// from imx317:
// The vertical sync signal XVS can be subsampled inside the sensor according to the SVR register. When using SVR =
// 1h, the frame rate becomes half. See “Electronic Shutter Timing” for details.
// also from imx317:
// The exposure start timing can be designated by setting the electronic shutter timing register SHR.
// Note that this setting value unit is 1[HMAX] *1 period regardless of the readout drive mode
// with foot note: 1[HMAX] = Setting value of register HMAX × 72MHz clock

/*
IMX415-AAQR 2/2-line binning & Window cropping 2568x1440 CSI-2_4lane 24MHz AD:10bit Output:12bit 1440Mbps Master Mode 119.988fps Integration Time 8ms Gain:6dB
Ver8.0
*/
// VMAX=0x066C=1644
// HMAX=0x016D=365
// 2568/2= 1284
// 2880/2/2=720
static __maybe_unused const struct regval imx415_linear_10bit_720p_120fps_bin_and_crop_regs[] = {
{0x3008,0x54},  // BCWAIT_TIME[9:0] | regarding INCK, different obviously | same as sheet
{0x300A,0x3B},  // CPWAIT_TIME[9:0] | regarding INCK, different obviously | same as sheet
{0x301C,0x04},  // WINMODE[3:0]     | yes, means cropping
{0x3020,0x01},  // HADD             | 1 means horizontal 2x2 binning
{0x3021,0x01},  // VADD             | 1 means vertical 2x2 binning
{0x3022,0x01},  // ADDMODE[1:0]     | 1 means horizontal/vertical binning
{0x3024,0x6C},  // VMAX[19:0]       | yes, vmax low
{0x3025,0x06},  //                  | yes, vmax high
{0x3028,0x6D},  // HMAX[15:0]       | yes, hmax low
{0x3029,0x01},  //                  | yes, hmax high
{0x3031,0x00},  // ADBIT[1:0]       | same as other, 0 means 10 bit
{0x3033,0x08},  // SYS_MODE[3:0]    | sometimes 0x05, sometimes 0x08 - well, gives the operating mode. 8 means 1140 / 1485 Mbps | same as sheet
{0x3040,0x88},  // PIX_HST[12:0]    | yes, here is the crop -> 0x0288=648
{0x3041,0x02},  //                  | ""
{0x3042,0x08},  // PIX_HWIDTH[12:0] | yes, crop   -> 0x0A08  = 2568
{0x3043,0x0A},  //                  | ""
{0x3044,0xF0},  // PIX_VST[12:0]    | yes, crop   -> 0x02F0  = 752
{0x3045,0x02},  //                  | ""
{0x3046,0x40},  // PIX_VWIDTH[12:0] | yes, crop    -> 0x0B40 = 2880
{0x3047,0x0B},  //
{0x3050,0x42},  // SHR0[19:0]       | yes, depends
{0x3090,0x14},  // GAIN_PCG_0[8:0]  | yes, but only in the main file -> IMX415_LF_GAIN_REG_L
{0x30C1,0x00},  // XVS_DRV[1:0]     | yes, same
{0x30D9,0x02},  // DIG_CLP_VSTART[4:0] | 2== binning
{0x30DA,0x01},  // DIG_CLP_VNUM[1:0]   | 1== binning
{0x3116,0x23},  // INCKSEL2[7:0]       | 0x24 or 0x28 | same as sheet
{0x3118,0xB4},  // INCKSEL3[10:0]      | you know | same as sheet
{0x311A,0xFC},  // INCKSEL4[10:0]      | you know | same as sheet
{0x311E,0x23},  // INCKSEL5[7:0]       | you know | same as sheet
{0x32D4,0x21},  // -                   | same
{0x32EC,0xA1},  // -                   | same
{0x344C,0x2B},  // -                   | no
{0x344D,0x01},  // -                   | no
{0x344E,0xED},  // -                   | no
{0x344F,0x01},  // -                   | no
{0x3450,0xF6},  // -                   | no
{0x3451,0x02},  // -                   | no
{0x3452,0x7F},  // -                   | same
{0x3453,0x03},  // -                   | same
{0x358A,0x04},  // -                   | same
{0x35A1,0x02},  // -                   | same
{0x35EC,0x27},  // -                   | no
{0x35EE,0x8D},  // -                   | no
{0x35F0,0x8D},  // -                   | no
{0x35F2,0x29},  // -                   | no
{0x36BC,0x0C},  // -                   | same
{0x36CC,0x53},  // -                   | same
{0x36CD,0x00},  // -                   | same
{0x36CE,0x3C},  // -                   | same
{0x36D0,0x8C},  // -                   | same
{0x36D1,0x00},  // -                   | same
{0x36D2,0x71},  // -                   | same
{0x36D4,0x3C},  // -                   | same
{0x36D6,0x53},  // -                   | same
{0x36D7,0x00},  // -                   | same
{0x36D8,0x71},  // -                   | same
{0x36DA,0x8C},  // -                   | same
{0x36DB,0x00},  // -                   | same
{0x3701,0x00},  // ADBIT1[7:0]         | same
{0x3720,0x00},  // -                   | no
{0x3724,0x02},  // -                   | same
{0x3726,0x02},  // -                   | same
{0x3732,0x02},  // -                   | same
{0x3734,0x03},  // -                   | same
{0x3736,0x03},  // -                   | same
{0x3742,0x03},  // -                   | same
{0x3862,0xE0},  // -                   | same
{0x38CC,0x30},  // -                   | same
{0x38CD,0x2F},  // -                   | same
{0x395C,0x0C},  // -                   | same
{0x39A4,0x07},  // -                   | no
{0x39A8,0x32},  // -                   | no
{0x39AA,0x32},  // -                   | no
{0x39AC,0x32},  // -                   | no
{0x39AE,0x32},  // -                   | no
{0x39B0,0x32},  // -                   | no
{0x39B2,0x2F},  // -                   | no
{0x39B4,0x2D},  // -                   | no
{0x39B6,0x28},  // -                   | no
{0x39B8,0x30},  // -                   | no
{0x39BA,0x30},  // -                   | no
{0x39BC,0x30},  // -                   | no
{0x39BE,0x30},  // -                   | no
{0x39C0,0x30},  // -                   | no
{0x39C2,0x2E},  // -                   | no
{0x39C4,0x2B},  // -                   | no
{0x39C6,0x25},  // -                   | no
{0x3A42,0xD1},  // -                   | same
{0x3A4C,0x77},  // -                   | same
{0x3AE0,0x02},  // -                   | same
{0x3AEC,0x0C},  // -                   | same
{0x3B00,0x2E},  // -                   | same
{0x3B06,0x29},  // -                   | same
{0x3B98,0x25},  // -                   | same
{0x3B99,0x21},  // -                   | same
{0x3B9B,0x13},  // -                   | same
{0x3B9C,0x13},  // -                   | same
{0x3B9D,0x13},  // -                   | same
{0x3B9E,0x13},  // -                   | same
{0x3BA1,0x00},  // -                   | same
{0x3BA2,0x06},  // -                   | same
{0x3BA3,0x0B},  // -                   | same
{0x3BA4,0x10},  // -                   | same
{0x3BA5,0x14},  // -                   | same
{0x3BA6,0x18},  // -                   | same
{0x3BA7,0x1A},  // -                   | same
{0x3BA8,0x1A},  // -                   | same
{0x3BA9,0x1A},  // -                   | same
{0x3BAC,0xED},  // -                   | same
{0x3BAD,0x01},  // -                   | same
{0x3BAE,0xF6},  // -                   | same
{0x3BAF,0x02},  // -                   | same
{0x3BB0,0xA2}, // -                    | same
{0x3BB1,0x03},  // -                   | same
{0x3BB2,0xE0},  // -                   | same
{0x3BB3,0x03},  // -                   | same
{0x3BB4,0xE0},  // -                   | same
{0x3BB5,0x03},  // -                   | same
{0x3BB6,0xE0},  // -                   | same
{0x3BB7,0x03},  // -                   | same
{0x3BB8,0xE0},  // -                   | same
{0x3BBA,0xE0},  // -                   | same
{0x3BBC,0xDA},  // -                   | same
{0x3BBE,0x88},  // -                   | same
{0x3BC0,0x44},  // -                   | same
{0x3BC2,0x7B},  // -                   | same
{0x3BC4,0xA2},  // -                   | same
{0x3BC8,0xBD},  // -                   | same
{0x3BCA,0xBD},  // -                   | same
{0x4004,0x00},  // TXCLKESC_FREQ[15:0] | is 0x48 in rockchip
{0x4005,0x06},  //                     | is 0x09 in rockchip
{0x4018,0x9F},  // TCLKPOST[15:0]      | | same 1440Mbps
{0x401A,0x57},  // TCLKPREPARE[15:0]   | | same 1440Mbps
{0x401C,0x57},  // TCLKTRAIL[15:0]     | | same 1440Mbps
{0x401E,0x87},  // TCLKZERO[15:0]      | | same 1440Mbps ,but 1 higher bit missing
// did he forget this one ?
{0x401F, IMX415_FETCH_16BIT_H(0x0187)},
// end did he forget
{0x4020,0x5F},  // THSPREPARE[15:0]    | | same 1440Mbps
{0x4022,0xA7},  // THSZERO[15:0]       | | same 1440Mbps
{0x4024,0x5F},  // THSTRAIL[15:0]      | | same 1440Mbps
{0x4026,0x97},  // THSEXIT[15:0]       | | same 1440Mbps
{0x4028,0x4F},  // TLPX[15:0]          | | same 1440Mbps

// stuff I don't know if needed
{0x302C, 0x00}, //cannot find
{0x302D, 0x00}, //cannot find
{IMX415_SHR0_L, 0x08}, //?
{IMX415_SHR0_M, 0x00}, //?
{0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)
// and missing i think:
{IMX415_MDBIT,0x0}, //0h: RAW10, 1h: RAW12
//0x3000,0x00  | is IMX415_REG_CTRL_MODE
//wait_ms(30)
//0x3002,0x00                       | same
// Don't forget
{REG_NULL, 0x00},
};


// Took the ones from INCK=37.125
// 1782Mbps / lane
// gives 60 fps, not 90 for some reason ?!
static __maybe_unused const struct regval imx415_linear_10bit_XxX_1782Mbps_regs_cropping[] = {
        {IMX415_BCWAIT_TIME_L,IMX415_FETCH_16BIT_L(0x07F)},
        {IMX415_BCWAIT_TIME_H,IMX415_FETCH_16BIT_H(0x07F)},
        {IMX415_CPWAIT_TIME_L,IMX415_FETCH_16BIT_L(0x05B)},
        {IMX415_CPWAIT_TIME_H,IMX415_FETCH_16BIT_H(0x05B)},
        {IMX415_SYS_MODE,0x4},
        {IMX415_INCKSEL1,0x00},
        {IMX415_INCKSEL2,0x24},
        {IMX415_INCKSEL3_L,IMX415_FETCH_16BIT_L(0x0C0)},
        {IMX415_INCKSEL3_H,IMX415_FETCH_16BIT_H(0x0C0)},
        {IMX415_INCKSEL4_L,IMX415_FETCH_16BIT_L(0x0E0)},
        {IMX415_INCKSEL4_H,IMX415_FETCH_16BIT_H(0x0E0)},
        {IMX415_INCKSEL5,0x24},
        {IMX415_TXCLKESC_FREQ_L,IMX415_FETCH_16BIT_L(0x0948)},
        {IMX415_TXCLKESC_FREQ_H,IMX415_FETCH_16BIT_H(0x0948)},
        {IMX415_INCKSEL6,0x1},
        {IMX415_INCKSEL7,0x0},
        /// end of inck / mbps
        {IMX415_WINMODE,0x0},
        {IMX415_HADD,0x1},
        {IMX415_VADD,0x1},
        {IMX415_ADDMODE,0x1},
        {IMX415_VMAX_L,IMX415_FETCH_24BIT_L(0x08CA)},
        {IMX415_VMAX_M,IMX415_FETCH_24BIT_M(0x08CA)},
        //{IMX415_VMAX_H,IMX415_FETCH_24BIT_H(0x0)},
        {IMX415_HMAX_L,IMX415_FETCH_16BIT_L(0x16D)}, //0x226
        {IMX415_HMAX_H,IMX415_FETCH_16BIT_H(0x16D)}, //0x226
        //HREVERSE - since set ext
        //VREVERSE -since set ext
        // removed since not in X {IMX415_ADBIT,0x0},
        // removed since not in X {IMX415_MDBIT,0x1},
        {IMX415_ADBIT,0x0}, //AD conversion bits setting. 0: AD 10 bit 1: AD 12 bit ( 11 bit + digital dither )
        {IMX415_MDBIT,0x0}, //0h: RAW10, 1h: RAW12
        // regarding ad/md-bit : both are set to 0 in the "10 bit big default regs array" anyways
        //SYS_MODE - since already above
        {IMX415_DIG_CLP_VSTAET,0x02},
        {IMX415_DIG_VLP_VNUM,0x1},
        // -- refer to --
        //LANEMODE - since we always use 4 lanes
        {IMX415_TCLKPOST,0x00B7},
        {IMX415_TCLKPREPARE,0x0067},
        {IMX415_TCLKTRAIL,0x006F},
        {IMX415_TCLKZERO_L,IMX415_FETCH_16BIT_L(0x01DF)},
        {IMX415_TCLKZERO_H,IMX415_FETCH_16BIT_H(0x01DF)},
        {IMX415_THSPREPARE,0x006F},
        {IMX415_THSZERO,0x00CF},
        {IMX415_THSTRAIL,0x006F},
        {IMX415_THSEXIT,0x00B7},
        {IMX415_TLPX,0x005F},
        // stuff I don't know if needed
        {0x302C, 0x00}, //cannot find - in original rockchip, set to 0 in linear, 1 in HDR
        {0x302D, 0x00}, //cannot find - in original rockchip, set to 0 in linear, 1 or 2 in HDR
        {IMX415_SHR0_L, 0x08}, //?
        {IMX415_SHR0_M, 0x00}, //?
        //{0x3054, 0x19}, //cannot find in spec, but is IMX415_SF1_EXPO_REG_L in rockchip
        //{0x3058, 0x3E}, //cannot find in spec, but is IMX415_SF2_EXPO_REG_L in rockchip
        //{0x3060, 0x25}, //cannot find in spec, but is IMX415_RHS1_REG_L     in rockchip
        //{0x3064, 0x4a}, //maybe same          ,but is IMX415_RHS2_REG_L     in rockchip
        //{0x30CF, 0x00}, //cannot find
        {0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)

        {REG_NULL, 0x00},
};

// VMAX=0x05E3=1507
// HMAX=0x016D=365
// PIX_HWIDTH=0x0A08=2568 | 2568/2=1284
// PIX_VWIDTH=0x0B40=2880 | 2880/2/2=720
// IMX415-AAQR 2/2-line binning & Window cropping 2568x1440 CSI-2_4lane 37.125Mhz AD:10bit (!Output:12bit!) 1485Mbps Master Mode 134.987fps Integration Time 0.999ms Gain:6dB
static __maybe_unused const struct regval imx415_linear_10bit_720p_134fps_bin_and_crop_regs[] = {
        {0x3008,0x7F},  // BCWAIT_TIME[9:0]
        {0x300A,0x5B},  // CPWAIT_TIME[9:0]
        {0x301C,0x04},  // WINMODE[3:0]
        {0x3020,0x01},  // HADD
        {0x3021,0x01},  // VADD
        {0x3022,0x01},  // ADDMODE[1:0]
        {0x3024,0xE3},  // VMAX[19:0]
        {0x3025,0x05},  //
        {0x3028,0x6D},  // HMAX[15:0]
        {0x3029,0x01},  //
        {0x3031,0x00},  // ADBIT[1:0]
        {0x3033,0x08},  // SYS_MODE[3:0]
        {0x3040,0x88},  // PIX_HST[12:0]
        {0x3041,0x02},  //
        {0x3042,0x08},  // PIX_HWIDTH[12:0]
        {0x3043,0x0A},  //
        {0x3044,0xF0},  // PIX_VST[12:0]
        {0x3045,0x02},  //
        {0x3046,0x40},  // PIX_VWIDTH[12:0]
        {0x3047,0x0B},  //
        {0x3050,0x18},  // SHR0[19:0]
        {0x3051,0x05},  //
        {0x3090,0x14},  // GAIN_PCG_0[8:0]
        {0x30C1,0x00},  // XVS_DRV[1:0]
        {0x30D9,0x02},  // DIG_CLP_VSTART[4:0]
        {0x30DA,0x01},  // DIG_CLP_VNUM[1:0]
        {0x3116,0x24},  // INCKSEL2[7:0]
        {0x3118,0xA0},  // INCKSEL3[10:0]
        {0x311E,0x24},  // INCKSEL5[7:0]
        {0x32D4,0x21},  // -
        {0x32EC,0xA1},  // -
        {0x344C,0x2B},  // -
        {0x344D,0x01},  // -
        {0x344E,0xED},  // -
        {0x344F,0x01},  // -
        {0x3450,0xF6},  // -
        {0x3451,0x02},  // -
        {0x3452,0x7F},  // -
        {0x3453,0x03},  // -
        {0x358A,0x04},  // -
        {0x35A1,0x02},  // -
        {0x35EC,0x27},  // -
        {0x35EE,0x8D},  // -
        {0x35F0,0x8D},  // -
        {0x35F2,0x29},  // -
        {0x36BC,0x0C},  // -
        {0x36CC,0x53},  // -
        {0x36CD,0x00},  // -
        {0x36CE,0x3C},  // -
        {0x36D0,0x8C},  // -
        {0x36D1,0x00},  // -
        {0x36D2,0x71},  // -
        {0x36D4,0x3C},  // -
        {0x36D6,0x53},  // -
        {0x36D7,0x00},  // -
        {0x36D8,0x71},  // -
        {0x36DA,0x8C},  // -
        {0x36DB,0x00},  // -
        {0x3701,0x00},  // ADBIT1[7:0]
        {0x3720,0x00},  // -
        {0x3724,0x02},  // -
        {0x3726,0x02},  // -
        {0x3732,0x02},  // -
        {0x3734,0x03},  // -
        {0x3736,0x03},  // -
        {0x3742,0x03},  // -
        {0x3862,0xE0},  // -
        {0x38CC,0x30},  // -
        {0x38CD,0x2F},  // -
        {0x395C,0x0C},  // -
        {0x39A4,0x07},  // -
        {0x39A8,0x32},  // -
        {0x39AA,0x32},  // -
        {0x39AC,0x32},  // -
        {0x39AE,0x32},  // -
        {0x39B0,0x32},  // -
        {0x39B2,0x2F},  // -
        {0x39B4,0x2D},  // -
        {0x39B6,0x28},  // -
        {0x39B8,0x30},  // -
        {0x39BA,0x30},  // -
        {0x39BC,0x30},  // -
        {0x39BE,0x30},  // -
        {0x39C0,0x30},  // -
        {0x39C2,0x2E},  // -
        {0x39C4,0x2B},  // -
        {0x39C6,0x25},  // -
        {0x3A42,0xD1},  // -
        {0x3A4C,0x77},  // -
        {0x3AE0,0x02},  // -
        {0x3AEC,0x0C},  // -
        {0x3B00,0x2E},  // -
        {0x3B06,0x29},  // -
        {0x3B98,0x25},  // -
        {0x3B99,0x21},  // -
        {0x3B9B,0x13},  // -
        {0x3B9C,0x13},  // -
        {0x3B9D,0x13},  // -
        {0x3B9E,0x13},  // -
        {0x3BA1,0x00},  // -
        {0x3BA2,0x06},  // -
        {0x3BA3,0x0B},  // -
        {0x3BA4,0x10},  // -
        {0x3BA5,0x14},  // -
        {0x3BA6,0x18},  // -
        {0x3BA7,0x1A},  // -
        {0x3BA8,0x1A},  // -
        {0x3BA9,0x1A},  // -
        {0x3BAC,0xED},  // -
        {0x3BAD,0x01},  // -
        {0x3BAE,0xF6},  // -
        {0x3BAF,0x02},  // -
        {0x3BB0,0xA2},  // -
        {0x3BB1,0x03},  // -
        {0x3BB2,0xE0},  // -
        {0x3BB3,0x03},  // -
        {0x3BB4,0xE0},  // -
        {0x3BB5,0x03},  // -
        {0x3BB6,0xE0},  // -
        {0x3BB7,0x03},  // -
        {0x3BB8,0xE0},  // -
        {0x3BBA,0xE0},  // -
        {0x3BBC,0xDA},  // -
        {0x3BBE,0x88},  // -
        {0x3BC0,0x44},  // -
        {0x3BC2,0x7B},  // -
        {0x3BC4,0xA2},  // -
        {0x3BC8,0xBD},  // -
        {0x3BCA,0xBD},  // -
        {0x4004,0x48},  // TXCLKESC_FREQ[15:0]
        {0x4005,0x09},  //
        {0x4018,0xA7},  // TCLKPOST[15:0]
        {0x401A,0x57},  // TCLKPREPARE[15:0]
        {0x401C,0x5F},  // TCLKTRAIL[15:0]
        {0x401E,0x97},  // TCLKZERO[15:0]
        // did he forget this one ?
        {0x401F, IMX415_FETCH_16BIT_H(0x0197)}, // IMX415_TCLKZERO_H
        // end forget
        {0x4020,0x5F},  // THSPREPARE[15:0]
        {0x4022,0xAF},  // THSZERO[15:0]
        {0x4024,0x5F},  // THSTRAIL[15:0]
        {0x4026,0x9F},  // THSEXIT[15:0]
        {0x4028,0x4F},  // TLPX[15:0]
        //--------------- begin appending stuff
        {0x302C, 0x00}, //cannot find
        {0x302D, 0x00}, //cannot find
        {IMX415_SHR0_L, 0x08}, //?
        {IMX415_SHR0_M, 0x00}, //?
        {0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)
        // and missing i think:
        {IMX415_MDBIT,0x0}, //0h: RAW10, 1h: RAW12
        // xxxxxx
        /*{IMX415_BCWAIT_TIME_L,IMX415_FETCH_16BIT_L(0x07F)},
        {IMX415_BCWAIT_TIME_H,IMX415_FETCH_16BIT_H(0x07F)},
        {IMX415_CPWAIT_TIME_L,IMX415_FETCH_16BIT_L(0x05B)},
        {IMX415_CPWAIT_TIME_H,IMX415_FETCH_16BIT_H(0x05B)},
        {IMX415_SYS_MODE,0x8},
        {IMX415_INCKSEL1,0x00},
        {IMX415_INCKSEL2,0x24},
        {IMX415_INCKSEL3_L,IMX415_FETCH_16BIT_L(0x0A0)},
        {IMX415_INCKSEL3_H,IMX415_FETCH_16BIT_H(0x0A0)},
        {IMX415_INCKSEL4_L,IMX415_FETCH_16BIT_L(0x0E0)},
        {IMX415_INCKSEL4_H,IMX415_FETCH_16BIT_H(0x0E0)},
        {IMX415_INCKSEL5,0x24},
        {IMX415_TXCLKESC_FREQ_L,IMX415_FETCH_16BIT_L(0x0948)},
        {IMX415_TXCLKESC_FREQ_H,IMX415_FETCH_16BIT_H(0x0948)},
        {IMX415_INCKSEL6,0x1},
        {IMX415_INCKSEL7,0x0},
        //-
        {IMX415_TCLKPOST,0x00A7}, //yes
        {IMX415_TCLKPREPARE,0x0057}, //yes
        {IMX415_TCLKTRAIL,0x005F},  //yes
        // Consti10: added default for High
        {IMX415_TCLKZERO_L,IMX415_FETCH_16BIT_L(0x0197)}, //yes(after add)
        {IMX415_TCLKZERO_H,IMX415_FETCH_16BIT_H(0x0197)}, // -"-
        {IMX415_THSPREPARE,0x005F}, //yes
        {IMX415_THSZERO,0x00AF}, //yes
        {IMX415_THSTRAIL,0x005F}, //yes
        {IMX415_THSEXIT,0x009F}, //yes
        {IMX415_TLPX,0x004F}, //yes*/
        //--------------- end appending stuff
        // Don't forget
        {REG_NULL, 0x00},
};

// VMAX=0xD4=212 ? or 0x08D4=2260
// HMAX=0x016D=365
// PIX_HWIDTH=0x00 ? or 0x0F00==3840
// PIX_VWIDTH=0x10E0=4320 | 4320/2=2160
// IMX415-AAQR 2/2-line binning & Window cropping 3840x2160 CSI-2_4lane 37.125Mhz AD:10bit Output:12bit 1485Mbps Master Mode 90.011fps Integration Time 1.001ms Gain:6dB
static __maybe_unused const struct regval imx415_linear_10bit_1080p_90fps_bin_and_crop_regs_X[] = {
        {0x3008,0x7F}, // BCWAIT_TIME[9:0]
        {0x300A,0x5B}, // CPWAIT_TIME[9:0]
        {0x301C,0x04}, // WINMODE[3:0]
        {0x3020,0x01}, // HADD
        {0x3021,0x01}, // VADD
        {0x3022,0x01}, // ADDMODE[1:0]
        {0x3024,0xD4}, // VMAX[19:0]
        //Consti10: what about 0x3025 aka VMAX_M
        {0x3025,0x08},
        //Consti10 end
        {0x3028,0x6D}, // HMAX[15:0]
        {0x3029,0x01}, //
        {0x3031,0x00}, // ADBIT[1:0]
        {0x3033,0x08}, // SYS_MODE[3:0]
        {0x3040,0x0C}, // PIX_HST[12:0]
        {0x3042,0x00}, // PIX_HWIDTH[12:0]
        // Consti10: what about 0x3043 aka PIX_HWIDTH_H
        {0x3043,0x0F},
        //Const10 end
        {0x3044,0x20}, // PIX_VST[12:0]
        {0x3045,0x00}, //
        {0x3046,0xE0}, // PIX_VWIDTH[12:0]
        {0x3047,0x10}, //
        {0x3050,0x09}, // SHR0[19:0]
        {0x3051,0x08}, //
        {0x3090,0x14}, // GAIN_PCG_0[8:0]
        {0x30C1,0x00}, // XVS_DRV[1:0]
        {0x30D9,0x02}, // DIG_CLP_VSTART[4:0]
        {0x30DA,0x01}, // DIG_CLP_VNUM[1:0]
        {0x3116,0x24}, // INCKSEL2[7:0]
        {0x3118,0xA0}, // INCKSEL3[10:0]
        {0x311E,0x24}, // INCKSEL5[7:0]
        {0x32D4,0x21}, // -
        {0x32EC,0xA1}, // -
        {0x344C,0x2B}, // -
        {0x344D,0x01}, // -
        {0x344E,0xED}, // -
        {0x344F,0x01}, // -
        {0x3450,0xF6}, // -
        {0x3451,0x02}, // -
        {0x3452,0x7F}, // -
        {0x3453,0x03}, // -
        {0x358A,0x04}, // -
        {0x35A1,0x02}, // -
        {0x35EC,0x27}, // -
        {0x35EE,0x8D}, // -
        {0x35F0,0x8D}, // -
        {0x35F2,0x29}, // -
        {0x36BC,0x0C}, // -
        {0x36CC,0x53}, // -
        {0x36CD,0x00}, // -
        {0x36CE,0x3C}, // -
        {0x36D0,0x8C}, // -
        {0x36D1,0x00}, // -
        {0x36D2,0x71}, // -
        {0x36D4,0x3C}, // -
        {0x36D6,0x53}, // -
        {0x36D7,0x00}, // -
        {0x36D8,0x71}, // -
        {0x36DA,0x8C}, // -
        {0x36DB,0x00}, // -
        {0x3701,0x00}, // ADBIT1[7:0]
        {0x3720,0x00}, // -
        {0x3724,0x02}, // -
        {0x3726,0x02}, // -
        {0x3732,0x02}, // -
        {0x3734,0x03}, // -
        {0x3736,0x03}, // -
        {0x3742,0x03}, // -
        {0x3862,0xE0}, // -
        {0x38CC,0x30}, // -
        {0x38CD,0x2F}, // -
        {0x395C,0x0C}, // -
        {0x39A4,0x07}, // -
        {0x39A8,0x32}, // -
        {0x39AA,0x32}, // -
        {0x39AC,0x32}, // -
        {0x39AE,0x32}, // -
        {0x39B0,0x32}, // -
        {0x39B2,0x2F}, // -
        {0x39B4,0x2D}, // -
        {0x39B6,0x28}, // -
        {0x39B8,0x30}, // -
        {0x39BA,0x30}, // -
        {0x39BC,0x30}, // -
        {0x39BE,0x30}, // -
        {0x39C0,0x30}, // -
        {0x39C2,0x2E}, // -
        {0x39C4,0x2B}, // -
        {0x39C6,0x25}, // -
        {0x3A42,0xD1}, // -
        {0x3A4C,0x77}, // -
        {0x3AE0,0x02}, // -
        {0x3AEC,0x0C}, // -
        {0x3B00,0x2E}, // -
        {0x3B06,0x29}, // -
        {0x3B98,0x25}, // -
        {0x3B99,0x21}, // -
        {0x3B9B,0x13}, // -
        {0x3B9C,0x13}, // -
        {0x3B9D,0x13}, // -
        {0x3B9E,0x13}, // -
        {0x3BA1,0x00}, // -
        {0x3BA2,0x06}, // -
        {0x3BA3,0x0B}, // -
        {0x3BA4,0x10}, // -
        {0x3BA5,0x14}, // -
        {0x3BA6,0x18}, // -
        {0x3BA7,0x1A}, // -
        {0x3BA8,0x1A}, // -
        {0x3BA9,0x1A}, // -
        {0x3BAC,0xED}, // -
        {0x3BAD,0x01}, // -
        {0x3BAE,0xF6}, // -
        {0x3BAF,0x02}, // -
        {0x3BB0,0xA2}, // -
        {0x3BB1,0x03}, // -
        {0x3BB2,0xE0}, // -
        {0x3BB3,0x03}, // -
        {0x3BB4,0xE0}, // -
        {0x3BB5,0x03}, // -
        {0x3BB6,0xE0}, // -
        {0x3BB7,0x03}, // -
        {0x3BB8,0xE0}, // -
        {0x3BBA,0xE0}, // -
        {0x3BBC,0xDA}, // -
        {0x3BBE,0x88}, // -
        {0x3BC0,0x44}, // -
        {0x3BC2,0x7B}, // -
        {0x3BC4,0xA2}, // -
        {0x3BC8,0xBD}, // -
        {0x3BCA,0xBD}, // -
        {0x4004,0x48}, // TXCLKESC_FREQ[15:0]
        {0x4005,0x09}, //
        {0x4018,0xA7}, // TCLKPOST[15:0]
        {0x401A,0x57}, // TCLKPREPARE[15:0]
        {0x401C,0x5F}, // TCLKTRAIL[15:0]
        {0x401E,0x97}, // TCLKZERO[15:0]
        {0x4020,0x5F}, // THSPREPARE[15:0]
        {0x4022,0xAF}, // THSZERO[15:0]
        {0x4024,0x5F}, // THSTRAIL[15:0]
        {0x4026,0x9F}, // THSEXIT[15:0]
        {0x4028,0x4F}, // TLPX[15:0]
        // did he forget this one ?
        {0x401F, IMX415_FETCH_16BIT_H(0x0197)}, // IMX415_TCLKZERO_H
        // end forget
        {0x4020,0x5F}, // THSPREPARE[15:0]
        {0x4022,0xAF}, // THSZERO[15:0]
        {0x4024,0x5F}, // THSTRAIL[15:0]
        {0x4026,0x9F}, // THSEXIT[15:0]
        {0x4028,0x4F}, // TLPX[15:0]
        //--------------- begin appending stuff
        {0x302C, 0x00}, //cannot find
        {0x302D, 0x00}, //cannot find
        {IMX415_SHR0_L, 0x08}, //?
        {IMX415_SHR0_M, 0x00}, //?
        {0x3260, 0x01}, //cannot find, but is mentioned in the rockchip comments (set to 0x01 in normal mode, something else in hdr)
        // and missing i think:
        {IMX415_MDBIT,0x0}, //0h: RAW10, 1h: RAW12
        //--------------- end appending stuff
        // Don't forget
        {REG_NULL, 0x00},
};
#endif //MEDIASEVER_IMX415_REGS_ROCKCHIP_H
