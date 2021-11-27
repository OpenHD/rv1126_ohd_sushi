//
// Created by consti10 on 27.11.21.
//

#ifndef RV1126_OHD_SUSHI_RKMEDIA_HELPER_H
#define RV1126_OHD_SUSHI_RKMEDIA_HELPER_H


static void print_rk_aiq_working_mode_t(rk_aiq_working_mode_t rkAiqWorkingMode){
    const char* s;
    switch (rkAiqWorkingMode) {
        case RK_AIQ_WORKING_MODE_NORMAL:
            s="RK_AIQ_WORKING_MODE_NORMAL";
            break;
        case RK_AIQ_WORKING_MODE_ISP_HDR2:
            s="RK_AIQ_WORKING_MODE_ISP_HDR2";
            break;
        case RK_AIQ_WORKING_MODE_ISP_HDR3:
            s="RK_AIQ_WORKING_MODE_ISP_HDR3";
            break;
        default:
            s="Unknown";
    }
    printf("ISP working mode: %s\n",s);
}

static rk_aiq_working_mode_t convert_rk_aiq_working_mode_t(int workModeInt){
    rk_aiq_working_mode_t ret;
    switch (workModeInt) {
        case 0:
            ret = RK_AIQ_WORKING_MODE_NORMAL;
            break;
        case 1:
            ret = RK_AIQ_WORKING_MODE_ISP_HDR2;
            break;
        case 2:
            ret = RK_AIQ_WORKING_MODE_ISP_HDR3;
            break;
        default:
            ret=RK_AIQ_WORKING_MODE_NORMAL;
            break;
    }
    return ret;
}

#endif //RV1126_OHD_SUSHI_RKMEDIA_HELPER_H
