/*
 *  Copyright (c) 2019 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include "RkAiqCalibDb.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define FUNC_ENTER LOG1("%s enter", __FUNCTION__);
#define FUNC_EXIT LOG1("%s exit", __FUNCTION__);
#define CALIBDB_PATH_LEN      256
#define CALIBDB_ENV_PATH_STR  "rkaiq_calibdb_path"

namespace RkCam {

map<string, CamCalibDbContext_t*> RkAiqCalibDb::mCalibDbsMap;
XCam::Mutex RkAiqCalibDb::mMutex{false};
static bool calibGetEnviromentPath(const char* variable, char* value)
{
    if (!variable || !value) {
        return false;
    }

    char* valueStr = getenv(variable);
    if (valueStr) {
        strncpy(value, valueStr, CALIBDB_PATH_LEN - 1);
        return true;
    }
    return false;
}

static void getFilePathName(char* iqFile, char *location)
{
    char dir[CALIBDB_PATH_LEN];
    char name[CALIBDB_PATH_LEN];
    char *srcfile = strdup(iqFile);
    char *pstart = strrchr(srcfile, '/');
    char *pend = strrchr(srcfile, '.');
    *pend = '\0';
    strcpy(name, pstart + 1);
    if (calibGetEnviromentPath(CALIBDB_ENV_PATH_STR, dir)) {
        sprintf(location, "%s/%s.bin", dir, name);
    } else {
        sprintf(location, "%s.bin", srcfile);
    }
    free(srcfile);
    LOGD("calibdb file is %s", location);
}

static bool isDataBinExist(char* iqFile) {
    char path[CALIBDB_PATH_LEN];

    getFilePathName(iqFile, path);
    if (0 == access(path, F_OK))
        return true;
    else
        return false;
}

static int persist(int fd, int payload_size, char* payload_ptr) {
    int ret = true;

    LOGD("persist payload_size %d bytes", payload_size);
    if (write(fd, payload_ptr, payload_size) <= 0) {
        LOGE("persist cell failed\n");
        return -1;
    }

    return 0;
}

static int reconstruct(int fd, int payload_size, char** payload_ptr) {
    *payload_ptr = (char*)malloc(payload_size);
    if (*payload_ptr == NULL)
        return -1;
    LOGD("malloc payload_ptr %p, size %d bytes", *payload_ptr, payload_size);
    if (read(fd, *payload_ptr, payload_size) <= 0) {
        LOGE("reconstruct cell failed %s!\n", strerror(errno));
        free(*payload_ptr);
        return -1;
    }

    return 0;
}

static int persistBayernr(int fd, CalibDb_BayerNr_2_t* bayernr) {
    int ret = 0;
    LOGD("%s:%d", __func__, __LINE__);
    ret |= persist(fd,
            sizeof(bayernr->enable),
            (char*)(&bayernr->enable));
    ret |= persist(fd,
            sizeof(bayernr->version),
            (char*)(bayernr->version));
    ret |= persist(fd,
            sizeof(bayernr->mode_num),
            (char*)(&bayernr->mode_num));
    ret |= persist(fd,
            sizeof(CalibDb_BayerNr_ModeCell_t) * bayernr->mode_num,
            (char*)(bayernr->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
   return ret;
}

static int reconstructBayernr(int fd, CalibDb_BayerNr_2_t* bayernr) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    if (read(fd, &bayernr->enable, sizeof(bayernr->enable)) <= 0)
        return -1;
    if (read(fd, bayernr->version, sizeof(bayernr->version)) <= 0)
        return -1;
    if (read(fd, &bayernr->mode_num, sizeof(bayernr->mode_num)) <= 0)
        return -1;

    ret |= reconstruct(fd,
            sizeof(CalibDb_BayerNr_ModeCell_t) * bayernr->mode_num,
            (char**)(&bayernr->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}

static int persistUvnr(int fd, CalibDb_UVNR_2_t* uvnr) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    ret |= persist(fd,
            sizeof(uvnr->enable),
            (char*)(&uvnr->enable));
    ret |= persist(fd,
            sizeof(uvnr->version),
            (char*)(uvnr->version));
    ret |= persist(fd,
            sizeof(uvnr->mode_num),
            (char*)(&uvnr->mode_num));
    ret |= persist(fd,
            sizeof(CalibDb_UVNR_ModeCell_t) * uvnr->mode_num,
            (char*)(uvnr->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
   return ret;
}

static int reconstructUvnr(int fd, CalibDb_UVNR_2_t* uvnr) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    if (read(fd, &uvnr->enable, sizeof(uvnr->enable)) <= 0)
        return -1;
    if (read(fd, uvnr->version, sizeof(uvnr->version)) <= 0)
        return -1;
    if (read(fd, &uvnr->mode_num, sizeof(uvnr->mode_num)) <= 0)
        return -1;

    ret |= reconstruct(fd,
            sizeof(CalibDb_UVNR_ModeCell_t) * uvnr->mode_num,
            (char**)(&uvnr->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}

static int persistYnr(int fd, CalibDb_YNR_2_t* ynr) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    ret |= persist(fd,
            sizeof(ynr->enable),
            (char*)(&ynr->enable));
    ret |= persist(fd,
            sizeof(ynr->version),
            (char*)(ynr->version));
    ret |= persist(fd,
            sizeof(ynr->mode_num),
            (char*)(&ynr->mode_num));
    ret |= persist(fd,
            sizeof(CalibDb_YNR_ModeCell_t) * ynr->mode_num,
            (char*)(ynr->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
   return ret;
}

static int reconstructYnr(int fd, CalibDb_YNR_2_t* ynr) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    if (read(fd, &ynr->enable, sizeof(ynr->enable)) <= 0)
        return -1;
    if (read(fd, ynr->version, sizeof(ynr->version)) <= 0)
        return -1;
    if (read(fd, &ynr->mode_num, sizeof(ynr->mode_num)) <= 0)
        return -1;

    ret |= reconstruct(fd,
            sizeof(CalibDb_YNR_ModeCell_t) * ynr->mode_num,
            (char**)(&ynr->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}


static int persistMfnr(int fd, CalibDb_MFNR_2_t* mfnr) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    ret |= persist(fd,
            sizeof(mfnr->enable),
            (char*)(&mfnr->enable));
    ret |= persist(fd,
            sizeof(mfnr->version),
            (char*)(mfnr->version));
    ret |= persist(fd,
            sizeof(mfnr->local_gain_en),
            (char*)(&mfnr->local_gain_en));
    ret |= persist(fd,
            sizeof(mfnr->motion_detect_en),
            (char*)(&mfnr->motion_detect_en));
    ret |= persist(fd,
            sizeof(mfnr->mode_3to1),
            (char*)(&mfnr->mode_3to1));
    ret |= persist(fd,
            sizeof(mfnr->max_level),
            (char*)(&mfnr->max_level));
    ret |= persist(fd,
            sizeof(mfnr->max_level_uv),
            (char*)(&mfnr->max_level_uv));
    ret |= persist(fd,
            sizeof(mfnr->back_ref_num),
            (char*)(&mfnr->back_ref_num));
    ret |= persist(fd,
            sizeof(mfnr->uv_ratio),
            (char*)(mfnr->uv_ratio));
    ret |= persist(fd,
            sizeof(mfnr->mode_num),
            (char*)(&mfnr->mode_num));
    ret |= persist(fd,
            sizeof(CalibDb_MFNR_ModeCell_t) * mfnr->mode_num,
            (char*)(mfnr->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
   return ret;
}

static int reconstructMfnr(int fd, CalibDb_MFNR_2_t* mfnr) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    if (read(fd, &mfnr->enable, sizeof(mfnr->enable)) <= 0)
        return -1;
    if (read(fd, mfnr->version, sizeof(mfnr->version)) <= 0)
        return -1;
    if (read(fd, &mfnr->local_gain_en, sizeof(mfnr->local_gain_en)) <= 0)
        return -1;
    if (read(fd, &mfnr->motion_detect_en, sizeof(mfnr->motion_detect_en)) <= 0)
        return -1;
    if (read(fd, &mfnr->mode_3to1, sizeof(mfnr->mode_3to1)) <= 0)
        return -1;
    if (read(fd, &mfnr->max_level, sizeof(mfnr->max_level)) <= 0)
        return -1;
    if (read(fd, &mfnr->max_level_uv, sizeof(mfnr->max_level_uv)) <= 0)
        return -1;
    if (read(fd, &mfnr->back_ref_num, sizeof(mfnr->back_ref_num)) <= 0)
        return -1;
    if (read(fd, mfnr->uv_ratio, sizeof(mfnr->uv_ratio)) <= 0)
        return -1;
    if (read(fd, &mfnr->mode_num, sizeof(mfnr->mode_num)) <= 0)
        return -1;

    ret |= reconstruct(fd,
            sizeof(CalibDb_MFNR_ModeCell_t) * mfnr->mode_num,
            (char**)(&mfnr->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}

static int persistSharp(int fd, CalibDb_Sharp_2_t* sharp) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    ret |= persist(fd,
            sizeof(sharp->enable),
            (char*)(&sharp->enable));
    ret |= persist(fd,
            sizeof(sharp->version),
            (char*)(sharp->version));
    ret |= persist(fd,
            sizeof(sharp->luma_point),
            (char*)(sharp->luma_point));
    ret |= persist(fd,
            sizeof(sharp->mode_num),
            (char*)(&sharp->mode_num));
    ret |= persist(fd,
            sizeof(CalibDb_Sharp_ModeCell_t) * sharp->mode_num,
            (char*)(sharp->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
   return ret;
}

static int reconstructSharp(int fd, CalibDb_Sharp_2_t* sharp) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    if (read(fd, &sharp->enable, sizeof(sharp->enable)) <= 0)
        return -1;
    if (read(fd, sharp->version, sizeof(sharp->version)) <= 0)
        return -1;
    if (read(fd, sharp->luma_point, sizeof(sharp->luma_point)) <= 0)
        return -1;
    if (read(fd, &sharp->mode_num, sizeof(sharp->mode_num)) <= 0)
        return -1;

    ret |= reconstruct(fd,
            sizeof(CalibDb_Sharp_ModeCell_t) * sharp->mode_num,
            (char**)(&sharp->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}

static int persistEdgeFilter(int fd, CalibDb_EdgeFilter_2_t* ef) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    ret |= persist(fd,
            sizeof(ef->enable),
            (char*)(&ef->enable));
    ret |= persist(fd,
            sizeof(ef->version),
            (char*)(ef->version));
    ret |= persist(fd,
            sizeof(ef->luma_point),
            (char*)(ef->luma_point));
    ret |= persist(fd,
            sizeof(ef->mode_num),
            (char*)(&ef->mode_num));
    ret |= persist(fd,
            sizeof(CalibDb_EdgeFilter_ModeCell_t) * ef->mode_num,
            (char*)(ef->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
   return ret;
}

static int reconstructEdgeFilter(int fd, CalibDb_EdgeFilter_2_t* ef) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    if (read(fd, &ef->enable, sizeof(ef->enable)) <= 0)
        return -1;
    if (read(fd, ef->version, sizeof(ef->version)) <= 0)
        return -1;
    if (read(fd, ef->luma_point, sizeof(ef->luma_point)) <= 0)
        return -1;
    if (read(fd, &ef->mode_num, sizeof(ef->mode_num)) <= 0)
        return -1;

    ret |= reconstruct(fd,
            sizeof(CalibDb_EdgeFilter_ModeCell_t) * ef->mode_num,
            (char**)(&ef->mode_cell));
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}

static int persistLsc(int fd, CalibDb_Lsc_t* lsc) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    ret |= persist(fd,
            sizeof(lsc->enable),
            (char*)(&lsc->enable));
    ret |= persist(fd,
            sizeof(lsc->damp_enable),
            (char*)(&lsc->damp_enable));
    ret |= persist(fd,
            sizeof(lsc->aLscCof),
            (char*)(&lsc->aLscCof));
    ret |= persist(fd,
            sizeof(lsc->tableAllNum),
            (char*)(&lsc->tableAllNum));
    ret |= persist(fd,
            sizeof(CalibDb_LscTableProfile_t) * lsc->tableAllNum,
            (char*)(lsc->tableAll));
    LOGD("%s:%d", __func__, __LINE__);
   return ret;
}

static int reconstructLsc(int fd, CalibDb_Lsc_t* lsc) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    if (read(fd, &lsc->enable, sizeof(lsc->enable)) <= 0)
        return -1;
    if (read(fd, &lsc->damp_enable, sizeof(lsc->damp_enable)) <= 0)
        return -1;
    if (read(fd, &lsc->aLscCof, sizeof(lsc->aLscCof)) <= 0)
        return -1;
    if (read(fd, &lsc->tableAllNum, sizeof(lsc->tableAllNum)) <= 0)
        return -1;

    ret |= reconstruct(fd,
            sizeof(CalibDb_LscTableProfile_t) * lsc->tableAllNum,
            (char**)(&lsc->tableAll));
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}

static int persistAf(int fd, CalibDb_AF_t* af) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    ret |= persist(fd, sizeof(CalibDb_AF_t), (char*)af);
    ret |= persist(fd,
        sizeof(float) * af->zoomfocus_tbl.tbl_len,
        (char*)(af->zoomfocus_tbl.focal_length));
    ret |= persist(fd,
        sizeof(signed short) * af->zoomfocus_tbl.tbl_len,
        (char*)(af->zoomfocus_tbl.zoomcode));
    for (int i = 0; i < af->zoomfocus_tbl.focuspos_len; i++) {
        ret |= persist(fd,
            sizeof(signed short) * af->zoomfocus_tbl.tbl_len,
            (char*)(af->zoomfocus_tbl.focuscode[i]));
    }
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}

static int reconstructAf(int fd, CalibDb_AF_t* af) {
    LOGD("%s:%d", __func__, __LINE__);
    int ret = 0;
    if (read(fd, af, sizeof(CalibDb_AF_t)) <= 0)
        return -1;
    ret |= reconstruct(fd,
        sizeof(float) * af->zoomfocus_tbl.tbl_len,
        (char**)(&af->zoomfocus_tbl.focal_length));
    ret |= reconstruct(fd,
        sizeof(signed short) * af->zoomfocus_tbl.tbl_len,
        (char**)(&af->zoomfocus_tbl.zoomcode));
    for (int i = 0; i < af->zoomfocus_tbl.focuspos_len; i++) {
        ret |= reconstruct(fd,
            sizeof(signed short) * af->zoomfocus_tbl.tbl_len,
            (char**)(&af->zoomfocus_tbl.focuscode[i]));
    }
    LOGD("%s:%d", __func__, __LINE__);
    return ret;
}

static int persistCalib(int fd, CamCalibDbContext_t* calib) {
    int ret = 0;

    ret |= persist(fd, sizeof(CalibDb_Header_t), (char*)(&calib->header));
    ret |= persist(fd, sizeof(CalibDb_Awb_Para_t), (char*)(&calib->awb));
    ret |= persist(fd, sizeof(CalibDb_Lut3d_t), (char*)(&calib->lut3d));
    ret |= persist(fd, sizeof(CalibDb_Aec_Para_t), (char*)(&calib->aec));
    ret |= persistAf(fd, &calib->af);
    ret |= persist(fd, sizeof(CalibDb_Ahdr_Para_t), (char*)(&calib->ahdr));
    ret |= persist(fd, sizeof(CalibDb_Blc_t), (char*)(&calib->blc));
    ret |= persist(fd, sizeof(CalibDb_Dpcc_t), (char*)(&calib->dpcc));
    ret |= persistBayernr(fd, &calib->bayerNr);
    ret |= persistLsc(fd, &calib->lsc);
    ret |= persist(fd, sizeof(CalibDb_RKDM_t), (char*)(&calib->dm));
    ret |= persist(fd, sizeof(CalibDb_Ccm_t), (char*)(&calib->ccm));
    ret |= persistUvnr(fd, &calib->uvnr);
    ret |= persist(fd, sizeof(CalibDb_Gamma_t), (char*)(&calib->gamma));
    ret |= persist(fd, sizeof(CalibDb_Degamma_t), (char*)(&calib->degamma));
    ret |= persistYnr(fd, &calib->ynr);
    ret |= persist(fd, sizeof(CalibDb_Gic_t), (char*)(&calib->gic));
    ret |= persistMfnr(fd, &calib->mfnr);
    ret |= persistSharp(fd, &calib->sharp);
    ret |= persistEdgeFilter(fd, &calib->edgeFilter);
    ret |= persist(fd, sizeof(CalibDb_Dehaze_t), (char*)(&calib->dehaze));
    ret |= persist(fd, sizeof(CalibDb_FEC_t), (char*)(&calib->afec));
    ret |= persist(fd, sizeof(CalibDb_LDCH_t), (char*)(&calib->aldch));
    ret |= persist(fd, sizeof(CalibDb_LUMA_DETECT_t), (char*)(&calib->lumaDetect));
    ret |= persist(fd, sizeof(CalibDb_ORB_t), (char*)(&calib->orb));
    ret |= persist(fd, sizeof(CalibDb_Sensor_Para_t), (char*)(&calib->sensor));
    ret |= persist(fd, sizeof(CalibDb_Module_Info_t), (char*)(&calib->module));
    ret |= persist(fd, sizeof(CalibDb_Cpsl_t), (char*)(&calib->cpsl));
    ret |= persist(fd, sizeof(CalibDb_ColorAsGrey_t), (char*)(&calib->colorAsGrey));
    ret |= persist(fd, sizeof(CalibDb_cProc_t), (char*)(&calib->cProc));
    ret |= persist(fd, sizeof(CalibDb_IE_t), (char*)(&calib->ie));
    ret |= persist(fd, sizeof(CalibDb_System_t), (char*)(&calib->sysContrl));
    return ret;
}

static int reconstructCalib(int fd, CamCalibDbContext_t* calib) {
    int ret = 0;

    if (read(fd, &calib->header, sizeof(CalibDb_Header_t)) <= 0)
        return -1;
    if (read(fd, &calib->awb, sizeof(CalibDb_Awb_Para_t)) <= 0)
        return -1;
    if (read(fd, &calib->lut3d, sizeof(CalibDb_Lut3d_t)) <= 0)
        return -1;
    if (read(fd, &calib->aec, sizeof(CalibDb_Aec_Para_t)) <= 0)
        return -1;
    ret |= reconstructAf(fd, &calib->af);
    if (read(fd, &calib->ahdr, sizeof(CalibDb_Ahdr_Para_t)) <= 0)
        return -1;
    if (read(fd, &calib->blc, sizeof(CalibDb_Blc_t)) <= 0)
        return -1;
    if (read(fd, &calib->dpcc, sizeof(CalibDb_Dpcc_t)) <= 0)
        return -1;
    ret |= reconstructBayernr(fd, &calib->bayerNr);
    ret |= reconstructLsc(fd, &calib->lsc);
    if (read(fd, &calib->dm, sizeof(CalibDb_RKDM_t)) <= 0)
        return -1;
    if (read(fd, &calib->ccm, sizeof(CalibDb_Ccm_t)) <= 0)
        return -1;
    ret |= reconstructUvnr(fd, &calib->uvnr);
    if (read(fd, &calib->gamma, sizeof(CalibDb_Gamma_t)) <= 0)
        return -1;
    if (read(fd, &calib->degamma, sizeof(CalibDb_Degamma_t)) <= 0)
        return -1;
    ret |= reconstructYnr(fd, &calib->ynr);
    if (read(fd, &calib->gic, sizeof(CalibDb_Gic_t)) <= 0)
        return -1;
    ret |= reconstructMfnr(fd, &calib->mfnr);
    ret |= reconstructSharp(fd, &calib->sharp);
    ret |= reconstructEdgeFilter(fd, &calib->edgeFilter);
    if (read(fd, &calib->dehaze, sizeof(CalibDb_Dehaze_t)) <= 0)
        return -1;
    if (read(fd, &calib->afec, sizeof(CalibDb_FEC_t)) <= 0)
        return -1;
    if (read(fd, &calib->aldch, sizeof(CalibDb_LDCH_t)) <= 0)
        return -1;
    if (read(fd, &calib->lumaDetect, sizeof(CalibDb_LUMA_DETECT_t)) <= 0)
        return -1;
    if (read(fd, &calib->orb, sizeof(CalibDb_ORB_t)) <= 0)
        return -1;
    if (read(fd, &calib->sensor, sizeof(CalibDb_Sensor_Para_t)) <= 0)
        return -1;
    if (read(fd, &calib->module, sizeof(CalibDb_Module_Info_t)) <= 0)
        return -1;
    if (read(fd, &calib->cpsl, sizeof(CalibDb_Cpsl_t)) <= 0)
        return -1;
    if (read(fd, &calib->colorAsGrey, sizeof(CalibDb_ColorAsGrey_t)) <= 0)
        return -1;
    if (read(fd, &calib->cProc, sizeof(CalibDb_cProc_t)) <= 0)
        return -1;
    if (read(fd, &calib->ie, sizeof(CalibDb_IE_t)) <= 0)
        return -1;
    if (read(fd, &calib->sysContrl, sizeof(CalibDb_System_t)) <= 0)
        return -1;
    return ret;
}

static bool calibSaveToFile(char* iqFile, CamCalibDbContext_t* calib)
{
    int fd;
     bool ret = true;
    char path[CALIBDB_PATH_LEN];

    getFilePathName(iqFile, path);
    fd = open(path, O_CREAT | O_TRUNC | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd < 0) {
        ret = false;
        LOGE("open %s failed!", path);
    } else {
        lseek(fd, 0, SEEK_SET);
        if (persistCalib(fd, calib)) {
            LOGE("write %s failed!\n", path);
            remove(path);
            ret = false;
        }
        close(fd);
    }
    return ret;
}

static bool calibReadFromFile(char* iqFile, CamCalibDbContext_t* calib)
{
    int fd;
    bool ret = true;
    char path[CALIBDB_PATH_LEN];

    getFilePathName(iqFile, path);
    fd = open(path, O_RDONLY | O_SYNC);
    if (fd < 0) {
        ret = false;
    } else {
        lseek(fd, 0, SEEK_SET);
        if (reconstructCalib(fd, calib)) {
            LOGE("read %s failed!\n", path);
            ret = false;
        }
        close(fd);
    }

    return ret;
}

static uint32_t _calc_checksum(CamCalibDbContext_t *pCalibDb) {

    uint32_t checkSum = 0;
    uint32_t i = 0;
    uint32_t *data = NULL;
    uint32_t size = 0;

#define CALC_SUM(data,size) \
    for(i = 0; i < sizeof(size) / 4; i++) { \
        checkSum += *data; \
        data++; \
    }

    // header
    data = (uint32_t*)(&pCalibDb->header);
    size = sizeof(CalibDb_Header_t) ;
    CALC_SUM(data, size);
    // awb
    data = (uint32_t*)(&pCalibDb->awb);
    size = sizeof(CalibDb_Awb_Para_t) ;
    CALC_SUM(data, size);

    // lut3d
    data = (uint32_t*)(&pCalibDb->lut3d);
    size = sizeof(CalibDb_Lut3d_t) ;
    CALC_SUM(data, size);

    // aec
    data = (uint32_t*)(&pCalibDb->aec);
    size = sizeof(CalibDb_Aec_Para_t) ;
    CALC_SUM(data, size);

    // af
    data = (uint32_t*)(&pCalibDb->af);
    size = sizeof(CalibDb_AF_t) ;
    CALC_SUM(data, size);

    // ahdr
    data = (uint32_t*)(&pCalibDb->ahdr);
    size = sizeof(CalibDb_Ahdr_Para_t) ;
    CALC_SUM(data, size);

    // ablc
    data = (uint32_t*)(&pCalibDb->blc);
    size = sizeof(CalibDb_Blc_t) ;
    CALC_SUM(data, size);

    // dpcc
    data = (uint32_t*)(&pCalibDb->dpcc);
    size = sizeof(CalibDb_Dpcc_t) ;
    CALC_SUM(data, size);

    // bayer nr
    data = (uint32_t*)(pCalibDb->bayerNr.mode_cell);
    size = pCalibDb->bayerNr.mode_num * sizeof(CalibDb_BayerNr_ModeCell_t);
    CALC_SUM(data, size);

    // lsc
    data = (uint32_t*)(&pCalibDb->lsc.aLscCof);
    size = sizeof(CalibDb_AlscCof_t);
    CALC_SUM(data, size);
    data = (uint32_t*)(pCalibDb->lsc.tableAll);
    size = pCalibDb->lsc.tableAllNum * sizeof(CalibDb_LscTableProfile_t);
    CALC_SUM(data, size);

    // rkdm
    data = (uint32_t*)(&pCalibDb->dm);
    size = sizeof(CalibDb_RKDM_t) ;
    CALC_SUM(data, size);

    // ccm
    data = (uint32_t*)(&pCalibDb->ccm);
    size = sizeof(CalibDb_Ccm_t) ;
    CALC_SUM(data, size);

    // uvnr
    data = (uint32_t*)(pCalibDb->uvnr.mode_cell);
    size = pCalibDb->uvnr.mode_num * sizeof(CalibDb_UVNR_ModeCell_t);
    CALC_SUM(data, size);

    // gamma
    data = (uint32_t*)(&pCalibDb->gamma);
    size = sizeof(CalibDb_Gamma_t ) ;
    CALC_SUM(data, size);

    // ynr
    data = (uint32_t*)(pCalibDb->ynr.mode_cell);
    size = pCalibDb->ynr.mode_num * sizeof(CalibDb_YNR_ModeCell_t);
    CALC_SUM(data, size);

    // gamma
    data = (uint32_t*)(&pCalibDb->gic);
    size = sizeof(CalibDb_Gic_t) ;
    CALC_SUM(data, size);

    // mfnr
    data = (uint32_t*)(pCalibDb->mfnr.mode_cell);
    size = pCalibDb->mfnr.mode_num * sizeof(CalibDb_MFNR_ModeCell_t);
    CALC_SUM(data, size);

    // sharp
    data = (uint32_t*)(pCalibDb->sharp.mode_cell);
    size = pCalibDb->sharp.mode_num * sizeof(CalibDb_Sharp_ModeCell_t);
    CALC_SUM(data, size);

    // edgefilter
    data = (uint32_t*)(pCalibDb->edgeFilter.mode_cell);
    size = pCalibDb->edgeFilter.mode_num * sizeof(CalibDb_EdgeFilter_ModeCell_t);
    CALC_SUM(data, size);

    // dehaze
    data = (uint32_t*)(&pCalibDb->dehaze);
    size = sizeof(CalibDb_Dehaze_t) ;
    CALC_SUM(data, size);

    // fec
    data = (uint32_t*)(&pCalibDb->afec);
    size = sizeof(CalibDb_FEC_t );
    CALC_SUM(data, size);

    // ldch
    data = (uint32_t*)(&pCalibDb->aldch);
    size = sizeof(CalibDb_LDCH_t );
    CALC_SUM(data, size);

    // lumaDetect
    data = (uint32_t*)(&pCalibDb->lumaDetect);
    size = sizeof(CalibDb_LUMA_DETECT_t);
    CALC_SUM(data, size);

    // orb
    data = (uint32_t*)(&pCalibDb->orb);
    size = sizeof(CalibDb_ORB_t);
    CALC_SUM(data, size);

    // sensor
    data = (uint32_t*)(&pCalibDb->sensor);
    size = sizeof(CalibDb_Sensor_Para_t);
    CALC_SUM(data, size);

    // module
    data = (uint32_t*)(&pCalibDb->module);
    size = sizeof(CalibDb_Module_Info_t );
    CALC_SUM(data, size);

    // cpsl
    data = (uint32_t*)(&pCalibDb->cpsl);
    size = sizeof(CalibDb_Cpsl_t );
    CALC_SUM(data, size);

    // colorAsGrey
    data = (uint32_t*)(&pCalibDb->colorAsGrey);
    size = sizeof(CalibDb_ColorAsGrey_t );
    CALC_SUM(data, size);

    // ie
    data = (uint32_t*)(&pCalibDb->ie);
    size = sizeof(CalibDb_IE_t);
    CALC_SUM(data, size);

    // sysContrl
    data = (uint32_t*)(&pCalibDb->sysContrl);
    size = sizeof(CalibDb_System_t);
    CALC_SUM(data, size);

    LOGD("%s(%d): iq data checksum:%u \n", __FUNCTION__, __LINE__, checkSum);

    LOGD("%s(%d): exit\n", __FUNCTION__, __LINE__);
    return checkSum;
}

CamCalibDbContext_t* RkAiqCalibDb::createCalibDb(char* iqFile)
{
    map<string, CamCalibDbContext_t*>::iterator it;

    string str(iqFile);

    it = mCalibDbsMap.find(str);
    if (it != mCalibDbsMap.end()) {
        LOGD("use cached calibdb for %s!", iqFile);
        return it->second;
    } else {
        CamCalibDbContext_t *pCalibDb = new CamCalibDbContext_t();
        if (pCalibDb) {
            initCalibDb(pCalibDb);
            if (0 == access(iqFile, F_OK)) {
                RkAiqCalibParser  parser(pCalibDb);
                mMutex.lock();
                bool ret = parser.doParse(iqFile);
                mMutex.unlock();
                if (ret) {
                    uint32_t magicCode = calib_check_calc_checksum();
                    if (magicCode != pCalibDb->header.magic_code)
                        LOGW("magic code is not matched! calculated:%u, readed:%u", magicCode, pCalibDb->header.magic_code);
                    mCalibDbsMap[str] = pCalibDb;
                    LOGD("create calibdb from %s success.", iqFile);
                    _calc_checksum(pCalibDb);
                    return pCalibDb;
                } else {
                    LOGE("parse %s failed.", iqFile);
                }
            } else if(isDataBinExist(iqFile)) {
                if (calibReadFromFile(iqFile, pCalibDb)) {
                    uint32_t magicCode = calib_check_calc_checksum();
                    if (magicCode != pCalibDb->header.magic_code)
                        LOGE("magic code is not matched! calculated:%u, readed:%u", magicCode, pCalibDb->header.magic_code);
                    mCalibDbsMap[str] = pCalibDb;
                    LOGD("get calibdb from bin success.");
                    _calc_checksum(pCalibDb);
                    return pCalibDb;
                } else {
                    LOGE("get calibdb from bin failed.");
                }
            } else {
                LOGE("calibdb %s and bin are all not exist!", iqFile);
            }
        } else {
            LOGE("alloc calibdb memory failed.");
        }
    }
    return NULL;
}

bool RkAiqCalibDb::generateCalibDb(char* iqFileRef, char* iqFileOutput, CamCalibDbContext_t* pCalibDb)
{
    //map<string, CamCalibDbContext_t*>::iterator it;

    string str(iqFileRef);

    //it = mCalibDbsMap.find(str);
    if (pCalibDb) {
        RkAiqCalibParser  parser(pCalibDb);
        if (parser.doGenerate(iqFileRef, iqFileOutput)) {
            mCalibDbsMap[str] = pCalibDb;
            LOGD("generate calibdb from %s to %s success.", iqFileRef, iqFileOutput);
            return true;
        }
        else {
            LOGE("generate %s to %s failed.", iqFileRef, iqFileOutput);
        }
    }
    else {
        LOGE("alloc generate memory failed.");
    }
    return false;
}

void RkAiqCalibDb::freeAfPart(CalibDb_AF_t *af)
{
    if (af->zoomfocus_tbl.focal_length) {
        free(af->zoomfocus_tbl.focal_length);
        af->zoomfocus_tbl.focal_length = NULL;
    }
    if (af->zoomfocus_tbl.zoomcode) {
        free(af->zoomfocus_tbl.zoomcode);
        af->zoomfocus_tbl.zoomcode = NULL;
    }
    if (af->zoomfocus_tbl.focuscode) {
        for (int i = 0; i < af->zoomfocus_tbl.focuspos_len; i++) {
            free(af->zoomfocus_tbl.focuscode[i]);
            af->zoomfocus_tbl.focuscode[i] = NULL;
        }
    }
}

void RkAiqCalibDb::releaseCalibDb()
{
    std::map<string, CamCalibDbContext_t*>::iterator it;
    for (it = mCalibDbsMap.begin(); it != mCalibDbsMap.end(); it++) {
        CamCalibDbContext_t *pCalibDb = it->second;
        if(pCalibDb) {
            freeAfPart(&pCalibDb->af);
            if(pCalibDb->lsc.tableAll != NULL){
                free(pCalibDb->lsc.tableAll);

            }
            LOGI("releaseCalibDb!");
			if(pCalibDb->bayerNr.mode_cell != NULL){
				free(pCalibDb->bayerNr.mode_cell);
				pCalibDb->bayerNr.mode_cell = NULL;
				pCalibDb->bayerNr.mode_num = 0;
			}
			if( pCalibDb->uvnr.mode_cell != NULL){
				free(pCalibDb->uvnr.mode_cell);
				pCalibDb->uvnr.mode_cell = NULL;
				pCalibDb->uvnr.mode_num = 0;
			}
			if(pCalibDb->ynr.mode_cell != NULL){
				free(pCalibDb->ynr.mode_cell);
				pCalibDb->ynr.mode_cell = NULL;
				pCalibDb->ynr.mode_num = 0;

			}
			if(pCalibDb->mfnr.mode_cell != NULL){
				free(pCalibDb->mfnr.mode_cell);
				pCalibDb->mfnr.mode_cell = NULL;
				pCalibDb->mfnr.mode_num = 0;
			}
			if( pCalibDb->sharp.mode_cell != NULL){
				free(pCalibDb->sharp.mode_cell);
				pCalibDb->sharp.mode_cell = NULL;
				pCalibDb->sharp.mode_num = 0;
			}
			if( pCalibDb->edgeFilter.mode_cell != NULL){
				free(pCalibDb->edgeFilter.mode_cell);
				pCalibDb->edgeFilter.mode_cell = NULL;
				pCalibDb->edgeFilter.mode_num = 0;
			}
            delete pCalibDb;
        }
    }
    mCalibDbsMap.clear();
}

CamCalibDbContext_t* RkAiqCalibDb::getCalibDb(char* iqFile)
{
    std::map<string, CamCalibDbContext_t*>::iterator it;

    std::string str(iqFile);

    it = mCalibDbsMap.find(str);
    if (it != mCalibDbsMap.end()) {
        return it->second;
    } else {
        LOGE("calibDb not found!");
        return NULL;
    }
}

void RkAiqCalibDb::createCalibDbBinFromXml(char* iqFile)
{
    CamCalibDbContext_t *pCalibDb = new CamCalibDbContext_t();
    if (pCalibDb) {
        if (0 == access(iqFile, F_OK)) {
            RkAiqCalibParser  parser(pCalibDb);
            if (parser.doParse(iqFile)) {
                uint32_t magicCode = calib_check_calc_checksum();
                if (magicCode != pCalibDb->header.magic_code)
                    LOGW("magic code is not matched! calculated:%u, readed:%u", magicCode, pCalibDb->header.magic_code);
                LOGI("create calibdb from %s success, magic code %u.", iqFile, magicCode);
                if (calibSaveToFile(iqFile, pCalibDb))
                    LOGD("save to bin success.");
                else
                    LOGE("save to bin failed.");
            } else {
                LOGE("parse %s failed.", iqFile);
            }
        } else {
            LOGE("%s is not found!", iqFile);
        }
        delete pCalibDb;
    }
}

void RkAiqCalibDb::initCalibDb(CamCalibDbContext_t* pCalibDb)
{
    if (pCalibDb) {
        memset(pCalibDb, 0, sizeof(CamCalibDbContext_t));
        /* initialize cproc */
        pCalibDb->cProc.enable = 1;
        pCalibDb->cProc.brightness = 128;
        pCalibDb->cProc.contrast = 128;
        pCalibDb->cProc.saturation = 128;
        pCalibDb->cProc.hue = 128;
    }
}

}; //namespace RkCam


