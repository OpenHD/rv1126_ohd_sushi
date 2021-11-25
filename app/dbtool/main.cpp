#include "js2db.h"
#include "db2js.h"
#include "help.h"
#include "diff.h"

int main(int argc, char* argv[]) {
    std::vector<std::string> arg_bank;
    std::string func_name = DEFAULT_FUNC_NAME;
    std::string db_path = DB_PATH;
    std::string json_path = JSON_PATH;
    std::string compare_path = COMPARE_PATH;
    std::string diff_path = DIFF_PATH;
    std::string diff_file_path = DIFF_FILES;
    int default_dw = 1;
    // get argument
    if (argc > 1) {
        argc -= 1;
        for (int i = 1; i <= argc;) {
            std::string argument_read_pre = argv[i];
            if (!argument_read_pre.compare(HELP_CMD) || !argument_read_pre.compare(HELP_CMD_SHORT)) {
                if (argc > i) {
                    std::string cmd = argv[i+1];
                    if (!cmd.compare(DB_OPTION)) {
                        show_db_style();
                    } else if (!cmd.compare(JS_OPTION)) {
                        show_json_style();
                    } else if (!cmd.compare(SYS_OPTION)) {
                        show_sys_tip();
                    } else {
                        undefined_cmd_tip();
                    }
                } else {
                    show_help_tip();
                }
                return 0;
            } else if (!argument_read_pre.compare(DBPATH_CMD) || !argument_read_pre.compare(DBPATH_CMD_SHORT)) {
                if (argc > i) {
                    db_path = argv[i+1];
                    default_dw = 0;
                    i += 1;
                } else {
                    undefined_cmd_tip();
                    return 0;
                }
            } else if (!argument_read_pre.compare(JSPATH_CMD) || !argument_read_pre.compare(JSPATH_CMD_SHORT)) {
                if (argc > i) {
                    json_path = argv[i+1];
                    i += 1;
                } else {
                    undefined_cmd_tip();
                    return 0;
                }
            } else if (!argument_read_pre.compare(COMPAREPATH_CMD) || !argument_read_pre.compare(COMPAREPATH_CMD_SHORT)) {
                if (argc > i) {
                    compare_path = argv[i+1];
                    i += 1;
                } else {
                    undefined_cmd_tip();
                    return 0;
                }
            } else if (!argument_read_pre.compare(DIFFPATH_CMD) || !argument_read_pre.compare(DIFFPATH_CMD_SHORT)) {
                if (argc > i) {
                    diff_path = argv[i+1];
                    i += 1;
                } else {
                    undefined_cmd_tip();
                    return 0;
                }
            } else if (!argument_read_pre.compare(DIFF_PATH_SET) || !argument_read_pre.compare(DIFF_PATH_SET_SHORT)) {
                if (argc > i) {
                    diff_file_path = argv[i+1];
                    i += 1;
                } else {
                    undefined_cmd_tip();
                    return 0;
                }
            } else if (!argument_read_pre.compare(MODE_CMD) || !argument_read_pre.compare(MODE_CMD_SHORT)) {
                if (argc > i) {
                    std::string cmd = argv[i+1];
                    if (!cmd.compare(JS2DB_NAME)) {
                        func_name = JS2DB_NAME;
                    } else if (!cmd.compare(DB2JS_NAME)) {
                        func_name = DB2JS_NAME;
                    } else if (!cmd.compare(JS2JS_NAME)) {
                        func_name = JS2JS_NAME;
                    } else if (!cmd.compare(GET_DIFF_NAME)) {
                        func_name = GET_DIFF_NAME;
                    } else if (!cmd.compare(PATCH_DIFF_NAME)) {
                        func_name = PATCH_DIFF_NAME;
                    } else if (!cmd.compare(DIFF_WORK) || !cmd.compare(DIFF_WORK_SHORT)) {
                        func_name = DIFF_WORK_NAME;
                    } else {
                        undefined_cmd_tip();
                        return 0;
                    }
                    i += 1;
                } else {
                    undefined_cmd_tip();
                    return 0;
                }
            } else if (!argument_read_pre.compare(AUTO_CHECK) || !argument_read_pre.compare(AUTO_CHECK_SHORT)) {
                run_auto_check();
            } else if (!argument_read_pre.compare(PARA_STRING) || !argument_read_pre.compare(PARA_STRING_SHORT)) {
                set_string_mode();
            } else {
                undefined_cmd_tip();
                return 0;
            }
            i++;
        }
    }
    // enter func
    if (!func_name.compare(DB2JS_NAME)) {
        db2js(db_path, json_path);
    } else if (!func_name.compare(JS2DB_NAME)) {
        if (is_file_exitst((char *)json_path.c_str())) {
            js2db(db_path,json_path);
        } else {
            alarm_tip(NOT_EXIST_JSON);
        }
    } else if (!func_name.compare(JS2JS_NAME)) {
        if (is_file_exitst((char *)json_path.c_str())) {
            js2db(db_path,json_path);
        } else {
            alarm_tip(NOT_EXIST_JSON);
        }
        // need check db exist
        json_path += ".modify";
        db2js(db_path, json_path);
    } else if (!func_name.compare(GET_DIFF_NAME)) {
        if (is_file_exitst((char *)json_path.c_str()) && is_file_exitst((char *)compare_path.c_str())) {
            get_diff(json_path, compare_path);
        } else {
            alarm_tip(NOT_EXIST_JSON);
        }
    } else if (!func_name.compare(PATCH_DIFF_NAME)) {
        if (is_file_exitst((char *)json_path.c_str()) && is_file_exitst((char *)diff_path.c_str())) {
            patch_diff(json_path, diff_path, NULL);
        } else {
            alarm_tip(NOT_EXIST_JSON);
        }
    } else if (!func_name.compare(DIFF_WORK_NAME)) {
        char *files = readFile((char *)diff_file_path.c_str());
        nlohmann::json files_json = nlohmann::json::parse(files);
        for (auto &x : nlohmann::json::iterator_wrapper(files_json)) {
            std::string one_diff = x.value();
            int div_index = one_diff.find_last_of("/");
            std::string one_diff_stand = one_diff.substr(div_index + 1);
            int point_index = one_diff_stand.find_first_of(".");
            std::string raw_name = one_diff_stand.substr(0, point_index);
            std::string diff_name = raw_name + ".json";
            std::string diff_db_name = db_name_relation(raw_name) + ".db";
            patch_diff(json_path, one_diff, diff_name);
            js2db(diff_db_name, diff_name);
        }
        if (default_dw)
            db_path = "sysconfig-2K.db";
        js2db(db_path, json_path);
    } else {
        undefined_cmd_tip();
    }
}