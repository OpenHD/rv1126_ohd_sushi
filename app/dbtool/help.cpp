#include "help.h"

void undefined_cmd_tip() {
    std::string tip = "useless cmd, using '-help' for more info";
    std::cout << (char*)tip.c_str() << std::endl;
}

void show_db_style() {
    std::string tip = "\nThere is a introduction about db struction.\n\n"
                      "Table name should be Pasca. eg: TableName\n"\
                      "Columns name should be Camel-Case. eg: columnsName\n"\
                      "Only INT and TEXT are allowed to using.\n"\
                      "Columns name whose type is INT should starts with i. eg: iNum INT.\n"\
                      "Columns name whose type is TEXT should starts with s. eg: sString TEXT.\n";
    std::cout << (char*)tip.c_str() << std::endl;
}

void show_json_style() {
    std::string sh = HELP_CMD_SHORT;
    // sh += (" " + SYS_OPTION);
    std::string tip = "\nThere is a introduction about json struction.\n\n"
                      "Json file should be a json-object.\n"\
                      "Key of that is table name.\n"\
                      "Value of that is the info to build table.\n\n"\
                      "Value is formed by default(json-array) and items (json-array).\n"\
                      "Default is using to create table.\n"\
                      "Each item of default is a json-object, whose name is column name and setting is condition to create db.\n"\
                      "Items are using to insert rows in table.\n"\
                      "Each item of Items is a json-object, whose key is column name and value is colunm value.\n"\
                      "\neg: {\"TableName\":{\n"\
                      "        \"default\":[\n"\
                      "            {\"name\":\"id\",\n"
                      "             \"setting\":\"INTEGER PRIMARY KEY AUTOINCREMENT\"},\n"\
                      "            {\"name\":\"sName\"\n"\
                      "             \"setting\":\"TEXT\"}\n"\
                      "            ],\n"\
                      "        \"items\":[\n"\
                      "            {\"id\":0,\n"\
                      "             \"sName\":\"test1\"},\n"\
                      "            {\"id\":1,\n"\
                      "             \"sName\":\"test2\"}\n"\
                      "            ]\n"\
                      "    }\n"\
                      "Remark:SystemPara is the table to define capbility, whose column \"para\" is special!\n"\
                      "using \'" + sh + " " + SYS_OPTION + "\' for more info about SystemPara.";
    std::cout << (char*)tip.c_str() << std::endl;
}

void show_help_tip() {
    std::string help_cmd = HELP_CMD;
    std::string tip = "\nThere is a command introduction.\n\n"\
                      + help_cmd + " | " + HELP_CMD_SHORT + " [option] :to get introduction of structure\n"\
                      "option only be " + DB_OPTION + ", " + JS_OPTION + " and " + SYS_OPTION + ".\n"\
                      + MODE_CMD + " | " + MODE_CMD_SHORT + " [option] :to set work function\n"\
                      "option only be " + JS2DB_NAME + ", " + DB2JS_NAME + ", " + JS2JS_NAME + ", " + GET_DIFF_NAME + ", " + DIFF_WORK_NAME + ", " + PATCH_DIFF_NAME".\n"\
                      + DBPATH_CMD + " | " + DBPATH_CMD_SHORT + " [option] :to set db address\n"\
                      "option is the db address.\n"\
                      + JSPATH_CMD + " | " + JSPATH_CMD_SHORT + " [option] :to set json file address\n"\
                      "option is the json file address.\n"\
                      + COMPAREPATH_CMD + " | " + COMPAREPATH_CMD_SHORT + " [option] :to set address of json file compared with\n"\
                      + DIFF_PATH_SET + " | " + DIFF_PATH_SET_SHORT + " [option] :to set address of diff file json\n"\
                      + DIFFPATH_CMD + " | " + DIFFPATH_CMD_SHORT + " [option] :to set address of patch file\n"\
                      "Below are additional options\n"\
                      + PARA_STRING + " | " + PARA_STRING_SHORT + " :get json string in SystemPara.\n"\
                      + AUTO_CHECK + " | " + AUTO_CHECK_SHORT + " :auto modify without check.\n\n"\
                      "When argument is empty, using default.\n"\
                      "default mode is json2db, dbpath is \"./"+ DB_PATH +"\", jspath is \"./" + JSON_PATH + "\".\n"\
                      "example:\n"\
                      "js2db: ./dbtool -m js2db -j sysconfig.json -d sysconfig.db\n"\
                      "db2js: ./dbtool -m db2js -j sysconfig.json -d sysconfig.db\n"\
                      GET_DIFF_NAME + ": .dbtool -m " + GET_DIFF_NAME + " -j sysconfig.json -c compare.json\n"\
                      PATCH_DIFF_NAME + ": .dbtool -m " + PATCH_DIFF_NAME + " -j sysconfig.json -f sysconfig.json.diff\n"\
                      "diffwork: ./dbtool -m diffword -j sysconfig.json -df ../diff/file.json";
    std::cout << (char*)tip.c_str() << std::endl;
}

void show_sys_tip() {
    std::string sys_cmd = SYS_PARA;
    std::string tip = "NOT COMPLETED!\n"\
                      + sys_cmd +" in json file using to special db. Generally, it is similar to capbility.\n\n"\
                      "\'name\' in this means table name in db. if not, that may use to define a relation.\n"\
                      "\'para\' in this means rule. Read document for detail about it.\n";
    std::cout << (char*)tip.c_str() << std::endl;
}

void alarm_tip(std::string tip) {
    if (IS_ALARM) {
        std::cout << (char*)tip.c_str() << std::endl;
    }
}

void not_table_tip(std::string table_name) {
    std::string tip = table_name + NOT_TABLE;
    alarm_tip(tip);
}

void normal_cout(std::string content) {
    std::cout << content << std::endl;
}

std::string db_name_relation(std::string raw_name) {
    if (!raw_name.compare("ipc-2K-2-stream")) {
        return raw_name;
    } else if (!raw_name.compare("ipc-4K-2-stream")) {
        return raw_name;
    } else if (!raw_name.compare("ipc-4K-3-stream")) {
        return "sysconfig-4K";
    } else if (!raw_name.compare("ipc-1080P-2-stream")) {
        return raw_name;
    } else if (!raw_name.compare("ipc-1080P-3-stream")) {
        return "sysconfig-1080P";
    } else if (!raw_name.compare("gate-2K-3-stream")) {
        return raw_name;
    } else if (!raw_name.compare("gate-1080P-3-stream")) {
        return "sysconfig";
    } else if (!raw_name.compare("ipc-2K-3-stream")) {
        return "sysconfig-2K";
    }
    return raw_name;
}
