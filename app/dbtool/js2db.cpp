#include <iostream>
#include <fstream>
#include "js2db.h"
#include "help.h"

bool auto_check = false;

void js2db(std::string db_path, std::string json_path) {
    // if exist rm
    if (is_file_exitst((char *)db_path.c_str())) {
        remove((char *)db_path.c_str());
    }
    // open db if not build one
    db_init((char *)db_path.c_str());
    // read json
    char* result = readFile((char *)json_path.c_str());
    if (result) {
        nlohmann::json js = nlohmann::json::parse(result);
        nlohmann::json stand = js;
        int rst;
        for (int i = 0; i < 5; i++) {
            rst = check_json(js);
            if (!rst) {
                std::string tip = "";
                tip += JSON_FILE_LOSS;
                tip += SYS_PARA;
                alarm_tip(tip);
                return;
            }
            nlohmann::json diff = nlohmann::json::diff(js, stand);
            if (diff.empty()) {
                break;
            } else {
                stand = js;
            }
            if (i == 4) {
                std::string error = "json file has cycle limit!";
                throw error;
            }
        }
        first_cycle(js);
        free(result);
    } else {
        alarm_tip(JSON_EMPTY);
    }
    db_deinit();
}

void first_cycle(nlohmann::json js) {
    for (auto& el : js.items()) {
        std::string table_name = el.value().at(TABLE_NAME);
        nlohmann::json one_table_data = el.value();
        try {
            nlohmann::json test = one_table_data.at(DEFAULT_CONTENT_NAME);
            test = one_table_data.at(ITEMS_CONTENT_NAME);
            test = NULL;
        }
        catch(...) {
            not_table_tip(table_name);
            continue;
        }
        if (!table_name.compare(SYS_PARA)) {
            nlohmann::json sys_para = el.value();
            register_sys_para(sys_para);
            continue;
        }
        register_func_normal(table_name, one_table_data);
    }
}

void cycle_register(nlohmann::json js) {
    for (auto& el : js.items()) {
        if (is_upper(el.key()[0])) {
            // get col para need check value type
            std::string table_name = el.key();
            nlohmann::json one_table_data = el.value();
            if (!table_name.compare(SYS_PARA)) {
                register_sys_para(one_table_data);
                continue;
            }
            register_func_normal(table_name, one_table_data);
        }
    }
}

void register_func_normal(std::string table_name, nlohmann::json table_data) {
    std::string create_sql = get_create_sql(table_data.at(DEFAULT_CONTENT_NAME));
    // create table
    create_sql = "CREATE TABLE " + table_name + create_sql;
    int ret = db_sql((char *)create_sql.c_str());
    // insert items
    std::string tip = "";
    if (ret == 101) {
        nlohmann::json items_data = table_data.at(ITEMS_CONTENT_NAME);
        if (!items_data.empty()) {
            int res = exec_insert_sql(table_name, table_data.at(ITEMS_CONTENT_NAME));
            if (res == 101) {
                tip = "register " + table_name + " success";
            } else {
                tip = "register " + table_name + " fail res";
            }
        } else {
            tip = "register " + table_name + " success, but empty";
        }
    } else {
        tip = ("ret %d ", ret);
        tip += ("register " + table_name + " fail");
    }
    alarm_tip(tip);
    table_data.erase(DEFAULT_CONTENT_NAME);
    table_data.erase(ITEMS_CONTENT_NAME);
    if (!table_data.empty()) {
        for (auto &x : nlohmann::json::iterator_wrapper(table_data)) {
            if (is_upper(x.key()[0])) {
                nlohmann::json new_json;
                new_json.emplace(x.key(), x.value());
                cycle_register(new_json);
            }
        }
    }
}

void register_sys_para(nlohmann::json js) {
    // create table
    std::string create_sql = get_create_sql(js.at(DEFAULT_CONTENT_NAME));
    std::string table_name = SYS_PARA;
    create_sql = "CREATE TABLE " + table_name + create_sql;
    int ret = db_sql((char *)create_sql.c_str());
    // insert items
    std::string register_tip = "";
    if (ret == 101) {
        int res = exec_insert_sys_para(table_name, js.at(ITEMS_CONTENT_NAME));
        if (res == 101) {
            register_tip = "register " + table_name + " success";
        } else {
            register_tip = "register " + table_name + " fail res";
        }
    } else {
        register_tip = "register " + table_name + " fail ret";
    }
    alarm_tip(register_tip);
}

int exec_insert_sys_para(std::string table_name, nlohmann::json js_part) {
    int ret = 0;
    for (auto &x : nlohmann::json::iterator_wrapper(js_part)) {
        std::string insert_sql_key = "(";
        std::string insert_value = "VALUES(";
        nlohmann::json one_item = x.value();
        for (auto& el : one_item.items()) {
            // add ,
            if (insert_sql_key.compare("(")) {
                insert_sql_key += ",";
            }
            if (insert_value.compare("VALUES(")) {
                insert_value += ",";
            }
            // judge type
            std::string ky = el.key();
            if (!ky.compare(SYS_PARA_KEY)) {
                insert_sql_key += ky;
                std::string para_string = el.value().dump();
                insert_value += ("'" + para_string + "'");
            } else {
                if (el.value().is_number()) {
                insert_sql_key += ky;
                std::string val = to_string(el.value());
                insert_value += val;
                } else if (el.value().is_string()) {
                    std::string val = el.value();
                    insert_sql_key += ky;
                    insert_value += ("'" + val + "'");
                } else {
                    std::string string_key = el.key();
                    std::string string_val = el.value();
                    std::string type_error_tip = string_key + " " + string_val + " type error1";
                    alarm_tip(type_error_tip);
                }
            }
        }
        insert_sql_key += ") ";
        insert_value += ");";
        std::string insert_sql = "INSERT INTO " + table_name + insert_sql_key + insert_value;
        ret = db_sql((char *)insert_sql.c_str());
    }
    return ret;
}

bool is_upper(char ch) {
    return ch >= 'A' && ch <= 'Z';
}

int exec_insert_sql(std::string table_name, nlohmann::json js_part) {
    int ret = 0;
    for (auto &x : nlohmann::json::iterator_wrapper(js_part)) {
        std::string insert_sql_key = "(";
        std::string insert_value = "VALUES(";
        nlohmann::json one_item = x.value();
        for (auto& el : one_item.items()) {
            // add ,
            if (insert_sql_key.compare("(")) {
                insert_sql_key += ",";
            }
            if (insert_value.compare("VALUES(")) {
                insert_value += ",";
            }
            // judge type
            if (el.value().is_number()) {
                std::string ky = el.key();
                std::string val = to_string(el.value());
                insert_sql_key += ky;
                insert_value += val;
            } else if (el.value().is_string()) {
                std::string ky = el.key();
                std::string val = el.value();
                insert_sql_key += ky;
                insert_value += ("'" + val + "'");
            } else {
                std::string string_key = el.key();
                std::string string_val = el.value();
                std::string type_error_tip = string_key + " " + string_val + " type error0";
                alarm_tip(type_error_tip);
            }
        }
        insert_sql_key += ") ";
        insert_value += ");";
        std::string insert_sql = "INSERT INTO " + table_name + insert_sql_key + insert_value;
        ret = db_sql((char *)insert_sql.c_str());
    }
    return ret;
}

std::string get_create_sql(nlohmann::json js_part) {
    std::string create_sql = "(";
    for (auto& el : js_part.items()) {
        if (create_sql.compare("(")) {
            create_sql += ",";
        }
        nlohmann::json one_default = el.value();
        std::string k = one_default.at(DEFAULT_KEY_NAME);
        std::string v = one_default.at(DEFAULT_KEY_VALUE);
        create_sql += (k + " " + v);
    }
    create_sql += ");";
    return create_sql;
}

int check_json(nlohmann::json &json_file) {
    nlohmann::json sys_json;
    try {
        sys_json = json_file.at(0);
    } catch(...) {
        return 0;
    }
    if (!sys_json[TABLE_NAME].empty() && sys_json[TABLE_NAME] == SYS_PARA) {
        nlohmann::json sys_para = sys_json.at(ITEMS_CONTENT_NAME);
        for (auto &y : nlohmann::json::iterator_wrapper(json_file)) {
            std::string table_name = y.value().at(TABLE_NAME);
            for (auto &x : nlohmann::json::iterator_wrapper(sys_para)) {
                std::string check_name = x.value().at("name");
                nlohmann::json pat_para = x.value().at(SYS_PARA_KEY);
                if (!table_name.compare(check_name)) {
                    y.value().at(ITEMS_CONTENT_NAME) = check_json_func(pat_para, y.value().at(ITEMS_CONTENT_NAME), sys_para, table_name);
                    break;
                }
            }
        }
        return 1;
    } else {
        return 0;
    }
}

nlohmann::json check_json_func(nlohmann::json para_json, nlohmann::json &data_json, nlohmann::json para_bank, std::string table_name) {
    // sort for check item
    std::vector<std::string> check_list;
    std::vector<std::string> check_bank;
    check_bank.push_back(STATIC_PARA);
    check_bank.push_back(DYNAMIC_PARA);
    check_bank.push_back(LIMIT_PARA);
    check_bank.push_back(DISABLED_PARA);
    for (auto const &value: check_bank) {
        if (!para_json[value].empty()) {
            check_list.push_back(value);
        } else {
            para_json.erase(value);
        }
    }
    /*
    check refer and get final para_json
    check_list include static_like_obj in para_json
    disabled_para not check, means refer can't be used in disabled_para
    */

    for (auto const &item : check_list) {
        nlohmann::json item_json = para_json.at(item);
        if (!item.compare(STATIC_PARA)) {
            for (auto &x : nlohmann::json::iterator_wrapper(item_json)) {
                if (x.value()[PARA_TYPE].empty()) {
                    x.value().erase(PARA_TYPE);
                    continue;
                }
                std::string type_name = x.value().at(PARA_TYPE);
                if (!type_name.compare(REFER_TYPE)) {
                    nlohmann::json refer_list = x.value().at(REFER_TYPE);
                    x.value() = get_refer_item(refer_list, 0, para_bank);
                }
            }
        } else if (!item.compare(DYNAMIC_PARA)) {
            for (auto &y : nlohmann::json::iterator_wrapper(item_json)) {
                for (auto &x : nlohmann::json::iterator_wrapper(y.value())) {
                    if (x.value()[PARA_TYPE].empty()) {
                        x.value().erase(PARA_TYPE);
                        continue;
                    }
                    std::string type_name = x.value().at(PARA_TYPE);
                    if (!type_name.compare(REFER_TYPE)) {
                        nlohmann::json refer_list = x.value().at(REFER_TYPE);
                        x.value() = get_refer_item(refer_list, 0, para_bank);
                    }
                }
            }
        } else if (!item.compare(LIMIT_PARA)) {
            alarm_tip("limit is undefined!");
        }
        para_json.at(item) = item_json;
    }
    /*
    cycle table and check item
    data_json refer to items
    row_data is one raw data, refer to items' one
    */
    nlohmann::json new_data_json;
    for (auto &row_data : nlohmann::json::iterator_wrapper(data_json)) {
        for (auto const &item : check_list) {
            // item_json is part of para, like static: {} .value
            nlohmann::json item_json = para_json.at(item);
            if (!item.compare(STATIC_PARA)) {
                // para: key is col name, value is type and option_like_obj
                for (auto &para : nlohmann::json::iterator_wrapper(item_json)) {
                    std::string key_name = para.key();
                    if (!row_data.value()[key_name].empty()) {
                        // row_val
                        nlohmann::json check_rst = check4type(para.value(), row_data.value(), key_name, table_name);
                        if (!check_rst.empty()) {
                            if (check_rst.is_string()) {
                                std::string delete_cmd = check_rst;
                                if (!delete_cmd.compare(DELETE_CMD)) {
                                    row_data.value().clear();
                                    break;
                                } else {
                                    row_data.value().at(key_name) = check_rst;
                                }
                            } else {
                                if (!check_rst.is_null()) {
                                    row_data.value().at(key_name) = check_rst;
                                }
                            }
                        }
                    } else {
                        row_data.value().erase(key_name);
                    }
                }
            } else if (!item.compare(DYNAMIC_PARA)) {
                for (auto &dy : nlohmann::json::iterator_wrapper(item_json)) {
                    std::string dy_key = dy.key();
                    if (!row_data.value()[dy_key].empty()) {
                        std::string match_type = TEXT_TYPE;
                        std::string be_matched_val_string;
                        int be_matched_val_int;
                        if (row_data.value().at(dy_key).is_number()) {
                            match_type = INT_TYPE;
                            be_matched_val_int = row_data.value().at(dy_key);
                        } else {
                            be_matched_val_string = row_data.value().at(dy_key);
                        }
                        for (auto &match_json : nlohmann::json::iterator_wrapper(dy.value())) {
                            std::string match_key = match_json.key();
                            if (!match_type.compare(INT_TYPE)) {
                                int match_int = atoi((char *)match_key.c_str());
                                if (match_int != be_matched_val_int) {
                                    continue;
                                }
                            } else {
                                if (match_key.compare(be_matched_val_string)) {
                                    continue;
                                }
                            }
                            for (auto &para : nlohmann::json::iterator_wrapper(match_json.value())) {
                                std::string key_name = para.key();
                                if (!row_data.value()[key_name].empty()) {
                                    // row_val
                                    nlohmann::json check_rst = check4type(para.value(), row_data.value(), key_name, table_name);
                                    if (check_rst.is_string()) {
                                        std::string delete_cmd = check_rst;
                                        if (!delete_cmd.compare(DELETE_CMD)) {
                                            row_data.value().clear();
                                            break;
                                        } else {
                                            row_data.value().at(key_name) = check_rst;
                                        }
                                    } else {
                                        if (!check_rst.is_null()) {
                                            row_data.value().at(key_name) = check_rst;
                                        }
                                    }
                                } else {
                                    row_data.value().erase(key_name);
                                }
                            }

                        }
                    } else {
                        row_data.value().erase(dy_key);
                    }
                }
            } else if (!item.compare(DISABLED_PARA)) {
                for (auto &dsa : nlohmann::json::iterator_wrapper(item_json)) {
                    std::string check_type = dsa.value().at(PARA_TYPE);
                    if (!check_type.compare(PARA_DISABLED)) {
                        continue;
                    }
                    // design for disabled/limit
                    std::string condition_key = dsa.value().at(DISABLED_CONDITION);
                    if (!row_data.value()[condition_key].empty()) {
                        if (row_data.value().at(condition_key).is_number()) {
                            int be_matched_val_int = row_data.value().at(condition_key);
                            for (auto &match_option : nlohmann::json::iterator_wrapper(dsa.value().at(PARA_OPTIONS))) {
                                std::string match_string = match_option.key();
                                int match_int = atoi((char *)match_string.c_str());
                                if (match_int == be_matched_val_int) {
                                    for (auto &alter_item : nlohmann::json::iterator_wrapper(match_option.value())) {
                                        std::string alter_key = alter_item.key();
                                        if (!row_data.value()[alter_key].empty()) {
                                            int old_data = row_data.value()[alter_key];
                                            int new_data = alter_item.value();
                                            if (old_data != new_data) {
                                                switch (get_customer_check(table_name, row_data.value(), alter_key)) {
                                                    case 1:
                                                        row_data.value().at(alter_key) = new_data;
                                                        break;
                                                    case -1:
                                                        row_data.value().clear();
                                                        break;
                                                }
                                            }
                                        } else {
                                            row_data.value().erase(alter_key);
                                        }
                                    }
                                }
                            }
                        } else {
                            std::string be_matched_val_string = row_data.value().at(condition_key);
                            for (auto &match_option : nlohmann::json::iterator_wrapper(dsa.value().at(PARA_OPTIONS))) {
                                std::string match_string = match_option.key();
                                if (!match_string.compare(be_matched_val_string)) {
                                    for (auto &alter_item : nlohmann::json::iterator_wrapper(match_option.value())) {
                                        std::string alter_key = alter_item.key();
                                        if (!row_data.value()[alter_key].empty()) {
                                            std::string old_data = row_data.value()[alter_key];
                                            std::string new_data = alter_item.value();
                                            if (old_data.compare(new_data)) {
                                                switch (get_customer_check(table_name, row_data.value(), alter_key)) {
                                                    case 1:
                                                        row_data.value().at(alter_key) = new_data;
                                                        break;
                                                    case -1:
                                                        row_data.value().clear();
                                                        break;
                                                }
                                            }
                                        } else {
                                            row_data.value().erase(alter_key);
                                        }
                                    }
                                }
                            }
                        }

                    } else {
                        row_data.value().erase(condition_key);
                    }
                }
            }
            if (row_data.value().empty()) {
                break;
            }
        }
        if (!row_data.value().empty()) {
            new_data_json.push_back(row_data.value());
        }
    }
    return new_data_json;
}

int get_customer_check(std::string table_name, nlohmann::json row_data, std::string col) {
    if (auto_check) {
        return 1;
    }
    std::string id;
    if (!row_data["id"].empty()) {
        id = to_string(row_data.at("id"));
    } else {
        row_data.erase("id");
        id = "undefine";
    }
    std::string error_data = row_data.at(col).dump();
    std::string tip = "in " + table_name + " id " + id + "," + col +  "=" + error_data + " is wrong, autoModify(a)/ignore(i)/delete(d)?";
    normal_cout(tip);
    std::string input;
    std::cin >> input;
    if (!input.compare("a") || !input.compare("autoModify")) {
        return 1;
    } else if (!input.compare("i") || !input.compare("ignore")) {
        return 0;
    } else if (!input.compare("d") || !input.compare("delete")) {
        return -1;
    } else {
        return get_customer_check(table_name, row_data, col);
    }
}

nlohmann::json get_refer_item(nlohmann::json refer_list, int refer_order, nlohmann::json refer_json) {
    int len = refer_list.size();
    if (len <= refer_order) {
        return refer_json;
    }
    nlohmann::json refer = refer_list.at(refer_order);
    if (refer.is_number()) {
        int refer_num = refer_list.at(refer_order);
        refer_json = refer_json.at(refer_num);
    } else if (refer.is_string()) {
        std::string refer_string = refer_list.at(refer_order);
        refer_json = refer_json.at(refer_string);
    }
    nlohmann::json rst_json = get_refer_item(refer_list, refer_order + 1, refer_json);
    return rst_json;
}

// need to deal return 
nlohmann::json check4type(nlohmann::json para, nlohmann::json row_data, std::string key_name, std::string table_name) {
    nlohmann::json return_json;
    return_json.clear();
    int row_val_int;
    std::string row_val_string;
    if (row_data.at(key_name).is_number()) {
        row_val_int = row_data.at(key_name);
    } else {
        row_val_string = row_data.at(key_name);
    }
    std::string check_type = para.at(PARA_TYPE);
    if (!check_type.compare(INPUT_TYPE)) {
        return return_json;
    }
    if (!check_type.compare(PARA_OPTIONS)) {
        // options_list include type and relate_option
        nlohmann::json options_list = para.at(PARA_OPTIONS);
        // option should only have one type
        std::string ctype = TEXT_TYPE;
        if (para.at(PARA_OPTIONS).at(0).is_number()) {
            ctype = INT_TYPE;
        }
        // cycle options
        int is_check = 0;
        for (auto &it : nlohmann::json::iterator_wrapper(options_list)) {
            if (!ctype.compare(INT_TYPE)) {
                int op = it.value();
                if (op == row_val_int) {
                    is_check = 1;
                    break;
                }
            } else {
                std::string op = it.value();
                if (!row_val_string.compare(op)) {
                    is_check = 1;
                    break;
                }
            }
        }
        if (is_check == 0) {
            switch (get_customer_check(table_name, row_data, key_name)) {
                case 1:
                    return_json = options_list.at(0);
                    break;
                case -1:
                    return_json = DELETE_CMD;
                    break;
            }
        }
    } else if (!check_type.compare(PARA_RANGE)) {
        float max_num = attempt_get_float(para.at(PARA_RANGE), MAX_MARK, INT_MAX);
        float min_num = attempt_get_float(para.at(PARA_RANGE), MIN_MARK, INT_MIN);
        if (row_val_int < min_num) {
            switch (get_customer_check(table_name, row_data, key_name)) {
                case 1:
                    return_json = (int)min_num;
                    break;
                case -1:
                    return_json = DELETE_CMD;
                    break;
            }
        } else if (row_val_int > max_num) {
            switch (get_customer_check(table_name, row_data, key_name)) {
                case 1:
                    return_json = (int)max_num;
                    break;
                case -1:
                    return_json = DELETE_CMD;
                    break;
            }
        }
    } else if (!check_type.compare(PARA_DYNAMIC_RANGE)) {
        float max_num = attempt_get_dynamic(para.at(PARA_DYNAMIC_RANGE), row_data, MAX_MARK, MAX_RATE_MARK, INT_MAX);
        float min_num = attempt_get_dynamic(para.at(PARA_DYNAMIC_RANGE), row_data, MIN_MARK, MIN_RATE_MARK, INT_MIN);
        if (row_val_int < min_num) {
            switch (get_customer_check(table_name, row_data, key_name)) {
                case 1:
                    return_json = (int)min_num;
                    break;
                case -1:
                    return_json = DELETE_CMD;
                    break;
            }
        } else if (row_val_int > max_num) {
            switch (get_customer_check(table_name, row_data, key_name)) {
                case 1:
                    return_json = (int)max_num;
                    break;
                case -1:
                    return_json = DELETE_CMD;
                    break;
            }
        }
    } else if (!check_type.compare(PARA_OPTIONS_DYNAMIC)) {
        // options_list include type and relate_option
        nlohmann::json options_list = para.at(PARA_OPTIONS);
        nlohmann::json quality_options;
        // get range
        float max_num = attempt_get_dynamic(para.at(PARA_DYNAMIC_RANGE), row_data, MAX_MARK, MAX_RATE_MARK, INT_MAX);
        float min_num = attempt_get_dynamic(para.at(PARA_DYNAMIC_RANGE), row_data, MIN_MARK, MIN_RATE_MARK, INT_MIN);
        // option should only have one type
        std::string ctype = TEXT_TYPE;
        if (para.at(PARA_OPTIONS).at(0).is_number()) {
            ctype = INT_TYPE;
        }
        // select quality_options
        for (auto &op : nlohmann::json::iterator_wrapper(options_list)) {
            float transfer_num;
            if (!ctype.compare(TEXT_TYPE)) {
                std::string op_val = op.value();
                transfer_num = advance_string2number(op_val);
            } else {
                transfer_num = op.value();
            }
            if (transfer_num >= min_num && transfer_num <= max_num) {
                nlohmann::json one_quality = op.value();
                quality_options.push_back(one_quality);
            }
        }
        int is_check = 0;
        // cycle options to check if in
        for (auto &it : nlohmann::json::iterator_wrapper(quality_options)) {
            if (!ctype.compare(TEXT_TYPE)) {
                std::string op = it.value();
                if (!row_val_string.compare(op)) {
                    is_check = 1;
                    break;
                }
            } else {
                int op = it.value();
                if (row_val_int == op) {
                    is_check = 1;
                    break;
                }
            }
        }
        if (is_check == 0) {
            switch (get_customer_check(table_name, row_data, key_name)) {
                case 1:
                    float row_val_string_num;
                    if (!ctype.compare(TEXT_TYPE)) {
                        row_val_string_num = advance_string2number(row_val_string);
                    } else {
                        row_val_string_num = row_val_int;
                    }
                    if (row_val_string_num > max_num) {
                        int len = quality_options.size();
                        return_json = quality_options.at(len - 1);
                    } else {
                        return_json = quality_options.at(0);
                    }
                        break;
                case -1:
                    return_json = DELETE_CMD;
                    break;
            }
        }
    }
    return return_json;
}

void run_auto_check() {
    auto_check = true;
}


