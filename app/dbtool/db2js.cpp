#include <iostream>
#include <fstream>
#include "db2js.h"
#include "help.h"

bool string_mode = false;

void db2js(std::string db_path, std::string json_path) {
    // open db
    db_init((char *)db_path.c_str());
    // read data
    std::string sql = "SELECT * FROM sqlite_master WHERE type='table'";
    nlohmann::json db_struct_json = db_search_normal(sql);
    nlohmann::json db_json;
    if (!db_struct_json.empty()) {
        db_json = get_db_tree(db_struct_json);
    } else {
        alarm_tip(DB_EMPTY);
        return;
    }
    // sort db name
    std::vector<std::string> db_name_bank;
    if (string_mode) {
        for (auto &x: nlohmann::json::iterator_wrapper(db_json)) {
            std::string one_db_name = x.value().at(TABLE_NAME);
            std::string one_db_search_sql = "SELECT * FROM " + one_db_name;
            x.value().at(ITEMS_CONTENT_NAME) = db_search_normal(one_db_search_sql);
        }
    } else {
        for (auto &x: nlohmann::json::iterator_wrapper(db_json)) {
            std::string one_db_name = x.value().at(TABLE_NAME);
            std::string one_db_search_sql = "SELECT * FROM " + one_db_name;
            if (!one_db_name.compare(SYS_PARA)) {
                x.value().at(ITEMS_CONTENT_NAME) = db_search_sys(one_db_search_sql);
            } else {
                x.value().at(ITEMS_CONTENT_NAME) = db_search_normal(one_db_search_sql);
            }
        }
    }
    // data 2 json
    std::string rst = db_json.dump(4);
    write_string_to_file(json_path, rst);
    // close db
    db_deinit();
}

nlohmann::json get_db_tree(nlohmann::json db_struct_json) {
    nlohmann::json table_tree;
    std::vector<std::string> favorite_table = get_favorite_table();
    for (auto const &x: favorite_table) {
        nlohmann::json table_stand;
        table_stand[TABLE_NAME] = x;
        table_tree.push_back(table_stand);
    }
    for (auto &x: nlohmann::json::iterator_wrapper(db_struct_json)) {
        nlohmann::json one_table_info = x.value();
        // check table name != sqlite_sequence
        std::string table_name = one_table_info.at("tbl_name");
        if (!table_name.compare("sqlite_sequence")) {
            continue;
        }
        // split sql
        std::string create_sql = one_table_info.at("sql");
        int pos_start = create_sql.find_first_of("(");
        int pos_end = create_sql.find_last_of(")");
        int length = pos_end - pos_start - 1;
        std::string cream_item = create_sql.substr(pos_start + 1, length);
        std::vector<std::string> sql_bank;
        split(cream_item, sql_bank, ",");
        nlohmann::json default_json;
        for (auto const &value: sql_bank) {
            nlohmann::json one_default;
            std::string one_sql = value;
            int len = one_sql.size();
            pos_start = one_sql.find_first_of(" ");
            std::string key_name = one_sql.substr(0, pos_start);
            std::string limit_val = one_sql.substr(pos_start + 1, len - 1);
            one_default[DEFAULT_KEY_NAME] = key_name;
            one_default[DEFAULT_KEY_VALUE] = limit_val;

            default_json.push_back(one_default);
        }
        int favorite_order = is_in_favorite_table(favorite_table, table_name);
        if (favorite_order >= 0) {
            table_tree.at(favorite_order)[DEFAULT_CONTENT_NAME] = default_json;
            table_tree.at(favorite_order)[ITEMS_CONTENT_NAME] = NULL;
        } else {
            nlohmann::json one_table_data;
            one_table_data[TABLE_NAME] = table_name;
            one_table_data[DEFAULT_CONTENT_NAME] = default_json;
            one_table_data[ITEMS_CONTENT_NAME] = NULL;
            table_tree.push_back(one_table_data);
        }
    }
    return table_tree;
}

std::vector<std::string> get_favorite_table() {
    std::vector<std::string> favorite_table;
    favorite_table.push_back(SYS_PARA);
    favorite_table.push_back("video");
    return favorite_table;
}

int is_in_favorite_table(std::vector<std::string> favorite_table, std::string check_table) {
    for(int i = 0; i < favorite_table.size(); i++) {
        if (!favorite_table[i].compare(check_table)) {
            return i;
        }
    }
    return -1;
}

void split(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters = " ") {
    std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    std::string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos) {
        tokens.push_back(s.substr(lastPos, pos - lastPos));//use emplace_back after C++11
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

void set_string_mode() {
    string_mode = true;
}
