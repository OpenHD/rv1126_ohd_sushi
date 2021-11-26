//
// Created by consti10 on 30.10.21.
//

#ifndef RV1126_RV1109_V2_2_0_20210825_IMX415_REGS_CONSTI_GLOBAL_H
#define RV1126_RV1109_V2_2_0_20210825_IMX415_REGS_CONSTI_GLOBAL_H

// INCK = 37.125
// 2376Mbps / lane
static __maybe_unused const struct regval imx415_regs_2376Mbps[] = {
        {IMX415_BCWAIT_TIME_L,IMX415_FETCH_16BIT_L(0x07F)},
        {IMX415_BCWAIT_TIME_H,IMX415_FETCH_16BIT_H(0x07F)},
        {IMX415_CPWAIT_TIME_L,IMX415_FETCH_16BIT_L(0x05B)},
        {IMX415_CPWAIT_TIME_H,IMX415_FETCH_16BIT_H(0x05B)},
        {IMX415_SYS_MODE,0x0},
        {IMX415_INCKSEL1,0x00},
        {IMX415_INCKSEL2,0x24},
        {IMX415_INCKSEL3_L,IMX415_FETCH_16BIT_L(0x100)},
        {IMX415_INCKSEL3_H,IMX415_FETCH_16BIT_H(0x100)},
        {IMX415_INCKSEL4_L,IMX415_FETCH_16BIT_L(0x0E0)},
        {IMX415_INCKSEL4_H,IMX415_FETCH_16BIT_H(0x0E0)},
        {IMX415_INCKSEL5,0x24},
        {IMX415_TXCLKESC_FREQ_L,IMX415_FETCH_16BIT_L(0x0948)},
        {IMX415_TXCLKESC_FREQ_H,IMX415_FETCH_16BIT_H(0x0948)},
        {IMX415_INCKSEL6,0x1},
        {IMX415_INCKSEL7,0x0},
};

// INCK = 37.125
// 1782Mbps / lane
static __maybe_unused const struct regval imx415_regs_1782Mbps[] = {
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
};

// INCK = 37.125
// 1485Mbps / lane
static __maybe_unused const struct regval imx415_regs_1485Mbps[] = {
        {IMX415_BCWAIT_TIME_L,IMX415_FETCH_16BIT_L(0x07F)},
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
};

// global timing for
// INCK=37.125
// 1782Mbps
static __maybe_unused const struct regval imx415_X_global_timing[] = {
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
};

// global timing for
// INCK=37.125
// 1485Mbps
// looked up from "2nd try, 1080p90"
// is the same as "2nd try, 720p120"
// note: 1485Mbps is for some reason not actually used in the spec sheet I got
// (e.g. I cannot got the global timing values for it from the spec sheet, in contrast
// to the other modes)
// actually, yes, you can find it in the spec sheet !
static __maybe_unused const struct regval imx415_X_global_timing2[] = {
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
        {IMX415_TLPX,0x004F}, //yes
};

#endif //RV1126_RV1109_V2_2_0_20210825_IMX415_REGS_CONSTI_GLOBAL_H
