/*
* rk_aiq_accm_algo.cpp

* for rockchip v2.0.0
*
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
/* for rockchip v2.0.0*/

#include "accm/rk_aiq_accm_algo.h"
#include "xcam_log.h"
#include "interpolation.h"

RKAIQ_BEGIN_DECLARE

XCamReturn Swinfo_wbgain_init(float awbGain[2], const CalibDb_Ccm_ModeCell_t *pCalib, const char* illuName)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOG1_ACCM("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    if (pCalib == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        LOGE_ACCM("%s(%d): invalid input params\n", __FUNCTION__, __LINE__);
        return ret;
    }

    bool lsFound;

    for(int i = 0; i < pCalib->aCcmCof.illuNum; i++) {
        if(strcmp(pCalib->aCcmCof.illAll[i].illuName, illuName) == 0) {
            awbGain[0] = pCalib->aCcmCof.illAll[i].awbGain[0];
            awbGain[1] = pCalib->aCcmCof.illAll[i].awbGain[1];
            lsFound = true;
            LOGD_ACCM("%s: accm lsForFirstFrame:%s", __FUNCTION__, pCalib->aCcmCof.illAll[i].illuName);
            break;
        }
    }
    if(pCalib->aCcmCof.illuNum > 0 && lsFound == false) {
        awbGain[0] = pCalib->aCcmCof.illAll[0].awbGain[0];
        awbGain[1] = pCalib->aCcmCof.illAll[0].awbGain[1];
        LOGD_ACCM("%s: accm lsForFirstFrame:%s", __FUNCTION__, pCalib->aCcmCof.illAll[0].illuName);
    }
    LOGI_ACCM("%s: accm illunum:%d", __FUNCTION__, pCalib->aCcmCof.illuNum);
    LOG1_ACCM( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return ret;
}

XCamReturn illuminant_index_estimation_ccm(int light_num, const CalibDb_AccmCof_ill_t illAll[], float awbGain[2], int* illuminant_index)
{

    LOG1_ACCM( "%s: (enter)\n", __FUNCTION__);
    float minDist = 9999999;
    float dist[8];
    float nRG, nBG;
    *illuminant_index = 0;
    XCamReturn ret = XCAM_RETURN_ERROR_FAILED;
    for(int i = 0; i < light_num; i++)
    {
        nRG = awbGain[0];
        nBG = awbGain[1];
        dist[i] = sqrt((nRG - illAll[i].awbGain[0]) * (nRG -  illAll[i].awbGain[0])
                       + (nBG -  illAll[i].awbGain[1]) * (nBG -  illAll[i].awbGain[1]));
        if(dist[i] < minDist)
        {
            minDist = dist[i];
            *illuminant_index = i;
            ret = XCAM_RETURN_NO_ERROR;
        }
    }
    if(ret != XCAM_RETURN_NO_ERROR) {
        LOGE_ACCM("fail to estimate illuminant!!!\n");
    }

    LOGD_ACCM( "wbGain:%f,%f, estimation illuminant  is %s(%d) \n", awbGain[0], awbGain[1],
               illAll[*illuminant_index].illuName, *illuminant_index);

    LOG1_ACCM( "%s: (exit)\n", __FUNCTION__);
    return ret;
}


static XCamReturn AwbOrderCcmProfilesBySaturation
(
    const CalibDb_CcmMatrixProfile_t* pCcmProfiles[],
    const int32_t   cnt
) {
    int32_t i, j;

    for (i = 0; i < (cnt - 1); ++i) {
        for (j = 0; j < (cnt - i - 1); ++j) {
            if (pCcmProfiles[j]->saturation < pCcmProfiles[j + 1]->saturation) {
                const CalibDb_CcmMatrixProfile_t* temp   = pCcmProfiles[j];
                pCcmProfiles[j]         = pCcmProfiles[j + 1];
                pCcmProfiles[j + 1]       = temp;
            }
        }
    }

    return (XCAM_RETURN_NO_ERROR);
}

XCamReturn pCcmMatrixAll_init(accm_context_t* accm_context, const CalibDb_Ccm_ModeCell_t *pCalib, int idx)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOG1_ACCM("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    if (pCalib == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        LOGE_ACCM("%s(%d): invalid input params\n", __FUNCTION__, __LINE__);
        return ret;
    }

    //Config  pCcmMatrixAll (normal and hdr)
       // 1) get and reorder para
    for(int i = 0; i < pCalib->aCcmCof.illuNum; i++) {
        for (int j = 0; j < pCalib->aCcmCof.illAll[i].matrixUsedNO; j++) {
            char name[CCM_PROFILE_NAME];
            sprintf(name, "%s", pCalib->aCcmCof.illAll[i].matrixUsed[j]);
            const CalibDb_CcmMatrixProfile_t* pCcmMatrixProfile = NULL;
            // get a ccm-profile from database
            ret = CamCalibDbGetCcmProfileByName(pCalib, name, &pCcmMatrixProfile);
            RETURN_RESULT_IF_DIFFERENT(ret, XCAM_RETURN_NO_ERROR);
            // store ccm-profile in pointer array
            accm_context->pCcmMatrixAll[idx][i][j] = pCcmMatrixProfile;
            LOGD_ACCM("CCM name  %s coef:%f,%f,%f  %f,%f,%f  %f,%f,%f  \n", name,
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[0],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[1],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[2],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[3],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[4],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[5],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[6],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[7],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkCoeff.fCoeff[8]);
            LOGD_ACCM("CCM name  %s off:%f,%f,%f  \n", name,
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkOffset.fCoeff[0],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkOffset.fCoeff[1],
                      accm_context->pCcmMatrixAll[idx][i][j]->CrossTalkOffset.fCoeff[2]);

        }
        // order ccm-profiles by saturation
        ret = AwbOrderCcmProfilesBySaturation(accm_context->pCcmMatrixAll[idx][i],
                                              pCalib->aCcmCof.illAll[i].matrixUsedNO);
        //RETURN_RESULT_IF_DIFFERENT(result, XCAM_RETURN_NO_ERROR);

    }

    LOG1_ACCM( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return ret;
}
static XCamReturn SatSelectCcmProfiles
(
    const float     fSaturation,
    int32_t         no_ccm,
    const CalibDb_CcmMatrixProfile_t* pCcmProfiles[],
    const CalibDb_CcmMatrixProfile_t** pCcmProfile1,
    const CalibDb_CcmMatrixProfile_t** pCcmProfile2
) {
    XCamReturn XCamReturn = XCAM_RETURN_NO_ERROR;

    if ((no_ccm == 0) || (pCcmProfiles == NULL)
            || (pCcmProfile1 == NULL) || (pCcmProfile2 == NULL)) {
        return (XCAM_RETURN_ERROR_PARAM);
    }

    if (fSaturation >= pCcmProfiles[0]->saturation) {
        *pCcmProfile1 = pCcmProfiles[0];
        *pCcmProfile2 = NULL;
        LOGD_ACCM( "select:%s \n", (*pCcmProfile1)->name);
        XCamReturn = XCAM_RETURN_ERROR_OUTOFRANGE;
    } else {
        int32_t nLast = no_ccm - 1;
        if (fSaturation <= pCcmProfiles[nLast]->saturation) {
            *pCcmProfile1 = pCcmProfiles[nLast];
            *pCcmProfile2 = NULL;
            LOGD_ACCM( "select:%s \n", (*pCcmProfile1)->name);
            XCamReturn = XCAM_RETURN_ERROR_OUTOFRANGE;
        } else {
            uint16_t n = 0;

            /* find the segment */
            while ((fSaturation <= pCcmProfiles[n]->saturation) && (n <= nLast)) {
                n++;
            }

            if (n > 0)
                n--;

            *pCcmProfile1 = pCcmProfiles[n];
            *pCcmProfile2 = pCcmProfiles[n + 1];

        }
    }

    return (XCamReturn);
}


/******************************************************************************
 * InterpolateMatrices
 *****************************************************************************/
static XCamReturn SatInterpolateMatrices
(
    const float             fSat,
    const CalibDb_CcmMatrixProfile_t*   pCcProfileA,
    const CalibDb_CcmMatrixProfile_t*   pCcProfileB,
    Cam3x3FloatMatrix_t*          pResMatrix
) {
    XCamReturn iXCamReturn = XCAM_RETURN_ERROR_PARAM;

    if ((pCcProfileA != NULL) && (pCcProfileA != NULL) && (pResMatrix != NULL)) {
        const Cam3x3FloatMatrix_t *pMatrixA = &pCcProfileA->CrossTalkCoeff;
        const Cam3x3FloatMatrix_t *pMatrixB = &pCcProfileB->CrossTalkCoeff;

        float fSatA = pCcProfileA->saturation;
        float fSatB = pCcProfileB->saturation;

        float f1 = ( fSatB - fSat ) / ( fSatB - fSatA ); // test: if fSat == fSatA => f1 = 1 => choose A: ok
        float f2 = 1.0f - f1;

        int i;

        for ( i = 0; i < 9; i++)
        {
            pResMatrix->fCoeff[i] = f1 * pMatrixA->fCoeff[i] + f2 * pMatrixB->fCoeff[i];
        }

        iXCamReturn = XCAM_RETURN_NO_ERROR;

    }

    return (iXCamReturn);
}

/******************************************************************************
 * InterpolateMatrices
 *****************************************************************************/
static XCamReturn SatInterpolateOffset
(
    const float             fSat,
    const CalibDb_CcmMatrixProfile_t*   pCcProfileA,
    const CalibDb_CcmMatrixProfile_t*   pCcProfileB,
    Cam1x3FloatMatrix_t     *pResOffset
) {
    XCamReturn result = XCAM_RETURN_ERROR_PARAM;

    if ( (pCcProfileA != NULL) && (pCcProfileB != NULL) && (pResOffset != NULL) )
    {
        const Cam1x3FloatMatrix_t *pOffsetA = &pCcProfileA->CrossTalkOffset;
        const Cam1x3FloatMatrix_t *pOffsetB = &pCcProfileB->CrossTalkOffset;

        float fSatA = pCcProfileA->saturation;
        float fSatB = pCcProfileB->saturation;

        float f1 = ( fSatB - fSat ) / ( fSatB - fSatA ); // test: if fSat == fSatA => f1 = 1 => choose A: ok
        float f2 = 1.0f - f1;

        pResOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_RED]
            = f1 * pOffsetA->fCoeff[CAM_3CH_COLOR_COMPONENT_RED] + f2 * pOffsetB->fCoeff[CAM_3CH_COLOR_COMPONENT_RED];
        pResOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN]
            = f1 * pOffsetA->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN] + f2 * pOffsetB->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN];
        pResOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE]
            = f1 * pOffsetA->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] + f2 * pOffsetB->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE];

        result = XCAM_RETURN_NO_ERROR;
    }

    return ( result );

}

/******************************************************************************
 * Damping
 *****************************************************************************/
static XCamReturn Damping
(
    const float         damp,                /**< damping coefficient */
    Cam3x3FloatMatrix_t *pMatrixUndamped,   /**< undamped new computed matrices */
    Cam3x3FloatMatrix_t *pMatrixDamped,     /**< old matrices and result */
    Cam1x3FloatMatrix_t *pOffsetUndamped,   /**< undamped new computed offsets */
    Cam1x3FloatMatrix_t *pOffsetDamped      /**< old offset and result */
)
{
    XCamReturn result = XCAM_RETURN_ERROR_PARAM;

    if ( (pMatrixUndamped != NULL) && (pMatrixDamped != NULL)
            && (pOffsetUndamped != NULL) && (pOffsetDamped != NULL) )
    {
        int32_t i;
        float f = (1.0f - damp);

        /* calc. damped cc matrix */
        for( i = 0; i < 9; i++ )
        {
            pMatrixDamped->fCoeff[i] = (damp * pMatrixDamped->fCoeff[i]) + (f *  pMatrixUndamped->fCoeff[i]);
        }

        /* calc. damped cc offsets */
        pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_RED]
            = (damp * pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_RED])
              + (f * pOffsetUndamped->fCoeff[CAM_3CH_COLOR_COMPONENT_RED]);
        pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN]
            = (damp * pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN])
              + (f * pOffsetUndamped->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN]);
        pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE]
            = (damp * pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE])
              + (f * pOffsetUndamped->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE]);

        result = XCAM_RETURN_NO_ERROR;
    }
    LOGD_ACCM( "dampfactor:%f", damp);
    LOGD_ACCM( " undampedCcmMatrix: %f,%f,%f,%f,%f,%f,%f,%f,%f  undampedCcOffset:%f,%f,%f  \n",
               pMatrixUndamped->fCoeff[0],
               pMatrixUndamped->fCoeff[1],
               pMatrixUndamped->fCoeff[2],
               pMatrixUndamped->fCoeff[3],
               pMatrixUndamped->fCoeff[4],
               pMatrixUndamped->fCoeff[5],
               pMatrixUndamped->fCoeff[6],
               pMatrixUndamped->fCoeff[7],
               pMatrixUndamped->fCoeff[8],
               pOffsetUndamped->fCoeff[0],
               pOffsetUndamped->fCoeff[1],
               pOffsetUndamped->fCoeff[2]
             );
    LOGD_ACCM( " dampedCcmMatrix: %f,%f,%f,%f,%f,%f,%f,%f,%f  dampedCcOffset:%f,%f,%f  \n",
               pMatrixDamped->fCoeff[0],
               pMatrixDamped->fCoeff[1],
               pMatrixDamped->fCoeff[2],
               pMatrixDamped->fCoeff[3],
               pMatrixDamped->fCoeff[4],
               pMatrixDamped->fCoeff[5],
               pMatrixDamped->fCoeff[6],
               pMatrixDamped->fCoeff[7],
               pMatrixDamped->fCoeff[8],
               pOffsetDamped->fCoeff[0],
               pOffsetDamped->fCoeff[1],
               pOffsetDamped->fCoeff[2]
             );


    return ( result );
}

void Saturationadjust(float fScale, accm_handle_t hAccm )
{
          Cam3x3FloatMatrix_t *pccMatrixA;
          Cam3x3FloatMatrix_t *pccMatrixB;
          Cam3x3FloatMatrix_t  Matrix_tmp;
    float flevel1 = hAccm->accmRest.color_saturation_level;
    if (fScale < DIVMIN) {
         if(fabs((flevel1-50))>DIVMIN){
                 LOGW_ACCM("fSclae is  %f  , so saturation adjust bypass\n",  fScale);
         }
     } else {
        flevel1 = (flevel1 - 50) / 50 + 1;
        LOGD_ACCM("Satura: %f \n", flevel1);
        pccMatrixA = &hAccm->accmRest.undampedCcmMatrix;
        memcpy(&Matrix_tmp, &pccMatrixA->fCoeff, sizeof(Cam3x3FloatMatrix_t));
        pccMatrixB = &Matrix_tmp;
        /* M =  (M0 - E) * fscale + E
           M' = ycbcr2rgb[ sat_matrix * rgb2ycbcr(M) ]
           M1 = (M' - E) / fscale + E
        */
        if ( (pccMatrixA != NULL) && (pccMatrixB != NULL) )
        {
            for(int i =0; i < 9; i++)
            {
                if (i == 0 || i == 4 || i == 8){
                  pccMatrixA->fCoeff[i] = (pccMatrixA->fCoeff[i] - 1)*fScale+1;
                }
                else{
                  pccMatrixA->fCoeff[i] = pccMatrixA->fCoeff[i]*fScale;
                }
            }
            pccMatrixB->fCoeff[0] = 0.299 * pccMatrixA->fCoeff[0] + 0.587 * pccMatrixA->fCoeff[3] + 0.114 * pccMatrixA->fCoeff[6];
            pccMatrixB->fCoeff[1] = 0.299 * pccMatrixA->fCoeff[1] + 0.587 * pccMatrixA->fCoeff[4] + 0.114 * pccMatrixA->fCoeff[7];
            pccMatrixB->fCoeff[2] = 0.299 * pccMatrixA->fCoeff[2] + 0.587 * pccMatrixA->fCoeff[5] + 0.114 * pccMatrixA->fCoeff[8];
            pccMatrixB->fCoeff[3] = -0.1687 * pccMatrixA->fCoeff[0] - 0.3313 * pccMatrixA->fCoeff[3] + 0.5 * pccMatrixA->fCoeff[6];
            pccMatrixB->fCoeff[4] = -0.1687 * pccMatrixA->fCoeff[1] - 0.3313 * pccMatrixA->fCoeff[4] + 0.5 * pccMatrixA->fCoeff[7];
            pccMatrixB->fCoeff[5] = -0.1687 * pccMatrixA->fCoeff[2] - 0.3313 * pccMatrixA->fCoeff[5] + 0.5 * pccMatrixA->fCoeff[8];
            pccMatrixB->fCoeff[6] = 0.5 * pccMatrixA->fCoeff[0]  - 0.4187 * pccMatrixA->fCoeff[3] - 0.0813 * pccMatrixA->fCoeff[6];
            pccMatrixB->fCoeff[7] = 0.5 * pccMatrixA->fCoeff[1]  - 0.4187 * pccMatrixA->fCoeff[4] - 0.0813 * pccMatrixA->fCoeff[7];
            pccMatrixB->fCoeff[8] = 0.5 * pccMatrixA->fCoeff[2]  - 0.4187 * pccMatrixA->fCoeff[5] - 0.0813 * pccMatrixA->fCoeff[8];

            for(int i = 3; i < 9; i++)
            {
                 pccMatrixB->fCoeff[i] = flevel1 * pccMatrixB->fCoeff[i];
             }
            pccMatrixA->fCoeff[0] = 1 * pccMatrixB->fCoeff[0] + 0 * pccMatrixB->fCoeff[3] + 1.402 * pccMatrixB->fCoeff[6];
            pccMatrixA->fCoeff[1] = 1 * pccMatrixB->fCoeff[1] + 0 * pccMatrixB->fCoeff[4] + 1.402 * pccMatrixB->fCoeff[7];
            pccMatrixA->fCoeff[2] = 1 * pccMatrixB->fCoeff[2] + 0 * pccMatrixB->fCoeff[5] + 1.402 * pccMatrixB->fCoeff[8];
            pccMatrixA->fCoeff[3] = 1 * pccMatrixB->fCoeff[0] - 0.34414 * pccMatrixB->fCoeff[3]  - 0.71414 * pccMatrixB->fCoeff[6];
            pccMatrixA->fCoeff[4] = 1 * pccMatrixB->fCoeff[1] - 0.34414 * pccMatrixB->fCoeff[4]  - 0.71414 * pccMatrixB->fCoeff[7];
            pccMatrixA->fCoeff[5] = 1 * pccMatrixB->fCoeff[2] - 0.34414 * pccMatrixB->fCoeff[5]  - 0.71414 * pccMatrixB->fCoeff[8];
            pccMatrixA->fCoeff[6] = 1 * pccMatrixB->fCoeff[0]  + 1.772 * pccMatrixB->fCoeff[3] + 0 * pccMatrixB->fCoeff[6];
            pccMatrixA->fCoeff[7] = 1 * pccMatrixB->fCoeff[1]  + 1.772 * pccMatrixB->fCoeff[4] + 0 * pccMatrixB->fCoeff[7];
            pccMatrixA->fCoeff[8] = 1 * pccMatrixB->fCoeff[2]  + 1.772 * pccMatrixB->fCoeff[5] + 0 * pccMatrixB->fCoeff[8];


            for(int i =0; i < 9; i++)
            {
                if (i == 0 || i == 4 || i == 8){
                  pccMatrixA->fCoeff[i] = (pccMatrixA->fCoeff[i] - 1)/fScale+1;
                }
                else{
                  pccMatrixA->fCoeff[i] = pccMatrixA->fCoeff[i]/fScale;
                }
            }
        }

      }

}


XCamReturn CamCalibDbGetCcmProfileByName(const CalibDb_Ccm_ModeCell_t *calibCcm, char* name, const CalibDb_CcmMatrixProfile_t **pCcmMatrixProfile)
{
    LOG1_ACCM("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_ERROR_FAILED;

    for(int i = 0; i <calibCcm->matrixAllNum; i++) {
        if(strcmp(calibCcm->matrixAll[i].name, name) == 0) {
            *pCcmMatrixProfile = &calibCcm->matrixAll[i];
            ret = XCAM_RETURN_NO_ERROR;
            break;
        }
    }
    if(ret != XCAM_RETURN_NO_ERROR) {
        LOGE_ACCM("can't find %s in matrixAll \n", name);
    }
    LOG1_ACCM("%s: (exit)\n", __FUNCTION__);

    return ret;
}
static void UpdateDominateIlluList(List *l, int illu, int listMaxSize)
{
    illu_node_t *pCurNode;
    illu_node_t *pDelNode;
    int sizeList;
    if(listMaxSize == 0) {
        return;
    }
    pCurNode = (illu_node_t*)malloc(sizeof(illu_node_t));
    pCurNode->value = illu;
    ListPrepareItem(pCurNode);
    ListAddTail(l, pCurNode);
    sizeList = ListNoItems(l);
    if (sizeList > listMaxSize)
    {
        pDelNode = (illu_node_t *)ListRemoveHead(l);
        free(pDelNode);
    }
}

static void StableIlluEstimation(List l, int listSize, int illuNum, float varianceLuma, float varianceLumaTh, bool awbConverged, int preIllu, int *newIllu)
{
    int sizeList = ListNoItems(&l);
    if(sizeList < listSize || listSize == 0) {
        return;
    }
    /*if( awbConverged) {
        *newIllu = preIllu;
        LOGD_ALSC("awb is converged , reserve the last illumination(%d) \n", preIllu );
        return;
    }*/
    /*if( varianceLuma <= varianceLumaTh) {
        *newIllu = preIllu;
        LOGD_ACCM("varianceLuma %f < varianceLumaTh %f , reserve the last illumination(%d) \n", varianceLuma,varianceLumaTh,preIllu );
        return;
    }*/
    List *pNextNode = ListHead(&l);
    illu_node_t *pL;
    int *illuSet = (int*)malloc(illuNum*sizeof(int));
    memset(illuSet, 0, illuNum*sizeof(int));
    while (NULL != pNextNode)
    {
        pL = (illu_node_t*)pNextNode;
        illuSet[pL->value]++;
        pNextNode = pNextNode->p_next;
    }
    int count2 = 0;
    int max_count = 0;
    for(int i=0; i<illuNum; i++){
        LOGV_ACCM("illu(%d), count(%d)\n", i,illuSet[i]);
        if(illuSet[i] > max_count){
            max_count = illuSet[i];
            *newIllu = i;
        }
    }
    free(illuSet);
    LOGD_ACCM("varianceLuma %f, varianceLumaTh %f final estmination illu is %d\n", varianceLuma,varianceLumaTh,*newIllu );

}

XCamReturn AccmAutoConfig
(
    accm_handle_t hAccm,
    int mode_idx
) {

    LOG1_ACCM("%s: (enter) count:%d\n", __FUNCTION__, hAccm->count);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    const CalibDb_AccmCof_ill_t* pDomIlluProfile = NULL;
    const CalibDb_CcmMatrixProfile_t* pCcmProfile1 = NULL;
    const CalibDb_CcmMatrixProfile_t* pCcmProfile2 = NULL;

    if (hAccm == NULL) {
        return XCAM_RETURN_ERROR_PARAM;
    }

    int dominateIlluProfileIdx;
    int resIdx;
    const CalibDb_Ccm_t * pCcm = NULL;
    //1)
    int dominateIlluListSize = 15;//to do from xml;
    float varianceLumaTh = 0.006;//to do from xml;
    if (hAccm->mCurAtt.mode == RK_AIQ_CCM_MODE_TOOL){
        LOGD_ACCM("%s (%d): ACCM TOOL MODE !\n", __FUNCTION__, __LINE__);
        pCcm = &hAccm->mCurAtt.stTool;
        ret = pCcmMatrixAll_init(hAccm, &hAccm->mCurAtt.stTool.mode_cell[mode_idx], mode_idx);
        RETURN_RESULT_IF_DIFFERENT(ret, XCAM_RETURN_NO_ERROR);
    } else {
        LOGD_ACCM("%s (%d): ACCM AUTO MODE !\n", __FUNCTION__, __LINE__);
        pCcm = hAccm->calibCcm;
    }

    ret = illuminant_index_estimation_ccm(pCcm->mode_cell[mode_idx].aCcmCof.illuNum, pCcm->mode_cell[mode_idx].aCcmCof.illAll,
                                          hAccm->accmSwInfo.awbGain, &dominateIlluProfileIdx);
    RETURN_RESULT_IF_DIFFERENT(ret, XCAM_RETURN_NO_ERROR);
    UpdateDominateIlluList(&hAccm->accmRest.dominateIlluList, dominateIlluProfileIdx, dominateIlluListSize);
    StableIlluEstimation(hAccm->accmRest.dominateIlluList, dominateIlluListSize, pCcm->mode_cell[mode_idx].aCcmCof.illuNum,
        hAccm->accmSwInfo.varianceLuma ,varianceLumaTh, hAccm->accmSwInfo.awbConverged,
        hAccm->accmRest.dominateIlluProfileIdx, &dominateIlluProfileIdx);

    hAccm->accmRest.dominateIlluProfileIdx = dominateIlluProfileIdx;

    // 2)
    float sensorGain =  hAccm->accmSwInfo.sensorGain;
    float fSaturation;
    pDomIlluProfile = &pCcm->mode_cell[mode_idx].aCcmCof.illAll[dominateIlluProfileIdx];
    interpolation(pDomIlluProfile->saturationCurve.pSensorGain,
                  pDomIlluProfile->saturationCurve.pSaturation,
                  pDomIlluProfile->saturationCurve.arraySize,
                  sensorGain, &fSaturation);

    hAccm->accmRest.fSaturation =  fSaturation;
    hAccm->mCurAtt.finalSat = hAccm->accmRest.fSaturation;

    //3)
    ret = SatSelectCcmProfiles( hAccm->accmRest.fSaturation, pDomIlluProfile->matrixUsedNO, hAccm->pCcmMatrixAll[mode_idx][dominateIlluProfileIdx],
                                &pCcmProfile1, &pCcmProfile2);
    if (ret == XCAM_RETURN_NO_ERROR) {
        if (pCcmProfile1 && pCcmProfile2)
            LOGD_ACCM("final fSaturation: %f (%f .. %f)\n",   hAccm->accmRest.fSaturation, pCcmProfile1->saturation, pCcmProfile2->saturation);
        ret = SatInterpolateMatrices( hAccm->accmRest.fSaturation, pCcmProfile1, pCcmProfile2,
                                      &hAccm->accmRest.undampedCcmMatrix);

        ret = SatInterpolateOffset( hAccm->accmRest.fSaturation, pCcmProfile1, pCcmProfile2,
                                    &hAccm->accmRest.undampedCcOffset);
        if (ret != XCAM_RETURN_NO_ERROR) {
            return (ret);
        }
    } else if (ret == XCAM_RETURN_ERROR_OUTOFRANGE) {
        /* we don't need to interpolate */
        LOGD_ACCM("final fSaturation: %f (%f)\n",   hAccm->accmRest.fSaturation, pCcmProfile1->saturation);
        memcpy(&hAccm->accmRest.undampedCcmMatrix, &pCcmProfile1->CrossTalkCoeff, sizeof(Cam3x3FloatMatrix_t));
        memcpy(&hAccm->accmRest.undampedCcOffset, &pCcmProfile1->CrossTalkOffset, sizeof(Cam1x3FloatMatrix_t));
    } else {
        return (ret);
    }
       hAccm->accmRest.pCcmProfile1 = pCcmProfile1;
       hAccm->accmRest.pCcmProfile2 = pCcmProfile2;
       memcpy(&hAccm->mCurAtt.usedCcmProf1, hAccm->accmRest.pCcmProfile1, sizeof(CalibDb_CcmMatrixProfile_t));

       if (hAccm->accmRest.pCcmProfile2)
           memcpy(&hAccm->mCurAtt.usedCcmProf2, hAccm->accmRest.pCcmProfile2, sizeof(CalibDb_CcmMatrixProfile_t));
   //end

    //4) calc scale for y_alpha_curve
    float fScale = 1.0;
#if 0
    //real use
    interpolation(hAccm->calibCcm->luma_ccm.alpha_gain,
                  hAccm->calibCcm->luma_ccm.alpha_scale,
                  hAccm->calibCcm->luma_ccm.gain_scale_cure_size,
                  sensorGain, &fScale);
#else
    //for test, to be same with demo
    for( int i = 0; i < 9; i++)
    {
        int j = uint16_t(sensorGain);
        j = (j > (1 << 8)) ? (1 << 8) : j;

        if(j <= (1 << i))
        {
            fScale = pCcm->mode_cell[mode_idx].luma_ccm.alpha_scale[i];
            break;
        }
    }
#endif
   // 5) color inhibition adjust for api
    float flevel2;
    interpolation(hAccm->mCurAtt.stAuto.color_inhibition.sensorGain,
                  hAccm->mCurAtt.stAuto.color_inhibition.level,
                  RK_AIQ_ACCM_COLOR_GAIN_NUM,
                  sensorGain, &flevel2);
    hAccm->accmRest.color_inhibition_level = flevel2;
    for(int i = 0; i < RK_AIQ_ACCM_COLOR_GAIN_NUM; i++) {
        LOGV_ACCM(" color_inhibition sensorGain[%d]: %f, color_inhibition level[%d]: %f,", i,
                  hAccm->mCurAtt.stAuto.color_inhibition.sensorGain[i], i,
                  hAccm->mCurAtt.stAuto.color_inhibition.level[i]);
    }
    if(flevel2 > 100 || flevel2 < 0) {
        LOGE_ACCM("flevel2: %f is invalid ,should be in the range [0 100]\n",  flevel2);
        ret = XCAM_RETURN_ERROR_PARAM;
        return(ret);
    }
    LOGD_ACCM("fScale: %f , color inhibition level:%f\n",  fScale, flevel2);
    fScale *= (100 - flevel2) / 100;
    LOGD_ACCM("final fScale: %f \n", fScale);

   // 6)   saturation adjust for api
    float flevel1;
    interpolation(hAccm->mCurAtt.stAuto.color_saturation.sensorGain,
                  hAccm->mCurAtt.stAuto.color_saturation.level,
                  RK_AIQ_ACCM_COLOR_GAIN_NUM,
                  sensorGain, &flevel1);
    for(int i = 0; i < RK_AIQ_ACCM_COLOR_GAIN_NUM; i++) {
        LOGV_ACCM("color_saturation sensorGain[%d]: %f, color_saturation level[%d]: %f,", i,
                  hAccm->mCurAtt.stAuto.color_saturation.sensorGain[i], i,
                  hAccm->mCurAtt.stAuto.color_saturation.level[i]);
    }
    if(flevel1 > 100 || flevel1 < 0) {
        LOGE_ACCM("flevel1: %f is invalid ,should be in the range [0 100]\n",  flevel1);
        ret = XCAM_RETURN_ERROR_PARAM;
        return(ret);
    }
    hAccm->accmRest.color_saturation_level = flevel1;
    LOGD_ACCM("fSaturation: %f , color saturation level:%f\n",  fSaturation, flevel1);

     Saturationadjust( fScale,  hAccm );




      // 7) . Damping
    ret = Damping((pCcm->mode_cell[mode_idx].damp_enable && hAccm->count > 1) ? hAccm->accmSwInfo.awbIIRDampCoef : 0,
                  &hAccm->accmRest.undampedCcmMatrix, &hAccm->accmRest.dampedCcmMatrix,
                  &hAccm->accmRest.undampedCcOffset, &hAccm->accmRest.dampedCcOffset);

      // 8)  set to ic  to do bit check

    //hAccm->ccmHwConf.ccmEnable = hAccm->calibCcm->enable;
    memcpy(hAccm->ccmHwConf.matrix, &hAccm->accmRest.dampedCcmMatrix, sizeof(Cam3x3FloatMatrix_t));
    memcpy(hAccm->ccmHwConf.offs, &hAccm->accmRest.dampedCcOffset, sizeof(Cam1x3FloatMatrix_t));
    for(int i = 0; i < CCM_CURVE_DOT_NUM; i++) {
        hAccm->ccmHwConf.alp_y[i] = fScale * pCcm->mode_cell[mode_idx].luma_ccm.y_alpha_curve[i];
    }


    LOG1_ACCM("%s: (exit)\n", __FUNCTION__);

    return (ret);
}

XCamReturn AccmManualConfig
(
    accm_handle_t hAccm
) {
    LOG1_ACCM("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    memcpy(hAccm->ccmHwConf.matrix, hAccm->mCurAtt.stManual.matrix, sizeof(hAccm->mCurAtt.stManual.matrix));
    memcpy(hAccm->ccmHwConf.offs, hAccm->mCurAtt.stManual.offs, sizeof(hAccm->mCurAtt.stManual.offs));
    memcpy(hAccm->ccmHwConf.alp_y, hAccm->mCurAtt.stManual.alp_y, sizeof(hAccm->mCurAtt.stManual.alp_y));

    LOG1_ACCM("%s: (exit)\n", __FUNCTION__);
    return ret;

}

XCamReturn AccmConfig
(
    accm_handle_t hAccm
) {
    LOG1_ACCM("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    LOGD_ACCM("%s: updateAtt: %d\n", __FUNCTION__, hAccm->updateAtt);
    if(hAccm->updateAtt) {
        hAccm->mCurAtt.mode = hAccm->mNewAtt.mode;
        hAccm->mCurAtt.byPass = hAccm->mNewAtt.byPass;

        if (hAccm->mCurAtt.mode == RK_AIQ_CCM_MODE_TOOL)
        {
            hAccm->mCurAtt.byPass = !(hAccm->mNewAtt.stTool.enable);
            hAccm->mCurAtt.stTool = hAccm->mNewAtt.stTool;
            }
        else if (hAccm->mCurAtt.mode == RK_AIQ_CCM_MODE_AUTO)
            hAccm->mCurAtt.stAuto = hAccm->mNewAtt.stAuto;
        else
            hAccm->mCurAtt.stManual = hAccm->mNewAtt.stManual;
    }

    memcpy(hAccm->mCurAtt.curr_wbgain, hAccm->accmSwInfo.awbGain, sizeof(hAccm->accmSwInfo.awbGain));

    LOGD_ACCM("%s: byPass: %d  mode:%d \n", __FUNCTION__, hAccm->mCurAtt.byPass, hAccm->mCurAtt.mode);
    if(hAccm->mCurAtt.byPass != true && hAccm->accmSwInfo.grayMode != true) {
        hAccm->ccmHwConf.ccmEnable = true;

        CalibDb_CcmHdrNormalMode_t currentHdrNormalMode = hAccm->accmRest.currentHdrNormalMode;

        if((hAccm->mCurAtt.mode == RK_AIQ_CCM_MODE_AUTO)|| (hAccm->mCurAtt.mode == RK_AIQ_CCM_MODE_TOOL)){
            AccmAutoConfig(hAccm, currentHdrNormalMode);
        } else if(hAccm->mCurAtt.mode == RK_AIQ_CCM_MODE_MANUAL) {
            AccmManualConfig(hAccm);
        }
        else {
            LOGE_ACCM("%s: hAccm->mCurAtt.mode(%d) is invalid \n", __FUNCTION__, hAccm->mCurAtt.mode);
        }
        memcpy(hAccm->mCurAtt.stManual.matrix, hAccm->ccmHwConf.matrix, sizeof(hAccm->ccmHwConf.matrix));
        memcpy(hAccm->mCurAtt.stManual.offs, hAccm->ccmHwConf.offs, sizeof(hAccm->ccmHwConf.offs));
        memcpy(hAccm->mCurAtt.stManual.alp_y, hAccm->ccmHwConf.alp_y, sizeof(hAccm->ccmHwConf.alp_y));

    } else {
        hAccm->ccmHwConf.ccmEnable = false;
    }

    LOGD_ACCM( " set to ic ccmEnable :%d  bound_bit:%f\n", hAccm->ccmHwConf.ccmEnable, hAccm->ccmHwConf.bound_bit);

    LOGD_ACCM( " set to ic CcmMatrix: %f,%f,%f,%f,%f,%f,%f,%f,%f  CcOffset:%f,%f,%f  \n",
               hAccm->ccmHwConf.matrix[0],
               hAccm->ccmHwConf.matrix[1],
               hAccm->ccmHwConf.matrix[2],
               hAccm->ccmHwConf.matrix[3],
               hAccm->ccmHwConf.matrix[4],
               hAccm->ccmHwConf.matrix[5],
               hAccm->ccmHwConf.matrix[6],
               hAccm->ccmHwConf.matrix[7],
               hAccm->ccmHwConf.matrix[8],
               hAccm->ccmHwConf.offs[0],
               hAccm->ccmHwConf.offs[1],
               hAccm->ccmHwConf.offs[2]
             );

    LOGD_ACCM( " set to ic rgb2y_para: %f,%f,%f\n",
               hAccm->ccmHwConf.rgb2y_para[0],
               hAccm->ccmHwConf.rgb2y_para[1],
               hAccm->ccmHwConf.rgb2y_para[2]

             );

    LOGD_ACCM( " set to ic alp_y: %f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f  \n",
               hAccm->ccmHwConf.alp_y[0],
               hAccm->ccmHwConf.alp_y[1],
               hAccm->ccmHwConf.alp_y[2],
               hAccm->ccmHwConf.alp_y[3],
               hAccm->ccmHwConf.alp_y[4],
               hAccm->ccmHwConf.alp_y[5],
               hAccm->ccmHwConf.alp_y[6],
               hAccm->ccmHwConf.alp_y[7],
               hAccm->ccmHwConf.alp_y[8],
               hAccm->ccmHwConf.alp_y[9],
               hAccm->ccmHwConf.alp_y[10],
               hAccm->ccmHwConf.alp_y[11],
               hAccm->ccmHwConf.alp_y[12],
               hAccm->ccmHwConf.alp_y[13],
               hAccm->ccmHwConf.alp_y[14],
               hAccm->ccmHwConf.alp_y[15],
               hAccm->ccmHwConf.alp_y[16]
             );


    hAccm->count = ((hAccm->count + 2) > (65536)) ? 2 : (hAccm->count + 1);

    LOG1_ACCM("%s: (exit)\n", __FUNCTION__);
    return ret;

}
/**********************************
*Update CCM Calib Para
*      Prepare init
*      Mode change: reinit
*      Res change: continue
*      Calib change: continue
***************************************/
static XCamReturn UpdateCcmCalibPara(accm_handle_t hAccm, CalibDb_CcmHdrNormalMode_t currentHdrNormalMode)
{
    LOG1_ACCM("%s: (enter)  \n", __FUNCTION__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    bool config_calib = !!(hAccm->accmSwInfo.prepare_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB);
    bool config_calib_mode = !!(hAccm->accmSwInfo.prepare_type & RK_AIQ_ALGO_CONFTYPE_CHANGEMODE);
    const CalibDb_Ccm_t* calib_ccm = hAccm->calibCcm;

    if (!(config_calib_mode |config_calib))
    {
        return(ret);
    }
    else if(config_calib)
    {
        hAccm->mCurAtt.stTool = *calib_ccm;
    }
    if (!calib_ccm->mode_cell[currentHdrNormalMode].valid){
        LOGE_ACCM("%s: Current mode (%d) is not found in iq.  \n", __FUNCTION__, hAccm->accmSwInfo.hdr_mode);
        return XCAM_RETURN_ERROR_FAILED;
    }
    hAccm->accmRest.currentHdrNormalMode = currentHdrNormalMode;
    LOGI_ACCM("%s: accm illunum:%d", __FUNCTION__, calib_ccm->mode_cell[currentHdrNormalMode].aCcmCof.illuNum);
    for (int idx = 0; idx < hAccm->mCurAtt.stTool.modecellNum; idx++){
        ret = pCcmMatrixAll_init(hAccm, &hAccm->mCurAtt.stTool.mode_cell[idx], idx);
    }

    hAccm->ccmHwConf.bound_bit = hAccm->mCurAtt.stTool.mode_cell[currentHdrNormalMode].luma_ccm.low_bound_pos_bit;
    memcpy( hAccm->ccmHwConf.rgb2y_para, hAccm->mCurAtt.stTool.mode_cell[currentHdrNormalMode].luma_ccm.rgb2y_para,
            sizeof(hAccm->mCurAtt.stTool.mode_cell[currentHdrNormalMode].luma_ccm.rgb2y_para));

    hAccm->mCurAtt.byPass = !(calib_ccm->enable);
    ClearList(&hAccm->accmRest.dominateIlluList);

#if 0 //awbGain get from awb module
    ret = Swinfo_wbgain_init(hAccm->accmSwInfo.awbGain, &hAccm->mCurAtt.stTool.mode_cell[currentHdrNormalMode], hAccm->lsForFirstFrame);
#endif

    LOGI_ACCM("%s: (exit)\n", __FUNCTION__);
    return(ret);
}


XCamReturn AccmInit(accm_handle_t *hAccm, const CamCalibDbContext_t* calib)
{
    LOGI_ACCM("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    *hAccm = (accm_context_t*)malloc(sizeof(accm_context_t));
    accm_context_t* accm_context = *hAccm;
    memset(accm_context, 0, sizeof(accm_context_t));

    if(calib == NULL) {
        return XCAM_RETURN_ERROR_FAILED;
    }
    const CalibDb_Ccm_t *calib_ccm = &calib->ccm;
    accm_context->calibCcm = calib_ccm;
    //accm_context->mCurAtt.stTool = calib->ccm;
    accm_context->accmSwInfo.sensorGain = 1.0;
    accm_context->accmSwInfo.awbIIRDampCoef = 0;
    accm_context->accmSwInfo.varianceLuma = 255;
    accm_context->accmSwInfo.awbConverged = false;

    accm_context->count = 0;
    accm_context->mCurAtt.mode = RK_AIQ_CCM_MODE_AUTO;
    CalibDb_CcmHdrNormalMode_t currentHdrNormalMode = CCM_FOR_MODE_NORMAL;

    if (accm_context->accmSwInfo.hdr_mode== RK_AIQ_WORKING_MODE_NORMAL){
        currentHdrNormalMode = CCM_FOR_MODE_NORMAL;
    }else if(accm_context->accmSwInfo.hdr_mode > RK_AIQ_WORKING_MODE_NORMAL && accm_context->accmSwInfo.hdr_mode <= RK_AIQ_ISP_HDR_MODE_3_LINE_HDR){
        currentHdrNormalMode = CCM_FOR_MODE_HDR;
    }else{
        LOGE_ACCM("%s: Current hdr mode (%d) is invalid!Defaults to normal mode.  \n", __FUNCTION__, accm_context->accmSwInfo.hdr_mode);
        currentHdrNormalMode = CCM_FOR_MODE_NORMAL;
    }
    accm_context->accmSwInfo.prepare_type = RK_AIQ_ALGO_CONFTYPE_UPDATECALIB | RK_AIQ_ALGO_CONFTYPE_NEEDRESET |RK_AIQ_ALGO_CONFTYPE_CHANGEMODE;
    ret = UpdateCcmCalibPara(accm_context, currentHdrNormalMode);
    LOGI_ACCM("%s: Initial Mode is %s (%d)\n", __FUNCTION__, accm_context->calibCcm->mode_cell[accm_context->accmRest.currentHdrNormalMode].name, accm_context->accmRest.currentHdrNormalMode);
    for(int i = 0; i < RK_AIQ_ACCM_COLOR_GAIN_NUM; i++) {
            accm_context->mCurAtt.stAuto.color_inhibition.sensorGain[i] = 1;
            accm_context->mCurAtt.stAuto.color_inhibition.level[i] = 0;
            accm_context->mCurAtt.stAuto.color_saturation.sensorGain[i] = 1;
            accm_context->mCurAtt.stAuto.color_saturation.level[i] = 50;
        }
    accm_context->accmRest.color_inhibition_level = 0;
    accm_context->accmRest.color_saturation_level = 100;

    LOGI_ACCM("%s: (exit)\n", __FUNCTION__);
    return(ret);
}

XCamReturn AccmRelease(accm_handle_t hAccm)
{
    LOGI_ACCM("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ClearList(&hAccm->accmRest.dominateIlluList);
    free(hAccm);

    LOGI_ACCM("%s: (exit)\n", __FUNCTION__);
    return(ret);

}

XCamReturn AccmPrepare(accm_handle_t hAccm)
{
    LOGI_ACCM("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    CalibDb_CcmHdrNormalMode_t currentHdrNormalMode = CCM_FOR_MODE_NORMAL;

    if (hAccm->accmSwInfo.hdr_mode== RK_AIQ_WORKING_MODE_NORMAL){
        currentHdrNormalMode = CCM_FOR_MODE_NORMAL;
    }else if(hAccm->accmSwInfo.hdr_mode > RK_AIQ_WORKING_MODE_NORMAL && hAccm->accmSwInfo.hdr_mode <= RK_AIQ_ISP_HDR_MODE_3_LINE_HDR){
        currentHdrNormalMode = CCM_FOR_MODE_HDR;

    }else{
        LOGE_ACCM("%s: Current hdr mode (%d) is invalid!Defaults to normal mode.  \n", __FUNCTION__, hAccm->accmSwInfo.hdr_mode);
        currentHdrNormalMode = CCM_FOR_MODE_NORMAL;
    }
    if (hAccm->accmRest.currentHdrNormalMode != currentHdrNormalMode)
        hAccm->accmSwInfo.prepare_type = hAccm->accmSwInfo.prepare_type |RK_AIQ_ALGO_CONFTYPE_CHANGEMODE;
    ret = UpdateCcmCalibPara(hAccm, currentHdrNormalMode);

    LOGI_ACCM("%s: Current Mode is %s (%d)\n", __FUNCTION__, hAccm->calibCcm->mode_cell[hAccm->accmRest.currentHdrNormalMode].name, hAccm->accmRest.currentHdrNormalMode);

    LOGI_ACCM("%s: (exit)\n", __FUNCTION__);
    return ret;
}
XCamReturn AccmPreProc(accm_handle_t hAccm)
{

    LOG1_ACCM("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    LOG1_ACCM("%s: (exit)\n", __FUNCTION__);
    return(ret);

}
XCamReturn AccmProcessing(accm_handle_t hAccm)
{
    LOG1_ACCM("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;


    LOG1_ACCM("%s: (exit)\n", __FUNCTION__);
    return(ret);
}




RKAIQ_END_DECLARE


