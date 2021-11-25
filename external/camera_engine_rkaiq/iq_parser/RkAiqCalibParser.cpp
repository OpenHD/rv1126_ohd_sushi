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
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "RkAiqCalibParser.h"
#include "calibtags.h"
#include "xmltags.h"
#include <fstream>
#include <iostream>

using namespace std;
namespace RkCam {

#define MEMSET( TARGET, C, LEN) memset(TARGET,C,LEN)
#define MEMCPY( DST, SRC, LEN)  memcpy(DST,SRC,LEN)

//#define DEBUG_LOG
//#define INFO_LOG

#ifdef INFO_LOG
#define INFO_PRINT(TAG_NAME) LOG1("[Line:%d, Tag: %s]\n",__LINE__,TAG_NAME.c_str())
#else
#define INFO_PRINT(TAG_NAME)
#endif

#define redirectOut std::cout

/******************************************************************************
* Toupper
*****************************************************************************/
char* Toupper(const char* s) {
    int i = 0;
    char* p = (char*)s;
    char tmp;
    int len = 0;

    if (p) {
        len = strlen(p);
        for (i = 0; i < len; i++) {
            tmp = p[i];

            if (tmp >= 'a' && tmp <= 'z')
                tmp = tmp - 32;

            p[i] = tmp;
        }
    }
    return p;
}

int RkAiqCalibParser::ParseCharToHex
(
    XmlTag*  tag,          /**< trimmed c string */
    uint32_t*    reg_value
) {
    bool ok;

    *reg_value = tag->ValueToUInt(&ok);
    if (!ok) {
        LOGE( "%s(%d): parse error: invalid register value:\n", __FUNCTION__, __LINE__, tag->Value());
        return (false);
    } else {
#ifdef DEBUG_LOG
        LOGD( "%s(%d): parse reg valevalue:\n", __FUNCTION__, __LINE__, *reg_value);
#endif
    }

    return 0;
}

void RkAiqCalibParser::parseReadWriteCtrl(bool ctrl)
{
    xmlParseReadWrite = ctrl;
}

/******************************************************************************
* ParseFloatArray
*****************************************************************************/
int RkAiqCalibParser::ParseFloatArray
(
    const XMLNode*  pNode,          /**< trimmed c string */
    float*       values,            /**< pointer to memory */
    const int   num,                /**< number of expected float values */
    int  printAccuracy              /**< accuracy of XML write */
) {

    if (!xmlParseReadWrite) // read
    {
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        float* value = values;
        char* str = (char*)c_string;
        int last = strlen(str);
        /* calc. end adress of string */
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {
#ifdef DEBUG_LOG
            LOGD( "%s(%d): start=%d,end=%d\n", __FUNCTION__, __LINE__, find_start, find_end);
#endif
            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        float f;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%f", &f);
            if (scanned != 1) {
                LOGE( "%s(%d): %f err\n", __FUNCTION__, __LINE__, f);
                goto err1;
            }
            else {
                value[cnt] = f;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }
        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%f,\n", value[i]);
        }
#endif

        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        char cmd[16] = {"%s%.0f "};
        if (printAccuracy > 9)
            printAccuracy = 9;
        cmd[4] = '0' + printAccuracy;
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();
        char str[8192];
        char* strp = str;
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++)
                snprintf(str, sizeof(str), cmd, strp, values[i * cols + k]);
            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);

        autoTabBackward();
        return (num);
    }



err1:
    MEMSET(values, 0, (sizeof(float)* num));

    return (0);

}

/******************************************************************************
* ParseDoubleArray
*****************************************************************************/
int RkAiqCalibParser::ParseDoubleArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    double*       values,            /**< pointer to memory */
    const int    num                 /**< number of expected float values */
) {
    if (xmlParseReadWrite == XML_PARSER_READ) // read
    {
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        double* value = values;
        char* str = (char*)c_string;
        int last = strlen(str);
        /* calc. end adress of string */
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {
#ifdef DEBUG_LOG
            LOGD( "%s(%d): start=%d,end=%d\n", __FUNCTION__, __LINE__, find_start, find_end);
#endif
            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        double d;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%lf", &d);
            if (scanned != 1) {
                LOGE( "%s(%d): %f err\n", __FUNCTION__, __LINE__, d);
                goto err2;
            }
            else {
                value[cnt] = d;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }
        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%f,\n", value[i]);
        }
#endif

        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();
        char str[8192];
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        char tmp_val_str[64];
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++) {
                snprintf(tmp_val_str, sizeof(tmp_val_str), "%.16e ", values[i * cols + k]);
                strcat(str, tmp_val_str);
            }
            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        autoTabBackward();

        return (num);
    }


err2:
    MEMSET(values, 0, (sizeof(float)* num));

    return (0);

}

/******************************************************************************
* ParseUintArray
*****************************************************************************/
int RkAiqCalibParser::ParseUintArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    uint32_t*      values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
) {
    if (!xmlParseReadWrite) // read
    {
        uint32_t* value = values;
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        char* str = (char*)c_string;

        int last = strlen(str);
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {
#ifdef DEBUG_LOG
            LOGE( "%s(%d): start=%d,end=%d\n", __FUNCTION__, __LINE__, find_start, find_end);
#endif
            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;

        /* calc. end adress of string */
        // char *str_last = str + (last-1);

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        uint32_t f;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%u", &f);
            if (scanned != 1) {
                LOGE( "%s(%d): f:%f error\n", __FUNCTION__, __LINE__, f);
                goto err1;
            }
            else {
                value[cnt] = f;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }
        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%u,\n", value[i]);
        }
#endif
        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();
        char str[8192];
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        char tmp_val_str[25];
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++) {
                snprintf(tmp_val_str, sizeof(tmp_val_str), "%d ", values[i * cols + k]);
                strcat(str, tmp_val_str);
            }
            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        autoTabBackward();

        return num;
    }


err1:
    MEMSET(values, 0, (sizeof(uint32_t)* num));

    return (0);
}

/******************************************************************************
* ParseIntArray
*****************************************************************************/
int RkAiqCalibParser::ParseIntArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    int32_t*      values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
) {

    if (!xmlParseReadWrite) // read
    {
        int32_t* value = values;
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        char* str = (char*)c_string;

        int last = strlen(str);
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {

            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;

        /* calc. end adress of string */
        // char *str_last = str + (last-1);

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        int32_t f;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%d", &f);
            if (scanned != 1) {
                LOGE( "%s(%d): f:%f error\n", __FUNCTION__, __LINE__, f);
                goto err1;
            }
            else {
                value[cnt] = f;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }
        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%d,\n", value[i]);
        }
#endif
        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();
        char str[8192];
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        char tmp_val_str[25];
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++) {
                snprintf(tmp_val_str, sizeof(tmp_val_str), "%d ", values[i * cols + k]);
                strcat(str, tmp_val_str);
            }

            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        autoTabBackward();

        return num;
    }


err1:
    MEMSET(values, 0, (sizeof(int32_t)* num));

    return (0);
}

/******************************************************************************
* ParseUchartArray//cxf
*****************************************************************************/
int RkAiqCalibParser::ParseUcharArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    uint8_t*     values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
) {

    if (!xmlParseReadWrite) // read
    {
        uint8_t* value = values;
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        char* str = (char*)c_string;

        int last = strlen(str);
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {

            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;


        /* calc. end adress of string */
        // char *str_last = str + (last-1);

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        uint16_t f;  // uint8_t f;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%hu", &f);  // %hhu
            if (scanned != 1) {
                LOGD( "%s(%d):f:%f\n", __FUNCTION__, __LINE__, f);
                goto err1;
            }
            else {
                value[cnt] = f;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }

        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%u,\n", value[i]);
        }
#endif

        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();
        char str[8192];
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        char tmp_val_str[25];
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++) {
                snprintf(tmp_val_str, sizeof(tmp_val_str), "%d ", values[i * cols + k]);
                strcat(str, tmp_val_str);
            }

            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        autoTabBackward();

        return num;
    }

err1:
    MEMSET(values, 0, (sizeof(uint8_t)* num));

    return (0);
}
/******************************************************************************
* ParseUchartArray
*****************************************************************************/
int RkAiqCalibParser::ParseCharArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    int8_t*      values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
) {

    XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();

    if (!xmlParseReadWrite) // read
    {
        int8_t*  value = values;
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        char* str = (char*)c_string;

        int last = strlen(str);
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {
#ifdef DEBUG_LOG
            LOGE( "%s(%d): start=%d,end=%d\n", __FUNCTION__, __LINE__, find_start, find_end);
#endif
            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;

        /* calc. end adress of string */
        // char *str_last = str + (last-1);

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        int16_t f;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%hd", &f);
            if (scanned != 1) {
                LOGE("%s(%d): error", __FUNCTION__, __LINE__);
                goto err1;
            }
            else {
                value[cnt] = f;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }
        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%d,\n", value[i]);
        }
#endif
        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();

        char str[8192];
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        char tmp_val_str[25];
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++) {
                snprintf(tmp_val_str, sizeof(tmp_val_str), "%d ", values[i * cols + k]);
                strcat(str, tmp_val_str);
            }

            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        autoTabBackward();

        return num;
    }

err1:
    MEMSET(values, 0, (sizeof(uint8_t)* num));

    return (0);
}


/******************************************************************************
* ParseUshortArray
*****************************************************************************/
int RkAiqCalibParser::ParseUshortArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    uint16_t*    values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
) {

    XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();

    if (!xmlParseReadWrite) // read
    {
        uint16_t* value = values;
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        char* str = (char*)c_string;
        int last = strlen(str);
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {
#ifdef DEBUG_LOG
            LOGE( "%s(%d): parse error! start:%s end:%s\n",
                  __FUNCTION__, __LINE__, find_start, find_end);
#endif
            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;


        /* calc. end adress of string */
        //char *str_last = str + (last-1);

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        uint16_t f;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%hu", &f);
            if (scanned != 1) {
                LOGD( "%s(%d): parse error!\n", __FUNCTION__, __LINE__);
                goto err1;
            }
            else {
                value[cnt] = f;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }
        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%u,\n", value[i]);
        }
#endif
        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();
        char str[8192];
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        char tmp_val_str[25];
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++) {
                snprintf(tmp_val_str, sizeof(tmp_val_str), "%d ", values[i * cols + k]);
                strcat(str, tmp_val_str);
            }

            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        autoTabBackward();

        return num;
    }


err1:
    MEMSET(values, 0, (sizeof(uint16_t)* num));

    return (0);

}


/******************************************************************************
* ParseShortArray
*****************************************************************************/
int RkAiqCalibParser::ParseShortArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    int16_t*     values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
) {
    XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();

    if (!xmlParseReadWrite) // read
    {
        int16_t* value = values;
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        char* str = (char*)c_string;

        int last = strlen(str);
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {
#ifdef DEBUG_LOG
            LOGE( "%s(%d): parse error!\n", __FUNCTION__, __LINE__);
#endif
            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;

        /* calc. end adress of string */
        // char *str_last = str + (last-1);

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        int16_t f;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%hd", &f);
            if (scanned != 1) {
                LOGE( "%s(%d): parse error!\n", __FUNCTION__, __LINE__);
                goto err1;
            }
            else {
                value[cnt] = f;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }
        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%d,\n", value[i]);
        }
#endif
        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();
        char str[8192];
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        char tmp_val_str[25];
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++) {
                snprintf(tmp_val_str, sizeof(tmp_val_str), "%d ", values[i * cols + k]);
                strcat(str, tmp_val_str);
            }

            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        autoTabBackward();

        return num;
    }


err1:
    MEMSET(values, 0, (sizeof(uint16_t)* num));

    return (0);

}


/******************************************************************************
* ParseByteArray
*****************************************************************************/
int RkAiqCalibParser::ParseByteArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    uint8_t*     values,            /**< pointer to memory */
    const int   num                 /**< number of expected float values */
) {

    XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();

    if (!xmlParseReadWrite) // read
    {
        uint8_t* value = values;
        const char* c_string = XmlTag(pNode->ToElement()).Value();

        char* str = (char*)c_string;
        int last = strlen(str);
        char* str_last = str + (last - 1);

        std::string s_string(str);
        size_t find_start = s_string.find("[", 0);
        size_t find_end = s_string.find("]", 0);

        if ((find_start == std::string::npos) || (find_end == std::string::npos)) {
#ifdef DEBUG_LOG
            LOGE( "%s(%d): parse error!\n", __FUNCTION__, __LINE__);
#endif
            return -1;
        }

        str = (char*)c_string + find_start;
        str_last = (char*)c_string + find_end;

        /* calc. end adress of string */
        //char *str_last = str + (last-1);

        /* skipped left parenthesis */
        str++;

        /* skip spaces */
        while (*str == 0x20 || *str == 0x09 || (*str == 0x0a) || (*str == 0x0d)) {
            str++;
        }

        int cnt = 0;
        int scanned;
        uint16_t f;

        /* parse the c-string */
        while ((str != str_last) && (cnt < num)) {
            scanned = sscanf(str, "%hu", &f);
            if (scanned != 1) {
                LOGE( "%s(%d): parse error!\n", __FUNCTION__, __LINE__);
                goto err1;
            }
            else {
                value[cnt] = (uint8_t)f;
                cnt++;
            }

            /* remove detected float */
            while ((*str != 0x20) && (*str != 0x09) && (*str != ',') && (*str != ']')) {
                str++;
            }

            /* skip spaces and comma */
            while ((*str == 0x20) || (*str == ',') || (*str == 0x09) || (*str == 0x0a) || (*str == 0x0d)) {
                str++;
            }
        }

#ifdef DEBUG_LOG
        for (int i = 0; i < cnt; i++) {
            LOGD( "%u,\n", value[i]);
        }
#endif
        return (cnt);

    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        int cols = XmlTag(pNode->ToElement()).SizeCol();
        int rows = XmlTag(pNode->ToElement()).SizeRow();
        char str[8192];
        snprintf(str, sizeof(str), "\n%s[", autoTabStr);
        char tmp_val_str[25];
        for (int i = 0; i < rows; i++)
        {
            for (int k = 0; k < cols; k++) {
                snprintf(tmp_val_str, sizeof(tmp_val_str), "%d ", values[i * cols + k]);
                strcat(str, tmp_val_str);
            }

            if (i < rows - 1) {
                strcat(str, "\n");
                strcat(str, autoTabStr);
            }
        }
        autoTabBackward();
        strcat(str, "]\n");
        strcat(str, autoTabStr);

        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        autoTabBackward();
        return num;
    }

err1:
    MEMSET(values, 0, (sizeof(uint8_t)* num));

    return (0);

}


/******************************************************************************
* ParseString
*****************************************************************************/
int RkAiqCalibParser::ParseString
(
    const XMLNode *pNode,          /**< trimmed c string */
    char*        values,            /**< pointer to memory */
    const int    size                 /**< size of memory */
) {
    if (xmlParseReadWrite == XML_PARSER_READ) // read
    {
        const char* c_string = XmlTag(pNode->ToElement()).Value();
        char* str = (char*)c_string;
        int last = strlen(str);
        char* str_last = str + (last - 1);

        if (*str == '[')
            str += 1;
        if (*str_last == ']')
            *str_last = '\0';

        memset(values, 0, size);
        strncpy(values, str, size - 1);
        //values[size] = '';

        return 0;
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE)
    {
        char str[8192];
        //sprintf(str, "[%s]", values);
        autoTabForward();
        autoTabForward();
        snprintf(str, sizeof(str), "\n%s%s", autoTabStr, values);
        autoTabBackward();
        strcat(str, "\n");
        strcat(str, autoTabStr);
        autoTabBackward();
        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);
        snprintf(str, sizeof(str), "[1 %u]", (unsigned int)(strlen(values)));
        XMLElement* pElement = (((XMLNode*)pNode)->ToElement());
        pElement->SetAttribute(CALIB_ATTRIBUTE_SIZE, str);
        return 0;
    }
    return 0;
}


/******************************************************************************
* ParseLscProfileArray
*****************************************************************************/
int RkAiqCalibParser::ParseLscProfileArray
(
    const XMLNode *pNode,          /**< trimmed c string */
    CalibDb_Lsc_ProfileName_t values[],           /**< pointer to memory */
    const int           num                 /**< number of expected float values */
) {

    if (!xmlParseReadWrite) // read
    {
        for (int i = 0; i < LSC_PROFILES_NUM_MAX; i++)
            memset(values[i], 0, LSC_PROFILE_NAME);

        const char *c_string = XmlTag(pNode->ToElement()).Value();

        Toupper(c_string);
        char *str = (char *)c_string;

        int last = strlen(str);

        /* calc. end adress of string */
        char *str_last = str + (last - 1);

        /* skip beginning spaces */
        while (*str == 0x20 || *str == 0x09)
        {
            str++;
        }

        /* skip ending spaces */
        while (*str_last == 0x20 || *str_last == 0x09)
        {
            str_last--;
        }

        int cnt = 0;
        int scanned;
        CalibDb_Lsc_ProfileName_t f;
        memset(f, 0, sizeof(f));

        /* parse the c-string */
        while ((str != str_last) && (cnt < num))
        {
            scanned = sscanf(str, "%s", f);
            if (scanned != 1)
            {
                LOGE("%s(%d): parse error!\n", __FUNCTION__, __LINE__);
                goto err1;
            }
            else
            {
                strncpy(values[cnt], f, strlen(f));
                values[cnt][strlen(f)] = '\0';
                cnt++;
            }

            /* remove detected string */
            while ((*str != 0x20) && (*str != ',') && (*str != ']') && (str != str_last))
            {
                str++;
            }

            if (str != str_last)
            {
                /* skip spaces and comma */
                while ((*str == 0x20) || (*str == ','))
                {
                    str++;
                }
            }

            memset(f, 0, sizeof(f));
        }

        return (cnt);
    }
    else if (xmlParseReadWrite == XML_PARSER_WRITE) // write
    {
        autoTabForward();
        autoTabForward();
        char str[8192];
        snprintf(str, sizeof(str), "\n%s", autoTabStr);
        int i = 0;
        size_t str_len = 0;
        while (values[i][0] != 0)
        {
            str_len += strlen(values[i]);
            strcat(str, values[i++]);
            strcat(str, " ");
            str_len++;
        }
        autoTabBackward();
        strcat(str, "\n");
        strcat(str, autoTabStr);
        XMLNode* pComment = (XMLNode*)pNode->ToElement()->FirstChild();
        pComment->SetValue((const char *)str);

        snprintf(str, sizeof(str), "[1 %u]", (unsigned int)(str_len - 1));
        pComment = (XMLNode *)pNode;
        pComment->ToElement()->SetAttribute(CALIB_ATTRIBUTE_SIZE, str);
        autoTabBackward();

        return num;
    }



err1:
    memset(values, 0, (sizeof(uint16_t)* num));

    return (0);
}

void RkAiqCalibParser::autoTabForward()
{
    if (autoTabIdx == 124)
        return;
    autoTabStr[autoTabIdx] = ' ';
    autoTabIdx += 4;
    autoTabStr[autoTabIdx] = '\0';
}

void RkAiqCalibParser::autoTabBackward()
{
    if (autoTabIdx == 0)
        return;
    autoTabStr[autoTabIdx] = ' ';
    autoTabIdx -= 4;
    autoTabStr[autoTabIdx] = '\0';
}

RkAiqCalibParser::RkAiqCalibParser(CamCalibDbContext_t *pCalibDb)
{
    mCalibDb = pCalibDb;
    xmlParseReadWrite = XML_PARSER_READ;
    memset(autoTabStr, ' ', sizeof(autoTabStr));
    autoTabIdx = 0;
    autoTabStr[0] = '\0';
}

RkAiqCalibParser::~RkAiqCalibParser()
{

}

bool RkAiqCalibParser::doParse
(
    const char* device
)
{
    int errorID;
    XMLDocument doc;

    bool res = true;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    errorID = doc.LoadFile(device);

    LOGD("%s(%d): doc.LoadFile filename:%s  errorID:%d\n",
         __FUNCTION__, __LINE__, device, errorID);

    if (doc.Error()) {
        LOGE("%s(%d): Error: Parse error errorID %d\n",
             __FUNCTION__, __LINE__, errorID);
        return (false);
    }
    XMLElement* proot = doc.RootElement();
    std::string tagname(proot->Name());
    if (tagname != CALIB_FILESTART_TAG) {
        LOGE("Error: Not a calibration data file");

        return (false);
    }
    // parse Read Mode
    parseReadWriteCtrl(XML_PARSER_READ);

    XML_CHECK_START(CALIB_FILESTART_TAG_ID, CALIB_FILESTART_TAG_ID);


    // parse header section
    XMLElement* pheader = proot->FirstChildElement(TAG_NAME(CALIB_HEADER_TAG_ID));
    XmlTag tag = XmlTag(pheader);
    if (pheader) {
        res = parseEntryHeader(pheader->ToElement(), NULL);
        if (!res) {
            return (res);
        }
        XML_CHECK_TOPTAG_MARK(CALIB_HEADER_TAG_ID, tag.Type(), tag.Size());
    }

    // parse sensor section
    XMLElement* psensor = proot->FirstChildElement(TAG_NAME(CALIB_SENSOR_TAG_ID));
    XmlTag tag1 = XmlTag(psensor);
    if (psensor) {
        res = parseEntrySensor(psensor->ToElement(), NULL);
        if (!res) {
            return (res);
        }
        XML_CHECK_TOPTAG_MARK(CALIB_SENSOR_TAG_ID, tag1.Type(), tag1.Size());
    }

    // parse system section
    XMLElement* psystem = proot->FirstChildElement(TAG_NAME(CALIB_SYSTEM_TAG_ID));
    XmlTag tag2 = XmlTag(psystem);
    if (psystem) {
        res = parseEntrySystem(psystem->ToElement(), NULL);
        if (!res) {
            return (res);
        }
        XML_CHECK_TOPTAG_MARK(CALIB_SYSTEM_TAG_ID, tag2.Type(), tag2.Size());
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);

    return (res);
}

bool RkAiqCalibParser::doGenerate
(
    const char* deviceRef,
    const char* deviceOutput
)
{
    int errorID;
    XMLDocument doc;

    bool res = true;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    errorID = doc.LoadFile(deviceRef);

    LOGD("%s(%d): doc.LoadFile filename:%s  errorID:%d\n",
         __FUNCTION__, __LINE__, deviceRef, errorID);

    if (doc.Error()) {
        LOGE("%s(%d): Error: Parse error errorID %d\n",
             __FUNCTION__, __LINE__, errorID);
        return (false);
    }
    XMLElement* proot = doc.RootElement();
    std::string tagname(proot->Name());
    if (tagname != CALIB_FILESTART_TAG) {
        LOGE("Error: Not a calibration data file");

        return (false);
    }
    // parse Read Mode
    parseReadWriteCtrl(XML_PARSER_WRITE);

    XML_CHECK_START(CALIB_FILESTART_TAG_ID, CALIB_FILESTART_TAG_ID);

    // parse header section
    XMLElement* pheader = proot->FirstChildElement(TAG_NAME(CALIB_HEADER_TAG_ID));
    XmlTag tag = XmlTag(pheader);
    if (pheader) {
        res = parseEntryHeader(pheader->ToElement(), NULL);
        if (!res) {
            return (res);
        }
        XML_CHECK_TOPTAG_MARK(CALIB_HEADER_TAG_ID, tag.Type(), tag.Size());
    }

    // parse sensor section
    XMLElement* psensor = proot->FirstChildElement(TAG_NAME(CALIB_SENSOR_TAG_ID));
    XmlTag tag1 = XmlTag(psensor);
    if (psensor) {
        res = parseEntrySensor(psensor->ToElement(), NULL);
        if (!res) {
            return (res);
        }
        XML_CHECK_TOPTAG_MARK(CALIB_SENSOR_TAG_ID, tag1.Type(), tag1.Size());
    }

    // parse system section
    XMLElement* psystem = proot->FirstChildElement(TAG_NAME(CALIB_SYSTEM_TAG_ID));
    XmlTag tag2 = XmlTag(psystem);
    if (psystem) {
        res = parseEntrySystem(psystem->ToElement(), NULL);
        if (!res) {
            return (res);
        }
        XML_CHECK_TOPTAG_MARK(CALIB_SYSTEM_TAG_ID, tag2.Type(), tag2.Size());
    }

    XML_CHECK_END();

    errorID = doc.SaveFile(deviceOutput);

    LOGD("%s(%d): doc.SaveFile filename:%s  errorID:%d\n",
         __FUNCTION__, __LINE__, deviceRef, errorID);

    if (doc.Error()) {
        LOGE("%s(%d): Error: Generate error ID %d\n",
             __FUNCTION__, __LINE__, errorID);
        return (false);
    }

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);

    return (res);
}

bool RkAiqCalibParser::parseCellNoElement
(
    const XMLElement*   pelement,
    int                 noElements,
    int                 &RealNo
) {
    int cnt = 0;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild && (cnt < noElements)) {
        pchild = pchild->NextSibling();
        cnt ++;
    }

    RealNo = cnt;

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);

    return true;
}

bool RkAiqCalibParser::parseEntryCell
(
    const XMLElement*   pelement,
    int                 noElements,
    parseCellContent    func,
    void*                param,
    uint32_t     cur_id,
    uint32_t     parent_id
) {
    int cnt = 0;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    int cell_size = 0;
    CALIB_IQ_TAG_ID_T cur_tag_id = (CALIB_IQ_TAG_ID_T)cur_id;
    CALIB_IQ_TAG_ID_T parent_tag_id = (CALIB_IQ_TAG_ID_T)parent_id;
    parseCellNoElement(pelement, noElements, cell_size);
    XML_CHECK_CELL_SET_SIZE(cell_size);
    if(cell_size != noElements) {
        LOGD("%s(%d): Warning: parent_tagname:%s tag_name:%s define %d cell, but only use %d cells !!!!\n",
             __FUNCTION__, __LINE__,
             TAG_NAME(parent_tag_id), TAG_NAME(cur_tag_id),
             noElements, cell_size);
    }

    LOGD("####@@@@@ cur_tag_id %d parent_tag_id %d cell_size %d\n",
         cur_tag_id, parent_tag_id, cell_size);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild && (cnt < noElements)) {
        XmlCellTag tag = XmlCellTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        if (tagname == TAG_NAME(CALIB_CELL_TAG_ID)) {
            autoTabForward();
            bool result = (this->*func)(pchild->ToElement(), /*param*/(void*)&cnt);
            autoTabBackward();
            if (!result) {
                return (result);
            }
        }
        else {
            LOGW("unknown cell tag: %s", tagname.c_str());

            //return (false);
        }

        pchild = pchild->NextSibling();
        cnt++;
    }

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);
}

bool RkAiqCalibParser::parseEntryCell2
(
    const XMLElement*   pelement,
    int                 noElements,
    parseCellContent2    func,
    void*                param,
    uint32_t     cur_id,
    uint32_t     parent_id
) {
    int cnt = 0;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    int cell_size = 0;
    CALIB_IQ_TAG_ID_T cur_tag_id = (CALIB_IQ_TAG_ID_T)cur_id;
    CALIB_IQ_TAG_ID_T parent_tag_id = (CALIB_IQ_TAG_ID_T)parent_id;
    parseCellNoElement(pelement, noElements, cell_size);
    XML_CHECK_CELL_SET_SIZE(cell_size);
    if(cell_size != noElements) {
        LOGD("%s(%d): Warning: parent_tagname:%s tag_name:%s define %d cell, but only use %d cells !!!!\n",
             __FUNCTION__, __LINE__,
             TAG_NAME(parent_tag_id), TAG_NAME(cur_tag_id),
             noElements, cell_size);
    }

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild && (cnt < noElements)) {
        XmlCellTag tag = XmlCellTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        if (tagname == TAG_NAME(CALIB_CELL_TAG_ID)) {
            autoTabForward();
            bool result = (this->*func)(pchild->ToElement(), param, cnt);
            autoTabBackward();
            if (!result) {
                return (result);
            }
        }
        else {
            LOGW("unknown cell tag: %s", tagname.c_str());

            //return (false);
        }

        pchild = pchild->NextSibling();
        cnt++;
    }

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);
}

bool RkAiqCalibParser::parseEntryCell3
(
    XMLElement*   pelement,
    int                 noElements,
    int                 noOutElements,
    parseCellContent    func,
    void*                param,
    uint32_t     cur_id,
    uint32_t     parent_id
) {
    int cnt = 0;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    int cell_size = 0;
    CALIB_IQ_TAG_ID_T cur_tag_id = (CALIB_IQ_TAG_ID_T)cur_id;
    CALIB_IQ_TAG_ID_T parent_tag_id = (CALIB_IQ_TAG_ID_T)parent_id;
    parseCellNoElement(pelement, noElements, cell_size);
    XML_CHECK_CELL_SET_SIZE(noOutElements);

    LOGD("####@@@@@ cur_tag_id %d parent_tag_id %d cell_size %d\n",
         cur_tag_id, parent_tag_id, cell_size);

    if (noOutElements > noElements)
    {
        //noOutElements = noElements;
        XMLNode* pchild_ref = pelement->FirstChild();
        while (pchild_ref->NextSibling() != NULL)
            pchild_ref = pchild_ref->NextSibling();
        for (; noElements < noOutElements; noElements++)
        {
            XMLNode* copy = pchild_ref->DeepClone(NULL);
            pelement->InsertEndChild(copy);
        }
    }

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild && (cnt < noOutElements)) {
        XmlCellTag tag = XmlCellTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        if (tagname == TAG_NAME(CALIB_CELL_TAG_ID)) {
            autoTabForward();
            bool result = (this->*func)(pchild->ToElement(), /*param*/(void*)&cnt);
            autoTabBackward();
            if (!result) {
                return (result);
            }
        }
        else {
            LOG1("unknown cell tag: %s", tagname.c_str());

            return (false);
        }

        pchild = pchild->NextSibling();
        cnt++;
    }

    XMLNode *pchild_del[100];
    int del_cnt = 0;
    while (pchild)
    {
        pchild_del[del_cnt] = (XMLNode *)pchild;
        pchild = pchild->NextSibling();
        cnt++;
        del_cnt++;
    }

    for (int i = 0; i < del_cnt; i++)
    {
        pelement->DeleteChild(pchild_del[i]);
    }

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    char str[20];
    snprintf(str, sizeof(str), "[1 %u]", (unsigned int)noOutElements);
    pelement->SetAttribute(CALIB_ATTRIBUTE_SIZE, str);

    return (true);
}

bool RkAiqCalibParser::parseEntryCell4
(
    XMLElement*   pelement,
    int                 noElements,
    int                 noOutElements,
    parseCellContent2    func,
    void*                param,
    uint32_t     cur_id,
    uint32_t     parent_id
) {
    int cnt = 0;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    int cell_size = 0;
    CALIB_IQ_TAG_ID_T cur_tag_id = (CALIB_IQ_TAG_ID_T)cur_id;
    CALIB_IQ_TAG_ID_T parent_tag_id = (CALIB_IQ_TAG_ID_T)parent_id;
    parseCellNoElement(pelement, noElements, cell_size);
    XML_CHECK_CELL_SET_SIZE(noOutElements);

    LOGD("####@@@@@ cur_tag_id %d parent_tag_id %d cell_size %d\n",
         cur_tag_id, parent_tag_id, cell_size);

    if (noOutElements > noElements)
    {
        //noOutElements = noElements;
        XMLNode* pchild_ref = pelement->FirstChild();
        while (pchild_ref->NextSibling() != NULL)
            pchild_ref = pchild_ref->NextSibling();
        for (; noElements < noOutElements; noElements++)
        {
            XMLNode* copy = pchild_ref->DeepClone(NULL);
            pelement->InsertEndChild(copy);
        }
    }

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild && (cnt < noOutElements)) {
        XmlCellTag tag = XmlCellTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        if (tagname == TAG_NAME(CALIB_CELL_TAG_ID)) {
            autoTabForward();
            bool result = (this->*func)(pchild->ToElement(), param, cnt);
            autoTabBackward();
            if (!result) {
                return (result);
            }
        }
        else {
            LOG1("unknown cell tag: %s", tagname.c_str());

            return (false);
        }

        pchild = pchild->NextSibling();
        cnt++;
    }

    XMLNode *pchild_del[100];
    int del_cnt = 0;
    while (pchild)
    {
        pchild_del[del_cnt] = (XMLNode *)pchild;
        pchild = pchild->NextSibling();
        cnt++;
        del_cnt++;
    }

    for (int i = 0; i < del_cnt; i++)
    {
        pelement->DeleteChild(pchild_del[i]);
    }

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    char str[20];
    snprintf(str, sizeof(str), "[1 %u]", (unsigned int)noOutElements);
    pelement->SetAttribute(CALIB_ATTRIBUTE_SIZE, str);

    return (true);
}


bool RkAiqCalibParser::parseEntryHeader
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_HEADER_TAG_ID, CALIB_FILESTART_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());

        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

#ifdef DEBUG_LOG
        LOG1("tag: %s", tagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_HEADER_CODE_XML_PARSE_VERSION_TAG_ID)) {
            ParseString(pchild, mCalibDb->header.parse_version,
                        sizeof(mCalibDb->header.parse_version));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_HEADER_CREATION_DATE_TAG_ID)) {
            ParseString(pchild, mCalibDb->header.date,
                        sizeof(mCalibDb->header.date));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_HEADER_CREATOR_TAG_ID)) {
            ParseString(pchild, mCalibDb->header.author,
                        sizeof(mCalibDb->header.author));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_HEADER_GENERATOR_VERSION_TAG_ID)) {
            ParseString(pchild, mCalibDb->header.gen_verion,
                        sizeof(mCalibDb->header.gen_verion));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_HEADER_SENSOR_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->header.sensor_name,
                        sizeof(mCalibDb->header.sensor_name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_HEADER_SAMPLE_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->header.sample_name,
                        sizeof(mCalibDb->header.sample_name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_HEADER_MAGIC_CODE_TAG_ID)) {
            ParseUintArray(pchild, &mCalibDb->header.magic_code, 1);
        }
        else {

            LOGW("parse error in header section (unknow tag: %s)", tagname.c_str());
            //return (false);
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);

}

bool RkAiqCalibParser::parseEntrySensor
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    (void)param;

    XML_CHECK_START(CALIB_SENSOR_TAG_ID, CALIB_FILESTART_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_TAG_ID)) {
            if (!parseEntrySensorAwb(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_TAG_ID)) {
            if (!parseEntrySensorAec(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_TAG_ID)) {
            if (!parseEntrySensorAhdrMerge(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_TAG_ID)) {
            if (!parseEntrySensorAhdrTmo(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_TAG_ID)) {
            if (!parseEntrySensorAWDR(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BLC_TAG_ID)) {
            if (!parseEntrySensorBlc(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUT3D_TAG_ID)) {
            if (!parseEntrySensorLut3d(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_TAG_ID)) {
            if (!parseEntrySensorDpcc(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_TAG_ID)) {
            if (!parseEntrySensorBayerNr(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TAG_ID)) {
            if (!parseEntrySensorLsc(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_TAG_ID)) {
            if (!parseEntrySensorRKDM(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_TAG_ID)) {
            if (!parseEntrySensorCCM(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_TAG_ID)) {
            if (!parseEntrySensorUVNR(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GAMMA_TAG_ID)) {
            if (!parseEntrySensorGamma(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_TAG_ID)) {
            if (!parseEntrySensorDegamma(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_TAG_ID)) {
            if (!parseEntrySensorYnr(pchild->ToElement()))
            {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_TAG_ID)) {
            if (!parseEntrySensorGic(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_TAG_ID)) {
            if (!parseEntrySensorMFNR(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_TAG_ID)) {
            if (!parseEntrySensorSharp(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_TAG_ID)) {
            if (!parseEntrySensorEdgeFilter(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_TAG_ID)) {
            if (!parseEntrySensorDehaze(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_TAG_ID)) {
            if (!parseEntrySensorAf(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LDCH_TAG_ID)) {
            if (!parseEntrySensorLdch(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_FEC_TAG_ID)) {
            if (!parseEntrySensorFec(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUMA_DETECT_TAG_ID)) {
            if (!parseEntrySensorLumaDetect(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_ORB_TAG_ID)) {

            if (!parseEntrySensorOrb(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_TAG_ID)) {
            if (!parseEntrySensorInfo(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MODULEINFO_TAG_ID)) {
            if (!parseEntrySensorModuleInfo(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_TAG_ID)) {
            if (!parseEntrySensorCpsl(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_COLOR_AS_GREY_TAG_ID)) {
            if (!parseEntrySensorColorAsGrey(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPROC_TAG_ID)) {
            if (!parseEntrySensorCproc(pchild->ToElement())) {
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_IE_TAG_ID)) {
            if (!parseEntrySensorIE(pchild->ToElement())) {
                return (false);
            }
        }
        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

/******************************************************************************
* RkAiqCalibParser::parseEntryAwb
*****************************************************************************/
bool RkAiqCalibParser::parseEntrySensorAwb
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();
    if (xmlParseReadWrite == XML_PARSER_READ) {
        memset(&mCalibDb->awb, 0, sizeof(mCalibDb->awb));
    }

    XML_CHECK_START(CALIB_SENSOR_AWB_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());

        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_AWB_ENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.awbEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            mCalibDb->awb.awbEnable = (tempVal == 0 ? false : true);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WB_BYPASS_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.wbBypass;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            mCalibDb->awb.wbBypass = (tempVal == 0 ? false : true);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_V200_TAG_ID)) {
            if (!parseEntrySensorAwbMeasureParaV200(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_V201_TAG_ID)) {
            if (!parseEntrySensorAwbMeasureParaV201(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_STATEGYPARA_TAG_ID)) {
            if (!parseEntrySensorAwbStategyPara(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_REMOSAICPARA_TAG_ID)) {
            if (!parseEntrySensorAwbRemosaicPara(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("parse error in AWB section (unknow tag:%s)", tagname.c_str());
            //return (false);
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    DCT_ASSERT((mCalibDb->awb.stategy_cfg.lightNum == mCalibDb->awb.measure_para_v200.lightNum));
    //DCT_ASSERT((mCalibDb->awb.stategy_cfg.lightNum == mCalibDb->awb.measure_para_v201.lightNum));
    //v201 to do
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbMeasureParaV200
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_V200_TAG_ID, CALIB_SENSOR_AWB_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());

        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID)) {
            if (!parseEntrySensorAwbMeasureGlobalsV200(pchild->ToElement())) {
                LOGE("parse error in AWB globals (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_V200_LIGHTSOURCES_TAG_ID)) {
            unsigned char lightNum = mCalibDb->awb.measure_para_v200.lightNum;
            mCalibDb->awb.measure_para_v200.lightNum = 0;
            if (xmlParseReadWrite == XML_PARSER_READ) // read
            {
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorAwbMeasureLightSourcesV200,
                                    param,
                                    (uint32_t)CALIB_SENSOR_AWB_V200_LIGHTSOURCES_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_AWB_V200_TAG_ID)) {
                    LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), lightNum,
                                     &RkAiqCalibParser::parseEntrySensorAwbMeasureLightSourcesV200,
                                     param,
                                     (uint32_t)CALIB_SENSOR_AWB_V200_LIGHTSOURCES_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_AWB_V200_TAG_ID)) {
                    LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                    return (false);
                }

            }
        }
        else {
            LOGW("parse error in AWB section (unknow tag:%s)", tagname.c_str());
            //return (false);
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbStategyPara
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_STATEGYPARA_TAG_ID, CALIB_SENSOR_AWB_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());

        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID)) {
            if (!parseEntrySensorAwbStategyGlobals(pchild->ToElement())) {
                LOGE("parse error in AWB globals (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_STRATEGYPARA_LIGHTSOURCES_TAG_ID)) {
            unsigned char lightNum = mCalibDb->awb.stategy_cfg.lightNum;
            mCalibDb->awb.stategy_cfg.lightNum = 0;
            if (xmlParseReadWrite == XML_PARSER_READ) // read
            {
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorAwbStategyLightSources,
                                    param,
                                    (uint32_t)CALIB_SENSOR_AWB_STRATEGYPARA_LIGHTSOURCES_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_AWB_STATEGYPARA_TAG_ID)) {
                    LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement*)pchild->ToElement(), tag.Size(), lightNum,
                                     &RkAiqCalibParser::parseEntrySensorAwbStategyLightSources,
                                     param,
                                     (uint32_t)CALIB_SENSOR_AWB_STRATEGYPARA_LIGHTSOURCES_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_AWB_STATEGYPARA_TAG_ID)) {
                    LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                    return (false);
                }
            }
        }
        else {
            LOGW("parse error in AWB section (unknow tag:%s)", tagname.c_str());
            //return (false);
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbMeasureGlobalsV200
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID, CALIB_SENSOR_AWB_V200_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_HDRFRAMECHOOSE_TAG_ID)) {
            if (!parseEntrySensorAwbFrameChoose(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LSCBYPASSENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v200.lscBypEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.lscBypEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_UVDETECTIONENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v200.uvDetectionEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.uvDetectionEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_XYDETECTIONENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v200.xyDetectionEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.xyDetectionEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_YUVDETECTIONENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v200.yuvDetectionEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.yuvDetectionEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LSUSEDFORYUVDET_TAG_ID)) {
            unsigned char lightnum = mCalibDb->awb.measure_para_v200.lsUsedForYuvDetNum;
            mCalibDb->awb.measure_para_v200.lsUsedForYuvDetNum = 0;
            if (xmlParseReadWrite == XML_PARSER_READ) // read
            {
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorAwbLsForYuvDet,
                                    param,
                                    (uint32_t)CALIB_SENSOR_AWB_LSUSEDFORYUVDET_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID)) {
                    LOGE("parse error in AWB  (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement*)pchild->ToElement(), tag.Size(), lightnum,
                                     &RkAiqCalibParser::parseEntrySensorAwbLsForYuvDet,
                                     param,
                                     (uint32_t)CALIB_SENSOR_AWB_LSUSEDFORYUVDET_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID)) {
                    LOGE("parse error in AWB  (%s)", tagname.c_str());
                    return (false);
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_DOWNSCALEMODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v200.dsMode, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_BLCKMEASUREMODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v200.blkMeasureMode, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MEASUREWINDOW_TAG_ID)) {
            if (!parseEntrySensorAwbMeasureWindow(pchild->ToElement())) {
                LOGE("parse error in AWB  (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MULTIWINDOWENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v200.multiwindow_en;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.multiwindow_en = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_TAG_ID)) {
            if (!parseEntrySensorAwbLimitRange(pchild->ToElement())) {
                LOGE("parse error in AWB  (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PSEUDOLUMWEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v200.rgb2tcs_param.pseudoLuminanceWeight, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_ROTATIONMAT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v200.rgb2tcs_param.rotationMat, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MULTIWINDOW_TAG_ID)) {
            //unsigned short tempVal[32];
            int no = ParseUshortArray(pchild, mCalibDb->awb.measure_para_v200.multiwindow[0], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no <= CALD_AWB_WINDOW_NUM_MAX * 4));
            //MEMCPY(mCalibDb->awb.measure_para_v200.multiwindow, tempVal, sizeof(short)*no);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_EXCLUDERANGE_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAwbGlobalsExclude,
                                param,
                                (uint32_t)CALIB_SENSOR_AWB_EXCLUDERANGE_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID)) {
                LOGE("parse error in AWB globals excluderange (%s)", tagname.c_str());
                return (false);
            }

        }
        else {
            LOGE("parse error in AWB section (unknow tag:%s)", tagname.c_str());
            //return ( false );
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();


    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbDampFactor
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_DAMPFACTOR_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_DFSTEP_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awb.stategy_cfg.dFStep, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_DFMIN_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awb.stategy_cfg.dFMin, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_DFMAX_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awb.stategy_cfg.dFMax, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LVIIRSIZE_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->awb.stategy_cfg.LvIIRsize, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LVVARTH_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awb.stategy_cfg.LvVarTh, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);

}

bool RkAiqCalibParser::parseEntrySensorAwbXyRegionStableSelection
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_XYREGIONSTABLESELECTION_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();

    autoTabForward();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_XYREGIONSIZE_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->awb.stategy_cfg.xyTypeListSize, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LVVARTH_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awb.stategy_cfg.varianceLumaTh, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);

}

bool RkAiqCalibParser::parseEntrySensorAwbStategyGlobals
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID, CALIB_SENSOR_AWB_STATEGYPARA_TAG_ID);
    int no1 = 0, no2 = 0, no3 = 0, no4 = 0;
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {

        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINADJUSTENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.stategy_cfg.wbGainAdjustEn;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.stategy_cfg.wbGainAdjustEn = (tempVal == 0 ? false : true);

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINDAYLIGHTCLIPENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.stategy_cfg.wbGainDaylightClipEn;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.stategy_cfg.wbGainDaylightClipEn = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINCLIPEANBLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.stategy_cfg.wbGainClipEn;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.stategy_cfg.wbGainClipEn = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LSFORFIRSTFRAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.stategy_cfg.lsForFirstFrame,
                        sizeof(mCalibDb->awb.stategy_cfg.lsForFirstFrame));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_UVRANGESMALLENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.stategy_cfg.uvRange_small_enable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.stategy_cfg.uvRange_small_enable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CA_ENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.stategy_cfg.ca_enable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.stategy_cfg.ca_enable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_TOLERANCE_TAG_ID)) {
            const XMLNode* psubchild = pchild->ToElement()->FirstChild();
            autoTabForward();
            mCalibDb->awb.stategy_cfg.tolerance.num = 0;
            XML_CHECK_START(CALIB_SENSOR_AWB_TOLERANCE_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);
            while (psubchild) {
                XmlTag subTag = XmlTag(psubchild->ToElement());
                std::string subTagname(psubchild->ToElement()->Name());
                XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
                if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LV_TAG_ID)) {
                    no1 = ParseFloatArray(psubchild, mCalibDb->awb.stategy_cfg.tolerance.LV, subTag.Size());
                    DCT_ASSERT((no1 == subTag.Size()));
                }
                else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_VALUE_TAG_ID)) {
                    no2 = ParseFloatArray(psubchild, mCalibDb->awb.stategy_cfg.tolerance.value, subTag.Size());
                    DCT_ASSERT((no2 == subTag.Size()));
                }
                psubchild = psubchild->NextSibling();
            }
            XML_CHECK_END();
            autoTabBackward();
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_RUNINTERNAL_TAG_ID)) {
            const XMLNode* psubchild = pchild->ToElement()->FirstChild();
            autoTabForward();
            mCalibDb->awb.stategy_cfg.runInterval.num = 0;
            XML_CHECK_START(CALIB_SENSOR_AWB_RUNINTERNAL_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);
            while (psubchild) {
                XmlTag subTag = XmlTag(psubchild->ToElement());
                std::string subTagname(psubchild->ToElement()->Name());
                XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
                if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LV_TAG_ID)) {
                    no3 = ParseFloatArray(psubchild, mCalibDb->awb.stategy_cfg.runInterval.LV, subTag.Size());
                    DCT_ASSERT((no3 == subTag.Size()));
                }
                else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_VALUE_TAG_ID)) {
                    no4 = ParseFloatArray(psubchild, mCalibDb->awb.stategy_cfg.runInterval.value, subTag.Size());
                    DCT_ASSERT((no4 == subTag.Size()));
                }
                psubchild = psubchild->NextSibling();
            }
            XML_CHECK_END();
            autoTabBackward();
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MULTIWINDOWMODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.stategy_cfg.multiwindowMode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_DAMPFACTOR_TAG_ID)) {
            if (!parseEntrySensorAwbDampFactor(pchild->ToElement())) {
                LOGE("parse error in AWB globals (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LVMATRIX_TAG_ID)) {
            int no = ParseUintArray(pchild, mCalibDb->awb.stategy_cfg.LVMatrix, tag.Size());
            DCT_ASSERT((no <= CALD_AWB_LV_NUM_MAX));
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.stategy_cfg.LV_NUM = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LV_THL_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.LV_THL, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LV_THL2_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.LV_THL2, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LV_THH_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.LV_THH, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LV_THH2_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.LV_THH2, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WP_THL_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.WP_THL, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WP_THH_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.WP_THH, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PRODIS_THL_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.proDis_THL, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PRODIS_THH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.proDis_THH, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PROLV_INDOOR_THL_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.proLV_Indoor_THL, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PROLV_INDOOR_THH_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.proLV_Indoor_THH, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PROLV_OUTDOOR_THL_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.proLV_Outdoor_THL, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PROLV_OUTDOOR_THH_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->awb.stategy_cfg.proLV_Outdoor_THH, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_SPATIALGAIN_L_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.spatialGain_L, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == 4));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_SPATIALGAIN_H_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.spatialGain_H, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == 4));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_TEMPORALDEFAULTGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.temporalDefaultGain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == 4));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_TEMPORALCALGAINSETSIZE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.stategy_cfg.temporalCalGainSetSize, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no <= CALD_AWB_TEMPORAL_GAIN_SIZE_MAX));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_TEMPORALGAINSETWEIGHT_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->awb.stategy_cfg.temporalGainSetWeight, tag.Size());
            DCT_ASSERT((no <= CALD_AWB_TEMPORAL_GAIN_SIZE_MAX));
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == mCalibDb->awb.stategy_cfg.temporalCalGainSetSize));//check
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPNUMPERCTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.wpNumPercTh, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_TEMPWEIGTH_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->awb.stategy_cfg.tempWeight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no <= CALD_AWB_LV_NUM_MAX));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CA_LACALCFACTOR_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.ca_LACalcFactor, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CA_TARGETGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.ca_targetGain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_SINGLECOLORPROCESS_TAG_ID)) {
            if (!parseEntrySensorAwbSingleColor(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CONVERGEDVARTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.convergedVarTh, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LINERGBG_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.lineRgBg, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LINERGBGPROJCCT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.lineRgProjCCT, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_XYREGIONSTABLESELECTION_TAG_ID)) {
            if (!parseEntrySensorAwbXyRegionStableSelection(pchild->ToElement())) {
                LOGE("parse error in AWB globals (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINADJUST_TAG_ID)) {
            if (!parseEntrySensorAwbwbGainAdjust(pchild->ToElement()), param) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINOFFSET_TAG_ID)) {
            if (!parseEntrySensorAwbwbGainOffset(pchild->ToElement()), param) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINDAYLIGHTCLIP_TAG_ID)) {
            if (!parseEntrySensorAwbwbGainDaylightClip(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINCLIP_TAG_ID)) {
            if (!parseEntrySensorAwbwbGainClip(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("parse error in AWB section (unknow tag:%s)", tagname.c_str());
            //return ( false );
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();
    DCT_ASSERT((no2 == no1));
    mCalibDb->awb.stategy_cfg.tolerance.num = no1;
    DCT_ASSERT((no3 == no4));
    mCalibDb->awb.stategy_cfg.runInterval.num = no3;

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbLsForYuvDet
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_LSUSEDFORYUVDET_TAG_ID, CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        LOGD("%s(%d): tag.Size %d\n", __FUNCTION__, __LINE__, tag.Size());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LSUSEDFORYUVDET_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.measure_para_v200.lsUsedForYuvDet[index],
                        sizeof(mCalibDb->awb.measure_para_v200.lsUsedForYuvDet[index]));//check
        } else {
            LOGW("UNKNOWN tag: %s", tagname.c_str());
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    mCalibDb->awb.measure_para_v200.lsUsedForYuvDetNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbFrameChoose
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_HDRFRAMECHOOSE_TAG_ID, CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v200.hdrFrameChooseMode, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_FRAMECHOOSE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v200.hdrFrameChoose, 1);
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbFrameChooseV201
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_HDRFRAMECHOOSE_TAG_ID, CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v201.hdrFrameChooseMode, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_FRAMECHOOSE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v201.hdrFrameChoose, 1);
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbMeasureWindow
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_MEASUREWINDOW_TAG_ID, CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MODE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->awb.measure_para_v200.measeureWindow.mode, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_RESALL_TAG_ID)) {
            mCalibDb->awb.measure_para_v200.measeureWindow.resNum = 0;
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAwbWindow,
                                param,
                                (uint32_t)CALIB_SENSOR_AWB_RESALL_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AWB_MEASUREWINDOW_TAG_ID)) {
                LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbLimitRange
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_LIMITRANGE_TAG_ID, CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_Y_TAG_ID)) {
            unsigned short tmpValue[2] = { mCalibDb->awb.measure_para_v200.minY, mCalibDb->awb.measure_para_v200.maxY };
            int no = ParseUshortArray(pchild, tmpValue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.minY = tmpValue[0];
            mCalibDb->awb.measure_para_v200.maxY = tmpValue[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_R_TAG_ID)) {
            unsigned short tmpValue[2] = { mCalibDb->awb.measure_para_v200.minR, mCalibDb->awb.measure_para_v200.maxR };
            int no = ParseUshortArray(pchild, tmpValue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.minR = tmpValue[0];
            mCalibDb->awb.measure_para_v200.maxR = tmpValue[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_G_TAG_ID)) {
            unsigned short tmpValue[2] = { mCalibDb->awb.measure_para_v200.minG, mCalibDb->awb.measure_para_v200.maxG };
            int no = ParseUshortArray(pchild, tmpValue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.minG = tmpValue[0];
            mCalibDb->awb.measure_para_v200.maxG = tmpValue[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_B_TAG_ID)) {
            unsigned short tmpValue[2] = { mCalibDb->awb.measure_para_v200.minB, mCalibDb->awb.measure_para_v200.maxB };
            int no = ParseUshortArray(pchild, tmpValue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.minB = tmpValue[0];
            mCalibDb->awb.measure_para_v200.maxB = tmpValue[1];
        }
        else {
            LOGW("unknown tag %s", tagname.c_str());
        }
        pchild = pchild->NextSibling();
    }


    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbWindow
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_RESALL_TAG_ID, CALIB_SENSOR_AWB_MEASUREWINDOW_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_RESOLUTION_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.measure_para_v200.measeureWindow.resName[index],
                        sizeof(mCalibDb->awb.measure_para_v200.measeureWindow.resName[index]));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MEASUREWINDOWSIZE_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->awb.measure_para_v200.measeureWindow.window[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no <= 4));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((index <= CALD_AWB_RRES_NUM_MAX));
    mCalibDb->awb.measure_para_v200.measeureWindow.resNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbwbGainAdjust
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_WBGAINADJUST_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CTGRID_NUM_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->awb.stategy_cfg.cct_lut_cfg[0].ct_grid_num, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CTINRANGE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.cct_lut_cfg[0].ct_range, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == 2));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CRIGRID_NUM_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->awb.stategy_cfg.cct_lut_cfg[0].cri_grid_num, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CRIINRANGE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.cct_lut_cfg[0].cri_range, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == 2));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LUTALL_TAG_ID)) {

            mCalibDb->awb.stategy_cfg.cct_lut_cfg_num = tag.Size();
            DCT_ASSERT((mCalibDb->awb.stategy_cfg.cct_lut_cfg_num <= CALD_AWB_CT_LV_NUM_MAX));
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAwbCctLutAll,
                                param,
                                (uint32_t)CALIB_SENSOR_AWB_LUTALL_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AWB_WBGAINADJUST_TAG_ID)) {
                LOGE("parse error in AWB wbGainAdjust (%s)", tagname.c_str());
                return (false);
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySensorAwbwbGainOffset
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_WBGAINOFFSET_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINOFFSET_ENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.stategy_cfg.wbGainOffset.enable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.stategy_cfg.wbGainOffset.enable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WBGAINOFFSET_OFFSET_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.wbGainOffset.offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == 4));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorAwbwbGainDaylightClip
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_WBGAINDAYLIGHTCLIP_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    autoTabForward();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_OUTDOORCCTMIN_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awb.stategy_cfg.cct_clip_cfg.outdoor_cct_min, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        psubchild = psubchild->NextSibling();
    }
    autoTabBackward();


    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbwbGainClip
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_WBGAINCLIP_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    autoTabForward();
    int cct_num = 0;
    int cri_bound_up_num = 0;
    int cri_bound_low_num = 0;
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CCT_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awb.stategy_cfg.cct_clip_cfg.cct, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            cct_num = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CRIBOUNDUP_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awb.stategy_cfg.cct_clip_cfg.cri_bound_up, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            cri_bound_up_num = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CRIBOUNDLOW_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awb.stategy_cfg.cct_clip_cfg.cri_bound_low, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            cri_bound_low_num = no;
        }
        psubchild = psubchild->NextSibling();
    }
    if((cct_num != cri_bound_up_num) || (cct_num != cri_bound_low_num)) {
        LOGE("para error in AWB section tag:%s  number of cct should be equal to cri_bound_up and cri_bound_low",
             pelement->Name());
        return ( false );
    }
    mCalibDb->awb.stategy_cfg.cct_clip_cfg.grid_num = cct_num;
    autoTabBackward();

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbCctLutAll
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_LUTALL_TAG_ID, CALIB_SENSOR_AWB_WBGAINADJUST_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LVVALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.cct_lut_cfg[index].lv, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CTOUT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.cct_lut_cfg[index].ct_lut_out, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_CRIOUT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.cct_lut_cfg[index].cri_lut_out, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbSingleColor
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();
    XML_CHECK_START(CALIB_SENSOR_AWB_SINGLECOLORPROCESS_TAG_ID, CALIB_SENSOR_AWB_STRATEGYPARA_GLOBALS_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_COLORBLOCK_TAG_ID)) {
            mCalibDb->awb.stategy_cfg.sSelColorNUM = 0;
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAwbColBlk,
                                param,
                                (uint32_t)CALIB_SENSOR_AWB_COLORBLOCK_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AWB_SINGLECOLORPROCESS_TAG_ID)) {
                LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCEUSEDFORESTIMIATION_TAG_ID)) {
            mCalibDb->awb.stategy_cfg.sIllEstNum = 0;
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAwbLsForEstimation,
                                param,
                                (uint32_t)CALIB_SENSOR_AWB_LIGHTSOURCEUSEDFORESTIMIATION_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AWB_SINGLECOLORPROCESS_TAG_ID)) {
                LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_ALPHA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.sAlpha, 1);
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySensorAwbColBlk
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_COLORBLOCK_TAG_ID, CALIB_SENSOR_AWB_SINGLECOLORPROCESS_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_INDEX_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->awb.stategy_cfg.sIndSelColor[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MEANC_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.sMeanCh[0][index], tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MEANH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.sMeanCh[1][index], tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((index <= CALD_AWB_SGC_NUM_MAX));
    mCalibDb->awb.stategy_cfg.sSelColorNUM++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}
bool RkAiqCalibParser::parseEntrySensorAwbLsForEstimation
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    XML_CHECK_START(CALIB_SENSOR_AWB_LIGHTSOURCEUSEDFORESTIMIATION_TAG_ID, CALIB_SENSOR_AWB_SINGLECOLORPROCESS_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LSUSEDFORESTIMIATION_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.stategy_cfg.sNameIllEst[index],
                        sizeof(mCalibDb->awb.stategy_cfg.sNameIllEst[index]));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_RGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.srGain[index], tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_BGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awb.stategy_cfg.sbGain[index], tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    mCalibDb->awb.stategy_cfg.sIllEstNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbGlobalsExclude
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_EXCLUDERANGE_TAG_ID, CALIB_SENSOR_AWB_V200_GLOBALS_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_EXCLUDERANGE_DOMAIN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v200.excludeWpRange[index].domain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_EXCLUDE_MODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v200.excludeWpRange[index].mode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_EXCLUDE_WINDOW_TAG_ID)) {
            int xuyv[4] = { mCalibDb->awb.measure_para_v200.excludeWpRange[index].xu[0],
                            mCalibDb->awb.measure_para_v200.excludeWpRange[index].xu[1],
                            mCalibDb->awb.measure_para_v200.excludeWpRange[index].yv[0],
                            mCalibDb->awb.measure_para_v200.excludeWpRange[index].yv[1]
                          };
            int no = ParseIntArray(pchild, xuyv, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v200.excludeWpRange[index].xu[0] = xuyv[0];
            mCalibDb->awb.measure_para_v200.excludeWpRange[index].xu[1] = xuyv[1];
            mCalibDb->awb.measure_para_v200.excludeWpRange[index].yv[0] = xuyv[2];
            mCalibDb->awb.measure_para_v200.excludeWpRange[index].yv[1] = xuyv[3];

        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();

    DCT_ASSERT((index <= CALD_AWB_EXCRANGE_NUM_MAX));
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbLightXYRegion
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_LIGHTSOURCES_LIGHTXYREGION_TAG_ID,
                    CALIB_SENSOR_AWB_V200_LIGHTSOURCES_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    autoTabForward();
    while (pchild) {
        XmlTag Tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), Tag.Type(), Tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_NORMAL_TAG_ID)) {
            float tempVal[4] = {mCalibDb->awb.measure_para_v200.xyRangeLight[index].NorrangeX[0],
                                mCalibDb->awb.measure_para_v200.xyRangeLight[index].NorrangeX[1],
                                mCalibDb->awb.measure_para_v200.xyRangeLight[index].NorrangeY[0],
                                mCalibDb->awb.measure_para_v200.xyRangeLight[index].NorrangeY[1]
                               };
            int no = ParseFloatArray(pchild, tempVal, Tag.Size());
            DCT_ASSERT((no == Tag.Size()));
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].NorrangeX[0] = tempVal[0];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].NorrangeX[1] = tempVal[1];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].NorrangeY[0] = tempVal[2];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].NorrangeY[1] = tempVal[3];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_BIG_TAG_ID)) {
            float tempVal[4] = { mCalibDb->awb.measure_para_v200.xyRangeLight[index].SperangeX[0],
                                 mCalibDb->awb.measure_para_v200.xyRangeLight[index].SperangeX[1],
                                 mCalibDb->awb.measure_para_v200.xyRangeLight[index].SperangeY[0],
                                 mCalibDb->awb.measure_para_v200.xyRangeLight[index].SperangeY[1]
                               };
            int no = ParseFloatArray(pchild, tempVal, Tag.Size());
            DCT_ASSERT((no == Tag.Size()));
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].SperangeX[0] = tempVal[0];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].SperangeX[1] = tempVal[1];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].SperangeY[0] = tempVal[2];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].SperangeY[1] = tempVal[3];

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_SMALL_TAG_ID)) {
            float tempVal[4] = { mCalibDb->awb.measure_para_v200.xyRangeLight[index].SmalrangeX[0],
                                 mCalibDb->awb.measure_para_v200.xyRangeLight[index].SmalrangeX[1],
                                 mCalibDb->awb.measure_para_v200.xyRangeLight[index].SmalrangeY[0],
                                 mCalibDb->awb.measure_para_v200.xyRangeLight[index].SmalrangeY[1]
                               };
            int no = ParseFloatArray(pchild, tempVal, Tag.Size());
            DCT_ASSERT((no == Tag.Size()));
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].SmalrangeX[0] = tempVal[0];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].SmalrangeX[1] = tempVal[1];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].SmalrangeY[0] = tempVal[2];
            mCalibDb->awb.measure_para_v200.xyRangeLight[index].SmalrangeY[1] = tempVal[3];
        }
        pchild = pchild->NextSibling();
    }
    autoTabBackward();


    XML_CHECK_END();

    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);

}

bool RkAiqCalibParser::parseEntrySensorAwbLightYUVRegion
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_LIGHTSOURCES_YUVREGION_TAG_ID,
                    CALIB_SENSOR_AWB_V200_LIGHTSOURCES_TAG_ID);

    int index = *((int*)param);

    const XMLNode* psubchild = pelement->FirstChild();

    autoTabForward();

    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_K2SET_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].slope_inv_neg_uv, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_B0SET_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].b_uv, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_K3SET_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].slope_factor_uv, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_K_YDISSET_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].slope_ydis, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_B_YDISSET_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].b_ydis, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_UREFSET_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].ref_u, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_VREFSET_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].ref_v, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_DISSET_TAG_ID)) {
            int no = ParseUshortArray(psubchild, mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].dis, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_THSET_TAG_ID)) {
            int no = ParseUcharArray(psubchild, mCalibDb->awb.measure_para_v200.yuv3DRange_param[index].th, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));

        }
        psubchild = psubchild->NextSibling();
    }
    autoTabBackward();

    XML_CHECK_END();

    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);

}

bool RkAiqCalibParser::parseEntrySensorAwbMeasureLightSourcesV200
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_V200_LIGHTSOURCES_TAG_ID, CALIB_SENSOR_AWB_V200_TAG_ID);


    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.measure_para_v200.lightName[index],
                        sizeof(mCalibDb->awb.measure_para_v200.lightName[index]));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_LIGHTUREGION_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v200.uvRange_param[index].pu_region, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_LIGHTVREGION_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v200.uvRange_param[index].pv_region, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_SMALLUREGION_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v200.uvRange_param_small[index].pu_region, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_SMALLVREGION_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v200.uvRange_param_small[index].pv_region, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_LIGHTXYREGION_TAG_ID)) {
            if (!parseEntrySensorAwbLightXYRegion(pchild->ToElement(), param)) {
                LOGE("parse error in AWB LightSources (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_YUVREGION_TAG_ID)) {
            if (!parseEntrySensorAwbLightYUVRegion(pchild->ToElement(), param)) {
                LOGE("parse error in AWB LightSources (%s)", tagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("UNKNOWN tag: %s", tagname.c_str());
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    mCalibDb->awb.measure_para_v200.lightNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}
bool RkAiqCalibParser::parseEntrySensorAwbStategyLightSources
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    XML_CHECK_START(CALIB_SENSOR_AWB_STRATEGYPARA_LIGHTSOURCES_TAG_ID, CALIB_SENSOR_AWB_STATEGYPARA_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.stategy_cfg.awb_light_info[index].light_name,
                        sizeof(mCalibDb->awb.stategy_cfg.awb_light_info[index].light_name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_DOORTYPE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.stategy_cfg.awb_light_info[index].doorType, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_STANDARDGAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.awb_light_info[index].standardGainValue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_STAWEIGTHSET_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->awb.stategy_cfg.awb_light_info[index].staWeight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_SPATIALGAIN_LV_THSET_TAG_ID)) {
            unsigned int tempVal[2] = { mCalibDb->awb.stategy_cfg.awb_light_info[index].spatialGain_LV_THL, mCalibDb->awb.stategy_cfg.awb_light_info[index].spatialGain_LV_THH};
            int no = ParseUintArray(pchild, tempVal, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.stategy_cfg.awb_light_info[index].spatialGain_LV_THL = tempVal[0];
            mCalibDb->awb.stategy_cfg.awb_light_info[index].spatialGain_LV_THH = tempVal[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_XYTYPE2ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.stategy_cfg.awb_light_info[index].xyType2Enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_WEIGHTCURVE_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.awb_light_info[index].weightCurve_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_WEIGHTCURVE_WEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.stategy_cfg.awb_light_info[index].weightCurve_weight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else {
            LOGW("UNKNOWN tag: %s", tagname.c_str());
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    mCalibDb->awb.stategy_cfg.lightNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbMeasureParaV201
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_V201_TAG_ID, CALIB_SENSOR_AWB_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID)) {
            if (!parseEntrySensorAwbMeasureGlobalsV201(pchild->ToElement())) {
                LOGE("parse error in AWB globals (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_V201_LIGHTSOURCES_TAG_ID)) {
            unsigned char lightNum = mCalibDb->awb.measure_para_v200.lightNum;
            mCalibDb->awb.measure_para_v201.lightNum = 0;
            if (xmlParseReadWrite == XML_PARSER_READ) // read
            {
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorAwbMeasureLightSourcesV201,
                                    param,
                                    (uint32_t)CALIB_SENSOR_AWB_V201_LIGHTSOURCES_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_AWB_V201_TAG_ID)) {
                    LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), lightNum,
                                     &RkAiqCalibParser::parseEntrySensorAwbMeasureLightSourcesV201,
                                     param,
                                     (uint32_t)CALIB_SENSOR_AWB_V201_LIGHTSOURCES_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_AWB_V201_TAG_ID)) {
                    LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                    return (false);
                }
            }
        }
        else {
            LOGW("parse error in AWB section (unknow tag:%s)", tagname.c_str());
            //return (false);
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbMeasureGlobalsV201
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID, CALIB_SENSOR_AWB_V201_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_HDRFRAMECHOOSE_TAG_ID)) {
            if (!parseEntrySensorAwbFrameChooseV201(pchild->ToElement())) {
                LOGE("parse error in AWB (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LSCBYPASSENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v201.lscBypEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.lscBypEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_UVDETECTIONENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v201.uvDetectionEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.uvDetectionEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_XYDETECTIONENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v201.xyDetectionEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.xyDetectionEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_YUVDETECTIONENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v201.yuvDetectionEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.yuvDetectionEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LSUSEDFORYUVDET_TAG_ID)) {
            unsigned char lightNum = mCalibDb->awb.measure_para_v201.lsUsedForYuvDetNum;
            mCalibDb->awb.measure_para_v201.lsUsedForYuvDetNum = 0;
            if (xmlParseReadWrite == XML_PARSER_READ) // read
            {
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorAwbLsForYuvDetV201,
                                    param,
                                    (uint32_t)CALIB_SENSOR_AWB_LSUSEDFORYUVDET_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID)) {
                    LOGE("parse error in AWB  (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), lightNum,
                                     &RkAiqCalibParser::parseEntrySensorAwbLsForYuvDetV201,
                                     param,
                                     (uint32_t)CALIB_SENSOR_AWB_LSUSEDFORYUVDET_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID)) {
                    LOGE("parse error in AWB  (%s)", tagname.c_str());
                    return (false);
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFWEIENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v201.wpDiffWeiEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.wpDiffWeiEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFBLKWEIENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v201.blkWeightEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.blkWeightEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_BLKSTATISTICSENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v201.blkStatisticsEnable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.blkStatisticsEnable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_DOWNSCALEMODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v201.dsMode, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_BLCKMEASUREMODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v201.blkMeasureMode, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MULTIWINDOWENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.measure_para_v201.multiwindow_en;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.multiwindow_en = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MEASUREWINDOW_TAG_ID)) {
            if (!parseEntrySensorAwbMeasureWindowV201(pchild->ToElement())) {
                LOGE("parse error in AWB  (%s)", tagname.c_str());
                return (false);
            }
        }

        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_TAG_ID)) {
            if (!parseEntrySensorAwbLimitRangeV201(pchild->ToElement())) {
                LOGE("parse error in AWB  (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PSEUDOLUMWEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v201.rgb2tcs_param.pseudoLuminanceWeight, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_ROTATIONMAT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v201.rgb2tcs_param.rotationMat, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_RGB2ROTATIONYUVMAT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v201.rgb2RYuv_matrix, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == 16));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MULTIWINDOW_TAG_ID)) {
            //unsigned short tempVal[32];
            int no = ParseUshortArray(pchild, mCalibDb->awb.measure_para_v201.multiwindow[0], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no <= CALD_AWB_WINDOW_NUM_MAX * 4));
            //MEMCPY(mCalibDb->awb.measure_para_v201.multiwindow, tempVal, sizeof(short)*no);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_EXCLUDERANGE_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAwbGlobalsExcludeV201,
                                param,
                                (uint32_t)CALIB_SENSOR_AWB_EXCLUDERANGE_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID)) {
                LOGE("parse error in AWB globals excluderange (%s)", tagname.c_str());
                return (false);
            }

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFWEIGHT_TAG_ID)) {
            if (!parseEntrySensorAwbWpDiffLumaWeight(pchild->ToElement())) {
                LOGE("parse error in AWB  (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFBLKWEIGHT_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->awb.measure_para_v201.blkWeight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no == CALD_AWB_GRID_NUM_TOTAL));
        }
        else {
            LOGW("parse error in AWB section (unknow tag:%s)", tagname.c_str());
            //return ( false );
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();


    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbLightXYRegionV201
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_LIGHTSOURCES_LIGHTXYREGION_TAG_ID,
                    CALIB_SENSOR_AWB_V201_LIGHTSOURCES_TAG_ID);

    int index = *((int*)param);

    const XMLNode* psubchild = pelement->FirstChild();
    autoTabForward();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_NORMAL_TAG_ID)) {
            float tempVal[4] = {mCalibDb->awb.measure_para_v201.xyRangeLight[index].NorrangeX[0],
                                mCalibDb->awb.measure_para_v201.xyRangeLight[index].NorrangeX[1],
                                mCalibDb->awb.measure_para_v201.xyRangeLight[index].NorrangeY[0],
                                mCalibDb->awb.measure_para_v201.xyRangeLight[index].NorrangeY[1]
                               };
            int no = ParseFloatArray(psubchild, tempVal, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].NorrangeX[0] = tempVal[0];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].NorrangeX[1] = tempVal[1];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].NorrangeY[0] = tempVal[2];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].NorrangeY[1] = tempVal[3];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_BIG_TAG_ID)) {
            float tempVal[4] = { mCalibDb->awb.measure_para_v201.xyRangeLight[index].SperangeX[0],
                                 mCalibDb->awb.measure_para_v201.xyRangeLight[index].SperangeX[1],
                                 mCalibDb->awb.measure_para_v201.xyRangeLight[index].SperangeY[0],
                                 mCalibDb->awb.measure_para_v201.xyRangeLight[index].SperangeY[1]
                               };
            int no = ParseFloatArray(psubchild, tempVal, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].SperangeX[0] = tempVal[0];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].SperangeX[1] = tempVal[1];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].SperangeY[0] = tempVal[2];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].SperangeY[1] = tempVal[3];

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_SMALL_TAG_ID)) {
            float tempVal[4] = { mCalibDb->awb.measure_para_v201.xyRangeLight[index].SmalrangeX[0],
                                 mCalibDb->awb.measure_para_v201.xyRangeLight[index].SmalrangeX[1],
                                 mCalibDb->awb.measure_para_v201.xyRangeLight[index].SmalrangeY[0],
                                 mCalibDb->awb.measure_para_v201.xyRangeLight[index].SmalrangeY[1]
                               };
            int no = ParseFloatArray(psubchild, tempVal, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].SmalrangeX[0] = tempVal[0];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].SmalrangeX[1] = tempVal[1];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].SmalrangeY[0] = tempVal[2];
            mCalibDb->awb.measure_para_v201.xyRangeLight[index].SmalrangeY[1] = tempVal[3];
        }
        psubchild = psubchild->NextSibling();
    }
    autoTabBackward();



    XML_CHECK_END();

    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);

}

bool RkAiqCalibParser::parseEntrySensorAwbLightRTYUVRegionV201
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_LIGHTSOURCES_RTYUVREGION_TAG_ID,
                    CALIB_SENSOR_AWB_V201_LIGHTSOURCES_TAG_ID);

    int index = *((int*)param);

    const XMLNode* psubchild = pelement->FirstChild();
    autoTabForward();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_THCURVE_U_SET_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awb.measure_para_v201.yuv3D2Range_param[index].thcurve_u, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_THCURVE_TH_SET_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awb.measure_para_v201.yuv3D2Range_param[index].thcure_th, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_LINEVECTOR_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awb.measure_para_v201.yuv3D2Range_param[index].line, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        psubchild = psubchild->NextSibling();
    }
    autoTabBackward();

    XML_CHECK_END();

    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);

}


bool RkAiqCalibParser::parseEntrySensorAwbMeasureLightSourcesV201
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_V201_LIGHTSOURCES_TAG_ID, CALIB_SENSOR_AWB_V201_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.measure_para_v201.lightName[index],
                        sizeof(mCalibDb->awb.measure_para_v201.lightName[index]));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_LIGHTUREGION_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v201.uvRange_param[index].pu_region, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_LIGHTVREGION_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v201.uvRange_param[index].pv_region, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_SMALLUREGION_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v201.uvRange_param_small[index].pu_region, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_SMALLVREGION_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.measure_para_v201.uvRange_param_small[index].pv_region, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_LIGHTXYREGION_TAG_ID)) {
            if (!parseEntrySensorAwbLightXYRegionV201(pchild->ToElement(), param)) {
                LOGE("parse error in AWB globals (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIGHTSOURCES_RTYUVREGION_TAG_ID)) {
            if (!parseEntrySensorAwbLightRTYUVRegionV201(pchild->ToElement(), param)) {
                LOGE("parse error in AWB globals (%s)", tagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("UNKNOWN tag: %s", tagname.c_str());
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    mCalibDb->awb.measure_para_v201.lightNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}
bool RkAiqCalibParser::parseEntrySensorAwbLsForYuvDetV201
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_LSUSEDFORYUVDET_TAG_ID, CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LSUSEDFORYUVDET_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.measure_para_v201.lsUsedForYuvDet[index],
                        sizeof(mCalibDb->awb.measure_para_v201.lsUsedForYuvDet[index]));//check
        } else {
            LOGW("UNKNOWN tag: %s", tagname.c_str());
        }
        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    DCT_ASSERT((index <= CALD_AWB_LS_NUM_MAX));
    mCalibDb->awb.measure_para_v201.lsUsedForYuvDetNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbMeasureWindowV201
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_MEASUREWINDOW_TAG_ID, CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MODE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->awb.measure_para_v201.measeureWindow.mode, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_RESALL_TAG_ID)) {
            mCalibDb->awb.measure_para_v201.measeureWindow.resNum = 0;
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAwbWindowV201,
                                param,
                                (uint32_t)CALIB_SENSOR_AWB_RESALL_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AWB_MEASUREWINDOW_TAG_ID)) {
                LOGE("parse error in AWB light sources (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbLimitRangeV201
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_LIMITRANGE_TAG_ID, CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_Y_TAG_ID)) {
            unsigned short tmpValue[2] = { mCalibDb->awb.measure_para_v201.minY, mCalibDb->awb.measure_para_v201.maxY };
            int no = ParseUshortArray(psubchild, tmpValue, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            mCalibDb->awb.measure_para_v201.minY = tmpValue[0];
            mCalibDb->awb.measure_para_v201.maxY = tmpValue[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_R_TAG_ID)) {
            unsigned short tmpValue[2] = { mCalibDb->awb.measure_para_v201.minR, mCalibDb->awb.measure_para_v201.maxR };
            int no = ParseUshortArray(psubchild, tmpValue, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            mCalibDb->awb.measure_para_v201.minR = tmpValue[0];
            mCalibDb->awb.measure_para_v201.maxR = tmpValue[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_G_TAG_ID)) {
            unsigned short tmpValue[2] = { mCalibDb->awb.measure_para_v201.minG, mCalibDb->awb.measure_para_v201.maxG };
            int no = ParseUshortArray(psubchild, tmpValue, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            mCalibDb->awb.measure_para_v201.minG = tmpValue[0];
            mCalibDb->awb.measure_para_v201.maxG = tmpValue[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LIMITRANGE_B_TAG_ID)) {
            unsigned short tmpValue[2] = { mCalibDb->awb.measure_para_v201.minB, mCalibDb->awb.measure_para_v201.maxB };
            int no = ParseUshortArray(psubchild, tmpValue, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            mCalibDb->awb.measure_para_v201.minB = tmpValue[0];
            mCalibDb->awb.measure_para_v201.maxB = tmpValue[1];
        }
        else {
            LOGW("unknown subTag %s", subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbWpDiffWeiEnableTh
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_WPDIFFWEIENABLETH_TAG_ID, CALIB_SENSOR_AWB_WPDIFFWEIGHT_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPNOTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->awb.measure_para_v201.wpDiffNoTh, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_LVVALUETH_TAG_ID)) {
            int no = ParseUintArray(psubsubchild, &mCalibDb->awb.measure_para_v201.wpDiffLvValueTh, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbWpDiffwei_w_HighLV
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_WPDIFFWEI_W_HIGHLV_TAG_ID, CALIB_SENSOR_AWB_WPDIFFWEIGHT_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPRATIO1_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->awb.measure_para_v201.wpDiffweiSet_w_HigLV[0], subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPRATIO2_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->awb.measure_para_v201.wpDiffweiSet_w_HigLV[1], subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPRATIO3_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->awb.measure_para_v201.wpDiffweiSet_w_HigLV[2], subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbWpDiffwei_w_LowLV
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_WPDIFFWEI_W_LOWLV_TAG_ID, CALIB_SENSOR_AWB_WPDIFFWEIGHT_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubsubTag.Type(), subsubsubTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPRATIO1_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->awb.measure_para_v201.wpDiffweiSet_w_LowLV[0], subsubsubTag.Size());
            DCT_ASSERT((no == subsubsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPRATIO2_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->awb.measure_para_v201.wpDiffweiSet_w_LowLV[1], subsubsubTag.Size());
            DCT_ASSERT((no == subsubsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPRATIO3_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->awb.measure_para_v201.wpDiffweiSet_w_LowLV[2], subsubsubTag.Size());
            DCT_ASSERT((no == subsubsubTag.Size()));
        }
        psubsubchild = psubsubchild->NextSibling();
    }


    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbWpDiffLumaWeight
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_WPDIFFWEIGHT_TAG_ID, CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFWEIENABLETH_TAG_ID)) {
            if (!parseEntrySensorAwbWpDiffWeiEnableTh(psubchild->ToElement())) {
                LOGE("parse error in AWB  (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFWEI_Y_TAG_ID)) {
            int no = ParseUcharArray(psubchild, mCalibDb->awb.measure_para_v201.wpDiffwei_y, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_PERFECTBINCONF_TAG_ID)) {
            int no = ParseUcharArray(psubchild, mCalibDb->awb.measure_para_v201.perfectBin, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFWEILVTH_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awb.measure_para_v201.wpDiffweiSet_w_LvValueTh, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFWEIRATIOTH_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awb.measure_para_v201.wpDiffWeiRatioTh, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFWEI_W_HIGHLV_TAG_ID)) {
            if (!parseEntrySensorAwbWpDiffwei_w_HighLV(psubchild->ToElement())) {
                LOGE("parse error in AWB  (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_WPDIFFWEI_W_LOWLV_TAG_ID)) {
            if (!parseEntrySensorAwbWpDiffwei_w_LowLV(psubchild->ToElement())) {
                LOGE("parse error in AWB  (%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("unknown tag %s", subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbWindowV201
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_RESALL_TAG_ID, CALIB_SENSOR_AWB_MEASUREWINDOW_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_RESOLUTION_TAG_ID)) {
            ParseString(pchild, mCalibDb->awb.measure_para_v201.measeureWindow.resName[index],
                        sizeof(mCalibDb->awb.measure_para_v201.measeureWindow.resName[index]));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_MEASUREWINDOWSIZE_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->awb.measure_para_v201.measeureWindow.window[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            DCT_ASSERT((no <= 4));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((index <= CALD_AWB_RRES_NUM_MAX));
    mCalibDb->awb.measure_para_v201.measeureWindow.resNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbGlobalsExcludeV201
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWB_EXCLUDERANGE_TAG_ID, CALIB_SENSOR_AWB_V201_GLOBALS_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_EXCLUDERANGE_DOMAIN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v201.excludeWpRange[index].domain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_EXCLUDE_MODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->awb.measure_para_v201.excludeWpRange[index].mode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_EXCLUDE_WINDOW_TAG_ID)) {
            int xuyv[4] = { mCalibDb->awb.measure_para_v201.excludeWpRange[index].xu[0],
                            mCalibDb->awb.measure_para_v201.excludeWpRange[index].xu[1],
                            mCalibDb->awb.measure_para_v201.excludeWpRange[index].yv[0],
                            mCalibDb->awb.measure_para_v201.excludeWpRange[index].yv[1]
                          };
            int no = ParseIntArray(pchild, xuyv, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.measure_para_v201.excludeWpRange[index].xu[0] = xuyv[0];
            mCalibDb->awb.measure_para_v201.excludeWpRange[index].xu[1] = xuyv[1];
            mCalibDb->awb.measure_para_v201.excludeWpRange[index].yv[0] = xuyv[2];
            mCalibDb->awb.measure_para_v201.excludeWpRange[index].yv[1] = xuyv[3];

        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((index <= CALD_AWB_EXCRANGE_NUM_MAX));
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAwbRemosaicPara
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWB_REMOSAICPARA_TAG_ID, CALIB_SENSOR_AWB_TAG_ID);


    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_REMOSAICPARA_ENABLE_TAG_ID)) {
            unsigned char tempVal = mCalibDb->awb.remosaic_cfg.enable;
            int no = ParseUcharArray(pchild, &tempVal, 1);
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->awb.remosaic_cfg.enable = (tempVal == 0 ? false : true);
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWB_REMOSAICPARA_WBGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->awb.remosaic_cfg.sensor_awb_gain, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else {
            LOGW("parse error in AWB section (unknow tag:%s)", tagname.c_str());
            //return ( false );
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();


    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecLinAlterExp
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AEC_SYNCTEST_LINALTEREXP_TAG_ID, CALIB_SENSOR_AEC_SYNCTEST_ALTEREXP_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();

    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());
#ifdef DEBUG_LOG
        redirectOut << "Tagname: " << Tagname << ",index:" << index << std::endl;
#endif
        DCT_ASSERT((index < AEC_ALTER_EXP_MAX_NUM));

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_TIMEVALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->aec.CommCtrl.stSyncTest.LinAlterExp.TimeValue[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_GAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->aec.CommCtrl.stSyncTest.LinAlterExp.GainValue[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_ISPGAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->aec.CommCtrl.stSyncTest.LinAlterExp.IspgainValue[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_DCGMODE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->aec.CommCtrl.stSyncTest.LinAlterExp.DcgMode[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_PIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->aec.CommCtrl.stSyncTest.LinAlterExp.PIrisGainValue[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else {
            redirectOut
                    << "parse error in LinAlterExp (unknow tag: "
                    << Tagname
                    << ")"
                    << std::endl;
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    mCalibDb->aec.CommCtrl.stSyncTest.LinAlterExp.array_size = index + 1;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorAecHdrAlterExp
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AEC_SYNCTEST_HDRALTEREXP_TAG_ID, CALIB_SENSOR_AEC_SYNCTEST_ALTEREXP_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();

    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());
#ifdef DEBUG_LOG
        redirectOut << "Tagname: " << Tagname << ",index:" << index << std::endl;
#endif
        DCT_ASSERT((index < AEC_ALTER_EXP_MAX_NUM));

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_TIMEVALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stSyncTest.HdrAlterExp.TimeValue[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_GAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stSyncTest.HdrAlterExp.GainValue[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_ISPGAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stSyncTest.HdrAlterExp.IspDGainValue[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_DCGMODE_TAG_ID)) {
            int no = ParseIntArray(pchild, mCalibDb->aec.CommCtrl.stSyncTest.HdrAlterExp.DcgMode[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_PIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->aec.CommCtrl.stSyncTest.HdrAlterExp.PIrisGainValue[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else {
            redirectOut
                    << "parse error in HdrAlterExp (unknow tag: "
                    << Tagname
                    << ")"
                    << std::endl;
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    mCalibDb->aec.CommCtrl.stSyncTest.HdrAlterExp.array_size = index + 1;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecAlterExp
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_SYNCTEST_ALTEREXP_TAG_ID, CALIB_SENSOR_AEC_SYNCTEST_TAG_ID);

#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    const XMLNode* psubchild = pelement->FirstChild();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_LINALTEREXP_TAG_ID)) {
            if (!parseEntryCell(psubchild->ToElement(), subTag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAecLinAlterExp,
                                param,
                                (uint32_t)CALIB_SENSOR_AEC_SYNCTEST_LINALTEREXP_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AEC_SYNCTEST_ALTEREXP_TAG_ID)) {
                LOGE("parse error in AEC-SyncTest LinAlterExp(%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_HDRALTEREXP_TAG_ID)) {
            if (!parseEntryCell(psubchild->ToElement(), subTag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAecHdrAlterExp,
                                param,
                                (uint32_t)CALIB_SENSOR_AEC_SYNCTEST_HDRALTEREXP_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AEC_SYNCTEST_ALTEREXP_TAG_ID)) {
                LOGE("parse error in AEC-SyncTest HdrAlterExp(%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("%s(%d): parse error in AEC-SyncTest AlterExp (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecSyncTest
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_SYNCTEST_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = [%s]\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->aec.CommCtrl.stSyncTest.enable, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_INTERVALFRM_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->aec.CommCtrl.stSyncTest.IntervalFrm, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_ALTEREXP_TAG_ID)) {
            if (!parseEntrySensorAecAlterExp(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec-SyncTest AlterExp(%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("%s(%d): parse error in Aec-SyncTest (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorAecSpeed
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_AECSPEED_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = [%s]\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_DAMPOVER_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.CommCtrl.stAuto.stAeSpeed.DampOver, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_DAMPUNDER_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.CommCtrl.stAuto.stAeSpeed.DampUnder, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_DAMPDARK2BRIGHT_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.CommCtrl.stAuto.stAeSpeed.DampDark2Bright, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_DAMPBRIGHT2DARK_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.CommCtrl.stAuto.stAeSpeed.DampBright2Dark, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in  AeSpeed (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecDelayFrmNum
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AEC_AECDELAYFRMNUM_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    autoTabForward();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_BLACKDELAY_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->aec.CommCtrl.stAuto.BlackDelayFrame, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_WHITEDELAY_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->aec.CommCtrl.stAuto.WhiteDelayFrame, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in  AeDelayFrame (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecVBNightMode
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AEC_AECDNSWITCH_VBNIGHTMODE_TAG_ID, CALIB_SENSOR_AEC_AECDNSWITCH_TAG_ID);

    XmlTag subTag = XmlTag(pelement);

    const XMLNode* psubsubchild = pelement->FirstChild();
    autoTabForward();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stVBNightMode.enable, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_N2DFRMCNT_TAG_ID)) {
            int no = ParseUcharArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stVBNightMode.Night2DayFrmCnt, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_N2DFACTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stVBNightMode.Night2DayFacTh, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in DNSwitch - VBNightMode(unknow tag: %s )\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
        }
        psubsubchild = psubsubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecIRNightMode
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AEC_AECDNSWITCH_IRNIGHTMODE_TAG_ID, CALIB_SENSOR_AEC_AECDNSWITCH_TAG_ID);

    XmlTag subTag = XmlTag(pelement);
    const XMLNode* psubsubchild = pelement->FirstChild();
    autoTabForward();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stIRNightMode.enable, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_N2DFRMCNT_TAG_ID)) {
            int no = ParseUcharArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stIRNightMode.Night2DayFrmCnt, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_N2DFACTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stIRNightMode.Night2DayFacTh, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_IR_RG_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stIRNightMode.IRRgain, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_IR_BG_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stIRNightMode.IRBgain, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_MAX_DIS_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stIRNightMode.MaxWbDis, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_VB_PERCENT_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.stIRNightMode.VbPercent, subTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        } else {
            LOGW("%s(%d): parse error in DNSwitch - IRNightMode(unknow tag: %s )\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorAecDNSwitch
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AEC_AECDNSWITCH_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    autoTabForward();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_DNTRIGGER_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.DNTrigger, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_DNMODE_TAG_ID)) {
            char* value = Toupper(subTag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_AECDNSWITCH_DNMODE_DAY) {
                    mCalibDb->aec.CommCtrl.stDNSwitch.DNMode = AEC_DNMODE_DAY;
                }
                else if (s_value == CALIB_SENSOR_AEC_AECDNSWITCH_DNMODE_NIGHT) {
                    mCalibDb->aec.CommCtrl.stDNSwitch.DNMode = AEC_DNMODE_NIGHT;
                }
                else {
                    mCalibDb->aec.CommCtrl.stDNSwitch.DNMode = AEC_DNMODE_MIN;
                    LOGE("%s(%d): invalid AEC DNSwitch-DNMode = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psubchild;
                if (mCalibDb->aec.CommCtrl.stDNSwitch.DNMode == AEC_DNMODE_DAY) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_AECDNSWITCH_DNMODE_DAY);
                }
                else if (mCalibDb->aec.CommCtrl.stDNSwitch.DNMode == AEC_DNMODE_NIGHT) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_AECDNSWITCH_DNMODE_NIGHT);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid AEC DNSwitch-DNMode = %d\n", __FUNCTION__, __LINE__, mCalibDb->aec.CommCtrl.stDNSwitch.DNMode);
                }

            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_FILLLIGHTMODE_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.FillLightMode, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_D2NFACTH_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.Day2NightFacTh, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_D2NFRMCNT_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->aec.CommCtrl.stDNSwitch.Day2NightFrmCnt, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_VBNIGHTMODE_TAG_ID)) {
            if (!parseEntrySensorAecVBNightMode(psubchild->ToElement())) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_IRNIGHTMODE_TAG_ID)) {
            if (!parseEntrySensorAecIRNightMode(psubchild->ToElement())) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("%s(%d): parse error in DNSwitch (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecAntiFlicker
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_ANTIFLICKER_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ANTIFLICKER_ENABLE_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stAntiFlicker.enable;
            int no = ParseUcharArray(psubchild, &temp, subTag.Size());
            mCalibDb->aec.CommCtrl.stAntiFlicker.enable = (temp == 0) ? false : true;
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ANTIFLICKER_FREQ_TAG_ID)) {
            char* value = Toupper(subTag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_ANTIFLICKER_FREQ_50HZ) {
                    mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency = AEC_FLICKER_FREQUENCY_50HZ;
                }
                else if (s_value == CALIB_SENSOR_AEC_ANTIFLICKER_FREQ_60HZ) {
                    mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency = AEC_FLICKER_FREQUENCY_60HZ;
                }
                else if (s_value == CALIB_SENSOR_AEC_ANTIFLICKER_FREQ_AUTO) {
                    mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency = AEC_FLICKER_FREQUENCY_AUTO;
                }
                else {
                    mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency = AEC_FLICKER_FREQUENCY_OFF;
                    LOGE("%s(%d): invalid stAntiFlicker.Frequency = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psubchild;
                if (mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency == AEC_FLICKER_FREQUENCY_50HZ) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_ANTIFLICKER_FREQ_50HZ);
                }
                else if (mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency == AEC_FLICKER_FREQUENCY_60HZ) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_ANTIFLICKER_FREQ_60HZ);
                }
                else if (mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency == AEC_FLICKER_FREQUENCY_AUTO) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_ANTIFLICKER_FREQ_AUTO);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid AEC stAntiFlicker.Frequency = %d\n", __FUNCTION__, __LINE__, mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency);
                }

            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ANTIFLICKER_MODE_TAG_ID)) {
            char* value = Toupper(subTag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_ANTIFLICKER_MODE_AUTO) {
                    mCalibDb->aec.CommCtrl.stAntiFlicker.Mode = AEC_ANTIFLICKER_AUTO_MODE;
                }
                else if (s_value == CALIB_SENSOR_AEC_ANTIFLICKER_MODE_NORMAL) {
                    mCalibDb->aec.CommCtrl.stAntiFlicker.Mode = AEC_ANTIFLICKER_NORMAL_MODE;
                }
                else {
                    mCalibDb->aec.CommCtrl.stAntiFlicker.Mode = AEC_ANTIFLICKER_AUTO_MODE;
                    LOGE("%s(%d): invalid stAntiFlicker.Mode = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psubchild;
                if (mCalibDb->aec.CommCtrl.stAntiFlicker.Mode == AEC_ANTIFLICKER_AUTO_MODE) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_ANTIFLICKER_MODE_AUTO);
                }
                else if (mCalibDb->aec.CommCtrl.stAntiFlicker.Mode == AEC_ANTIFLICKER_NORMAL_MODE) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_ANTIFLICKER_MODE_NORMAL);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid AEC stAntiFlicker.Mode = %d\n", __FUNCTION__, __LINE__, mCalibDb->aec.CommCtrl.stAntiFlicker.Mode);
                }

            }
        }
        else {
            redirectOut << "parse error in  stAntiFlicker (unknow tag: "
                        << subTagname
                        << ")"
                        << std::endl;
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecFrameRateMode
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_AECFRAMERATEMODE_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ISFPSFIX_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stAuto.stFrmRate.isFpsFix;
            int no = ParseUcharArray(psubchild, &temp, subTag.Size());
            mCalibDb->aec.CommCtrl.stAuto.stFrmRate.isFpsFix = (temp == 0) ? false : true;
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_FPSVALUE_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.CommCtrl.stAuto.stFrmRate.FpsValue, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else {
            redirectOut << "parse error in  stFrmRate (unknow tag: "
                        << subTagname
                        << ")"
                        << std::endl;
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecInitValueLinearAE
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();
    XML_CHECK_START(CALIB_SENSOR_AEC_AECINITVALUE_LINEARAE_TAG_ID, CALIB_SENSOR_AEC_AECINITVALUE_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITTIMEVALUE_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stInitExp.stLinExpInitExp.InitTimeValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            mCalibDb->aec.CommCtrl.stInitExp.stLinExpInitExp.array_size = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITGAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stInitExp.stLinExpInitExp.InitGainValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITISPDGAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stInitExp.stLinExpInitExp.InitIspDGainValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITPIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stInitExp.stLinExpInitExp.InitPIrisGainValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITDCIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stInitExp.stLinExpInitExp.InitDCIrisDutyValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in  stLinExpInitExp (unknow tag: %s )\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecInitValueHdrAE
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();
    XML_CHECK_START(CALIB_SENSOR_AEC_AECINITVALUE_HDRAE_TAG_ID, CALIB_SENSOR_AEC_AECINITVALUE_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITTIMEVALUE_TAG_ID)) {
            float tempVal[3];
            for (int i = 0; i < 3; i++)
                tempVal[i] = mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.InitTimeValue.fCoeff[i];
            int no = ParseFloatArray(psubsubchild, tempVal, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            for (int i = 0; i < 3; i++)
                mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.InitTimeValue.fCoeff[i] = tempVal[i];
            mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.array_size = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITGAINVALUE_TAG_ID)) {
            float tempVal[3];
            for (int i = 0; i < 3; i++)
                tempVal[i] = mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.InitGainValue.fCoeff[i];
            int no = ParseFloatArray(psubsubchild, tempVal, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            for (int i = 0; i < 3; i++)
                mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.InitGainValue.fCoeff[i] = tempVal[i];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITISPDGAINVALUE_TAG_ID)) {
            float tempVal[3];
            for (int i = 0; i < 3; i++)
                tempVal[i] = mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.InitIspDGainValue.fCoeff[i];
            int no = ParseFloatArray(psubsubchild, tempVal, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            for (int i = 0; i < 3; i++)
                mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.InitIspDGainValue.fCoeff[i] = tempVal[i];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITPIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.InitPIrisGainValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_INITDCIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stInitExp.stHdrExpInitExp.InitDCIrisDutyValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in  stHdrExpInitExp (unknow tag: %s )\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorAecInitValue
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_AECINITVALUE_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();

#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECINITVALUE_LINEARAE_TAG_ID)) {
            if (!parseEntrySensorAecInitValueLinearAE(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECINITVALUE_HDRAE_TAG_ID)) {
            if (!parseEntrySensorAecInitValueHdrAE(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("%s(%d): parse error in  stInitExp (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecGridWeight
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_AECGRIDWEIGHT_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    XMLNode* psubchild = (XMLNode*)pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif

    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_DAYGRIDWEIGHTS_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                char str[20];
                if (mCalibDb->aec.CommCtrl.DayWeightNum == 25)
                    snprintf(str, sizeof(str), "[5 5]");
                else
                    snprintf(str, sizeof(str), "[15 15]");
                psubchild->ToElement()->SetAttribute(CALIB_ATTRIBUTE_SIZE, str);
            }

            int no = ParseUcharArray(psubchild, mCalibDb->aec.CommCtrl.DayGridWeights.uCoeff, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            DCT_ASSERT((no == AEC_RAWAEBIG_WIN_NUM || no == AEC_RAWAELITE_WIN_NUM));
            mCalibDb->aec.CommCtrl.DayWeightNum = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_NIGHTGRIDWEIGHTS_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                char str[20];
                if (mCalibDb->aec.CommCtrl.NightWeightNum == 25)
                    snprintf(str, sizeof(str), "[5 5]");
                else
                    snprintf(str, sizeof(str), "[15 15]");
                psubchild->ToElement()->SetAttribute(CALIB_ATTRIBUTE_SIZE, str);
            }

            int no = ParseUcharArray(psubchild, mCalibDb->aec.CommCtrl.NightGridWeights.uCoeff, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
            DCT_ASSERT((no == AEC_RAWAEBIG_WIN_NUM || no == AEC_RAWAELITE_WIN_NUM));
            mCalibDb->aec.CommCtrl.NightWeightNum = no;
        }
        else {
            redirectOut
                    << "parse error in  Gridweights (unknow tag: "
                    << subTagname
                    << ")"
                    << std::endl;
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecIrisCtrlPAttr
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_IRISCTRL_P_ATTR_TAG_ID, CALIB_SENSOR_AEC_IRISCTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_P_ATTR_TOTALSTEP_TAG_ID)) {
            int no = ParseUshortArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.PIrisAttr.TotalStep, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            DCT_ASSERT((no <= AEC_PIRIS_STAP_TABLE_MAX));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_P_ATTR_EFFCSTEP_TAG_ID)) {
            int no = ParseUshortArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.PIrisAttr.EffcStep, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            DCT_ASSERT((no <= AEC_PIRIS_STAP_TABLE_MAX));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_P_ATTR_ZEROISMAX_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stIris.PIrisAttr.ZeroIsMax;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stIris.PIrisAttr.ZeroIsMax = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_P_ATTR_STEPTABLE_TAG_ID)) {
            int no = ParseUshortArray(psubsubchild, mCalibDb->aec.CommCtrl.stIris.PIrisAttr.StepTable, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            DCT_ASSERT((no <= AEC_PIRIS_STAP_TABLE_MAX));
        }
        else {
            LOGW("%s(%d): parse error in AecIrisCtrl PAttr (unknow tag: %s )\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecIrisCtrlDCAttr
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_TAG_ID, CALIB_SENSOR_AEC_IRISCTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_KP_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.DCIrisAttr.Kp, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_KI_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.DCIrisAttr.Ki, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_KD_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.DCIrisAttr.Kd, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_MIN_PWMDUTY_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.DCIrisAttr.MinPwmDuty, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_MAX_PWMDUTY_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.DCIrisAttr.MaxPwmDuty, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_OPEN_PWMDUTY_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.DCIrisAttr.OpenPwmDuty, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_CLOSE_PWMDUTY_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stIris.DCIrisAttr.ClosePwmDuty, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in AecIrisCtrl DCAttr (unknow tag: %s )\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecIrisCtrl
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_IRISCTRL_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->aec.CommCtrl.stIris.enable, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_TYPE_TAG_ID)) {
            char* value = Toupper(subTag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_IRISCTRL_TYPE_P) {
                    mCalibDb->aec.CommCtrl.stIris.IrisType = IRIS_P_TYPE;
                }
                else if (s_value == CALIB_SENSOR_AEC_IRISCTRL_TYPE_DC) {
                    mCalibDb->aec.CommCtrl.stIris.IrisType = IRIS_DC_TYPE;
                }
                else {
                    mCalibDb->aec.CommCtrl.stIris.IrisType = IRIS_INVALID_TYPE;
                    LOGE("%s(%d): invalid stIris.IrisType = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psubchild;
                if (mCalibDb->aec.CommCtrl.stIris.IrisType == IRIS_P_TYPE) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_IRISCTRL_TYPE_P);
                }
                else if (mCalibDb->aec.CommCtrl.stIris.IrisType == IRIS_DC_TYPE) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_IRISCTRL_TYPE_DC);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid AEC stAntiFlicker.Frequency = %d\n", __FUNCTION__, __LINE__, mCalibDb->aec.CommCtrl.stAntiFlicker.Frequency);
                }

            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_P_ATTR_TAG_ID)) {
            if (!parseEntrySensorAecIrisCtrlPAttr(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec IrisCtrl (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_DC_ATTR_TAG_ID)) {
            if (!parseEntrySensorAecIrisCtrlDCAttr(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec IrisCtrl (%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("%s(%d): parse error in Aec IrisCtrl (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecManualCtrlLinearAE
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_MANUALCTRL_LINEARAE_TAG_ID, CALIB_SENSOR_AEC_MANUALCTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_TIMEEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stManual.stLinMe.ManualTimeEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stManual.stLinMe.ManualTimeEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_GAINEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stManual.stLinMe.ManualGainEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stManual.stLinMe.ManualGainEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_ISPDGAINEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stManual.stLinMe.ManualIspDgainEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stManual.stLinMe.ManualIspDgainEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_IRISEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stManual.stLinMe.ManualIrisEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stManual.stLinMe.ManualIrisEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_TIMEVALUE_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stManual.stLinMe.TimeValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_GAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stManual.stLinMe.GainValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_ISPDGAINVALUE_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.CommCtrl.stManual.stLinMe.IspDGainValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_PIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stManual.stLinMe.PIrisGainValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_DCIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stManual.stLinMe.DCIrisValue, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in  stManual.stLinMe (unknow tag: %s )\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecManualCtrlHdrAE
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_MANUALCTRL_HDRAE_TAG_ID, CALIB_SENSOR_AEC_MANUALCTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_TIMEEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stManual.stHdrMe.ManualTimeEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stManual.stHdrMe.ManualTimeEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_GAINEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stManual.stHdrMe.ManualGainEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stManual.stHdrMe.ManualGainEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_ISPDGAINEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stManual.stHdrMe.ManualIspDgainEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stManual.stHdrMe.ManualIspDgainEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_IRISEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.CommCtrl.stManual.stHdrMe.ManualIrisEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.CommCtrl.stManual.stHdrMe.ManualIrisEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_TIMEVALUE_TAG_ID)) {
            float tempVal[3];
            for (int i = 0; i < 3; i++)
                tempVal[i] = mCalibDb->aec.CommCtrl.stManual.stHdrMe.TimeValue.fCoeff[i];
            int no = ParseFloatArray(psubsubchild, tempVal, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            for (int i = 0; i < 3; i++)
                mCalibDb->aec.CommCtrl.stManual.stHdrMe.TimeValue.fCoeff[i] = tempVal[i];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_GAINVALUE_TAG_ID)) {
            float tempVal[3];
            for (int i = 0; i < 3; i++)
                tempVal[i] = mCalibDb->aec.CommCtrl.stManual.stHdrMe.GainValue.fCoeff[i];
            int no = ParseFloatArray(psubsubchild, tempVal, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            for (int i = 0; i < 3; i++)
                mCalibDb->aec.CommCtrl.stManual.stHdrMe.GainValue.fCoeff[i] = tempVal[i];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_ISPDGAINVALUE_TAG_ID)) {
            float tempVal[3];
            for (int i = 0; i < 3; i++)
                tempVal[i] = mCalibDb->aec.CommCtrl.stManual.stHdrMe.IspDGainValue.fCoeff[i];
            int no = ParseFloatArray(psubsubchild, tempVal, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
            for (int i = 0; i < 3; i++)
                mCalibDb->aec.CommCtrl.stManual.stHdrMe.IspDGainValue.fCoeff[i] = tempVal[i];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_PIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stManual.stHdrMe.PIrisGainValue, subsubTag.Size());
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_DCIRISVALUE_TAG_ID)) {
            int no = ParseIntArray(psubsubchild, &mCalibDb->aec.CommCtrl.stManual.stHdrMe.DCIrisValue, subsubTag.Size());
        }
        else {
            LOGW("%s(%d): parse error in  stManual.stHdrMe (unknow tag: %s )\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecManualCtrl
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_MANUALCTRL_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_LINEARAE_TAG_ID)) {
            if (!parseEntrySensorAecManualCtrlLinearAE(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec ManualCtrl (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_HDRAE_TAG_ID)) {
            if (!parseEntrySensorAecManualCtrlHdrAE(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec ManualCtrl (%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("%s(%d): parse error in  stManual (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecRoute
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_AECROUTE_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    const XMLNode* psubchild = pelement->FirstChild();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECROUTE_LINEARAE_TAG_ID)) {
            if (!parseEntryCell(psubchild->ToElement(), subTag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAecLinearAeRoute,
                                param,
                                (uint32_t)CALIB_SENSOR_AEC_AECROUTE_LINEARAE_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AEC_AECROUTE_TAG_ID)) {
                LOGE("parse error in AEC LinAeRoute(%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECROUTE_HDRAE_TAG_ID)) {
            if (!parseEntryCell(psubchild->ToElement(), subTag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAecHdrAeRoute,
                                param,
                                (uint32_t)CALIB_SENSOR_AEC_AECROUTE_HDRAE_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AEC_AECROUTE_TAG_ID)) {
                LOGE("parse error in AEC HdrAeRoute (%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("%s(%d): parse error in  AecRoute (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorLinearAECtrlBackLight
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_LINAECTRL_BACKLIGHT_CONFIG_TAG_ID, CALIB_SENSOR_AEC_LINEARAE_CTRL_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
#ifdef DEBUG_LOG
        redirectOut << "secsubTagname: " << secsubTagname << std::endl;
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.BackLightConf.enable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else  if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_MEASAREA_TAG_ID)) {
            char* value = Toupper(secsubtag.Value());
            std::string s_value(value);
#ifdef DEBUG_LOG
            redirectOut << "value:" << value << std::endl;
            redirectOut << s_value << std::endl;
#endif
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_MEASAREA_AUTO) {
                    mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea = AEC_MEASURE_AREA_AUTO;
                }
                else if (s_value == CALIB_SENSOR_AEC_MEASAREA_CENTER) {
                    mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea = AEC_MEASURE_AREA_CENTER;
                }
                else if (s_value == CALIB_SENSOR_AEC_MEASAREA_UP) {
                    mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea = AEC_MEASURE_AREA_UP;
                }
                else if (s_value == CALIB_SENSOR_AEC_MEASAREA_BOTTOM) {
                    mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea = AEC_MEASURE_AREA_BOTTOM;
                }
                else if (s_value == CALIB_SENSOR_AEC_MEASAREA_LEFT) {
                    mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea = AEC_MEASURE_AREA_LEFT;
                }
                else if (s_value == CALIB_SENSOR_AEC_MEASAREA_RIGHT) {
                    mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea = AEC_MEASURE_AREA_RIGHT;
                }
                else {
                    mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea = AEC_MEASURE_AREA_AUTO;
                    redirectOut << "invalid BackLit MeasArea (" << s_value << ")" << std::endl;
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode* pNode = (XMLNode *)psecsubchild;
                if (mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea == AEC_MEASURE_AREA_AUTO)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_MEASAREA_AUTO);
                else if (mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea == AEC_MEASURE_AREA_CENTER)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_MEASAREA_CENTER);
                else if (mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea == AEC_MEASURE_AREA_UP)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_MEASAREA_UP);
                else if (mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea == AEC_MEASURE_AREA_BOTTOM)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_MEASAREA_BOTTOM);
                else if (mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea == AEC_MEASURE_AREA_LEFT)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_MEASAREA_LEFT);
                else if (mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea == AEC_MEASURE_AREA_RIGHT)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_MEASAREA_RIGHT);
                else {
                    pNode->FirstChild()->SetValue("INVALID");
                    redirectOut << "(XML Write)invalid BackLit MeasArea  (" << mCalibDb->aec.LinearAeCtrl.BackLightConf.MeasArea << ")" << std::endl;
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_OEROI_LOWTH_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.BackLightConf.OEROILowTh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_LV_HIGHTH_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.BackLightConf.LvHightTh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_LV_LOWTH_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.BackLightConf.LvLowTh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_LUMADISTTH_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.BackLightConf.LumaDistTh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_LOWLIGHTPDFTH_TAG_ID)) {
            int i = (sizeof(mCalibDb->aec.LinearAeCtrl.BackLightConf.LowLightPdfTh) / sizeof(mCalibDb->aec.LinearAeCtrl.BackLightConf.LowLightPdfTh.fCoeff[0]));
            int no = ParseFloatArray(psecsubchild, mCalibDb->aec.LinearAeCtrl.BackLightConf.LowLightPdfTh.fCoeff, i);
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_NONOEPDFTH_TAG_ID)) {
            int i = (sizeof(mCalibDb->aec.LinearAeCtrl.BackLightConf.NonOEPdfTh) / sizeof(mCalibDb->aec.LinearAeCtrl.BackLightConf.NonOEPdfTh.fCoeff[0]));
            int no = ParseFloatArray(psecsubchild, mCalibDb->aec.LinearAeCtrl.BackLightConf.NonOEPdfTh.fCoeff, i);
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_EXPLEVEL_TAG_ID)) {
            int i = (sizeof(mCalibDb->aec.LinearAeCtrl.BackLightConf.ExpLevel) / sizeof(mCalibDb->aec.LinearAeCtrl.BackLightConf.ExpLevel.fCoeff[0]));
            int no = ParseFloatArray(psecsubchild, mCalibDb->aec.LinearAeCtrl.BackLightConf.ExpLevel.fCoeff, i);
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_TARGETLLLUMA_TAG_ID)) {
            int i = (sizeof(mCalibDb->aec.LinearAeCtrl.BackLightConf.TargetLLLuma) / sizeof(mCalibDb->aec.LinearAeCtrl.BackLightConf.TargetLLLuma.fCoeff[0]));
            int no = ParseFloatArray(psecsubchild, mCalibDb->aec.LinearAeCtrl.BackLightConf.TargetLLLuma.fCoeff, i);
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else {
            redirectOut
                    << "parse error in LINAECTRL backlight section (unknow tag: "
                    << secsubTagname
                    << ")"
                    << std::endl;
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorLinearAECtrlOverExp
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_LINAECTRL_OVEREXP_CONTROL_TAG_ID, CALIB_SENSOR_AEC_LINEARAE_CTRL_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
#ifdef DEBUG_LOG
        redirectOut << "secsubTagname: " << secsubTagname << std::endl;
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_OVEREXP_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.OverExpCtrl.enable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_OVEREXP_LOWLIGHT_TH_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.OverExpCtrl.LowLightTh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_OVEREXP_HIGHLIGHT_TH_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.OverExpCtrl.HighLightTh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_OVEREXP_MAXWEIGHT_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->aec.LinearAeCtrl.OverExpCtrl.MaxWeight, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_OVEREXP_OEPDF_TAG_ID)) {
            int i = (sizeof(mCalibDb->aec.LinearAeCtrl.OverExpCtrl.OEpdf) / sizeof(mCalibDb->aec.LinearAeCtrl.OverExpCtrl.OEpdf.fCoeff[0]));
            int no = ParseFloatArray(psecsubchild, mCalibDb->aec.LinearAeCtrl.OverExpCtrl.OEpdf.fCoeff, i);
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_OVEREXP_HIGHLIGHT_WEIGHT_TAG_ID)) {
            int i = (sizeof(mCalibDb->aec.LinearAeCtrl.OverExpCtrl.HighLightWeight) / sizeof(mCalibDb->aec.LinearAeCtrl.OverExpCtrl.HighLightWeight.fCoeff[0]));
            int no = ParseFloatArray(psecsubchild, mCalibDb->aec.LinearAeCtrl.OverExpCtrl.HighLightWeight.fCoeff, i);
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_OVEREXP_LOWLIGHT_WEIGHT_TAG_ID)) {
            int i = (sizeof(mCalibDb->aec.LinearAeCtrl.OverExpCtrl.LowLightWeight) / sizeof(mCalibDb->aec.LinearAeCtrl.OverExpCtrl.LowLightWeight.fCoeff[0]));
            int no = ParseFloatArray(psecsubchild, mCalibDb->aec.LinearAeCtrl.OverExpCtrl.LowLightWeight.fCoeff, i);
            DCT_ASSERT((no == secsubtag.Size()));
        }
        else {
            redirectOut
                    << "parse error in LINAECTRL OverExpCtrl section (unknow tag: "
                    << secsubTagname
                    << ")"
                    << std::endl;
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorLinearAECtrl
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_LINEARAE_CTRL_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_RAWSTATS_EN_TAG_ID)) {
            int no = ParseUcharArray(psubchild, &mCalibDb->aec.LinearAeCtrl.RawStatsEn, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if(XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SETPOINT_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.LinearAeCtrl.SetPoint, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_NIGHTSETPOINT_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.LinearAeCtrl.NightSetPoint, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_DYSETPOINTEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.LinearAeCtrl.DySetPointEn;
            ParseUcharArray(psubchild, &temp, subTag.Size());
            mCalibDb->aec.LinearAeCtrl.DySetPointEn = (temp == 0) ? false : true;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_EVBIAS_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.LinearAeCtrl.Evbias, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_DYNAMICSETPOINT_TAG_ID)) {
            if (!parseEntryCell(psubchild->ToElement(), subTag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAecLinearAeDynamicPoint,
                                param,
                                (uint32_t)CALIB_SENSOR_AEC_DYNAMICSETPOINT_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AEC_LINEARAE_CTRL_TAG_ID)) {
                LOGE("parse error in AEC linear dynamic setpoint (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_TOLERANCE_IN_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.LinearAeCtrl.ToleranceIn, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_TOLERANCE_OUT_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.LinearAeCtrl.ToleranceOut, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_STRATEGYMODE_TAG_ID)) {
            char* value = Toupper(subTag.Value());
            std::string s_value(value);
#ifdef DEBUG_LOG
            LOGE("%s(%d): s_value = %s\n", __FUNCTION__, __LINE__, s_value);
#endif
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_STRATEGYMODE_LOWLIGHT) {
                    mCalibDb->aec.LinearAeCtrl.StrategyMode = RKAIQ_AEC_STRATEGY_MODE_LOWLIGHT_PRIOR;
                }
                else if (s_value == CALIB_SENSOR_AEC_STRATEGYMODE_HIGHLIGHT) {
                    mCalibDb->aec.LinearAeCtrl.StrategyMode = RKAIQ_AEC_STRATEGY_MODE_HIGHLIGHT_PRIOR;
                }
                else {
                    mCalibDb->aec.LinearAeCtrl.StrategyMode = RKAIQ_AEC_STRATEGY_MODE_AUTO;
                    redirectOut << "invalid AEC LinAe StrategyMode (" << s_value << ")" << std::endl;
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode *)psubchild;
                if (mCalibDb->aec.LinearAeCtrl.StrategyMode == RKAIQ_AEC_STRATEGY_MODE_LOWLIGHT_PRIOR)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_STRATEGYMODE_LOWLIGHT);
                else if (mCalibDb->aec.LinearAeCtrl.StrategyMode == RKAIQ_AEC_STRATEGY_MODE_HIGHLIGHT_PRIOR)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_STRATEGYMODE_HIGHLIGHT);
                else {
                    pNode->FirstChild()->SetValue("AUTO");
                    redirectOut << "(XML Wrtie)invalid AEC LinAe StrategyMode (" << mCalibDb->aec.LinearAeCtrl.StrategyMode << ")" << std::endl;
                }
            }

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_BACKLIGHT_CONFIG_TAG_ID)) {
            if (!parseEntrySensorLinearAECtrlBackLight(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LINAECTRL_OVEREXP_CONTROL_TAG_ID)) {
            if (!parseEntrySensorLinearAECtrlOverExp(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }

        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorHdrAECtrlExpRatioCtrl
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_HDRAECTRL_EXPRATIOCTRL_TAG_ID, CALIB_SENSOR_AEC_HDRAECTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_EXPRATIOTYPE_TAG_ID)) {
            char* value = Toupper(subsubTag.Value());
            std::string s_value(value);
#ifdef DEBUG_LOG
            redirectOut << "value:" << value << std::endl;
            redirectOut << s_value << std::endl;
#endif
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_HDRAECTRL_EXPRATIOTYPE_AUTO) {
                    mCalibDb->aec.HdrAeCtrl.ExpRatioType = RKAIQ_HDRAE_RATIOTYPE_MODE_AUTO;
                }
                else if (s_value == CALIB_SENSOR_AEC_HDRAECTRL_EXPRATIOTYPE_FIX) {
                    mCalibDb->aec.HdrAeCtrl.ExpRatioType = RKAIQ_HDRAE_RATIOTYPE_MODE_FIX;
                }
                else {
                    mCalibDb->aec.HdrAeCtrl.ExpRatioType = RKAIQ_HDRAE_RATIOTYPE_MODE_INVALID;
                    redirectOut << "invalid AEC HdrAe ExpRatioType (" << s_value << ")" << std::endl;
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psubsubchild;
                if (mCalibDb->aec.HdrAeCtrl.ExpRatioType == RKAIQ_HDRAE_RATIOTYPE_MODE_AUTO)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HDRAECTRL_EXPRATIOTYPE_AUTO);
                else if (mCalibDb->aec.HdrAeCtrl.ExpRatioType == RKAIQ_HDRAE_RATIOTYPE_MODE_FIX)
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HDRAECTRL_EXPRATIOTYPE_FIX);
                else
                {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HDRAECTRL_EXPRATIOTYPE_AUTO);
                    redirectOut << "(XML Write)invalid AEC HdrAe ExpRatioType (" << mCalibDb->aec.HdrAeCtrl.ExpRatioType << ")" << std::endl;
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_RATIOEXPDOT_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.RatioExpDot.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_M2SRATIOFIX_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.M2SRatioFix.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_L2MRATIOFIX_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.L2MRatioFix.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_M2SRATIOMAX_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.M2SRatioMax.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_L2MRATIOMAX_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.L2MRatioMax.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            redirectOut
                    << "parse error in AEC HdrAeCtrl (unknow tag: "
                    << subsubTagname
                    << ")"
                    << std::endl;
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorHdrAECtrlLframeMode
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_TAG_ID, CALIB_SENSOR_AEC_HDRAECTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_MODE_TAG_ID)) {
            char* value = Toupper(subsubTag.Value());
            std::string s_value(value);
#ifdef DEBUG_LOG
            redirectOut << "value:" << value << std::endl;
            redirectOut << s_value << std::endl;
#endif
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_NORMAL) {
                    mCalibDb->aec.HdrAeCtrl.LongfrmMode = RKAIQ_AEC_HDR_LONGFRMMODE_NORMAL;
                }
                else if (s_value == CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_LONGFRAME) {
                    mCalibDb->aec.HdrAeCtrl.LongfrmMode = RKAIQ_AEC_HDR_LONGFRMMODE_LONG_FRAME;
                }
                else if (s_value == CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_AUTO_LONGFRAME) {
                    mCalibDb->aec.HdrAeCtrl.LongfrmMode = RKAIQ_AEC_HDR_LONGFRMMODE_AUTO_LONG_FRAME;
                }
                else {
                    mCalibDb->aec.HdrAeCtrl.LongfrmMode = RKAIQ_AEC_HDR_LONGFRMMODE_NORMAL;
                    redirectOut << "invalid AEC HdrAe LongFrmMode (" << s_value << ")" << std::endl;
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psubsubchild;
                if (mCalibDb->aec.HdrAeCtrl.LongfrmMode == RKAIQ_AEC_HDR_LONGFRMMODE_NORMAL) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_NORMAL);
                }
                else if (mCalibDb->aec.HdrAeCtrl.LongfrmMode == RKAIQ_AEC_HDR_LONGFRMMODE_LONG_FRAME) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_LONGFRAME);
                }
                else if (mCalibDb->aec.HdrAeCtrl.LongfrmMode == RKAIQ_AEC_HDR_LONGFRMMODE_AUTO_LONG_FRAME) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_AUTO_LONGFRAME);
                }
                else {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_NORMAL);
                    redirectOut << "(XML Write)invalid AEC HdrAe LongFrmMode (" << mCalibDb->aec.HdrAeCtrl.LongfrmMode << ")" << std::endl;
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_LFRMMODEEXPTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.HdrAeCtrl.LfrmModeExpTh, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_SFRMMINLINE_TAG_ID)) {
            int no = ParseUshortArray(psubsubchild, &mCalibDb->aec.HdrAeCtrl.SfrmMinLine, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        } else {
            redirectOut
                    << "parse error in AEC LongFrmMode (unknow tag: "
                    << subsubTagname
                    << ")"
                    << std::endl;
        }
        psubsubchild = psubsubchild->NextSibling();
    }
    autoTabBackward();
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorHdrAECtrlLframeCtrl
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_HDRAECTRL_LFRAMECTRL_TAG_ID, CALIB_SENSOR_AEC_HDRAECTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    autoTabForward();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_OEROILOWTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.HdrAeCtrl.LframeCtrl.OEROILowTh, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LVHIGHTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.HdrAeCtrl.LframeCtrl.LvHighTh, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LVLOWTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.HdrAeCtrl.LframeCtrl.LvLowTh, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LEXPLEVEL_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.LframeCtrl.LExpLevel.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LSETPOINT_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.LframeCtrl.LSetPoint.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_TARGETLLLUMA_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.LframeCtrl.TargetLLLuma.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_NONOEPDFTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.LframeCtrl.NonOEPdfTh.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LOWLIGHTPDFTH_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.LframeCtrl.LowLightPdfTh.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            redirectOut
                    << "parse error in AEC HdrAeCtrl LframeCtrl (unknow tag: "
                    << subsubTagname
                    << ")"
                    << std::endl;
        }
        psubsubchild = psubsubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorHdrAECtrlMframeCtrl
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_HDRAECTRL_MFRAMECTRL_TAG_ID, CALIB_SENSOR_AEC_HDRAECTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_MEXPLEVEL_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.MframeCtrl.MExpLevel.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_MSETPOINT_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.MframeCtrl.MSetPoint.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            redirectOut
                    << "parse error in AEC HdrAeCtrl MframeCtrl (unknow tag: "
                    << subsubTagname
                    << ")"
                    << std::endl;
        }
        psubsubchild = psubsubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorHdrAECtrlSframeCtrl
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_HDRAECTRL_SFRAMECTRL_TAG_ID, CALIB_SENSOR_AEC_HDRAECTRL_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subsubTagname = %s\n", __FUNCTION__, __LINE__, subsubTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_SEXPLEVEL_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.SframeCtrl.SExpLevel.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_HLROIEXPANDEN_TAG_ID)) {
            uint8_t temp = mCalibDb->aec.HdrAeCtrl.SframeCtrl.HLROIExpandEn;
            int no = ParseUcharArray(psubsubchild, &temp, subsubTag.Size());
            mCalibDb->aec.HdrAeCtrl.SframeCtrl.HLROIExpandEn = (temp == 0) ? false : true;
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_TARGETHLLUMA_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.SframeCtrl.TargetHLLuma.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_SSETPOINT_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, mCalibDb->aec.HdrAeCtrl.SframeCtrl.SSetPoint.fCoeff, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_HLLUMATOLERANCE_TAG_ID)) {
            int no = ParseFloatArray(psubsubchild, &mCalibDb->aec.HdrAeCtrl.SframeCtrl.HLLumaTolerance, subsubTag.Size());
            DCT_ASSERT((no == subsubTag.Size()));
        }
        else {
            redirectOut
                    << "parse error in AEC HdrAeCtrl SframeCtrl (unknow tag: "
                    << subsubTagname
                    << ")"
                    << std::endl;
        }
        psubsubchild = psubsubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorHdrAECtrl
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_HDRAECTRL_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_TOLERANCE_IN_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.HdrAeCtrl.ToleranceIn, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_TOLERANCE_OUT_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.HdrAeCtrl.ToleranceOut, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_EVBIAS_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.HdrAeCtrl.Evbias, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_STRATEGYMODE_TAG_ID)) {
            char* value = Toupper(subTag.Value());
            std::string s_value(value);
#ifdef DEBUG_LOG
            redirectOut << "value:" << value << std::endl;
            redirectOut << s_value << std::endl;
#endif
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_STRATEGYMODE_LOWLIGHT) {
                    mCalibDb->aec.HdrAeCtrl.StrategyMode = RKAIQ_AEC_STRATEGY_MODE_LOWLIGHT_PRIOR;
                }
                else if (s_value == CALIB_SENSOR_AEC_STRATEGYMODE_HIGHLIGHT) {
                    mCalibDb->aec.HdrAeCtrl.StrategyMode = RKAIQ_AEC_STRATEGY_MODE_HIGHLIGHT_PRIOR;
                }
                else {
                    mCalibDb->aec.HdrAeCtrl.StrategyMode = RKAIQ_AEC_STRATEGY_MODE_AUTO;
                    redirectOut << "invalid AEC HdrAe StrategyMode (" << s_value << ")" << std::endl;
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psubchild;
                if (mCalibDb->aec.HdrAeCtrl.StrategyMode == RKAIQ_AEC_STRATEGY_MODE_LOWLIGHT_PRIOR) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_STRATEGYMODE_LOWLIGHT);
                }
                else if (mCalibDb->aec.HdrAeCtrl.StrategyMode == RKAIQ_AEC_STRATEGY_MODE_HIGHLIGHT_PRIOR) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_STRATEGYMODE_HIGHLIGHT);
                }
                else {
                    pNode->FirstChild()->SetValue("AUTO");
                    redirectOut << "(XML Write)invalid AEC HdrAe StrategyMode (" << mCalibDb->aec.HdrAeCtrl.StrategyMode << ")" << std::endl;
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_EXPRATIOCTRL_TAG_ID)) {
            if (!parseEntrySensorHdrAECtrlExpRatioCtrl(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LUMADISTTH_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.HdrAeCtrl.LumaDistTh, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LFRAMECTRL_TAG_ID)) {
            if (!parseEntrySensorHdrAECtrlLframeCtrl(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_MFRAMECTRL_TAG_ID)) {
            if (!parseEntrySensorHdrAECtrlMframeCtrl(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_SFRAMECTRL_TAG_ID)) {
            if (!parseEntrySensorHdrAECtrlSframeCtrl(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_LONGFRMMODE_TAG_ID)) {
            if (!parseEntrySensorHdrAECtrlLframeMode(psubchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", subTagname.c_str());
                return (false);
            }
        }
        else {
            redirectOut
                    << "parse error in AEC HdrAeCtrl  (unknow tag: "
                    << subTagname
                    << ")"
                    << std::endl;
        }
        psubchild = psubchild->NextSibling();
    }
    autoTabBackward();

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecEnvLvCalib
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_ENVLVCALIB_TAG_ID, CALIB_SENSOR_AEC_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ENVLVCALIB_CALIBFNUMBER_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->aec.CommCtrl.stEnvLvCalib.CalibFN, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ENVLVCALIB_CURVECOEFF_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->aec.CommCtrl.stEnvLvCalib.Curve.fCoeff, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in  AecEnvLv (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorAec
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;

    LOGI("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AEC_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

#ifdef DEBUG_LOG
        LOGE("%s(%d): tagname: [%s]\n", __FUNCTION__, __LINE__, tagname.c_str());
#endif

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ENABLE_TAG_ID)) {
            ParseUcharArray(pchild, &mCalibDb->aec.CommCtrl.enable, tag.Size());
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HISTSTATSMODE_TAG_ID)) {
            char* value = Toupper(tag.Value());
            char* value2 = Toupper(tag.Value());
            char* value3 = value2;
            int i = 0;
            while (*value != '\0') {
                if (*value == 'R') {
                    *value2++ = 'R';
                    i++;
                }
                else if (*value == 'G') {
                    *value2++ = 'G';
                    i++;
                }
                else if (*value == 'B') {
                    *value2++ = 'B';
                    i++;
                }
                else if (*value == 'Y') {
                    *value2++ = 'Y';
                    i++;
                }
                value++;
            }
            *value2 = '\0';
            std::string s_value(value3);
#ifdef DEBUG_LOG
            LOGE("%s(%d): value: %s\n", __FUNCTION__, __LINE__, s_value.c_str());
#endif
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_HISTSTATSMODE_R) {
                    mCalibDb->aec.CommCtrl.HistStatsMode = CAM_HIST_MODE_R;
                }
                else if (s_value == CALIB_SENSOR_AEC_HISTSTATSMODE_G) {
                    mCalibDb->aec.CommCtrl.HistStatsMode = CAM_HIST_MODE_G;
                }
                else if (s_value == CALIB_SENSOR_AEC_HISTSTATSMODE_B) {
                    mCalibDb->aec.CommCtrl.HistStatsMode = CAM_HIST_MODE_B;
                }
                else if (s_value == CALIB_SENSOR_AEC_HISTSTATSMODE_RGB) {
                    mCalibDb->aec.CommCtrl.HistStatsMode = CAM_HIST_MODE_RGB_COMBINED;
                }
                else if (s_value == CALIB_SENSOR_AEC_HISTSTATSMODE_Y) {
                    mCalibDb->aec.CommCtrl.HistStatsMode = CAM_HIST_MODE_Y;
                }
                else {
                    mCalibDb->aec.CommCtrl.HistStatsMode = CAM_HIST_MODE_INVALID;
                    LOGE("%s(%d): invalid AEC HistStatsMode = %s end\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)pchild;
                switch (mCalibDb->aec.CommCtrl.HistStatsMode)
                {
                case CAM_HIST_MODE_R:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HISTSTATSMODE_R);
                    break;
                case CAM_HIST_MODE_G:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HISTSTATSMODE_G);
                    break;
                case CAM_HIST_MODE_B:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HISTSTATSMODE_B);
                    break;
                case CAM_HIST_MODE_RGB_COMBINED:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HISTSTATSMODE_RGB);
                    break;
                case CAM_HIST_MODE_Y:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_HISTSTATSMODE_Y);
                    break;
                default:
                    pNode->FirstChild()->SetValue("INVALID");
                    LOGE("%s(%d): (XML Write)invalid AEC HistStatsMode = %d end\n", __FUNCTION__, __LINE__, mCalibDb->aec.CommCtrl.HistStatsMode);
                    break;
                }
            }

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_RAWSTATSMODE_TAG_ID)) {
            char* value = Toupper(tag.Value());
            char* value2 = Toupper(tag.Value());
            char* value3 = value2;
            int i = 0;
            while (*value != '\0') {
                if (*value == 'R') {
                    *value2++ = 'R';
                    i++;
                }
                else if (*value == 'G') {
                    *value2++ = 'G';
                    i++;
                }
                else if (*value == 'B') {
                    *value2++ = 'B';
                    i++;
                }
                else if (*value == 'Y') {
                    *value2++ = 'Y';
                    i++;
                }
                value++;
            }
            *value2 = '\0';
            std::string s_value(value3);
#ifdef DEBUG_LOG
            LOGE("%s(%d): value = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
#endif
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_RAWSTATSMODE_R) {
                    mCalibDb->aec.CommCtrl.RawStatsMode = CAM_RAWSTATS_MODE_R;
                }
                else if (s_value == CALIB_SENSOR_AEC_RAWSTATSMODE_G) {
                    mCalibDb->aec.CommCtrl.RawStatsMode = CAM_RAWSTATS_MODE_G;
                }
                else if (s_value == CALIB_SENSOR_AEC_RAWSTATSMODE_B) {
                    mCalibDb->aec.CommCtrl.RawStatsMode = CAM_RAWSTATS_MODE_B;
                }
                else if (s_value == CALIB_SENSOR_AEC_RAWSTATSMODE_Y) {
                    mCalibDb->aec.CommCtrl.RawStatsMode = CAM_RAWSTATS_MODE_Y;
                }
                else {
                    mCalibDb->aec.CommCtrl.RawStatsMode = CAM_RAWSTATS_MODE_INVALID;
                    LOGE("%s(%d): invalid AEC RawStatsMode = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)pchild;
                switch (mCalibDb->aec.CommCtrl.RawStatsMode)
                {
                case CAM_RAWSTATS_MODE_R:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_RAWSTATSMODE_R);
                    break;
                case CAM_RAWSTATS_MODE_G:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_RAWSTATSMODE_G);
                    break;
                case CAM_RAWSTATS_MODE_B:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_RAWSTATSMODE_B);
                    break;
                case CAM_RAWSTATS_MODE_Y:
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_RAWSTATSMODE_Y);
                    break;
                default:
                    pNode->FirstChild()->SetValue("INVALID");
                    LOGE("%s(%d): (XML Write)invalid AEC RawStatsMode = %d end\n", __FUNCTION__, __LINE__, mCalibDb->aec.CommCtrl.RawStatsMode);
                    break;
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_YRANGEMODE_TAG_ID)) {
            char* value = Toupper(tag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_YRANGEMODE_FULL) {
                    mCalibDb->aec.CommCtrl.YRangeMode = CAM_YRANGE_MODE_FULL;
                }
                else if (s_value == CALIB_SENSOR_AEC_YRANGEMODE_LIMITED) {
                    mCalibDb->aec.CommCtrl.YRangeMode = CAM_YRANGE_MODE_LIMITED;
                }
                else {
                    mCalibDb->aec.CommCtrl.YRangeMode = CAM_YRANGE_MODE_INVALID;
                    LOGE("%s(%d): invalid AEC YRangeMode = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)pchild;
                if (mCalibDb->aec.CommCtrl.YRangeMode == CAM_YRANGE_MODE_FULL) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_YRANGEMODE_FULL);
                }
                else if (mCalibDb->aec.CommCtrl.YRangeMode == CAM_YRANGE_MODE_LIMITED) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_YRANGEMODE_LIMITED);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid AEC YRangeMode = %d\n", __FUNCTION__, __LINE__, mCalibDb->aec.CommCtrl.YRangeMode);
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECRUNINTERVAL_TAG_ID)) {
            ParseUcharArray(pchild, &mCalibDb->aec.CommCtrl.AecRunInterval, tag.Size());
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECOPTYPE_TAG_ID)) {
            char* value = Toupper(tag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_AEC_AECOPTYPE_AUTO) {
                    mCalibDb->aec.CommCtrl.AecOpType = RK_AIQ_OP_MODE_AUTO;
                }
                else if (s_value == CALIB_SENSOR_AEC_AECOPTYPE_MANUAL) {
                    mCalibDb->aec.CommCtrl.AecOpType = RK_AIQ_OP_MODE_MANUAL;
                }
                else {
                    mCalibDb->aec.CommCtrl.AecOpType = RK_AIQ_OP_MODE_INVALID;
                    LOGE("%s(%d): invalid AEC AecOpType = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)pchild;
                if (mCalibDb->aec.CommCtrl.AecOpType == RK_AIQ_OP_MODE_AUTO) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_AECOPTYPE_AUTO);
                }
                else if (mCalibDb->aec.CommCtrl.AecOpType == RK_AIQ_OP_MODE_MANUAL) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_AEC_AECOPTYPE_MANUAL);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid AEC AecOpType = %d\n", __FUNCTION__, __LINE__, mCalibDb->aec.CommCtrl.AecOpType);
                }

            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SYNCTEST_TAG_ID)) {
            if (!parseEntrySensorAecSyncTest(pchild->ToElement()), param) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECSPEED_TAG_ID)) {
            if (!parseEntrySensorAecSpeed(pchild->ToElement()), param) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDELAYFRMNUM_TAG_ID)) {
            if (!parseEntrySensorAecDelayFrmNum(pchild->ToElement())) {
                LOGE("parse error in Aec (%s)", tagname.c_str(), param);
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECDNSWITCH_TAG_ID)) {
            if (!parseEntrySensorAecDNSwitch(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ANTIFLICKER_TAG_ID)) {
            if (!parseEntrySensorAecAntiFlicker(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECFRAMERATEMODE_TAG_ID)) {
            if (!parseEntrySensorAecFrameRateMode(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECINITVALUE_TAG_ID)) {
            if (!parseEntrySensorAecInitValue(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECGRIDWEIGHT_TAG_ID)) {
            if (!parseEntrySensorAecGridWeight(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_IRISCTRL_TAG_ID)) {
            if (!parseEntrySensorAecIrisCtrl(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MANUALCTRL_TAG_ID)) {
            if (!parseEntrySensorAecManualCtrl(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_AECROUTE_TAG_ID)) {
            if (!parseEntrySensorAecRoute(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ENVLVCALIB_TAG_ID)) {
            if (!parseEntrySensorAecEnvLvCalib(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_LINEARAE_CTRL_TAG_ID)) {
            if (!parseEntrySensorLinearAECtrl(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_HDRAECTRL_TAG_ID)) {
            if (!parseEntrySensorHdrAECtrl(pchild->ToElement(), param)) {
                LOGE("parse error in Aec (%s)", tagname.c_str());
                return (false);
            }
        }
        else {
            LOGW("parse error in AEC section (unknow tag:%s)", tagname.c_str());
            //return (false);
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();

    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecLinearAeRoute
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AEC_AECROUTE_LINEARAE_TAG_ID, CALIB_SENSOR_AEC_AECROUTE_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();

    int nTimeDot = 0;
    int nGainDot = 0;
    int nIspDGainDot = 0;
    int nPIrisDot = 0;

    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());
#ifdef DEBUG_LOG
        redirectOut << "Tagname: " << Tagname << ",index:" << index << std::endl;
#endif

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_NAME_TAG_ID)) {
            char* value = Toupper(tag.Value());
            ParseString(pchild,
                        mCalibDb->aec.CommCtrl.stAeRoute.LinAeSeperate[index].name,
                        sizeof(mCalibDb->aec.CommCtrl.stAeRoute.LinAeSeperate[index].name));
#ifdef DEBUG_LOG
            redirectOut << "value:" << value << std::endl;
            redirectOut << mCalibDb->aec.CommCtrl.stAeRoute.LinAeSeperate[index].name << std::endl;
#endif
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_TIMEDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.LinAeSeperate[index].TimeDot, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            nTimeDot = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_GAINDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.LinAeSeperate[index].GainDot, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            nGainDot = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_ISPDGAINDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.LinAeSeperate[index].IspgainDot, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            nIspDGainDot = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_PIRISDOT_TAG_ID)) {
            int no = ParseIntArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.LinAeSeperate[index].PIrisGainDot, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            nPIrisDot = no;
        }
        else {
            redirectOut
                    << "parse error in LinAe Route (unknow tag: "
                    << Tagname
                    << ")"
                    << std::endl;
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((nGainDot == nTimeDot));
    DCT_ASSERT((nGainDot == nIspDGainDot));
    mCalibDb->aec.CommCtrl.stAeRoute.LinAeSeperate[index].array_size = nTimeDot;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecHdrAeRoute
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);


    XML_CHECK_START(CALIB_SENSOR_AEC_AECROUTE_HDRAE_TAG_ID, CALIB_SENSOR_AEC_AECROUTE_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    int nTimeDot = 0;
    int nGainDot = 0;
    int nIspDGainDot = 0;
    int nPIrisDot = 0;

    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_NAME_TAG_ID)) {
            char* value = Toupper(tag.Value());
            ParseString(pchild,
                        mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].name,
                        sizeof(mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].name));
#ifdef DEBUG_LOG
            redirectOut << "value:" << value << std::endl;
            redirectOut << mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].name << std::endl;
#endif
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_LTIMEDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrTimeDot[2], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            nTimeDot = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MTIMEDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrTimeDot[1], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            DCT_ASSERT((nTimeDot == no));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_STIMEDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrTimeDot[0], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            DCT_ASSERT((nTimeDot == no));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_LGAINDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrGainDot[2], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            nGainDot = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MGAINDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrGainDot[1], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            DCT_ASSERT((no == nGainDot));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SGAINDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrGainDot[0], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            DCT_ASSERT((no == nGainDot));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_LISPDGAINDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrIspDGainDot[2], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            nIspDGainDot = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_MISPDGAINDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrIspDGainDot[1], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            DCT_ASSERT((nIspDGainDot == no));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_SISPDGAINDOT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].HdrIspDGainDot[0], tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            DCT_ASSERT((nIspDGainDot == no));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_PIRISDOT_TAG_ID)) {
            int no = ParseIntArray(pchild, mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].PIrisGainDot, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_ROUTE_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_ROUTE_MAX_NODES);
                return false;
            }
            nPIrisDot = no;
        }
        else {
            redirectOut
                    << "parse error in HdrAe Route (unknow tag: "
                    << Tagname
                    << ")"
                    << std::endl;
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((nGainDot == nTimeDot));
    DCT_ASSERT((nGainDot == nIspDGainDot));
    mCalibDb->aec.CommCtrl.stAeRoute.HdrAeSeperate[index].array_size = nTimeDot;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAecLinearAeDynamicPoint
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AEC_DYNAMICSETPOINT_TAG_ID, CALIB_SENSOR_AEC_LINEARAE_CTRL_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();

    int nExpValue = 0;
    int nDysetpoint = 0;
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());
        INFO_PRINT(Tagname);
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_NAME_TAG_ID)) {
            char* value = Toupper(tag.Value());
            ParseString(pchild,
                        mCalibDb->aec.LinearAeCtrl.DySetpoint[index].name,
                        sizeof(mCalibDb->aec.LinearAeCtrl.DySetpoint[index].name));
#ifdef DEBUG_LOG
            redirectOut << "value:" << value << std::endl;
            redirectOut << mCalibDb->aec.LinearAeCtrl.DySetpoint[index].name << std::endl;
#endif
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_EXPLEVEL_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.LinearAeCtrl.DySetpoint[index].ExpValue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_SETPOINT_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_SETPOINT_MAX_NODES);
                return false;
            }
            nExpValue = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AEC_DYSETPOINT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->aec.LinearAeCtrl.DySetpoint[index].DySetpoint, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            if(tag.Size() > AEC_SETPOINT_MAX_NODES) {
                LOGE("array size: %d out of Maximum range(%d)", tag.Size(), AEC_SETPOINT_MAX_NODES);
                return false;
            }
            nDysetpoint = no;
        }
        else {
            redirectOut
                    << "parse error in DynamicPoint (unknow tag: "
                    << Tagname
                    << ")"
                    << std::endl;
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    DCT_ASSERT((nDysetpoint == nExpValue));
    mCalibDb->aec.LinearAeCtrl.DySetpoint[index].array_size = nDysetpoint;

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorInfoGainRange
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_SENSORINFO_GAINRANGE_TAG_ID, CALIB_SENSOR_SENSORINFO_TAG_ID);

    const XMLNode* pthrdsubchild = pelement->FirstChild();
    while (pthrdsubchild) {
        XmlTag secsubTag = XmlTag(pthrdsubchild->ToElement());
        const char* value = secsubTag.Value();
        std::string secsubTagname(pthrdsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubTag.Type(), secsubTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_GAINRANGE_ISLINEAR_TAG_ID)) {
            uint8_t tempVal = mCalibDb->sensor.GainRange.IsLinear;
            int no = ParseUcharArray(pthrdsubchild, &tempVal, 1);
            mCalibDb->sensor.GainRange.IsLinear = (tempVal == 0 ? false : true);
            DCT_ASSERT((no == secsubTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_GAINRANGE_LINEAR_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) {
                int i = secsubTag.Size();
                int no = ParseFloatArray(pthrdsubchild, mCalibDb->sensor.GainRange.pGainRange, i);
                DCT_ASSERT((no == secsubTag.Size()));
                DCT_ASSERT((no <= CALD_AEC_GAIN_RANGE_MAX_LEN));
                DCT_ASSERT(((i % 7) == 0));
                mCalibDb->sensor.GainRange.array_size = i;
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE) {
                char str[10];
                int i = mCalibDb->sensor.GainRange.array_size;
                DCT_ASSERT(((i % 7) == 0));
                snprintf(str, sizeof(str), "[%u 7]", (i / 7));
                XMLElement* pElement = (((XMLNode*)pthrdsubchild)->ToElement());
                pElement->SetAttribute(CALIB_ATTRIBUTE_SIZE, str);
                int no = ParseFloatArray(pthrdsubchild, mCalibDb->sensor.GainRange.pGainRange, i);
                DCT_ASSERT((no <= CALD_AEC_GAIN_RANGE_MAX_LEN));
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_GAINRANGE_NONLINEAR_TAG_ID)) {
            char* value = Toupper(secsubTag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SENSOR_SENSORINFO_GAINRANGE_NONLINEAR_DB) {
                    mCalibDb->sensor.GainRange.GainMode = RKAIQ_EXPGAIN_MODE_NONLINEAR_DB;
                }
                else {
                    mCalibDb->sensor.GainRange.GainMode = RKAIQ_EXPGAIN_MODE_NONLINEAR_DB;
                    LOGE("%s(%d): invalid GainRange Mode = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)pthrdsubchild;
                if (mCalibDb->sensor.GainRange.GainMode == RKAIQ_EXPGAIN_MODE_NONLINEAR_DB) {
                    pNode->FirstChild()->SetValue(CALIB_SENSOR_SENSORINFO_GAINRANGE_NONLINEAR_DB);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid GainRange Mode = %d\n", __FUNCTION__, __LINE__, mCalibDb->sensor.GainRange.GainMode);
                }
            }
        }
        pthrdsubchild = pthrdsubchild->NextSibling();
    }

    const XMLNode* pchild = pelement->FirstChild();


    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorInfo
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGI("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_SENSORINFO_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, tagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_GAINRANGE_TAG_ID)) {
            if (!parseEntrySensorInfoGainRange(pchild->ToElement())) {
                LOGE("parse error in SensorInfo GainRange (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_TIMEFACTOR_TAG_ID)) {
            int i = (sizeof(mCalibDb->sensor.TimeFactor) / sizeof(mCalibDb->sensor.TimeFactor[0]));
            int no = ParseFloatArray(pchild, mCalibDb->sensor.TimeFactor, i);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISHDRTIMEREG_SUMFAC_TAG_ID)) {
            int i = (sizeof(mCalibDb->sensor.CISHdrTimeRegSumFac) / sizeof(mCalibDb->sensor.CISHdrTimeRegSumFac.fCoeff[0]));
            int no = ParseFloatArray(pchild, mCalibDb->sensor.CISHdrTimeRegSumFac.fCoeff, i);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISLINTIMEREG_MAXFAC_TAG_ID)) {
            int i = (sizeof(mCalibDb->sensor.CISLinTimeRegMaxFac) / sizeof(mCalibDb->sensor.CISLinTimeRegMaxFac.fCoeff[0]));
            int no = ParseFloatArray(pchild, mCalibDb->sensor.CISLinTimeRegMaxFac.fCoeff, i);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISTIMEREG_ODEVITY_TAG_ID)) {
            int i = (sizeof(mCalibDb->sensor.CISTimeRegOdevity) / sizeof(mCalibDb->sensor.CISTimeRegOdevity.fCoeff[0]));
            int no = ParseFloatArray(pchild, mCalibDb->sensor.CISTimeRegOdevity.fCoeff, i);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISHDRTIMEREG_ODEVITY_TAG_ID)) {
            int i = (sizeof(mCalibDb->sensor.CISHdrTimeRegOdevity) / sizeof(mCalibDb->sensor.CISHdrTimeRegOdevity.fCoeff[0]));
            int no = ParseFloatArray(pchild, mCalibDb->sensor.CISHdrTimeRegOdevity.fCoeff, i);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISTIMEREG_MIN_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->sensor.CISTimeRegMin, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISHDRTIMEREG_MIN_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->sensor.CISHdrTimeRegMin, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISHDRTIMEREG_MAX_TAG_ID)) {
            int i = (sizeof(mCalibDb->sensor.CISHdrTimeRegMax) / sizeof(mCalibDb->sensor.CISHdrTimeRegMax.Coeff[0]));
            int no = ParseShortArray(pchild, mCalibDb->sensor.CISHdrTimeRegMax.Coeff, i);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISTIMEREG_UNEQUALEN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->sensor.CISTimeRegUnEqualEn, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISMINFPS_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->sensor.CISMinFps, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISAGAIN_RANGE_TAG_ID)) {
            float tempVal[2] = { mCalibDb->sensor.CISAgainRange.Min, mCalibDb->sensor.CISAgainRange.Max };
            int no = ParseFloatArray(pchild, tempVal, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->sensor.CISAgainRange.Min = tempVal[0];
            mCalibDb->sensor.CISAgainRange.Max = tempVal[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISEXTRAAGAIN_RANGE_TAG_ID)) {
            float tempVal[2] = { mCalibDb->sensor.CISExtraAgainRange.Min, mCalibDb->sensor.CISExtraAgainRange.Max };
            int no = ParseFloatArray(pchild, tempVal, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->sensor.CISExtraAgainRange.Min = tempVal[0];
            mCalibDb->sensor.CISExtraAgainRange.Max = tempVal[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISDGAIN_RANGE_TAG_ID)) {
            float tempVal[2] = { mCalibDb->sensor.CISDgainRange.Min, mCalibDb->sensor.CISDgainRange.Max };
            int no = ParseFloatArray(pchild, tempVal, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->sensor.CISDgainRange.Min = tempVal[0];
            mCalibDb->sensor.CISDgainRange.Max = tempVal[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISISPDGAIN_RANGE_TAG_ID)) {
            float tempVal[2] = { mCalibDb->sensor.CISIspDgainRange.Min, mCalibDb->sensor.CISIspDgainRange.Max };
            int no = ParseFloatArray(pchild, tempVal, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->sensor.CISIspDgainRange.Min = tempVal[0];
            mCalibDb->sensor.CISIspDgainRange.Max = tempVal[1];
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORINFO_CISHDRGAININDSETEN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->sensor.CISHdrGainIndSetEn, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SENSORSETTING_FLIP_ID)) {
            unsigned char flip = mCalibDb->sensor.flip;
            int no = ParseUcharArray(pchild, &flip, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->sensor.flip = flip;
        }
        else {
            redirectOut
                    << "parse error in SensorInfo section (unknow tag: "
                    << tagname
                    << ")"
                    << std::endl;
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorModuleInfo
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGI("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_MODULEINFO_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
#ifdef DEBUG_LOG
        LOGE("%s(%d): Tagname = %s\n", __FUNCTION__, __LINE__, tagname.c_str());
#endif
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MODULEINFO_FNUMBER_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->module.FNumber, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MODULEINFO_EFL_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->module.EFL, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MODULEINFO_LENS_TRANSMITTANCE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->module.LensT, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MODULEINFO_IRCUT_TRANSMITTANCE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->module.IRCutT, 1);
            DCT_ASSERT((no == tag.Size()));
        }
        else {
            redirectOut
                    << "parse error in ModuleInfo section (unknow tag: "
                    << tagname
                    << ")"
                    << std::endl;
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAhdrMerge
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AHDR_MERGE_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_ENVLV_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.merge.envLevel, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_OECURVE_SMOOTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.merge.oeCurve_smooth, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_OECURVE_OFFSET_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.merge.oeCurve_offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_MOVECOEF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.merge.moveCoef, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_MDCURVELM_SMOOTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.merge.mdCurveLm_smooth, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_MDCURVELM_OFFSET_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.merge.mdCurveLm_offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_MDCURVEMS_SMOOTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.merge.mdCurveMs_smooth, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_MDCURVEMS_OFFSET_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.merge.mdCurveMs_offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_OECURVE_DAMP_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.merge.oeCurve_damp, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_MDCURVELM_DAMP_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.merge.mdCurveLm_damp, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_MERGE_MDCURVEMS_DAMP_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.merge.mdCurveMs_damp, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAhdrTmo
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AHDR_TMO_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_TMO_EN_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAhdrTmoEn,
                                param,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_TMO_EN_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_TAG_ID)) {
                LOGE("parse error in TMO GlobalLuma Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALLUMA_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAhdrTmoGlobalLuma,
                                param,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_GLOBALLUMA_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_TAG_ID)) {
                LOGE("parse error in TMO GlobalLuma Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DETAILSHIGHLIGHT_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAhdrTmoDetailsHighLight,
                                param,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_DETAILSHIGHLIGHT_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_TAG_ID)) {
                LOGE("parse error in TMO DetailsHighLight Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DETAILSLOWLIGHT_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAhdrTmoDetailsLowLight,
                                param,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_DETAILSLOWLIGHT_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_TAG_ID)) {
                LOGE("parse error in TMO DetailsLowLight Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAhdrGlobalTMO,
                                param,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_GLOBALTMO_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_TAG_ID)) {
                LOGE("parse error in TMO GlobalTMO Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_LOCALTMO_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAhdrLocalTMO,
                                param,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_LOCALTMO_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AHDR_TMO_TAG_ID)) {
                LOGE("parse error in TMO LocalTMO Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DAMP_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.damp, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAhdrTmoEn
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AHDR_TMO_TMO_EN_TAG_ID, CALIB_SENSOR_AHDR_TMO_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->ahdr.tmo.en[index].name, sizeof(mCalibDb->ahdr.tmo.en[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_TMO_EN_CELL_EN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.en[index].en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAhdrTmoGlobalLuma
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AHDR_TMO_GLOBALLUMA_TAG_ID, CALIB_SENSOR_AHDR_TMO_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->ahdr.tmo.luma[index].name, sizeof(mCalibDb->ahdr.tmo.luma[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALLUMAMODE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.luma[index].GlobalLumaMode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_ENVLV_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.luma[index].envLevel, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.luma[index].ISO, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TOLERANCE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.luma[index].Tolerance, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALLUMA_GLOBALLUMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.luma[index].globalLuma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAhdrTmoDetailsHighLight
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AHDR_TMO_DETAILSHIGHLIGHT_TAG_ID, CALIB_SENSOR_AHDR_TMO_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->ahdr.tmo.HighLight[index].name, sizeof(mCalibDb->ahdr.tmo.HighLight[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DETAILSHIGHLIGHTMODE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.HighLight[index].DetailsHighLightMode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_OEPDF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.HighLight[index].OEPdf, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_ENVLV_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.HighLight[index].EnvLv, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TOLERANCE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.HighLight[index].Tolerance, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DETAILSHIGHLIGHT_DETAILSHIGHLIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.HighLight[index].detailsHighLight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAhdrTmoDetailsLowLight
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AHDR_TMO_DETAILSLOWLIGHT_TAG_ID, CALIB_SENSOR_AHDR_TMO_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->ahdr.tmo.LowLight[index].name, sizeof(mCalibDb->ahdr.tmo.LowLight[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DETAILSLOWLIGHTMODE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.LowLight[index].DetailsLowLightMode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_FOCUSLUMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.LowLight[index].FocusLuma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DARKPDF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.LowLight[index].DarkPdf, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.LowLight[index].ISO, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TOLERANCE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.LowLight[index].Tolerance, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DETAILSLOWLIGHT_DETAILSLOWLIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.LowLight[index].detailsLowLight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAhdrLocalTMO
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AHDR_TMO_LOCALTMO_TAG_ID, CALIB_SENSOR_AHDR_TMO_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->ahdr.tmo.LocalTMO[index].name, sizeof(mCalibDb->ahdr.tmo.LocalTMO[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_LOCALTMOMODE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.LocalTMO[index].LocalTMOMode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_DYNAMICRANGE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.LocalTMO[index].DynamicRange, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_ENVLV_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.LocalTMO[index].EnvLv, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TOLERANCE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.LocalTMO[index].Tolerance, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_LOCALTMO_STRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.LocalTMO[index].Strength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAhdrGlobalTMO
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_TAG_ID, CALIB_SENSOR_AHDR_TMO_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->ahdr.tmo.GlobaTMO[index].name, sizeof(mCalibDb->ahdr.tmo.GlobaTMO[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_EN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.GlobaTMO[index].en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_IIR_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.GlobaTMO[index].iir, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_MODE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.GlobaTMO[index].mode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_DYNAMICRANGE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.GlobaTMO[index].DynamicRange, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_ENVLV_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.GlobaTMO[index].EnvLv, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_TOLERANCE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->ahdr.tmo.GlobaTMO[index].Tolerance, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AHDR_TMO_GLOBALTMO_STRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->ahdr.tmo.GlobaTMO[index].Strength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAWDR
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWDR_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_ENABLE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awdr.Enbale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAWDRMode,
                                param,
                                (uint32_t)CALIB_SENSOR_AWDR_MODE_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AWDR_TAG_ID)) {
                LOGE("parse error in TMO GlobalLuma Setting (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAWDRMode
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AWDR_MODE_TAG_ID, CALIB_SENSOR_AWDR_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_NAME_TAG_ID)) {
            ParseString(pchild,  mCalibDb->awdr.Mode[index].name, sizeof(mCalibDb->awdr.Mode[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_ENABLE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awdr.Mode[index].SceneEnbale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_MODE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->awdr.Mode[index].mode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_STRENGTH_TAG_ID)) {
            if (!parseEntrySensorAWDRModeStrength(pchild->ToElement(), index)) {
                LOGE("parse error in wdr Strength(%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_TAG_ID)) {
            if (!parseEntrySensorAWDRModeConfig(pchild->ToElement(), index)) {
                LOGE("parse error in wdr Config(%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAWDRModeStrength
(
    const XMLElement*   pelement,
    int         index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWDR_MODE_SCENE_STRENGTH_TAG_ID, CALIB_SENSOR_AWDR_MODE_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();

    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_STRENGTH_ENVLV_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awdr.Mode[index].WdrStrength.Envlv, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_STRENGTH_LEVEL_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awdr.Mode[index].WdrStrength.Level, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_STRENGTH_DAMP_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrStrength.damp, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_STRENGTH_TOLERENCE_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrStrength.Tolerance, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }

        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAWDRModeConfig
(
    const XMLElement*   pelement,
    int         index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_TAG_ID, CALIB_SENSOR_AWDR_MODE_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();

    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_LOCAL_CURVE_TAG)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awdr.Mode[index].WdrConfig.LocalCurve, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_GLOBAL_CURVE_TAG)) {
            int no = ParseFloatArray(psubchild, mCalibDb->awdr.Mode[index].WdrConfig.GlobalCurve, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_NOISE_RATIO_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_noiseratio, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_BEST_LIGHT_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_bestlight, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_GAIN_OFF1_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_gain_off1, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_PYM_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_pym_cc, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_EPSILON_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_epsilon, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_LVL_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_lvl_en, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_FLT_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_flt_sel, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_GAIN_MAX_CLIP_ENABLE_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_gain_max_clip_enable, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_BAVG_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_bavg_clip, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_NONL_SEGM_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_nonl_segm, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_NONL_OPEN_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_nonl_open, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_NONL_MODE1_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_nonl_mode1, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_COE0_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_coe0, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_COE1_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_coe1, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_COE2_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_coe2, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AWDR_MODE_SCENE_CONFIG_COE_OFF_TAG)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->awdr.Mode[index].WdrConfig.wdr_coe_off, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }

        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorBlcModeCell
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    int index = *((int*)param);
    XML_CHECK_START(CALIB_SENSOR_BLC_MODE_CELL_TAG_ID, CALIB_SENSOR_BLC_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BLC_MODE_NAME_TAG_ID)) {
            ParseString(pchild,  mCalibDb->blc.mode_cell[index].name, sizeof(mCalibDb->blc.mode_cell[index].name));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BLC_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->blc.mode_cell[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BLC_BLACK_LEVEL_TAG_ID)) {
            int no = ParseFloatArray(pchild, (float *)mCalibDb->blc.mode_cell[index].level, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();

    }

    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorBlc
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_BLC_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BLC_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->blc.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BLC_MODE_CELL_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorBlcModeCell,
                                param,
                                (uint32_t)CALIB_SENSOR_BLC_MODE_CELL_TAG_ID,
                                (uint32_t)CALIB_SENSOR_BLC_TAG_ID)) {
                LOGE("parse error in blc mode cell (%s)", tagname.c_str());
                return (false);
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorLut3d
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_LUT3D_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUT3D_ENABLE_TAG_ID)) {
            unsigned char enable = mCalibDb->lut3d.enable;
            int no = ParseUcharArray(pchild, &enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->lut3d.enable = enable;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUT3D_R_LUT_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lut3d.look_up_table_r, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUT3D_G_LUT_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lut3d.look_up_table_g, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUT3D_B_LUT_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lut3d.look_up_table_b, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpcc
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_DPCC_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->dpcc.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_VERSION_TAG_ID)) {
            ParseString(pchild, mCalibDb->dpcc.version,  sizeof(mCalibDb->dpcc.version));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_TAG_ID)) {
            if (!parseEntrySensorDpccFastMode(pchild->ToElement())) {
                LOGE("parse error in dpcc Fast Mode (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_EXPERT_MODE_TAG_ID)) {
            if (!parseEntrySensorDpccExpertMode(pchild->ToElement())) {
                LOGE("parse error in dpcc Expert Mode (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_TAG_ID)) {
            if (!parseEntrySensorDpccPdaf(pchild->ToElement())) {
                LOGE("parse error in dpcc pdaf (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SENSOR_TAG_ID)) {
            if (!parseEntrySensorDpccSensor(pchild->ToElement())) {
                LOGE("parse error in DPCC Sensor (%s)", tagname.c_str());
                return (false);
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccFastMode
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_DPCC_FAST_MODE_TAG_ID, CALIB_SENSOR_DPCC_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->dpcc.fast.fast_mode_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_ISO_TAG_ID)) {
            int no = ParseIntArray(pchild, mCalibDb->dpcc.fast.ISO, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_SINGLE_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->dpcc.fast.fast_mode_single_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_SINGLE_LEVEL_TAG_ID)) {
            int no = ParseIntArray(pchild, mCalibDb->dpcc.fast.fast_mode_single_level, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_DOUBLE_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->dpcc.fast.fast_mode_double_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_DOUBLE_LEVEL_TAG_ID)) {
            int no = ParseIntArray(pchild, mCalibDb->dpcc.fast.fast_mode_double_level, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_TRIPLE_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->dpcc.fast.fast_mode_triple_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_FAST_MODE_TRIPLE_LEVEL_TAG_ID)) {
            int no = ParseIntArray(pchild, mCalibDb->dpcc.fast.fast_mode_triple_level, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccExpertMode
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_DPCC_EXPERT_MODE_TAG_ID, CALIB_SENSOR_DPCC_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dpcc.expert.iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_Enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_GRAYSCALE_MODE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dpcc.expert.grayscale_mode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RK_OUT_SEL_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.rk_out_sel, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_DPCC_OUT_SEL_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.dpcc_out_sel, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_RB_3X3_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_rb_3x3, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_G_3X3_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_g_3x3, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_INC_RB_CENTER_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_inc_rb_center, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_INC_G_CENTER_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_inc_g_center, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_USE_FIX_SET_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_use_fix_set, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_USE_SET1_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_use_set1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_USE_SET2_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_use_set2, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }  else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_STAGE1_USE_SET3_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.expert.stage1_use_set3, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SET_CELL_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorDpccSetCell,
                                param,
                                (uint32_t)CALIB_SENSOR_DPCC_SET_CELL_TAG_ID,
                                (uint32_t)CALIB_SENSOR_DPCC_EXPERT_MODE_TAG_ID)) {
                LOGE("parse error in dpcc set cell (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_TAG_ID)) {
            if (!parseEntrySensorDpccPdaf(pchild->ToElement())) {
                LOGE("parse error in dpcc pdaf (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SENSOR_TAG_ID)) {
            if (!parseEntrySensorDpccSensor(pchild->ToElement())) {
                LOGE("parse error in DPCC Sensor (%s)", tagname.c_str());
                return (false);
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccSetCell
(
    const XMLElement*   pelement,
    void*               param
) {
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_DPCC_SET_CELL_TAG_ID, CALIB_SENSOR_DPCC_EXPERT_MODE_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SET_CELL_RK_TAG_ID)) {
            if (!parseEntrySensorDpccSetCellRK(pchild->ToElement(), index)) {
                LOGE("parse error in dpcc set cell RK (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SET_CELL_LC_TAG_ID)) {
            if (!parseEntrySensorDpccSetCellLC(pchild->ToElement(), index)) {
                LOGE("parse error in dpcc set cell LC (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SET_CELL_PG_TAG_ID)) {
            if (!parseEntrySensorDpccSetCellPG(pchild->ToElement(), index)) {
                LOGE("parse error in dpcc set cell PG (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SET_CELL_RND_TAG_ID)) {
            if (!parseEntrySensorDpccSetCellRND(pchild->ToElement(), index)) {
                LOGE("parse error in dpcc set cell RND (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SET_CELL_RG_TAG_ID)) {
            if (!parseEntrySensorDpccSetCellRG(pchild->ToElement(), index)) {
                LOGE("parse error in dpcc set cell RG (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SET_CELL_RO_TAG_ID)) {
            if (!parseEntrySensorDpccSetCellRO(pchild->ToElement(), index)) {
                LOGE("parse error in dpcc set cell RO (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccSetCellRK
(
    const XMLElement*   pelement,
    int index
) {
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_DPCC_SET_CELL_RK_TAG_ID, CALIB_SENSOR_DPCC_SET_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());


        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RK_RED_BLUE_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rk.rb_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RK_GREEN_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rk.g_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RK_RED_BLUE_SW_MINDIS_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rk.rb_sw_mindis, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RK_GREEN_SW_MINDIS_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rk.g_sw_mindis, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RK_SW_DIS_SCALE_MIN_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rk.sw_dis_scale_min, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RK_SW_DIS_SCALE_MAX_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rk.sw_dis_scale_max, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccSetCellLC
(
    const XMLElement*   pelement,
    int index
) {
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_DPCC_SET_CELL_LC_TAG_ID, CALIB_SENSOR_DPCC_SET_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());


        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_LC_RED_BLUE_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].lc.rb_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_LC_GREEN_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].lc.g_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_LC_RED_BLUE_LINE_MAD_FAC_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].lc.rb_line_mad_fac, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_LC_GREEN_LINE_MAD_FAC_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].lc.g_line_mad_fac, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_LC_RED_BLUE_LINE_THR_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].lc.rb_line_thr, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_LC_GREEN_LINE_THR_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].lc.g_line_thr, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccSetCellPG
(
    const XMLElement*   pelement,
    int index
) {
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_DPCC_SET_CELL_PG_TAG_ID, CALIB_SENSOR_DPCC_SET_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());


        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PG_RED_BLUE_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].pg.rb_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PG_GREEN_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].pg.g_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PG_RED_BLUE_FAC_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].pg.rb_pg_fac, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PG_GREEN_FAC_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].pg.g_pg_fac, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccSetCellRND
(
    const XMLElement*   pelement,
    int index
) {
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_DPCC_SET_CELL_RND_TAG_ID, CALIB_SENSOR_DPCC_SET_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());


        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RND_RED_BLUE_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rnd.rb_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RND_GREEN_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rnd.g_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RND_RED_BLUE_THR_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rnd.rb_rnd_thr, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RND_GREEN_THR_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rnd.g_rnd_thr, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RND_RED_BLUE_OFFS_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rnd.rb_rnd_offs, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RND_GREEN_OFFS_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rnd.g_rnd_offs, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccSetCellRG
(
    const XMLElement*   pelement,
    int index
) {
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_DPCC_SET_CELL_RG_TAG_ID, CALIB_SENSOR_DPCC_SET_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());


        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RG_RED_BLUE_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rg.rb_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RG_GREEN_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rg.g_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RG_RED_BLUE_FAC_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rg.rb_rg_fac, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RG_GREEN_FAC_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].rg.g_rg_fac, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccSetCellRO
(
    const XMLElement*   pelement,
    int index
) {
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_DPCC_SET_CELL_RO_TAG_ID, CALIB_SENSOR_DPCC_SET_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());


        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RO_RED_BLUE_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].ro.rb_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RO_GREEN_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].ro.g_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RO_RED_BLUE_LIM_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].ro.rb_ro_lim, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_RO_GREEN_LIM_TAG_ID)) {
            int no = ParseUcharArray(pchild, (uint8_t *)mCalibDb->dpcc.expert.set[index].ro.g_ro_lim, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccPdaf
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_DPCC_PDAF_TAG_ID, CALIB_SENSOR_DPCC_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dpcc.pdaf.en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_POINT_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.pdaf.point_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_OFFSETX_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->dpcc.pdaf.offsetx, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_OFFSETY_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->dpcc.pdaf.offsety, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_WRAPX_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dpcc.pdaf.wrapx, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_WRAPY_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dpcc.pdaf.wrapy, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_WRAPX_NUM_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->dpcc.pdaf.wrapx_num, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_WRAPY_NUM_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->dpcc.pdaf.wrapy_num, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_POINT_X_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.pdaf.point_x, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_POINT_Y_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dpcc.pdaf.point_y, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_PDAF_POINT_FORWARD_MED_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dpcc.pdaf.forward_med, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDpccSensor
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_DPCC_SENSOR_TAG_ID, CALIB_SENSOR_DPCC_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string Tagname(pchild->ToElement()->Name());

        XML_CHECK_WHILE_SUBTAG_MARK((char *)(Tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SENSOR_AUTO_ENABLE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dpcc.sensor_dpcc.en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SENSOR_MAX_LEVEL_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dpcc.sensor_dpcc.max_level, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dpcc.sensor_dpcc.iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SENSOR_LEVEL_SINGLE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dpcc.sensor_dpcc.level_single, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DPCC_SENSOR_LEVEL_MULTIPLE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dpcc.sensor_dpcc.level_multiple, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorBayerNrSetting
(
    const XMLElement*   pelement,
    void*               param,
    int                 index
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    CalibDb_BayerNr_ModeCell_t *pModeCell = (CalibDb_BayerNr_ModeCell_t *)param;

    XML_CHECK_START(CALIB_SENSOR_BAYERNR_SETTING_TAG_ID, CALIB_SENSOR_BAYERNR_MODE_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_SETTING_SNR_MODE_TAG_ID)) {
            ParseString(pchild,  pModeCell->setting[index].snr_mode, sizeof(pModeCell->setting[index].snr_mode));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_SETTING_SENSOR_MODE_TAG_ID)) {
            ParseString(pchild,  pModeCell->setting[index].sensor_mode, sizeof(pModeCell->setting[index].sensor_mode));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_FILTPARA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].filtPara, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_LURATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, (float*)pModeCell->setting[index].luRatio, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_FIXW_TAG_ID)) {
            int no = ParseFloatArray(pchild, (float*)pModeCell->setting[index].fixW, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_LULEVEL_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].luLevel, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_LULEVELVAL_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].luLevelVal, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_LAMDA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].lamda, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_GAUSS_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &pModeCell->setting[index].gauss_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_RGAINOFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].RGainOff, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_RGAINFILP_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].RGainFilp, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_BGAINOFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].BGainOff, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_BGAINFILP_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].BGainFilp, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_EDGESOFTNESS_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].edgeSoftness, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_GAUSSWEIGHT0_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].gaussWeight0, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_GAUSSWEIGHT1_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].gaussWeight1, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_BILEDGEFILTER_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].bilEdgeFilter, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_BILFILTERSTRENG_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].bilFilterStreng, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_BILEDGESOFTRATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].bilEdgeSoftRatio, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_BILREGWGT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].bilRegWgt, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_BILEDGESOFT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].bilEdgeSoft, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorBayerNrModeCell
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    int index = *((int*)param);
    XML_CHECK_START(CALIB_SENSOR_BAYERNR_MODE_CELL_TAG_ID, CALIB_SENSOR_BAYERNR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_MODE_NAME_TAG_ID)) {
            ParseString(pchild,  mCalibDb->bayerNr.mode_cell[index].name, sizeof(mCalibDb->bayerNr.mode_cell[index].name));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_SETTING_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorBayerNrSetting,
                                 &mCalibDb->bayerNr.mode_cell[index],
                                 (uint32_t)CALIB_SENSOR_BAYERNR_SETTING_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_BAYERNR_MODE_CELL_TAG_ID)) {
                LOGE("parse error in BayerNR setting (%s)\n", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorBayerNr
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_BAYERNR_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->bayerNr.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_VERSION_TAG_ID)) {
            ParseString(pchild,  mCalibDb->bayerNr.version, sizeof(mCalibDb->bayerNr.version));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_BAYERNR_MODE_CELL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) {
                int cell_size = 0;
                parseCellNoElement(pchild->ToElement(), tag.Size(), cell_size);
                mCalibDb->bayerNr.mode_num = cell_size;
                mCalibDb->bayerNr.mode_cell = (CalibDb_BayerNr_ModeCell_t *)malloc(cell_size * sizeof(CalibDb_BayerNr_ModeCell_t));
                memset(mCalibDb->bayerNr.mode_cell, 0x00, cell_size * sizeof(CalibDb_BayerNr_ModeCell_t));
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorBayerNrModeCell,
                                    param,
                                    (uint32_t)CALIB_SENSOR_BAYERNR_MODE_CELL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_BAYERNR_TAG_ID)) {
                    LOGE("parse error in BayerNR Mode cell (%s)\n", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), mCalibDb->bayerNr.mode_num,
                                     &RkAiqCalibParser::parseEntrySensorBayerNrModeCell,
                                     param,
                                     (uint32_t)CALIB_SENSOR_BAYERNR_MODE_CELL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_BAYERNR_TAG_ID)) {
                    LOGE("parse error in BayerNR Mode cell (%s)\n", tagname.c_str());
                    return (false);
                }
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorLsc
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_LSC_TAG_ID, CALIB_SENSOR_TAG_ID);

    unsigned char tableNum = mCalibDb->lsc.tableAllNum;
    mCalibDb->lsc.tableAllNum = 0;
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ENABLE_TAG_ID)) {
            uint8_t enable = mCalibDb->lsc.enable;
            int no = ParseUcharArray(pchild, &enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->lsc.enable = bool(enable);
            //mCalibDb->lsc.damp_enable = damp_enable;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_DAMP_ENABLE_TAG_ID)) {
            uint8_t damp_enable = mCalibDb->lsc.damp_enable;
            int no = ParseUcharArray(pchild, &damp_enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->lsc.damp_enable = bool(damp_enable);
            //mCalibDb->lsc.damp_enable = damp_enable;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_TAG_ID)) {
            if (!parseEntrySensorLscAlscCof(pchild->ToElement())) {
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) // read
            {
                mCalibDb->lsc.tableAll = (CalibDb_LscTableProfile_t*)malloc(sizeof(CalibDb_LscTableProfile_t) * tag.Size());
                memset(mCalibDb->lsc.tableAll, 0, sizeof(CalibDb_LscTableProfile_t) * tag.Size());
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorLscTableAll,
                                    param,
                                    (uint32_t)CALIB_SENSOR_LSC_TABLEALL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_LSC_TAG_ID)) {
                    LOGE("parse error in LSC tableAll (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), tableNum,
                                     &RkAiqCalibParser::parseEntrySensorLscTableAll,
                                     param,
                                     (uint32_t)CALIB_SENSOR_LSC_TABLEALL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_LSC_TAG_ID)) {
                    LOGE("parse error in LSC tableAll (%s)", tagname.c_str());
                    return (false);
                }
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySensorLscAlscCof
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_LSC_ALSCCOF_TAG_ID, CALIB_SENSOR_LSC_TAG_ID);

    unsigned char lightNum = 0;
    for (int i = 0; i < USED_FOR_CASE_MAX; i++)
        lightNum += mCalibDb->lsc.aLscCof.illuNum[i];
    memset(mCalibDb->lsc.aLscCof.illuNum, 0, sizeof(mCalibDb->lsc.aLscCof.illuNum));
    unsigned char resNum = mCalibDb->lsc.aLscCof.lscResNum;
    mCalibDb->lsc.aLscCof.lscResNum = 0;

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_IllALL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) // read
            {
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorLscAlscCofIllAll,
                                    param,
                                    (uint32_t)CALIB_SENSOR_LSC_ALSCCOF_IllALL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_LSC_ALSCCOF_TAG_ID)) {
                    LOGE("parse error in LSC aLscCof illAll (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), lightNum,
                                     &RkAiqCalibParser::parseEntrySensorLscAlscCofIllAll,
                                     param,
                                     (uint32_t)CALIB_SENSOR_LSC_ALSCCOF_IllALL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_LSC_ALSCCOF_TAG_ID)) {
                    LOGE("parse error in LSC aLscCof illAll (%s)", tagname.c_str());
                    return (false);
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_RESALL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorLscAlscCofResAll,
                                    param,
                                    (uint32_t)CALIB_SENSOR_LSC_ALSCCOF_RESALL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_LSC_ALSCCOF_TAG_ID)) {
                    LOGE("parse error in LSC aLscCof resolutionAll(%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), resNum,
                                     &RkAiqCalibParser::parseEntrySensorLscAlscCofResAll,
                                     param,
                                     (uint32_t)CALIB_SENSOR_LSC_ALSCCOF_RESALL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_LSC_ALSCCOF_TAG_ID)) {
                    LOGE("parse error in LSC aLscCof resolutionAll(%s)", tagname.c_str());
                    return (false);
                }
            }
        }


        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySensorLscAlscCofResAll
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_LSC_ALSCCOF_RESALL_TAG_ID, CALIB_SENSOR_LSC_ALSCCOF_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_RESALL_NAME_TAG_ID)) {
            ParseString(pchild,
                        mCalibDb->lsc.aLscCof.lscResName[index],
                        sizeof(mCalibDb->lsc.aLscCof.lscResName[index]));//check
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    mCalibDb->lsc.aLscCof.lscResNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorLscAlscCofIllAll
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_LSC_ALSCCOF_IllALL_TAG_ID, CALIB_SENSOR_LSC_ALSCCOF_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    int no1 = 0;
    int no2 = 0;
    int usedForCase = 0;

    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_CASE_TAG_ID)) {
            usedForCase = mCalibDb->lsc.aLscCof.usedForCaseAll[index];
            int no = ParseIntArray(pchild, &usedForCase, tag.Size());
            mCalibDb->lsc.aLscCof.usedForCaseAll[index] = usedForCase;
            index = mCalibDb->lsc.aLscCof.illuNum[usedForCase];
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_NAME_TAG_ID)) {
            ParseString(pchild,
                        mCalibDb->lsc.aLscCof.illAll[usedForCase][index].illuName,
                        sizeof(mCalibDb->lsc.aLscCof.illAll[usedForCase][index].illuName));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_WBGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->lsc.aLscCof.illAll[usedForCase][index].awbGain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_TABLEUSED_TAG_ID)) {
            char* lsc_profiles = Toupper(value);
            int no = ParseLscProfileArray(pchild, mCalibDb->lsc.aLscCof.illAll[usedForCase][index].tableUsed, LSC_PROFILES_NUM_MAX);
            DCT_ASSERT((no <= LSC_PROFILES_NUM_MAX));
            mCalibDb->lsc.aLscCof.illAll[usedForCase][index].tableUsedNO = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_GAINS_TAG_ID)) {
            no1 = ParseFloatArray(pchild, mCalibDb->lsc.aLscCof.illAll[usedForCase][index].vignettingCurve.pSensorGain, tag.Size());
            mCalibDb->lsc.aLscCof.illAll[usedForCase][index].vignettingCurve.arraySize = no1;
            DCT_ASSERT((no1 == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_ALSCCOF_VIG_TAG_ID)) {
            no2 = ParseFloatArray(pchild, mCalibDb->lsc.aLscCof.illAll[usedForCase][index].vignettingCurve.pVignetting, tag.Size());
            mCalibDb->lsc.aLscCof.illAll[usedForCase][index].vignettingCurve.arraySize = no2;
            DCT_ASSERT((no2 == tag.Size()));
        }
        else {
            LOGW("UNKNOWN tag: %s", tagname.c_str());
        }
        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    DCT_ASSERT(no1 == no2);
    mCalibDb->lsc.aLscCof.illuNum[usedForCase]++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}
static int AwbLscGradientCheck
(
    CalibDb_LscTableProfile_t* pLscProfile
) {
    uint32_t i;

    LOGI("%s: (enter)\n", __FUNCTION__);

    if (NULL == pLscProfile) {
        return (1);
    }

    if (0 < pLscProfile->LscYo) {
        for (i = 0; i < (sizeof(pLscProfile->LscYGradTbl) / sizeof(pLscProfile->LscYGradTbl[0])); ++i) {
            if (0 < pLscProfile->LscYSizeTbl[i]) {
                pLscProfile->LscYGradTbl[i] = (uint16_t)((double)(1UL << pLscProfile->LscYo) / pLscProfile->LscYSizeTbl[i] + 0.5);
            }
            else {
                return (2);
            }
        }
    }
    else {
        memset(pLscProfile->LscYGradTbl, 0, sizeof(pLscProfile->LscYGradTbl));
    }

    if (0 < pLscProfile->LscXo) {
        for (i = 0; i < (sizeof(pLscProfile->LscXGradTbl) / sizeof(pLscProfile->LscXGradTbl[0])); ++i) {
            if (0 < pLscProfile->LscXSizeTbl[i]) {
                pLscProfile->LscXGradTbl[i] = (uint16_t)((double)(1UL << pLscProfile->LscXo) / pLscProfile->LscXSizeTbl[i] + 0.5);
            }
            else {
                return (2);
            }
        }
    }
    else {
        memset(pLscProfile->LscXGradTbl, 0, sizeof(pLscProfile->LscXGradTbl));
    }

    LOGI("%s: (exit)\n", __FUNCTION__);

    return (0);
}

bool RkAiqCalibParser::parseEntrySensorLscTableAll
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_LSC_TABLEALL_TAG_ID, CALIB_SENSOR_LSC_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_NAME_TAG_ID)) {
            ParseString(pchild,
                        mCalibDb->lsc.tableAll[index].name,
                        sizeof(mCalibDb->lsc.tableAll[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_RESOLUTION_TAG_ID)) {
            ParseString(pchild,
                        mCalibDb->lsc.tableAll[index].resolution,
                        sizeof(mCalibDb->lsc.tableAll[index].resolution));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_ILLUMINATION_TAG_ID)) {
            ParseString(pchild,
                        mCalibDb->lsc.tableAll[index].illumination,
                        sizeof(mCalibDb->lsc.tableAll[index].illumination));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_SECTORS_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->lsc.tableAll[index].LscSectors, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_NO_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->lsc.tableAll[index].LscNo, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_XO_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->lsc.tableAll[index].LscXo, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_YO_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->lsc.tableAll[index].LscYo, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_SECTOR_SIZE_X_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lsc.tableAll[index].LscXSizeTbl, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_SECTOR_SIZE_Y_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lsc.tableAll[index].LscYSizeTbl, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_VIGNETTING_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->lsc.tableAll[index].vignetting, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_SAMPLES_RED_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lsc.tableAll[index].LscMatrix[0].uCoeff, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_SAMPLES_GREENR_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lsc.tableAll[index].LscMatrix[1].uCoeff, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_SAMPLES_GREENB_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lsc.tableAll[index].LscMatrix[2].uCoeff, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LSC_TABLEALL_LSC_SAMPLES_BLUE_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->lsc.tableAll[index].LscMatrix[3].uCoeff, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else {
            LOGW("UNKNOWN tag: %s", tagname.c_str());
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    int ret = AwbLscGradientCheck(&mCalibDb->lsc.tableAll[index]);
    DCT_ASSERT(ret == 0);
    mCalibDb->lsc.tableAllNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorRKDM
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_RKDM_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_FILTER1_TAG_ID)) {
            int no = ParseCharArray(pchild, mCalibDb->dm.debayer_filter1, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_FILTER2_TAG_ID)) {
            int no = ParseCharArray(pchild, mCalibDb->dm.debayer_filter2, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_GAIN_OFFSET_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_gain_offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_ISO_TAG_ID)) {
            int no = ParseIntArray(pchild, mCalibDb->dm.ISO, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_SHARP_STRENGTH_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dm.sharp_strength, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_HF_OFFSET_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->dm.debayer_hf_offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_OFFSET_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_CLIP_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_clip_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_FILTER_G_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_filter_g_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_FILTER_C_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_filter_c_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_THED0_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_thed0, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_THED1_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_thed1, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_DIST_SCALE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_dist_scale, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_CNR_STRENGTH_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_cnr_strength, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_RKDM_DEBAYER_SHIFT_NUM_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->dm.debayer_shift_num, tag.Size());
            DCT_ASSERT((no == tag.Size()));

        }
        else {
            LOGW("UNKNOWN tag: %s", tagname.c_str());
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorlumaCCMGAC
(
    const XMLElement*   pelement,
    void*                param,
    int         index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_CCM_LUMA_CCM_GAIN_ALPHASCALE_CURVE_TAG_ID, CALIB_SENSOR_CCM_LUMA_CCM_TAG_ID);

    const XMLNode* psubsubchild = pelement->FirstChild();
    int no1 = 0, no2 = 0;
    while (psubsubchild) {
        XmlTag subsubTag = XmlTag(psubsubchild->ToElement());
        std::string subsubTagname(psubsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subsubTagname.c_str()), subsubTag.Type(), subsubTag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_LUMA_CCM_GAIN_TAG_ID)) {
            no1 = ParseFloatArray(psubsubchild, mCalibDb->ccm.mode_cell[index].luma_ccm.alpha_gain, subsubTag.Size());
            DCT_ASSERT((no1 == subsubTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_LUMA_CCM_SCALE_TAG_ID)) {
            no2 = ParseFloatArray(psubsubchild, mCalibDb->ccm.mode_cell[index].luma_ccm.alpha_scale, subsubTag.Size());
            DCT_ASSERT((no2 == subsubTag.Size()));
        }
        psubsubchild = psubsubchild->NextSibling();
    }
    DCT_ASSERT(no1 == no2);
    mCalibDb->ccm.mode_cell[index].luma_ccm.gain_scale_cure_size = no1;
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorlumaCCM
(
    const XMLElement*   pelement,
    void*                param,
    int         index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_CCM_LUMA_CCM_TAG_ID, CALIB_SENSOR_CCM_MODE_CELL_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_LUMA_CCM_RGB2Y_PARA_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->ccm.mode_cell[index].luma_ccm.rgb2y_para, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_LUMA_CCM_LOW_BOUND_POS_BIT_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->ccm.mode_cell[index].luma_ccm.low_bound_pos_bit, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_LUMA_CCM_Y_ALPHA_CURVE_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->ccm.mode_cell[index].luma_ccm.y_alpha_curve, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));

        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_LUMA_CCM_GAIN_ALPHASCALE_CURVE_TAG_ID)) {
            if (!parseEntrySensorlumaCCMGAC(psubchild->ToElement(), param, index)) {
                LOGE("parse error in CCM lumaCCM (%s)\n", subTagname.c_str());
                return (false);
            }
        }
        psubchild = psubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorCCMModeCell
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_CCM_MODE_CELL_TAG_ID, CALIB_SENSOR_CCM_TAG_ID);

    int index = *((int*)param);
    bool indexValid = false;
    unsigned int allNum = mCalibDb->ccm.mode_cell[index].matrixAllNum;
    mCalibDb->ccm.mode_cell[index].matrixAllNum = 0;

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_MODE_NAME_TAG_ID)) {
            char mode[CCM_PROFILE_NAME];
            //ParseString(pchild,  mode, sizeof(mode));
            ParseString(pchild, mCalibDb->ccm.mode_cell[index].name, sizeof(mCalibDb->ccm.mode_cell[index].name));
            indexValid = true;
            if (0 == strcmp(mCalibDb->ccm.mode_cell[index].name, "normal")) {
                index = CCM_FOR_MODE_NORMAL;
            }
            else if (0 == strcmp(mCalibDb->ccm.mode_cell[index].name, "hdr")) {
                index = CCM_FOR_MODE_HDR;
            }
            else {
                LOGE("mode: %s in CCM ModeCell is invalid. instead normal or hdr\n", mCalibDb->ccm.mode_cell[index].name);
                return(false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_DAMP_ENABLE_TAG_ID) && indexValid) {
            uint8_t uValue = mCalibDb->ccm.mode_cell[index].damp_enable;
            int no = ParseUcharArray(pchild, &uValue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->ccm.mode_cell[index].damp_enable = (bool)uValue;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_LUMA_CCM_TAG_ID) && indexValid) {
            INFO_PRINT(tagname.c_str());
            if (!parseEntrySensorlumaCCM(pchild->ToElement(), param, index)) {
                LOGE("parse error in CCM lumaCCM (%s)\n", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_ACCMCOF_TAG_ID) && indexValid) {
            if (!parseEntrySensorCcmAccmCof(pchild->ToElement(), param, index)) {
                LOGE("parse error in CCM aCCmCof (%s)\n", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_MATRIXALL_TAG_ID) && indexValid) {
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                     &RkAiqCalibParser::parseEntrySensorCcmMatrixAll,
                                     &mCalibDb->ccm.mode_cell[index],
                                     (uint32_t)CALIB_SENSOR_CCM_MATRIXALL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_CCM_MODE_CELL_TAG_ID)) {
                    LOGE("parse error in CCM matrixall (%s)\n", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell4((XMLElement*)pchild->ToElement(), tag.Size(), allNum,
                                     &RkAiqCalibParser::parseEntrySensorCcmMatrixAll,
                                     &mCalibDb->ccm.mode_cell[index],
                                     (uint32_t)CALIB_SENSOR_CCM_MATRIXALL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_CCM_MODE_CELL_TAG_ID)) {
                    LOGE("parse error in CCM matrixall (%s)\n", tagname.c_str());
                    return (false);
                }
            }
        }
        pchild = pchild->NextSibling();
    }
    DCT_ASSERT((indexValid == true));
    XML_CHECK_END();
    mCalibDb->ccm.modecellNum++;
    mCalibDb->ccm.mode_cell[index].valid = true;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorCCM
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_CCM_TAG_ID, CALIB_SENSOR_TAG_ID);

    unsigned char cellNum = mCalibDb->ccm.modecellNum;
    mCalibDb->ccm.modecellNum = 0;
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_ENABLE_TAG_ID)) {
            uint8_t uValue = mCalibDb->ccm.enable;
            int no = ParseUcharArray(pchild, &uValue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            mCalibDb->ccm.enable = (bool)uValue;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_MODE_CELL_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorCCMModeCell,
                                param,
                                (uint32_t)CALIB_SENSOR_CCM_MODE_CELL_TAG_ID,
                                (uint32_t)CALIB_SENSOR_CCM_TAG_ID)) {
                LOGE("parse error in ccm mode cell (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorCcmAccmCof
(
    const XMLElement*   pelement,
    void*               param,
    int         index
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_CCM_ACCMCOF_TAG_ID, CALIB_SENSOR_CCM_MODE_CELL_TAG_ID);

    //mCalibDb->ccm.mode_cell[index].aCcmCof.illuNum = 0;

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_ACCMCOF_IllALL_TAG_ID)) {
            unsigned char lightNum = mCalibDb->ccm.mode_cell[index].aCcmCof.illuNum;
            mCalibDb->ccm.mode_cell[index].aCcmCof.illuNum = 0;
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                     &RkAiqCalibParser::parseEntrySensorCcmAccmCofIllAll,
                                     &mCalibDb->ccm.mode_cell[index],
                                     (uint32_t)CALIB_SENSOR_CCM_ACCMCOF_IllALL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_CCM_ACCMCOF_TAG_ID)) {
                    LOGE("parse error in LSC aCcmCof illAll (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell4((XMLElement*)pchild->ToElement(), tag.Size(), lightNum,
                                     &RkAiqCalibParser::parseEntrySensorCcmAccmCofIllAll,
                                     &mCalibDb->ccm.mode_cell[index],
                                     (uint32_t)CALIB_SENSOR_CCM_ACCMCOF_IllALL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_CCM_ACCMCOF_TAG_ID)) {
                    LOGE("parse error in LSC aCcmCof illAll (%s)", tagname.c_str());
                    return (false);
                }
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorCcmAccmCofIllAll
(
    const XMLElement*   pelement,
    void*                param,
    int                 index
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    CalibDb_Ccm_ModeCell_t *pModeCell = (CalibDb_Ccm_ModeCell_t *)param;

    XML_CHECK_START(CALIB_SENSOR_CCM_ACCMCOF_IllALL_TAG_ID, CALIB_SENSOR_CCM_ACCMCOF_TAG_ID);


    //int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    int no1 = 0;
    int no2 = 0;
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_ACCMCOF_NAME_TAG_ID)) {
            ParseString(pchild,
                        pModeCell->aCcmCof.illAll[index].illuName,
                        sizeof(pModeCell->aCcmCof.illAll[index].illuName));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_ACCMCOF_WBGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->aCcmCof.illAll[index].awbGain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_ACCMCOF_MATRIXUSED_TAG_ID)) {
            char* ccm_profiles = Toupper(value);
            int no = ParseLscProfileArray(pchild, pModeCell->aCcmCof.illAll[index].matrixUsed, CCM_PROFILES_NUM_MAX);
            DCT_ASSERT((no <= CCM_PROFILES_NUM_MAX));
            pModeCell->aCcmCof.illAll[index].matrixUsedNO = no;
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_ACCMCOF_GAINS_TAG_ID)) {
            no1 = ParseFloatArray(pchild, pModeCell->aCcmCof.illAll[index].saturationCurve.pSensorGain, tag.Size());
            pModeCell->aCcmCof.illAll[index].saturationCurve.arraySize = no1;
            DCT_ASSERT((no1 == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_ACCMCOF_SAT_TAG_ID)) {
            no2 = ParseFloatArray(pchild, pModeCell->aCcmCof.illAll[index].saturationCurve.pSaturation, tag.Size());
            pModeCell->aCcmCof.illAll[index].saturationCurve.arraySize = no2;
            DCT_ASSERT((no2 == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();

    DCT_ASSERT(no1 == no2);
    pModeCell->aCcmCof.illuNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorCcmMatrixAll
(
    const XMLElement*   pelement,
    void*                param,
    int                 index
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    CalibDb_Ccm_ModeCell_t *pModeCell = (CalibDb_Ccm_ModeCell_t *)param;

    XML_CHECK_START(CALIB_SENSOR_CCM_MATRIXALL_TAG_ID, CALIB_SENSOR_CCM_MODE_CELL_TAG_ID);

//    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_MATRIXALL_NAME_TAG_ID)) {
            ParseString(pchild,
                        pModeCell->matrixAll[index].name,
                        sizeof(pModeCell->matrixAll[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_MATRIXALL_ILLUMINATION_TAG_ID)) {
            ParseString(pchild,
                        pModeCell->matrixAll[index].illumination,
                        sizeof(pModeCell->matrixAll[index].illumination));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_MATRIXALL_SAT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->matrixAll[index].saturation, 1);
            DCT_ASSERT((no == 1));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_MATRIXALL_MATRIX_TAG_ID)) {
            int i = (sizeof(pModeCell->matrixAll[index].CrossTalkCoeff)
                     / sizeof(pModeCell->matrixAll[index].CrossTalkCoeff.fCoeff[0]));
            int no = ParseFloatArray(pchild, pModeCell->matrixAll[index].CrossTalkCoeff.fCoeff, i);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CCM_MATRIXALL_OFFSET_TAG_ID))
        {
            int i = (sizeof(pModeCell->matrixAll[index].CrossTalkOffset)
                     / sizeof(pModeCell->matrixAll[index].CrossTalkOffset.fCoeff[0]));
            int no = ParseFloatArray(pchild, pModeCell->matrixAll[index].CrossTalkOffset.fCoeff, i);
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    pModeCell->matrixAllNum++;
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorUVNRSetting
(
    const XMLElement*   pelement,
    void*               param,
    int                 index
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    CalibDb_UVNR_ModeCell_t * pModeCell = (CalibDb_UVNR_ModeCell_t *)param;
    XML_CHECK_START(CALIB_SENSOR_UVNR_SETTING_TAG_ID, CALIB_SENSOR_UVNR_MODE_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_SETTING_SNR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].snr_mode, sizeof(pModeCell->setting[index].snr_mode));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_SETTING_SENSOR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].sensor_mode, sizeof(pModeCell->setting[index].sensor_mode));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].ISO, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP0_UVGRAD_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step0_uvgrad_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP0_UVGRAD_OFFSET_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step0_uvgrad_offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_NONMED1_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_nonMed1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_NONBF1_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_nonBf1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_DOWNSAMPLE_W_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_downSample_w, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_DOWNSAMPLE_H_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_downSample_h, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_DOWNSAMPLE_MEANSIZE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_downSample_meansize, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_MEDIAN_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_median_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_MEDIAN_SIZE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_median_size, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_MEDIAN_IIR_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_median_IIR, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_BF_SIGMAR_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_bf_sigmaR, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_BF_UVGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_bf_uvgain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_BF_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_bf_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_BF_SIZE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_bf_size, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_BF_SIGMAD_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_bf_sigmaD, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_BF_ISROWIIR_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_bf_isRowIIR, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP1_BF_ISYCOPY_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step1_bf_isYcopy, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_NONEXT_BLOCK_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_nonExt_block, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_NONMED_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_nonMed, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_NONBF_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_nonBf, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_DOWNSAMPLE_W_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_downSample_w, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_DOWNSAMPLE_H_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_downSample_h, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_DOWNSAMPLE_MEANSIZE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_downSample_meansize, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_MEDIAN_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_median_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_MEDIAN_SIZE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_median_size, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_MEDIAN_IIR_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_median_IIR, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_BF_SIGMAR_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_bf_sigmaR, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_BF_UVGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_bf_uvgain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_BF_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_bf_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_BF_SIZE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_bf_size, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_BF_SIGMAD_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_bf_sigmaD, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_BF_ISROWIIR_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_bf_isRowIIR, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP2_BF_ISYCOPY_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step2_bf_isYcopy, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP3_NONBF3_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step3_nonBf3, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP3_BF_SIGMAR_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step3_bf_sigmaR, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP3_BF_UVGAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step3_bf_uvgain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP3_BF_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step3_bf_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP3_BF_SIZE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step3_bf_size, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP3_BF_SIGMAD_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step3_bf_sigmaD, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP3_BF_ISROWIIR_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step3_bf_isRowIIR, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_STEP3_BF_ISYCOPY_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].step3_bf_isYcopy, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_KERNEL_3X3_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].kernel_3x3, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_KERNEL_5X5_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].kernel_5x5, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_KERNEL_9X9_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].kernel_9x9, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_KERNEL_9X9_NUM_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pModeCell->setting[index].kernel_9x9_num, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_SIGMA_ADJ_LUMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].sigma_adj_luma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_SIGMA_ADJ_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].sigma_adj_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_THRESHOLD_ADJ_LUMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].threshold_adj_luma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_THRESHOLD_ADJ_THRE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pModeCell->setting[index].threshold_adj_thre, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorUVNRModeCell
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    int index = *((int*)param);
    XML_CHECK_START(CALIB_SENSOR_UVNR_MODE_CELL_TAG_ID, CALIB_SENSOR_UVNR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_MODE_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->uvnr.mode_cell[index].name, sizeof(mCalibDb->uvnr.mode_cell[index].name));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_SETTING_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorUVNRSetting,
                                 &mCalibDb->uvnr.mode_cell[index],
                                 (uint32_t)CALIB_SENSOR_UVNR_SETTING_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_UVNR_MODE_CELL_TAG_ID)) {
                LOGE("parse error in BayerNR setting (%s)\n", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorUVNR
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_UVNR_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->uvnr.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_VERSION_TAG_ID)) {
            ParseString(pchild, mCalibDb->uvnr.version, sizeof(mCalibDb->uvnr.version));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_UVNR_MODE_CELL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) {
                int cell_size = 0;
                parseCellNoElement(pchild->ToElement(), tag.Size(), cell_size);
                mCalibDb->uvnr.mode_num = cell_size;
                mCalibDb->uvnr.mode_cell = (CalibDb_UVNR_ModeCell_t *)malloc(cell_size * sizeof(CalibDb_UVNR_ModeCell_t));
                memset(mCalibDb->uvnr.mode_cell, 0x00, cell_size * sizeof(CalibDb_UVNR_ModeCell_t));
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorUVNRModeCell,
                                    param,
                                    (uint32_t)CALIB_SENSOR_UVNR_MODE_CELL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_UVNR_TAG_ID)) {
                    LOGE("parse error in BayerNR setting (%s)\n", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), mCalibDb->uvnr.mode_num,
                                     &RkAiqCalibParser::parseEntrySensorUVNRModeCell,
                                     param,
                                     (uint32_t)CALIB_SENSOR_UVNR_MODE_CELL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_UVNR_TAG_ID)) {
                    LOGE("parse error in BayerNR setting (%s)\n", tagname.c_str());
                    return (false);
                }
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorGamma
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_GAMMA_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GAMMA_GAMMA_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->gamma.gamma_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GAMMA_GAMMA_OUT_SEGNUM_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->gamma.gamma_out_segnum, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GAMMA_GAMMA_OUT_OFFSET_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->gamma.gamma_out_offset, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GAMMA_CURVE_NORMAL_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->gamma.curve_normal, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GAMMA_CURVE_HDR_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->gamma.curve_hdr, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GAMMA_CURVE_NIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->gamma.curve_night, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDegamma
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_DEGAMMA_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->degamma.degamma_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_MODE_CELL_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorDegammaModeCell,
                                param,
                                (uint32_t)CALIB_SENSOR_DEGAMMA_MODE_CELL_TAG_ID,
                                (uint32_t)CALIB_SENSOR_DEGAMMA_TAG_ID)) {
                LOGE("parse error in degamma mode cell (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDegammaModeCell
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    int index = *((int*)param);
    XML_CHECK_START(CALIB_SENSOR_DEGAMMA_MODE_CELL_TAG_ID, CALIB_SENSOR_DEGAMMA_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_MODE_NAME_TAG_ID)) {
            ParseString(pchild,  mCalibDb->degamma.mode[index].name, sizeof(mCalibDb->degamma.mode[index].name));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_SCENE_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->degamma.mode[index].degamma_scene_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_CURVE_X_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->degamma.mode[index].X_axis, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_CURVE_R_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->degamma.mode[index].curve_R, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_CURVE_G_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->degamma.mode[index].curve_G, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEGAMMA_CURVE_B_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->degamma.mode[index].curve_B, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();

    }

    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorYnr
(
    const XMLElement*   pelement,
    void*                param
)
{
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_YNR_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->ynr.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_VERSION_TAG_ID)) {
            ParseString(pchild, mCalibDb->ynr.version, sizeof(mCalibDb->ynr.version));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_MODE_CELL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) {
                int cell_size = 0;
                parseCellNoElement(pchild->ToElement(), tag.Size(), cell_size);
                mCalibDb->ynr.mode_num = cell_size;
                mCalibDb->ynr.mode_cell = (CalibDb_YNR_ModeCell_t *)malloc(cell_size * sizeof(CalibDb_YNR_ModeCell_t));
                memset(mCalibDb->ynr.mode_cell, 0x00, cell_size * sizeof(CalibDb_YNR_ModeCell_t));
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorYnrModeCell,
                                    param,
                                    (uint32_t)CALIB_SENSOR_YNR_MODE_CELL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_YNR_TAG_ID)) {
                    LOGE("parse error in YNR mode cell (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), mCalibDb->ynr.mode_num,
                                     &RkAiqCalibParser::parseEntrySensorYnrModeCell,
                                     param,
                                     (uint32_t)CALIB_SENSOR_YNR_MODE_CELL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_YNR_TAG_ID)) {
                    LOGE("parse error in YNR mode cell (%s)", tagname.c_str());
                    return (false);
                }
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}



bool RkAiqCalibParser::parseEntrySensorYnrModeCell
(
    const XMLElement*   pelement,
    void*                param
)
{
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    int index = *((int*)param);
    XML_CHECK_START(CALIB_SENSOR_YNR_MODE_CELL_TAG_ID, CALIB_SENSOR_YNR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_MODE_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->ynr.mode_cell[index].name, sizeof(mCalibDb->ynr.mode_cell[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_SETTING_CELL_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorYnrSetting,
                                 (void *)&mCalibDb->ynr.mode_cell[index],
                                 (uint32_t)CALIB_SENSOR_YNR_SETTING_CELL_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_YNR_MODE_CELL_TAG_ID)) {
                LOGE("parse error in YNR setting (%s)", tagname.c_str());
                return (false);
            }
        }


        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorYnrSetting
(
    const XMLElement*   pelement,
    void*                param,
    int                  index
)
{
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    CalibDb_YNR_ModeCell_t *pModeCell = (CalibDb_YNR_ModeCell_t *)param;
    XML_CHECK_START(CALIB_SENSOR_YNR_SETTING_CELL_TAG_ID, CALIB_SENSOR_YNR_MODE_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_SETTING_SNR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].snr_mode, sizeof(pModeCell->setting[index].snr_mode));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_SETTING_SENSOR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].sensor_mode, sizeof(pModeCell->setting[index].sensor_mode));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_ISO_CELL_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorYnrISO,
                                 (void*)&pModeCell->setting[index],
                                 (uint32_t)CALIB_SENSOR_YNR_ISO_CELL_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_YNR_SETTING_CELL_TAG_ID)) {
                LOGE("parse error in YNR iso (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorYnrISO
(
    const XMLElement*   pelement,
    void*                param,
    int                  index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_YNR_ISO_CELL_TAG_ID, CALIB_SENSOR_YNR_SETTING_CELL_TAG_ID);

    CalibDb_YNR_Setting_t *pSetting = (CalibDb_YNR_Setting_t *)param;

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->ynr_iso[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_SIGMA_CURVE_TAG_ID)) {
            int no = ParseDoubleArray(pchild, pSetting->ynr_iso[index].sigma_curve, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_YNR_LCI_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].ynr_lci, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_YNR_LHCI_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].ynr_lhci, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_YNR_HLCI_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].ynr_hlci, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_YNR_HHCI_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].ynr_hhci, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_LO_LUMAPOINT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].lo_lumaPoint, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_LO_LUMARATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].lo_lumaRatio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_LO_DIRECTIONSTRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->ynr_iso[index].lo_directionStrength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_LO_BFSCALE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].lo_bfScale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_IMERGE_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->ynr_iso[index].imerge_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_IMERGE_BOUND_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->ynr_iso[index].imerge_bound, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_DENOISE_WEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].denoise_weight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HI_LUMAPOINT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hi_lumaPoint, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HI_LUMARATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hi_lumaRatio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HI_BFSCALE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hi_bfScale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HWITH_D_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hwith_d, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HI_DENOISESTRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->ynr_iso[index].hi_denoiseStrength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HI_DETAILMINADJDNW_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->ynr_iso[index].hi_detailMinAdjDnW, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HI_DENOISEWEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hi_denoiseWeight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_Y_LUMA_POINT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].y_luma_point, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HGRAD_Y_LEVEL1_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hgrad_y_level1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HGRAD_Y_LEVEL2_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hgrad_y_level2, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HGRAD_Y_LEVEL3_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hgrad_y_level3, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HGRAD_Y_LEVEL4_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hgrad_y_level4, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_YNR_HI_SOFT_THRESH_SCALE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->ynr_iso[index].hi_soft_thresh_scale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorGic
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_GIC_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->gic.gic_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_EDGE_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->gic.edge_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_GR_RATION_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->gic.gr_ration, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_NOISE_CUT_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->gic.noise_cut_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_ISO_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorGicISO,
                                param,
                                (uint32_t)CALIB_SENSOR_GIC_ISO_TAG_ID,
                                (uint32_t)CALIB_SENSOR_GIC_TAG_ID)) {
                LOGE("parse error in GIC ISO (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorGicISO
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_GIC_ISO_TAG_ID, CALIB_SENSOR_GIC_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_ISO_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_MIN_BUSY_THRE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].min_busy_thre, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_MIN_GRAD_THR1_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].min_grad_thr1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_MIN_GRAD_THR2_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].min_grad_thr2, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_K_GRAD1_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].k_grad1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_K_GRAD2_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].k_grad2, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_SMOOTHNESS_GB_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].smoothness_gb, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_SMOOTHNESS_GB_WEAK_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].smoothness_gb_weak, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_GB_THRE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].gb_thre, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_MAXCORV_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].maxCorV, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_MAXCORVBOTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].maxCorVboth, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_MAXCUTV_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].maxCutV, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_DARK_THRE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].dark_thre, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_DARK_THREHI_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].dark_threHi, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_K_GRAD1_DARK_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].k_grad1_dark, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_K_GRAD2_DARK_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].k_grad2_dark, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_MIN_GRAD_THR_DARK1_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].min_grad_thr_dark1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_MIN_GRAD_THR_DARK2_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].min_grad_thr_dark2, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_GVALUELIMITLO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].GValueLimitLo, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_GVALUELIMITHI_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].GValueLimitHi, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_TEXTURESTRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].textureStrength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_SCALELO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].ScaleLo, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_SCALEHI_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].ScaleHi, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_NOISECURVE_0_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].noiseCurve_0, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_NOISECURVE_1_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].noiseCurve_1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_GLOBALSTRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].globalStrength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_NOISE_COEA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].noise_coea, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_NOISE_COEB_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].noise_coeb, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_GIC_DIFF_CLIP_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->gic.gic_iso[index].diff_clip, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorMFNR
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_MFNR_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->mfnr.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_VERSION_TAG_ID)) {
            ParseString(pchild, mCalibDb->mfnr.version, sizeof(mCalibDb->mfnr.version));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_LOCAL_GAIN_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->mfnr.local_gain_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECT_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->mfnr.motion_detect_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MODE_3TO1_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->mfnr.mode_3to1, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MAX_LEVEL_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->mfnr.max_level, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MAX_LEVEL_UV_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->mfnr.max_level_uv, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_BACK_REF_NUM_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->mfnr.back_ref_num, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_AWB_UV_RATIO_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorMFNRAwbUvRatio,
                                param,
                                (uint32_t)CALIB_SENSOR_MFNR_AWB_UV_RATIO_TAG_ID,
                                (uint32_t)CALIB_SENSOR_MFNR_TAG_ID)) {
                LOGE("parse error in MFNR awb_uv_ratio (%s)\n", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MODE_CELL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) {
                int cell_size = 0;
                parseCellNoElement(pchild->ToElement(), tag.Size(), cell_size);
                mCalibDb->mfnr.mode_num = cell_size;
                mCalibDb->mfnr.mode_cell = (CalibDb_MFNR_ModeCell_t *)malloc(cell_size * sizeof(CalibDb_MFNR_ModeCell_t));
                memset(mCalibDb->mfnr.mode_cell, 0x00, cell_size * sizeof(CalibDb_MFNR_ModeCell_t));
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorMFNRModeCell,
                                    param,
                                    (uint32_t)CALIB_SENSOR_MFNR_MODE_CELL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_MFNR_TAG_ID)) {
                    LOGE("parse error in MFNR MFNR_ISO (%s)\n", tagname.c_str());
                    return (false);
                }
            }
            else
            {
                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), mCalibDb->mfnr.mode_num,
                                     &RkAiqCalibParser::parseEntrySensorMFNRModeCell,
                                     param,
                                     (uint32_t)CALIB_SENSOR_MFNR_MODE_CELL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_MFNR_TAG_ID)) {
                    LOGE("parse error in MFNR MFNR_ISO (%s)\n", tagname.c_str());
                    return (false);
                }
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorMFNRAwbUvRatio
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);


    XML_CHECK_START(CALIB_SENSOR_MFNR_AWB_UV_RATIO_TAG_ID, CALIB_SENSOR_MFNR_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_AWB_UV_RATIO_NAME_TAG_ID)) {
            ParseString(pchild,
                        mCalibDb->mfnr.uv_ratio[index].illum,
                        sizeof(mCalibDb->mfnr.uv_ratio[index].illum));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_AWB_UV_RATIO_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->mfnr.uv_ratio[index].ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorMFNRDynamic
(
    const XMLElement*   pelement,
    void*                param,
    int         index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_MFNR_DYNAMIC_TAG_ID, CALIB_SENSOR_MFNR_MODE_CELL_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = [%s]\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_DYNAMIC_ENABLE_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->mfnr.mode_cell[index].dynamic.enable, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_DYNAMIC_LOWTH_ISO_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->mfnr.mode_cell[index].dynamic.lowth_iso, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_DYNAMIC_LOWTH_TIME_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->mfnr.mode_cell[index].dynamic.lowth_time, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_DYNAMIC_HIGHTH_ISO_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->mfnr.mode_cell[index].dynamic.highth_iso, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_DYNAMIC_HIGHTH_TIME_TAG_ID)) {
            int no = ParseFloatArray(psubchild, &mCalibDb->mfnr.mode_cell[index].dynamic.highth_time, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else {
            LOGW("%s(%d): parse error in  mfnr dynamic (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorMFNRMotionDetection
(
    const XMLElement*   pelement,
    void*                param,
    int         index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_MFNR_MOTION_DETECTION_TAG_ID, CALIB_SENSOR_MFNR_MODE_CELL_TAG_ID);

    const XMLNode* psubchild = pelement->FirstChild();
#ifdef DEBUG_LOG
    LOGE("%s(%d): Tagname = [%s]\n", __FUNCTION__, __LINE__, pelement->Name());
#endif
    while (psubchild) {
        XmlTag subTag = XmlTag(psubchild->ToElement());
        std::string subTagname(psubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());

#ifdef DEBUG_LOG
        LOGE("%s(%d): subTagname = %s\n", __FUNCTION__, __LINE__, subTagname.c_str());
#endif

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_ENABLE_TAG_ID)) {
            int no = ParseIntArray(psubchild, &mCalibDb->mfnr.mode_cell[index].motion.enable, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_ISO_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.iso, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_SIGMAHSCALE_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.sigmaHScale, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_SIGMALSCALE_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.sigmaLScale, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_LIGHT_CLP_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.lightClp, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_UV_WEIGHT_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.uvWeight, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_MFNR_SIGMA_SCALE_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.mfnrSigmaScale, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_YUVNR_GAIN_SCALE0_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.yuvnrGainScale0, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_YUVNR_GAIN_SCALE1_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.yuvnrGainScale1, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_YUVNR_GAIN_SCALE2_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.yuvnrGainScale2, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED0_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.reserved0, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED1_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.reserved1, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED2_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.reserved2, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED3_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.reserved3, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED4_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.reserved4, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED5_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.reserved5, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED6_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.reserved6, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED7_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.reserved7, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED8_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.frame_limit_uv, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_RESERVED9_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.frame_limit_y, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_FRAME_LIMIT_Y_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.frame_limit_y, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_FRAME_LIMIT_UV_TAG_ID)) {
            int no = ParseFloatArray(psubchild, mCalibDb->mfnr.mode_cell[index].motion.frame_limit_uv, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else {
            LOGE("%s(%d): parse error in  mfnr dynamic (unknow tag: %s )\n", __FUNCTION__, __LINE__, subTagname.c_str());
        }
        psubchild = psubchild->NextSibling();
    }
    XML_CHECK_END();
    autoTabBackward();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}


bool RkAiqCalibParser::parseEntrySensorMFNRModeCell
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    int index = *((int*)param);
    XML_CHECK_START(CALIB_SENSOR_MFNR_MODE_CELL_TAG_ID, CALIB_SENSOR_MFNR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MODE_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->mfnr.mode_cell[index].name, sizeof(mCalibDb->mfnr.mode_cell[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_DYNAMIC_TAG_ID)) {
            if (!parseEntrySensorMFNRDynamic(pchild->ToElement(), param, index)) {
                LOGE("parse error in MFNR dynamic(%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_SETTING_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorMFNRSetting,
                                 (void *)&mCalibDb->mfnr.mode_cell[index],
                                 (uint32_t)CALIB_SENSOR_MFNR_SETTING_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_MFNR_MODE_CELL_TAG_ID)) {
                LOGE("parse error in MFNR MFNR_ISO (%s)\n", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_MOTION_DETECTION_TAG_ID)) {
            if (!parseEntrySensorMFNRMotionDetection(pchild->ToElement(), param, index)) {
                LOGE("parse error in MFNR motion detection(%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);

    return (true);

}

bool RkAiqCalibParser::parseEntrySensorMFNRSetting
(
    const XMLElement*   pelement,
    void*               param,
    int                 index
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    CalibDb_MFNR_ModeCell_t *pModeCell = (CalibDb_MFNR_ModeCell_t *)param;
    XML_CHECK_START(CALIB_SENSOR_MFNR_SETTING_TAG_ID, CALIB_SENSOR_MFNR_MODE_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_SETTING_SNR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].snr_mode, sizeof(pModeCell->setting[index].snr_mode));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_SETTING_SENSOR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].sensor_mode, sizeof(pModeCell->setting[index].sensor_mode));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorMFNRISO,
                                 (void *)&pModeCell->setting[index],
                                 (uint32_t)CALIB_SENSOR_MFNR_ISO_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_MFNR_SETTING_TAG_ID)) {
                LOGE("parse error in MFNR MFNR_ISO (%s)\n", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);

    return (true);
}

bool RkAiqCalibParser::parseEntrySensorMFNRISO
(
    const XMLElement*   pelement,
    void*                param,
    int                  index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_MFNR_ISO_TAG_ID, CALIB_SENSOR_MFNR_SETTING_TAG_ID);

    CalibDb_MFNR_Setting_t * pSetting = (CalibDb_MFNR_Setting_t *)param;
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->mfnr_iso[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_WEIGHT_LIMIT_Y_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].weight_limit_y, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_WEIGHT_LIMIT_UV_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].weight_limit_uv, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_RATIO_FRQ_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].ratio_frq, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_LUMA_W_IN_CHROMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].luma_w_in_chroma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_NOISE_CURVE_TAG_ID)) {
            int no = ParseDoubleArray(pchild, pSetting->mfnr_iso[index].noise_curve, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_NOISE_CURVE_X00_TAG_ID)) {
            int no = ParseDoubleArray(pchild, &pSetting->mfnr_iso[index].noise_curve_x00, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LO_NOISEPROFILE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lo_noiseprofile, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_HI_NOISEPROFILE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_hi_noiseprofile, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LO_DENOISEWEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lo_denoiseweight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_HI_DENOISEWEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_hi_denoiseweight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LO_BFSCALE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lo_bfscale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_HI_BFSCALE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_hi_bfscale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LUMANRPOINT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lumanrpoint, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LUMANRCURVE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lumanrcurve, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_DENOISESTRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->mfnr_iso[index].y_denoisestrength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LO_LVL0_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lo_lvl0_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_HI_LVL0_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_hi_lvl0_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LO_LVL1_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lo_lvl1_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_HI_LVL1_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_hi_lvl1_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LO_LVL2_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lo_lvl2_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_HI_LVL2_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_hi_lvl2_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_LO_LVL3_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_lo_lvl3_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_Y_HI_LVL3_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].y_hi_lvl3_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_LO_NOISEPROFILE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_lo_noiseprofile, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_HI_NOISEPROFILE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_hi_noiseprofile, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_LO_DENOISEWEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_lo_denoiseweight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_HI_DENOISEWEIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_hi_denoiseweight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_LO_BFSCALE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_lo_bfscale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_HI_BFSCALE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_hi_bfscale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_LUMANRPOINT_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_lumanrpoint, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_LUMANRCURVE_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_lumanrcurve, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_DENOISESTRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->mfnr_iso[index].uv_denoisestrength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_LO_LVL0_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_lo_lvl0_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_HI_LVL0_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_hi_lvl0_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_LO_LVL1_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_lo_lvl1_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_HI_LVL1_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_hi_lvl1_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_LO_LVL2_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_lo_lvl2_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_UV_HI_LVL2_GFDELTA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].uv_hi_lvl2_gfdelta, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_LVL0_GFSIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].lvl0_gfsigma, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_LVL1_GFSIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].lvl1_gfsigma, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_LVL2_GFSIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].lvl2_gfsigma, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_MFNR_ISO_LVL3_GFSIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->mfnr_iso[index].lvl3_gfsigma, tag.Size(), 5);
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorSharp
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    if (xmlParseReadWrite == XML_PARSER_READ) // write
    {
        memset(&mCalibDb->sharp, 0x00, sizeof(CalibDb_Sharp_2_t));
    }
    XML_CHECK_START(CALIB_SENSOR_SHARP_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->sharp.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_VERSION_TAG_ID)) {
            ParseString(pchild, mCalibDb->sharp.version, sizeof(mCalibDb->sharp.version));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_LUMA_POINT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.luma_point, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_MODE_CELL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) {
                int cell_size = 0;
                parseCellNoElement(pchild->ToElement(), tag.Size(), cell_size);
                mCalibDb->sharp.mode_num = cell_size;
                mCalibDb->sharp.mode_cell = (CalibDb_Sharp_ModeCell_t *)malloc(cell_size * sizeof(CalibDb_Sharp_ModeCell_t));
                memset(mCalibDb->sharp.mode_cell, 0x00, cell_size * sizeof(CalibDb_Sharp_ModeCell_t));
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorSharpModeCell,
                                    param,
                                    (uint32_t)CALIB_SENSOR_SHARP_MODE_CELL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_SHARP_TAG_ID)) {
                    LOGE("parse error in SHARP mode cell (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {

                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), mCalibDb->sharp.mode_num,
                                     &RkAiqCalibParser::parseEntrySensorSharpModeCell,
                                     param,
                                     (uint32_t)CALIB_SENSOR_SHARP_MODE_CELL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_SHARP_TAG_ID)) {
                    LOGE("parse error in SHARP mode cell (%s)", tagname.c_str());
                    return (false);
                }
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorSharpModeCell
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    int index = *((int*)param);
    XML_CHECK_START(CALIB_SENSOR_SHARP_MODE_CELL_TAG_ID, CALIB_SENSOR_SHARP_TAG_ID);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_MODE_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->sharp.mode_cell[index].name, sizeof(mCalibDb->sharp.mode_cell[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_GAUSS_LUMA_COEFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].gauss_luma_coeff, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_MBF_COEFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].mbf_coeff, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
#if 1
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_PBF_COEFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].pbf_coeff_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            memcpy(mCalibDb->sharp.mode_cell[index].pbf_coeff_h, mCalibDb->sharp.mode_cell[index].pbf_coeff_l, sizeof(mCalibDb->sharp.mode_cell[index].pbf_coeff_l));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_RF_M_COEFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].rf_m_coeff_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            memcpy(mCalibDb->sharp.mode_cell[index].rf_m_coeff_h, mCalibDb->sharp.mode_cell[index].rf_m_coeff_l, sizeof(mCalibDb->sharp.mode_cell[index].rf_m_coeff_l));
        }

        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_RF_H_COEFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].rf_h_coeff_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            memcpy(mCalibDb->sharp.mode_cell[index].rf_h_coeff_h, mCalibDb->sharp.mode_cell[index].rf_h_coeff_l, sizeof(mCalibDb->sharp.mode_cell[index].rf_h_coeff_l));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_HBF_COEFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].hbf_coeff_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
            memcpy(mCalibDb->sharp.mode_cell[index].hbf_coeff_h, mCalibDb->sharp.mode_cell[index].hbf_coeff_l, sizeof(mCalibDb->sharp.mode_cell[index].hbf_coeff_l));
        }
#endif
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_PBF_COEFF_L_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].pbf_coeff_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_PBF_COEFF_H_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].pbf_coeff_h, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_RF_M_COEFF_L_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].rf_m_coeff_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_RF_M_COEFF_H_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].rf_m_coeff_h, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_RF_H_COEFF_L_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].rf_h_coeff_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_RF_H_COEFF_H_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].rf_h_coeff_h, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_HBF_COEFF_L_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].hbf_coeff_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_HBF_COEFF_H_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->sharp.mode_cell[index].hbf_coeff_h, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SETTING_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorSharpSetting,
                                 (void *)&mCalibDb->sharp.mode_cell[index],
                                 (uint32_t)CALIB_SENSOR_SHARP_SETTING_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_SHARP_MODE_CELL_TAG_ID)) {
                LOGE("parse error in SHARP SHARP_Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);

    return (true);
}


bool RkAiqCalibParser::parseEntrySensorSharpSetting
(
    const XMLElement*   pelement,
    void*               param,
    int                 index
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    CalibDb_Sharp_ModeCell_t *pModeCell = (CalibDb_Sharp_ModeCell_t *)param;

    XML_CHECK_START(CALIB_SENSOR_SHARP_SETTING_TAG_ID, CALIB_SENSOR_SHARP_MODE_CELL_TAG_ID);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SETTING_SNR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].snr_mode, sizeof(pModeCell->setting[index].snr_mode));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SETTING_SENSOR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].sensor_mode, sizeof(pModeCell->setting[index].sensor_mode));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorSharpISO,
                                 (void *)&pModeCell->setting[index],
                                 (uint32_t)CALIB_SENSOR_SHARP_SHARP_ISO_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_SHARP_SETTING_TAG_ID)) {
                LOGE("parse error in SHARP SHARP_ISO (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);

    return (true);
}

bool RkAiqCalibParser::parseEntrySensorSharpISO
(
    const XMLElement*   pelement,
    void*                param,
    int                  index
) {

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_SHARP_SHARP_ISO_TAG_ID, CALIB_SENSOR_SHARP_SETTING_TAG_ID);

    CalibDb_Sharp_Setting_t *pSetting = (CalibDb_Sharp_Setting_t *)param;
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_HRATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].hratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_LRATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].lratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_MF_SHARP_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].mf_sharp_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_HF_SHARP_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].hf_sharp_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_LUMA_SIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->sharp_iso[index].luma_sigma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_PBF_GAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].pbf_gain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_PBF_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].pbf_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_PBF_ADD_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].pbf_add, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_MF_CLIP_POS_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->sharp_iso[index].mf_clip_pos, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_MF_CLIP_NEG_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->sharp_iso[index].mf_clip_neg, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_HF_CLIP_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->sharp_iso[index].hf_clip, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_MBF_GAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].mbf_gain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_HBF_GAIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].hbf_gain, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_HBF_RATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].hbf_ratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_MBF_ADD_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].mbf_add, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_HBF_ADD_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].hbf_add, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_ISO_LOCAL_SHARP_STRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].local_sharp_strength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_PBF_COEFF_PERCENT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].pbf_coeff_percent, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_RF_M_COEFF_PERCENT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].rf_m_coeff_percent, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_RF_H_COEFF_PERCENT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].rf_h_coeff_percent, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_SHARP_SHARP_HBF_COEFF_PERCENT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->sharp_iso[index].hbf_coeff_percent, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorEdgeFilter
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_EDGEFILTER_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->edgeFilter.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_VERSION_TAG_ID)) {
            ParseString(pchild, mCalibDb->edgeFilter.version, sizeof(mCalibDb->edgeFilter.version));
        }
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_LUMA_POINT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->edgeFilter.luma_point, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_MODE_CELL_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ) {
                int cell_size = 0;
                parseCellNoElement(pchild->ToElement(), tag.Size(), cell_size);
                mCalibDb->edgeFilter.mode_num = cell_size;
                mCalibDb->edgeFilter.mode_cell = (CalibDb_EdgeFilter_ModeCell_t *)malloc(cell_size * sizeof(CalibDb_EdgeFilter_ModeCell_t));
                memset(mCalibDb->edgeFilter.mode_cell, 0x00, cell_size * sizeof(CalibDb_EdgeFilter_ModeCell_t));
                if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                    &RkAiqCalibParser::parseEntrySensorEdgeFilterModeCell,
                                    param,
                                    (uint32_t)CALIB_SENSOR_EDGEFILTER_MODE_CELL_TAG_ID,
                                    (uint32_t)CALIB_SENSOR_EDGEFILTER_TAG_ID)) {
                    LOGE("parse error in EDGEFILTER mode cell (%s)", tagname.c_str());
                    return (false);
                }
            }
            else
            {

                if (!parseEntryCell3((XMLElement *)pchild->ToElement(), tag.Size(), mCalibDb->edgeFilter.mode_num,
                                     &RkAiqCalibParser::parseEntrySensorEdgeFilterModeCell,
                                     param,
                                     (uint32_t)CALIB_SENSOR_EDGEFILTER_MODE_CELL_TAG_ID,
                                     (uint32_t)CALIB_SENSOR_EDGEFILTER_TAG_ID)) {
                    LOGE("parse error in EDGEFILTER mode cell (%s)", tagname.c_str());
                    return (false);
                }
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorEdgeFilterModeCell
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    int index = *((int*)param);

    XML_CHECK_START(CALIB_SENSOR_EDGEFILTER_MODE_CELL_TAG_ID, CALIB_SENSOR_EDGEFILTER_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_MODE_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->edgeFilter.mode_cell[index].name, sizeof(mCalibDb->edgeFilter.mode_cell[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_KERNEL_L_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->edgeFilter.mode_cell[index].dog_kernel_l, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_KERNEL_H_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->edgeFilter.mode_cell[index].dog_kernel_h, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_SETTING_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorEdgeFilterSetting,
                                 (void *)&mCalibDb->edgeFilter.mode_cell[index],
                                 (uint32_t)CALIB_SENSOR_EDGEFILTER_SETTING_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_EDGEFILTER_MODE_CELL_TAG_ID)) {
                LOGE("parse error in EDGEFILTER setting (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorEdgeFilterSetting
(
    const XMLElement*   pelement,
    void*               param,
    int                 index
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    CalibDb_EdgeFilter_ModeCell_t *pModeCell = (CalibDb_EdgeFilter_ModeCell_t *)param;

    XML_CHECK_START(CALIB_SENSOR_EDGEFILTER_SETTING_TAG_ID, CALIB_SENSOR_EDGEFILTER_MODE_CELL_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_SETTING_SNR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].snr_mode, sizeof(pModeCell->setting[index].snr_mode));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_SETTING_SENSOR_MODE_TAG_ID)) {
            ParseString(pchild, pModeCell->setting[index].sensor_mode, sizeof(pModeCell->setting[index].sensor_mode));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_TAG_ID)) {
            if (!parseEntryCell2(pchild->ToElement(), tag.Size(),
                                 &RkAiqCalibParser::parseEntrySensorEdgeFilterISO,
                                 (void *)&pModeCell->setting[index],
                                 (uint32_t)CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_TAG_ID,
                                 (uint32_t)CALIB_SENSOR_EDGEFILTER_SETTING_TAG_ID)) {
                LOGE("parse error in EDGEFILTER EDGEFILTER_ISO (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorEdgeFilterISO
(
    const XMLElement*   pelement,
    void*                param,
    int                 index
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_TAG_ID,
                    CALIB_SENSOR_EDGEFILTER_SETTING_TAG_ID);

    CalibDb_EdgeFilter_Setting_t *pSetting = (CalibDb_EdgeFilter_Setting_t *)param;
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->edgeFilter_iso[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_EDGE_THED_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->edgeFilter_iso[index].edge_thed, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_SRC_WGT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->edgeFilter_iso[index].src_wgt, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_ALPHA_ADP_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, &pSetting->edgeFilter_iso[index].alpha_adp_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_LOCAL_ALPHA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->edgeFilter_iso[index].local_alpha, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_GLOBAL_ALPHA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->edgeFilter_iso[index].global_alpha, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_NOISE_CLIP_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].noise_clip, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_CLIP_POS_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].dog_clip_pos, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_CLIP_NEG_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].dog_clip_neg, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_ALPHA_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].dog_alpha, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DIRECT_FILTER_COEFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].direct_filter_coeff, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
#if 1
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_KERNEL_ROW0_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].dog_kernel_row0, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_KERNEL_ROW1_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].dog_kernel_row1, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_KERNEL_ROW2_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].dog_kernel_row2, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_KERNEL_ROW3_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].dog_kernel_row3, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_KERNEL_ROW4_TAG_ID)) {
            int no = ParseFloatArray(pchild, pSetting->edgeFilter_iso[index].dog_kernel_row4, tag.Size(), 6);
            DCT_ASSERT((no == tag.Size()));
        }
#endif
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_EDGEFILTER_EDGEFILTER_ISO_DOG_KERNEL_PERCENT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &pSetting->edgeFilter_iso[index].dog_kernel_percent, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDehaze
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_DEHAZE_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_EN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_CFG_ALPHA_NORMAL_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.cfg_alpha_normal, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_CFG_ALPHA_HDR_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.cfg_alpha_hdr, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_CFG_ALPHA_NIGHT_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.cfg_alpha_night, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorDehazeSetting,
                                param,
                                (uint32_t)CALIB_SENSOR_DEHAZE_SETTING_TAG_ID,
                                (uint32_t)CALIB_SENSOR_DEHAZE_TAG_ID)) {
                LOGE("parse error in Dehaze Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_ENHANCE_SETTING_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorEnhanceSetting,
                                param,
                                (uint32_t)CALIB_SENSOR_ENHANCE_SETTING_TAG_ID,
                                (uint32_t)CALIB_SENSOR_DEHAZE_TAG_ID)) {
                LOGE("parse error in Enhance Setting (%s)", tagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorHistSetting,
                                param,
                                (uint32_t)CALIB_SENSOR_HIST_SETTING_TAG_ID,
                                (uint32_t)CALIB_SENSOR_DEHAZE_TAG_ID)) {
                LOGE("parse error in Hist setting (%s)", tagname.c_str());
                return (false);
            }
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorDehazeSetting
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_DEHAZE_SETTING_TAG_ID, CALIB_SENSOR_DEHAZE_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->dehaze.dehaze_setting[index].name, sizeof(mCalibDb->sharp.mode_cell[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_EN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.dehaze_setting[index].en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_DC_MIN_TH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].dc_min_th, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_DC_MAX_TH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].dc_max_th, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_YHIST_TH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].yhist_th, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_YBLK_TH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].yblk_th, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_DARK_TH_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].dark_th, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_BRIGHT_MIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].bright_min, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_BRIGHT_MAX_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].bright_max, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_WT_MAX_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].wt_max, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_AIR_MIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].air_min, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_AIR_MAX_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].air_max, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_TMAX_BASE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].tmax_base, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_TMAX_OFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].tmax_off, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_TMAX_MAX_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].tmax_max, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_CFG_WT_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].cfg_wt, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_CFG_AIR_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].cfg_air, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_CFG_TMAX_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].cfg_tmax, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_DC_THED_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].dc_thed, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_DC_WEITCUR_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].dc_weitcur, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_AIR_THED_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].air_thed, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_DEHAZE_SETTING_AIR_WEITCUR_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.dehaze_setting[index].air_weitcur, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_IIR_SETTING_STAB_FNUM_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.dehaze_setting[index].IIR_setting.stab_fnum, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_IIR_SETTING_SIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.dehaze_setting[index].IIR_setting.sigma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_IIR_SETTING_WT_SIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.dehaze_setting[index].IIR_setting.wt_sigma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_IIR_SETTING_AIR_SIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.dehaze_setting[index].IIR_setting.air_sigma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_IIR_SETTING_TMAX_SIGMA_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.dehaze_setting[index].IIR_setting.tmax_sigma, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorEnhanceSetting
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_ENHANCE_SETTING_TAG_ID, CALIB_SENSOR_DEHAZE_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_ENHANCE_SETTING_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->dehaze.enhance_setting[index].name, sizeof(mCalibDb->sharp.mode_cell[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_ENHANCE_SETTING_EN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.enhance_setting[index].en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_ENHANCE_SETTING_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.enhance_setting[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_ENHANCE_SETTING_ENHANCE_VALUE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.enhance_setting[index].enhance_value, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorHistSetting
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_HIST_SETTING_TAG_ID, CALIB_SENSOR_DEHAZE_TAG_ID);
    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_NAME_TAG_ID)) {
            ParseString(pchild, mCalibDb->dehaze.hist_setting[index].name, sizeof(mCalibDb->sharp.mode_cell[index].name));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_EN_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->dehaze.hist_setting[index].en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_ISO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.hist_setting[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_HIST_CHANNEL_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dehaze.hist_setting[index].hist_channel, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_HIST_PARA_EN_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->dehaze.hist_setting[index].hist_para_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_HIST_GRATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.hist_setting[index].hist_gratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_HIST_TH_OFF_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.hist_setting[index].hist_th_off, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_HIST_K_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.hist_setting[index].hist_k, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_HIST_MIN_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.hist_setting[index].hist_min, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_HIST_SCALE_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.hist_setting[index].hist_scale, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_HIST_SETTING_CFG_GRATIO_TAG_ID)) {
            int no = ParseFloatArray(pchild, mCalibDb->dehaze.hist_setting[index].cfg_gratio, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }

        pchild = pchild->NextSibling();

    }


    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfWindow
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_WINDOW_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_H_OFFS_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.win_h_offs, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_V_OFFS_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.win_v_offs, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_H_SIZE_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.win_h_size, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_V_SIZE_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.win_v_size, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfFixedMode
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_FIXED_MODE_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_DEF_CODE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.fixed_mode.code, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfMacroMode
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_MACRO_MODE_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_DEF_CODE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.macro_mode.code, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfInfinityMode
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_INFINITY_MODE_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_DEF_CODE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.infinity_mode.code, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfContrastAf
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_CONTRAST_AF_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_CONTRAST_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.contrast_af.enable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SEARCH_STRATEGY_TAG_ID)) {
            char* value = Toupper(secsubtag.Value());
            std::string s_value(value);
            if (s_value == CALIB_SENSOR_AF_SEARCH_STRATEGY_ADAPTIVE) {
                mCalibDb->af.contrast_af.Afss = CAM_AFM_FSS_ADAPTIVE_RANGE;
            } else if (s_value == CALIB_SENSOR_AF_SEARCH_STRATEGY_HILLCLIMB) {
                mCalibDb->af.contrast_af.Afss = CAM_AFM_FSS_HILLCLIMBING;
            } else if (s_value == CALIB_SENSOR_AF_SEARCH_STRATEGY_FULL) {
                mCalibDb->af.contrast_af.Afss = CAM_AFM_FSS_FULLRANGE;
            } else if (s_value == CALIB_SENSOR_AF_SEARCH_STRATEGY_MUTIWIN) {
                mCalibDb->af.contrast_af.Afss = CAM_AFM_FSS_MUTIWINDOW;
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FULL_DIR_TAG_ID)) {
            char* value = Toupper(secsubtag.Value());
            std::string s_value(value);
            if (s_value == CALIB_SENSOR_AF_DIR_POSITIVE) {
                mCalibDb->af.contrast_af.FullDir = CAM_AFM_POSITIVE_SEARCH;
            } else if (s_value == CALIB_SENSOR_AF_DIR_NEGATIVE) {
                mCalibDb->af.contrast_af.FullDir = CAM_AFM_NEGATIVE_SEARCH;
            } else if (s_value == CALIB_SENSOR_AF_DIR_ADAPTIVE) {
                mCalibDb->af.contrast_af.FullDir = CAM_AFM_ADAPTIVE_SEARCH;
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FULL_RANGE_TBL_TAG_ID)) {
            int ArraySize     = secsubtag.Size();
            mCalibDb->af.contrast_af.FullSteps = ArraySize;
            int no = ParseUshortArray(psecsubchild, mCalibDb->af.contrast_af.FullRangeTbl, ArraySize);
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ADAPTIVE_DIR_TAG_ID)) {
            char* value = Toupper(secsubtag.Value());
            std::string s_value(value);
            if (s_value == CALIB_SENSOR_AF_DIR_POSITIVE) {
                mCalibDb->af.contrast_af.AdaptiveDir = CAM_AFM_POSITIVE_SEARCH;
            } else if (s_value == CALIB_SENSOR_AF_DIR_NEGATIVE) {
                mCalibDb->af.contrast_af.AdaptiveDir = CAM_AFM_NEGATIVE_SEARCH;
            } else if (s_value == CALIB_SENSOR_AF_DIR_ADAPTIVE) {
                mCalibDb->af.contrast_af.AdaptiveDir = CAM_AFM_ADAPTIVE_SEARCH;
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ADAPTIVE_RANGE_TBL_TAG_ID)) {
            int ArraySize     = secsubtag.Size();
            mCalibDb->af.contrast_af.AdaptiveSteps = ArraySize;
            int no = ParseUshortArray(psecsubchild, mCalibDb->af.contrast_af.AdaptRangeTbl, ArraySize);
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_QUICKFOUND_THERS_ZOOMIDX_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, mCalibDb->af.contrast_af.QuickFoundThersZoomIdx, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_QUICKFOUND_THERS_TAG_ID)) {
            int ArraySize     = secsubtag.Size();
            mCalibDb->af.contrast_af.QuickFoundThersNum = ArraySize;
            int no = ParseFloatArray(psecsubchild, mCalibDb->af.contrast_af.QuickFoundThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SEARCH_STEP_ZOOMIDX_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, mCalibDb->af.contrast_af.SearchStepZoomIdx, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SEARCH_STEP_TAG_ID)) {
            int ArraySize     = secsubtag.Size();
            mCalibDb->af.contrast_af.SearchStepNum = ArraySize;
            int no = ParseUshortArray(psecsubchild, mCalibDb->af.contrast_af.SearchStep, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_STOP_STEP_ZOOMIDX_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, mCalibDb->af.contrast_af.StopStepZoomIdx, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_STOP_STEP_TAG_ID)) {
            int ArraySize     = secsubtag.Size();
            mCalibDb->af.contrast_af.StopStepNum = ArraySize;
            int no = ParseUshortArray(psecsubchild, mCalibDb->af.contrast_af.StopStep, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SKIP_HIGHPASS_ZOOMIDX_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.contrast_af.SkipHighPassZoomIdx, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SKIP_HIGHPASS_GAIN_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.SkipHighPassGain, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_TRIG_THERS_TAG_ID)) {
            int ArraySize     = secsubtag.Size();
            mCalibDb->af.contrast_af.TrigThersNums = ArraySize;
            int no = ParseFloatArray(psecsubchild, mCalibDb->af.contrast_af.TrigThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_TRIG_THERS_FV_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, mCalibDb->af.contrast_af.TrigThersFv, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LUMA_TRIG_THERS_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.LumaTrigThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_EXP_TRIG_THERS_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.ExpTrigThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_STABLE_THERS_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.StableThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_STABLE_FRAMES_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.contrast_af.StableFrames, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_STABLE_TIME_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.contrast_af.StableTime, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SCENE_DIFF_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.contrast_af.SceneDiffEnable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SCENE_DIFF_THERS_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.SceneDiffThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SCENE_DIFF_BLK_THERS_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.contrast_af.SceneDiffBlkThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_CENTER_SCENE_DIFF_THERS_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.CenterSceneDiffThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_VALID_MAX_MIN_RATIO_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.ValidMaxMinRatio, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_VALID_VALUE_THERS_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.ValidValueThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_OUT_FOCUS_VALUE_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.OutFocusValue, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_OUT_FOCUS_POS_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.contrast_af.OutFocusPos, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FLAT_VALUE_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.FlatValue, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_WEIGHT_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.contrast_af.WeightEnable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_WEIGHT_MATRIX_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, mCalibDb->af.contrast_af.Weight, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SEARCH_PAUSE_LUMA_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.contrast_af.SearchPauseLumaEnable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SEARCH_PAUSE_LUMA_THERS_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.SearchPauseLumaThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SEARCH_PAUSE_LUMA_STABLE_FRAMES_TAG_ID)) {
            int no = ParseUshortArray(psecsubchild, &mCalibDb->af.contrast_af.SearchLumaStableFrames, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_SEARCH_PAUSE_LUMA_STABLE_THERS_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.contrast_af.SearchLumaStableThers, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfLaserAf
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_LASER_AF_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LASERAF_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.laser_af.enable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LASER_AF_VCMDOT_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, mCalibDb->af.laser_af.vcmDot, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LASER_AF_DISTANCEDOT_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, mCalibDb->af.laser_af.distanceDot, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfPdaf
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_PDAF_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_PDAF_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.pdaf.enable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfVcmCfg
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_VCM_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_VCM_START_CURRENT_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.vcmcfg.start_current, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_VCM_RATED_CURRENT_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.vcmcfg.rated_current, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_VCM_STEP_MODE_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.vcmcfg.step_mode, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_EXTRA_DELAY_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.vcmcfg.extra_delay, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfLdgParam
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_LDG_PARAM_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LDG_PARAM_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.ldg_param.enable, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LDG_PARAM_XL_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.ldg_param.ldg_xl, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LDG_PARAM_YL_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.ldg_param.ldg_yl, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LDG_PARAM_KL_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.ldg_param.ldg_kl, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LDG_PARAM_XH_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.ldg_param.ldg_xh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LDG_PARAM_YH_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.ldg_param.ldg_yh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LDG_PARAM_KH_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.ldg_param.ldg_kh, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfHighlight
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_HIGHLIGHT_PARAM_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_HIGHLIGHT_PARAM_THERS0_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.highlight.ther0, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_HIGHLIGHT_PARAM_THERS1_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.highlight.ther1, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfMeasISO
(
    const XMLElement*   pelement,
    void*                param
) {

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AF_MEAS_ISO_TAG_ID, CALIB_SENSOR_AF_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_MEAS_ISO_ISO_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->af.measiso_cfg[index].iso, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_MEAS_ISO_AFMTHRES_TAG_ID)) {
            int no = ParseUshortArray(pchild, &mCalibDb->af.measiso_cfg[index].afmThres, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_MEAS_ISO_GAMMA_Y_TAG_ID)) {
            int no = ParseUshortArray(pchild, mCalibDb->af.measiso_cfg[index].gammaY, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_MEAS_ISO_GAUSS_WEIGHT_TAG_ID)) {
            int no = ParseUcharArray(pchild, mCalibDb->af.measiso_cfg[index].gaussWeight, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfZoomFocusDist
(
    const XMLElement*   pelement,
    void*               param
) {

    LOGD("%s(%d): (enter)\n", __FUNCTION__, __LINE__);

    XML_CHECK_START(CALIB_SENSOR_AF_ZOOM_FOCUS_FOCUSPOS_TAG_ID, CALIB_SENSOR_AF_ZOOM_FOCUS_TBL_TAG_ID);

    int index = *((int*)param);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        const char* value = tag.Value();
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_FOCUSPOS_DISTANCE_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->af.zoomfocus_tbl.focuspos[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_FOCUSPOS_POSIYION_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ)
                mCalibDb->af.zoomfocus_tbl.focuscode[index] = (signed short *)malloc(sizeof(signed short) * tag.Size());
            int no = ParseShortArray(pchild, mCalibDb->af.zoomfocus_tbl.focuscode[index], tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAfZoomFocusTbl
(
    const XMLElement*   pelement,
    void*                param
) {
    (void)param;
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();
    XML_CHECK_START(CALIB_SENSOR_AF_ZOOM_FOCUS_TBL_TAG_ID, CALIB_SENSOR_AF_TAG_ID);
    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag secsubtag = XmlTag(psecsubchild->ToElement());
        std::string secsubTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubtag.Type(), secsubtag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_WIDE_MODULE_DEVIATION_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.widemod_deviate, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_TELE_MODULE_DEVIATION_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.telemod_deviate, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_ZOOM_MOVE_DOT_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, mCalibDb->af.zoomfocus_tbl.zoom_move_dot, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_ZOOM_MOVE_STEP_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, mCalibDb->af.zoomfocus_tbl.zoom_move_step, secsubtag.Size());
            mCalibDb->af.zoomfocus_tbl.zoom_move_tbl_len = no;
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_FOCUS_LENGTH_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ)
                mCalibDb->af.zoomfocus_tbl.focal_length = (float *)malloc(sizeof(float) * secsubtag.Size());
            int no = ParseFloatArray(psecsubchild, mCalibDb->af.zoomfocus_tbl.focal_length, secsubtag.Size());
            mCalibDb->af.zoomfocus_tbl.tbl_len = no;
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_ZOOMPOS_TAG_ID)) {
            if (xmlParseReadWrite == XML_PARSER_READ)
                mCalibDb->af.zoomfocus_tbl.zoomcode = (signed short *)malloc(sizeof(signed short) * secsubtag.Size());
            int no = ParseShortArray(psecsubchild, mCalibDb->af.zoomfocus_tbl.zoomcode, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_FOCUSPOS_TAG_ID)) {
            if (!parseEntryCell(psecsubchild->ToElement(), secsubtag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAfZoomFocusDist,
                                param,
                                (uint32_t)CALIB_SENSOR_AF_ZOOM_FOCUS_FOCUSPOS_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AF_ZOOM_FOCUS_TBL_TAG_ID)) {
                LOGE("parse error in Af focuspos (%s)", secsubTagname.c_str());
                return (false);
            }
            mCalibDb->af.zoomfocus_tbl.focuspos_len = secsubtag.Size();
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_SEARCH_TABLE_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, mCalibDb->af.zoomfocus_tbl.ZoomSearchTbl, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
            mCalibDb->af.zoomfocus_tbl.ZoomSearchTblNum = no;
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_SEARCH_REFCURVE_INDEX_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.ZoomSearchRefCurveIdx, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FOCUS_SEARCH_MARGIN_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.FocusSearchMargin, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FOCUS_SEARCH_PLUS_RANGE_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, mCalibDb->af.zoomfocus_tbl.FocusSearchPlusRange, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FOCUS_SEARCH_STAGE1_STEP_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.FocusStage1Step, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FOCUS_SEARCH_ZOOM_RANGE_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.searchZoomRange, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FOCUS_SEARCH_FOCUS_RANGE_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.searchFocusRange, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FOCUS_SEARCH_EAVG_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.searchEavg, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FOCUS_SEARCH_EMAX_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.searchEmax, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_POSREC_VALID_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->af.zoomfocus_tbl.IsZoomFocusRec, secsubtag.Size());
            DCT_ASSERT((no == secsubtag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_POSREC_DIR_TAG_ID)) {
            ParseString(psecsubchild, mCalibDb->af.zoomfocus_tbl.ZoomFocusRecDir, sizeof(mCalibDb->af.zoomfocus_tbl.ZoomFocusRecDir));
        }
        psecsubchild = psecsubchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorAf
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_AF_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    mCalibDb->af.af_mode = -1;
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_MODE_TAG_ID)) {
            int no = ParseCharArray(pchild, &mCalibDb->af.af_mode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_WINDOW_TAG_ID)) {
            if (!parseEntrySensorAfWindow(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_FIXED_MODE_TAG_ID)) {
            if (!parseEntrySensorAfFixedMode(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_MACRO_MODE_TAG_ID)) {
            if (!parseEntrySensorAfMacroMode(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_INFINITY_MODE_TAG_ID)) {
            if (!parseEntrySensorAfInfinityMode(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_CONTRAST_AF_TAG_ID)) {
            if (!parseEntrySensorAfContrastAf(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LASER_AF_TAG_ID)) {
            if (!parseEntrySensorAfLaserAf(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_PDAF_TAG_ID)) {
            if (!parseEntrySensorAfPdaf(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_VCM_TAG_ID)) {
            if (!parseEntrySensorAfVcmCfg(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_MEAS_ISO_TAG_ID)) {
            if (!parseEntryCell(pchild->ToElement(), tag.Size(),
                                &RkAiqCalibParser::parseEntrySensorAfMeasISO,
                                param,
                                (uint32_t)CALIB_SENSOR_AF_MEAS_ISO_TAG_ID,
                                (uint32_t)CALIB_SENSOR_AF_TAG_ID)) {
                LOGE("parse error in Af meas_iso (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_ZOOM_FOCUS_TBL_TAG_ID)) {
            if (!parseEntrySensorAfZoomFocusTbl(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_LDG_PARAM_TAG_ID)) {
            if (!parseEntrySensorAfLdgParam(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_AF_HIGHLIGHT_PARAM_TAG_ID)) {
            if (!parseEntrySensorAfHighlight(pchild->ToElement())) {
                LOGE("parse error in Af (%s)", tagname.c_str());
                return (false);
            }
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorLdch
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_LDCH_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LDCH_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->aldch.ldch_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LDCH_MESH_FILE_TAG_ID)) {
            char mesh_filename[256];
            ParseString(pchild, mCalibDb->aldch.meshfile, sizeof(mesh_filename));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LDCH_CORRECT_LEVEL_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->aldch.correct_level, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LDCH_CORRECT_LEVEL_MAX_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->aldch.correct_level_max, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LDCH_LIGHT_CENTER_TAG_ID)) {
            int no = ParseDoubleArray(pchild, mCalibDb->aldch.light_center, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LDCH_DISTORTION_COEFF_ID)) {
            int no = ParseDoubleArray(pchild, mCalibDb->aldch.coefficient, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorLumaDetect
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_LUMA_DETECT_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUMA_DETECT_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->lumaDetect.luma_detect_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUMA_DETECT_FIXED_TIMES_TAG_ID)) {
            int no =  ParseIntArray(pchild, &mCalibDb->lumaDetect.fixed_times, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUMA_DETECT_THRESHOLD_TAG_ID)) {
            int no =  ParseFloatArray(pchild, &mCalibDb->lumaDetect.mutation_threshold, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_LUMA_DETECT_THRESHOLD_LEVEL2_TAG_ID)) {
            int no =  ParseFloatArray(pchild, &mCalibDb->lumaDetect.mutation_threshold_level2, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorFec
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_FEC_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());

        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_FEC_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->afec.fec_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_FEC_MESH_FILE_TAG_ID)) {
            char mesh_filename[256];
            ParseString(pchild, mCalibDb->afec.meshfile, sizeof(mesh_filename));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_FEC_CORRECT_LEVEL_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->afec.correct_level, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_FEC_LIGHT_CENTER_TAG_ID)) {
            int no = ParseDoubleArray(pchild, mCalibDb->afec.light_center, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_FEC_DISTORTION_COEFF_ID)) {
            int no = ParseDoubleArray(pchild, mCalibDb->afec.coefficient, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorOrb
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_ORB_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_ORB_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->orb.orb_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorCpsl
(
    const XMLElement* pelement,
    void* param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SENSOR_CPSL_TAG_ID, CALIB_SENSOR_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->cpsl.support_en, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_MODE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->cpsl.mode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_FORCE_GRAY_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->cpsl.gray, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_LGHT_SRC_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->cpsl.lght_src, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_AUTO_ADJUST_SENS_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->cpsl.ajust_sens, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_AUTO_ON2OFF_TH_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->cpsl.on2off_th, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_AUTO_OFF2ON_TH_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->cpsl.off2on_th, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_AUTO_SW_INTERVAL_TAG_ID)) {
            int no = ParseUintArray(pchild, &mCalibDb->cpsl.sw_interval, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_MANUAL_ON_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->cpsl.cpsl_on, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPSL_MANUAL_STRENGTH_TAG_ID)) {
            int no = ParseFloatArray(pchild, &mCalibDb->cpsl.strength, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);

    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorColorAsGrey
(
    const XMLElement* pelement,
    void* param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();
    XML_CHECK_START(CALIB_SENSOR_COLOR_AS_GREY_TAG_ID, CALIB_SENSOR_TAG_ID);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_COLOR_AS_GREY_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->colorAsGrey.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorIE
(
    const XMLElement* pelement,
    void* param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();
    XML_CHECK_START(CALIB_SENSOR_IE_TAG_ID, CALIB_SENSOR_TAG_ID);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_IE_ENABLE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->ie.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_IE_MODE_TAG_ID)) {
            int no = ParseIntArray(pchild, &mCalibDb->ie.mode, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySensorCproc
(
    const XMLElement* pelement,
    void* param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();
    XML_CHECK_START(CALIB_SENSOR_CPROC_TAG_ID, CALIB_SENSOR_TAG_ID);
    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPROC_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->cProc.enable, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPROC_BRIGHTNESS_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->cProc.brightness, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPROC_CONTRAST_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->cProc.contrast, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPROC_SATURATION_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->cProc.saturation, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SENSOR_CPROC_HUE_TAG_ID)) {
            int no = ParseUcharArray(pchild, &mCalibDb->cProc.hue, tag.Size());
            DCT_ASSERT((no == tag.Size()));
        }
        pchild = pchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD("%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySystemHDR
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_HDR_TAG_ID, CALIB_SYSTEM_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag subTag = XmlTag(psecsubchild->ToElement());
        const char* value = subTag.Value();
        std::string subTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_HDR_ENABLE_TAG_ID)) {
            int no = ParseUcharArray(psecsubchild, &mCalibDb->sysContrl.hdr_en, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_HDR_SUPPORT_MODE_TAG_ID)) {
            char hdr_mode[32];
            ParseString(psecsubchild, hdr_mode, sizeof(hdr_mode));
            if (xmlParseReadWrite == XML_PARSER_READ) {
                if (0 == strcasecmp(hdr_mode, HDR_MODE_2_FRAME_STR))
                    mCalibDb->sysContrl.hdr_mode = RK_AIQ_ISP_HDR_MODE_2_FRAME_HDR;
                else if (0 == strcasecmp(hdr_mode, HDR_MODE_2_LINE_STR))
                    mCalibDb->sysContrl.hdr_mode = RK_AIQ_ISP_HDR_MODE_2_LINE_HDR;
                else if (0 == strcasecmp(hdr_mode, HDR_MODE_3_FRAME_STR))
                    mCalibDb->sysContrl.hdr_mode = RK_AIQ_ISP_HDR_MODE_3_FRAME_HDR;
                else if (0 == strcasecmp(hdr_mode, HDR_MODE_3_LINE_STR))
                    mCalibDb->sysContrl.hdr_mode = RK_AIQ_ISP_HDR_MODE_3_LINE_HDR;
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE) {
                XMLNode *pNode = (XMLNode*)psecsubchild;
                char tmpStr[64];
                memset(tmpStr, 0, 64);
                if (mCalibDb->sysContrl.hdr_mode == RK_AIQ_ISP_HDR_MODE_2_FRAME_HDR)
                    strcpy(tmpStr, HDR_MODE_2_FRAME_STR);
                else if (mCalibDb->sysContrl.hdr_mode == RK_AIQ_ISP_HDR_MODE_2_LINE_HDR)
                    strcpy(tmpStr, HDR_MODE_2_LINE_STR);
                else if (mCalibDb->sysContrl.hdr_mode == RK_AIQ_ISP_HDR_MODE_3_FRAME_HDR)
                    strcpy(tmpStr, HDR_MODE_3_FRAME_STR);
                else if (mCalibDb->sysContrl.hdr_mode == RK_AIQ_ISP_HDR_MODE_3_LINE_HDR)
                    strcpy(tmpStr, HDR_MODE_3_LINE_STR);
                ParseString(psecsubchild, tmpStr, strlen(tmpStr));
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_HDR_LINE_MODE_TAG_ID)) {
            char line_mode[16];
            ParseString(psecsubchild, line_mode, sizeof(line_mode));
            if (xmlParseReadWrite == XML_PARSER_READ) {
                if (0 == strcasecmp(line_mode, HDR_LINE_MODE_DCG_STR))
                    mCalibDb->sysContrl.line_mode = RK_AIQ_SENSOR_HDR_LINE_MODE_DCG;
                else if (0 == strcasecmp(line_mode, HDR_LINE_MODE_STAGGER_STR))
                    mCalibDb->sysContrl.line_mode = RK_AIQ_SENSOR_HDR_LINE_MODE_STAGGER;
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE) {
                XMLNode *pNode = (XMLNode*)psecsubchild;
                if (mCalibDb->sysContrl.line_mode == RK_AIQ_SENSOR_HDR_LINE_MODE_DCG)
                    pNode->FirstChild()->SetValue(HDR_LINE_MODE_DCG_STR);
                else if (mCalibDb->sysContrl.line_mode == RK_AIQ_SENSOR_HDR_LINE_MODE_STAGGER)
                    pNode->FirstChild()->SetValue(HDR_LINE_MODE_STAGGER_STR);
            }
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySystemDcgNormalGainCtrl
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_DCG_GAINCTRL_TAG_ID, CALIB_SYSTEM_DCG_SETTING_NORMAL_TAG_ID);

    const XMLNode* pthrdsubchild = pelement->FirstChild();
    while (pthrdsubchild) {
        XmlTag secsubTag = XmlTag(pthrdsubchild->ToElement());
        const char* value = secsubTag.Value();
        std::string secsubTagname(pthrdsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubTag.Type(), secsubTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_ENABLE_TAG_ID)) {
            uint8_t tmp = mCalibDb->sysContrl.dcg.Normal.gainCtrl_en;
            int no = ParseUcharArray(pthrdsubchild, &tmp, secsubTag.Size());
            mCalibDb->sysContrl.dcg.Normal.gainCtrl_en = (tmp == 1) ? true : false;
            DCT_ASSERT((no == secsubTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_LCG2HCG_TH_TAG_ID)) {
            int no = ParseFloatArray(pthrdsubchild, &mCalibDb->sysContrl.dcg.Normal.lcg2hcg_gain_th, secsubTag.Size());
            DCT_ASSERT((no == secsubTag.Size()));
        } else if(XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_HCG2LCG_TH_TAG_ID)) {
            int no = ParseFloatArray(pthrdsubchild, &mCalibDb->sysContrl.dcg.Normal.hcg2lcg_gain_th, secsubTag.Size());
            DCT_ASSERT((no == secsubTag.Size()));
        }
        pthrdsubchild = pthrdsubchild->NextSibling();
    }

    const XMLNode* pchild = pelement->FirstChild();


    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySystemDcgHdrGainCtrl
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_DCG_GAINCTRL_TAG_ID, CALIB_SYSTEM_DCG_SETTING_HDR_TAG_ID);

    const XMLNode* pthrdsubchild = pelement->FirstChild();
    while (pthrdsubchild) {
        XmlTag secsubTag = XmlTag(pthrdsubchild->ToElement());
        const char* value = secsubTag.Value();
        std::string secsubTagname(pthrdsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubTag.Type(), secsubTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_ENABLE_TAG_ID)) {
            uint8_t tmp = mCalibDb->sysContrl.dcg.Hdr.gainCtrl_en;
            int no = ParseUcharArray(pthrdsubchild, &tmp, secsubTag.Size());
            mCalibDb->sysContrl.dcg.Hdr.gainCtrl_en = (tmp == 1) ? true : false;
            DCT_ASSERT((no == secsubTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_LCG2HCG_TH_TAG_ID)) {
            int no = ParseFloatArray(pthrdsubchild, &mCalibDb->sysContrl.dcg.Hdr.lcg2hcg_gain_th, secsubTag.Size());
            DCT_ASSERT((no == secsubTag.Size()));
        } else if(XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_HCG2LCG_TH_TAG_ID)) {
            int no = ParseFloatArray(pthrdsubchild, &mCalibDb->sysContrl.dcg.Hdr.hcg2lcg_gain_th, secsubTag.Size());
            DCT_ASSERT((no == secsubTag.Size()));
        }
        pthrdsubchild = pthrdsubchild->NextSibling();
    }

    const XMLNode* pchild = pelement->FirstChild();


    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySystemDcgHdrEnvCtrl
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_DCG_ENVCTRL_TAG_ID, CALIB_SYSTEM_DCG_SETTING_HDR_TAG_ID);

    const XMLNode* pthrdsubchild = pelement->FirstChild();
    while (pthrdsubchild) {
        XmlTag secsubTag = XmlTag(pthrdsubchild->ToElement());
        const char* value = secsubTag.Value();
        std::string secsubTagname(pthrdsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubTag.Type(), secsubTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_ENABLE_TAG_ID)) {
            uint8_t tmp = mCalibDb->sysContrl.dcg.Hdr.envCtrl_en;
            int no = ParseUcharArray(pthrdsubchild, &tmp, secsubTag.Size());
            mCalibDb->sysContrl.dcg.Hdr.envCtrl_en = (tmp == 1) ? true : false;
            DCT_ASSERT((no == secsubTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_LCG2HCG_TH_TAG_ID)) {
            int no = ParseFloatArray(pthrdsubchild, &mCalibDb->sysContrl.dcg.Hdr.lcg2hcg_env_th, secsubTag.Size());
            DCT_ASSERT((no == secsubTag.Size()));
        } else if(XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_HCG2LCG_TH_TAG_ID)) {
            int no = ParseFloatArray(pthrdsubchild, &mCalibDb->sysContrl.dcg.Hdr.hcg2lcg_env_th, secsubTag.Size());
            DCT_ASSERT((no == secsubTag.Size()));
        }
        pthrdsubchild = pthrdsubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySystemDcgNormalEnvCtrl
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_DCG_ENVCTRL_TAG_ID, CALIB_SYSTEM_DCG_SETTING_NORMAL_TAG_ID);

    const XMLNode* pthrdsubchild = pelement->FirstChild();
    while (pthrdsubchild) {
        XmlTag secsubTag = XmlTag(pthrdsubchild->ToElement());
        const char* value = secsubTag.Value();
        std::string secsubTagname(pthrdsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(secsubTagname.c_str()), secsubTag.Type(), secsubTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_ENABLE_TAG_ID)) {
            uint8_t tmp = mCalibDb->sysContrl.dcg.Normal.envCtrl_en;
            int no = ParseUcharArray(pthrdsubchild, &tmp, secsubTag.Size());
            mCalibDb->sysContrl.dcg.Normal.envCtrl_en = (tmp == 1) ? true : false;
            DCT_ASSERT((no == secsubTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_LCG2HCG_TH_TAG_ID)) {
            int no = ParseFloatArray(pthrdsubchild, &mCalibDb->sysContrl.dcg.Normal.lcg2hcg_env_th, secsubTag.Size());
            DCT_ASSERT((no == secsubTag.Size()));
        } else if(XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_HCG2LCG_TH_TAG_ID)) {
            int no = ParseFloatArray(pthrdsubchild, &mCalibDb->sysContrl.dcg.Normal.hcg2lcg_env_th, secsubTag.Size());
            DCT_ASSERT((no == secsubTag.Size()));
        }
        pthrdsubchild = pthrdsubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySystemDcgHdrSetting
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_DCG_SETTING_HDR_TAG_ID, CALIB_SYSTEM_DCG_SETTING_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag subTag = XmlTag(psecsubchild->ToElement());
        const char* value = subTag.Value();
        std::string subTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_SUPPORT_EN_TAG_ID)) {
            uint8_t tmp = mCalibDb->sysContrl.dcg.Hdr.support_en;
            int no = ParseUcharArray(psecsubchild, &tmp, subTag.Size());
            mCalibDb->sysContrl.dcg.Hdr.support_en = (tmp == 1) ? true : false;
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_OPTYPE_TAG_ID)) {
            char* value = Toupper(subTag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SYSTEM_DCG_OPTYPE_AUTO) {
                    mCalibDb->sysContrl.dcg.Hdr.dcg_optype = RK_AIQ_OP_MODE_AUTO;
                }
                else if (s_value == CALIB_SYSTEM_DCG_OPTYPE_MANUAL) {
                    mCalibDb->sysContrl.dcg.Hdr.dcg_optype = RK_AIQ_OP_MODE_MANUAL;
                }
                else {
                    mCalibDb->sysContrl.dcg.Hdr.dcg_optype = RK_AIQ_OP_MODE_INVALID;
                    LOGE("%s(%d): invalid dcg Hdr.OpType = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psecsubchild;
                if (mCalibDb->sysContrl.dcg.Hdr.dcg_optype == RK_AIQ_OP_MODE_AUTO) {
                    pNode->FirstChild()->SetValue(CALIB_SYSTEM_DCG_OPTYPE_AUTO);
                }
                else if (mCalibDb->sysContrl.dcg.Hdr.dcg_optype == RK_AIQ_OP_MODE_MANUAL) {
                    pNode->FirstChild()->SetValue(CALIB_SYSTEM_DCG_OPTYPE_MANUAL);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid dcg Hdr.OpType = %d\n", __FUNCTION__, __LINE__, mCalibDb->sysContrl.dcg.Hdr.dcg_optype);
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_MODE_INIT_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, mCalibDb->sysContrl.dcg.Hdr.dcg_mode.Coeff, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_RATIO_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->sysContrl.dcg.Hdr.dcg_ratio, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_SYNC_SWITCH_TAG_ID)) {
            uint8_t tmp = mCalibDb->sysContrl.dcg.Hdr.sync_switch;
            int no = ParseUcharArray(psecsubchild, &tmp, subTag.Size());
            mCalibDb->sysContrl.dcg.Hdr.sync_switch = (tmp == 1) ? true : false;
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_GAINCTRL_TAG_ID)) {
            if (!parseEntrySystemDcgHdrGainCtrl(psecsubchild->ToElement())) {
                LOGE("parse error in System DCG HdrGainCtrl (%s)", subTagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_ENVCTRL_TAG_ID)) {
            if (!parseEntrySystemDcgHdrEnvCtrl(psecsubchild->ToElement())) {
                LOGE("parse error in System DCG HdrEnvCtrl (%s)", subTagname.c_str());
                return (false);
            }
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySystemDcgNormalSetting
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_DCG_SETTING_NORMAL_TAG_ID, CALIB_SYSTEM_DCG_SETTING_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag subTag = XmlTag(psecsubchild->ToElement());
        const char* value = subTag.Value();
        std::string subTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_SUPPORT_EN_TAG_ID)) {
            uint8_t tmp = mCalibDb->sysContrl.dcg.Normal.support_en;
            int no = ParseUcharArray(psecsubchild, &tmp, subTag.Size());
            mCalibDb->sysContrl.dcg.Normal.support_en = (tmp == 1) ? true : false;
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_OPTYPE_TAG_ID)) {
            char* value = Toupper(subTag.Value());
            std::string s_value(value);
            if (xmlParseReadWrite == XML_PARSER_READ)
            {
                if (s_value == CALIB_SYSTEM_DCG_OPTYPE_AUTO) {
                    mCalibDb->sysContrl.dcg.Normal.dcg_optype = RK_AIQ_OP_MODE_AUTO;
                }
                else if (s_value == CALIB_SYSTEM_DCG_OPTYPE_MANUAL) {
                    mCalibDb->sysContrl.dcg.Normal.dcg_optype = RK_AIQ_OP_MODE_MANUAL;
                }
                else {
                    mCalibDb->sysContrl.dcg.Normal.dcg_optype = RK_AIQ_OP_MODE_INVALID;
                    LOGE("%s(%d): invalid dcg OpType = %s\n", __FUNCTION__, __LINE__, s_value.c_str());
                }
            }
            else if (xmlParseReadWrite == XML_PARSER_WRITE)
            {
                XMLNode *pNode = (XMLNode*)psecsubchild;
                if (mCalibDb->sysContrl.dcg.Normal.dcg_optype == RK_AIQ_OP_MODE_AUTO) {
                    pNode->FirstChild()->SetValue(CALIB_SYSTEM_DCG_OPTYPE_AUTO);
                }
                else if (mCalibDb->sysContrl.dcg.Normal.dcg_optype == RK_AIQ_OP_MODE_MANUAL) {
                    pNode->FirstChild()->SetValue(CALIB_SYSTEM_DCG_OPTYPE_MANUAL);
                }
                else {
                    pNode->FirstChild()->SetValue("Invalid");
                    LOGE("%s(%d): (XML Write)invalid dcg OpType = %d\n", __FUNCTION__, __LINE__, mCalibDb->sysContrl.dcg.Normal.dcg_optype);
                }
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_MODE_INIT_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, mCalibDb->sysContrl.dcg.Normal.dcg_mode.Coeff, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_RATIO_TAG_ID)) {
            int no = ParseFloatArray(psecsubchild, &mCalibDb->sysContrl.dcg.Normal.dcg_ratio, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_GAINCTRL_TAG_ID)) {
            if (!parseEntrySystemDcgNormalGainCtrl(psecsubchild->ToElement())) {
                LOGE("parse error in System DCG NormalGainCtrl(%s)", subTagname.c_str());
                return (false);
            }
        }
        else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_ENVCTRL_TAG_ID)) {
            if (!parseEntrySystemDcgNormalEnvCtrl(psecsubchild->ToElement())) {
                LOGE("parse error in System DCG NormalEnvCtrl(%s)", subTagname.c_str());
                return (false);
            }
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySystemDcgSetting
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_DCG_SETTING_TAG_ID, CALIB_SYSTEM_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag subTag = XmlTag(psecsubchild->ToElement());
        const char* value = subTag.Value();
        std::string subTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_SETTING_NORMAL_TAG_ID)) {
            if (!parseEntrySystemDcgNormalSetting(psecsubchild->ToElement())) {
                LOGE("parse error in Dcg NormalSetting (%s)", subTagname.c_str());
                return (false);
            }
        } else if(XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_SETTING_HDR_TAG_ID)) {
            if (!parseEntrySystemDcgHdrSetting(psecsubchild->ToElement())) {
                LOGE("parse error in Dcg HdrSetting (%s)", subTagname.c_str());
                return (false);
            }
        }
        psecsubchild = psecsubchild->NextSibling();
    }

    XML_CHECK_END();

    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySystemExpDelayHdr
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_EXP_DELAY_HDR_TAG_ID, CALIB_SYSTEM_EXP_DELAY_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag subTag = XmlTag(psecsubchild->ToElement());
        const char* value = subTag.Value();
        std::string subTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_TIME_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->sysContrl.exp_delay.Hdr.time_delay, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_GAIN_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->sysContrl.exp_delay.Hdr.gain_delay, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_DCG_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->sysContrl.exp_delay.Hdr.dcg_delay, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}
bool RkAiqCalibParser::parseEntrySystemExpDelayNormal
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_EXP_DELAY_NORMAL_TAG_ID, CALIB_SYSTEM_EXP_DELAY_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag subTag = XmlTag(psecsubchild->ToElement());
        const char* value = subTag.Value();
        std::string subTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_TIME_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->sysContrl.exp_delay.Normal.time_delay, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_GAIN_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->sysContrl.exp_delay.Normal.gain_delay, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_DCG_TAG_ID)) {
            int no = ParseIntArray(psecsubchild, &mCalibDb->sysContrl.exp_delay.Normal.dcg_delay, subTag.Size());
            DCT_ASSERT((no == subTag.Size()));
        }
        psecsubchild = psecsubchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySystemExpDelay
(
    const XMLElement*   pelement,
    void*               param
) {
    (void)param;

    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_EXP_DELAY_TAG_ID, CALIB_SYSTEM_TAG_ID);

    const XMLNode* psecsubchild = pelement->FirstChild();
    while (psecsubchild) {
        XmlTag subTag = XmlTag(psecsubchild->ToElement());
        const char* value = subTag.Value();
        std::string subTagname(psecsubchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(subTagname.c_str()), subTag.Type(), subTag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_NORMAL_TAG_ID)) {
            if (!parseEntrySystemExpDelayNormal(psecsubchild->ToElement())) {
                LOGE("parse error in ExpDelay Normal setting (%s)", subTagname.c_str());
                return (false);
            }
        } else if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_HDR_TAG_ID)) {
            if (!parseEntrySystemExpDelayHdr(psecsubchild->ToElement())) {
                LOGE("parse error in ExpDelay Hdr setting (%s)", subTagname.c_str());
                return (false);
            }
        }
        psecsubchild = psecsubchild->NextSibling();
    }
    XML_CHECK_END();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    autoTabBackward();
    return (true);
}

bool RkAiqCalibParser::parseEntrySystem
(
    const XMLElement*   pelement,
    void*                param
) {
    LOGD( "%s(%d): (enter)\n", __FUNCTION__, __LINE__);
    autoTabForward();

    XML_CHECK_START(CALIB_SYSTEM_TAG_ID, CALIB_FILESTART_TAG_ID);

    const XMLNode* pchild = pelement->FirstChild();
    while (pchild) {
        XmlTag tag = XmlTag(pchild->ToElement());
        std::string tagname(pchild->ToElement()->Name());
        XML_CHECK_WHILE_SUBTAG_MARK((char *)(tagname.c_str()), tag.Type(), tag.Size());
        if (XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_HDR_TAG_ID)) {
            if (!parseEntrySystemHDR(pchild->ToElement())) {
                LOGE("parse error in System (%s)", tagname.c_str());
                return (false);
            }
        } else if(XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_DCG_SETTING_TAG_ID)) {
            if (!parseEntrySystemDcgSetting(pchild->ToElement())) {
                LOGE("parse error in System (%s)", tagname.c_str());
                return (false);
            }
        } else if(XML_CHECK_TAGID_COMPARE(CALIB_SYSTEM_EXP_DELAY_TAG_ID)) {
            if (!parseEntrySystemExpDelay(pchild->ToElement())) {
                LOGE("parse error in System (%s)", tagname.c_str());
                return (false);
            }
        }
        pchild = pchild->NextSibling();
    }

    XML_CHECK_END();

    autoTabBackward();
    LOGD( "%s(%d): (exit)\n", __FUNCTION__, __LINE__);
    return (true);
}

#undef DCT_ASSERT
};
