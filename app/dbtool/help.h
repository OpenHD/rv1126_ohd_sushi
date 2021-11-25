#ifndef __HELP_H__
#define __HELP_H__

#include <iostream>

#define HELP_CMD "--help"
#define HELP_CMD_SHORT "-h"
#define DB_OPTION "db"
#define JS_OPTION "json"
#define SYS_OPTION "sys"

#define DBPATH_CMD "--dbpath"
#define DBPATH_CMD_SHORT "-d"
#define DB_PATH "sysconfig.db"

#define JSPATH_CMD "--jspath"
#define JSPATH_CMD_SHORT "-j"
#define JSON_PATH "sysconfig.json"

#define COMPAREPATH_CMD "--comparepath"
#define COMPAREPATH_CMD_SHORT "-c"
#define COMPARE_PATH "compare.json"

#define DIFFPATH_CMD "--diffpath"
#define DIFFPATH_CMD_SHORT "-f"
#define DIFF_PATH "sysconfig.json.diff"

#define MODE_CMD "--mode"
#define MODE_CMD_SHORT "-m"
#define DEFAULT_FUNC_NAME "js2db"
#define JS2DB_NAME "js2db"
#define DB2JS_NAME "db2js"
#define JS2JS_NAME "js2js"
#define DIFF_WORK_NAME "diffwork"
#define GET_DIFF_NAME "getdiff"
#define PATCH_DIFF_NAME "patchdiff"

#define AUTO_CHECK "--auto"
#define AUTO_CHECK_SHORT "-a"
#define PARA_STRING "--string"
#define PARA_STRING_SHORT "-s"
#define IS_ALARM 1

#define DIFF_WORK "diffwork"
#define DIFF_WORK_SHORT "dw"
#define DIFF_PATH_SET "--difffile"
#define DIFF_PATH_SET_SHORT "-df"
#define DIFF_FILES "../diff/file.json"

#define INT_TYPE "INT"
#define TEXT_TYPE "TEXT"
#define TABLE_NAME "1.tableName"
#define DEFAULT_CONTENT_NAME "3.default"
#define DEFAULT_KEY_NAME "columnName"
#define DEFAULT_KEY_VALUE "setting"
#define ITEMS_CONTENT_NAME "2.items"

#define SYS_PARA "SystemPara"
#define SYS_PARA_KEY "para"
#define STATIC_PARA "static"
#define DYNAMIC_PARA "dynamic"
#define RELATION_PARA "relation"
#define LIMIT_PARA "limit"
#define DISABLED_PARA "disabled"
#define LAYOUT_PARA "layout"
#define PARA_TYPE "type"
#define INPUT_TYPE "input"
#define REFER_TYPE "refer"
#define PARA_OPTIONS "options"
#define PARA_RANGE "range"
#define MAX_MARK "max"
#define MAX_RATE_MARK "maxRate"
#define MIN_MARK "min"
#define MIN_RATE_MARK "minRate"
#define PARA_DYNAMIC_RANGE "dynamicRange"
#define PARA_OPTIONS_DYNAMIC "options/dynamicRange"
#define DISABLED_CONDITION "name"
#define PARA_DISABLED "disabled"
#define PARA_DISABLED_LIMIT "disabled/limit"
#define DELETE_CMD "delete"

#define NOT_TABLE " isn't a table"
#define NOT_EXIST_JSON "json file doesn't exitst!"
#define JSON_EMPTY "json file is empty!"
#define DB_EMPTY "db is empty!"
#define JSON_FILE_LOSS "json file lose "

#define INT_MAX ((unsigned)(-1)>>1)
#define INT_MIN 0

void undefined_cmd_tip();
void show_db_style();
void show_json_style();
void show_help_tip();
void alarm_tip(std::string tip);
void not_table_tip(std::string table_name);
void normal_cout(std::string content);
void show_sys_tip();
std::string db_name_relation(std::string raw_name);

#endif