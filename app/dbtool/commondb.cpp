#include "commondb.h"
#include "help.h"
#include <iostream>
#include <fstream>

static sqlite3 *db;

nlohmann::json db_search_sys(std::string sql) {
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, (char *)sql.c_str(), -1, &stmt, NULL);
    // read data
    int step_rst = sqlite3_step(stmt);
    // read name
    int col_num = 0;
    int sql_type = sqlite3_column_type(stmt, col_num);
    std::vector<std::string> type_js;
    std::vector<std::string> col_name;
    while (sql_type != SQLITE_NULL) {
        std::string one_col_name = sqlite3_column_name(stmt, col_num);
        col_name.push_back(one_col_name);
        if (sql_type == SQLITE_INTEGER) {
            type_js.push_back(INT_TYPE);
        } else if (sql_type == SQLITE3_TEXT) {
            type_js.push_back(TEXT_TYPE);
        }
        ++col_num;
        sql_type = sqlite3_column_type(stmt, col_num);
    }
    // read items
    nlohmann::json items_json;
    while (step_rst == SQLITE_ROW) {
        nlohmann::json item_json;
        for (int i = 0; i < col_num; i++) {
            std::string read_type = type_js.at(i);
            std::string read_col_name = col_name.at(i);
            if (!read_col_name.compare(SYS_PARA_KEY)) {
                char *str = (char *)sqlite3_column_text(stmt, i);
                nlohmann::json para_json = nlohmann::json::parse(str);
                item_json[read_col_name] = para_json;
            } else {
                if (!read_type.compare(INT_TYPE)) {
                    item_json[read_col_name] = sqlite3_column_int(stmt, i);
                } else if (!read_type.compare(TEXT_TYPE)) {
                    item_json[read_col_name] = (char *)sqlite3_column_text(stmt, i);
                }
            }
        }
        items_json.push_back(item_json);
        step_rst = sqlite3_step(stmt);
    }
    // end db search
    sqlite3_finalize(stmt);
    // integration
    // nlohmann::json table_json;
    // table_json[DEFAULT_CONTENT_NAME] = default_json;
    // table_json[ITEMS_CONTENT_NAME] = items_json;
    return items_json;
}

nlohmann::json db_search_normal(std::string sql) {
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, (char *)sql.c_str(), -1, &stmt, NULL);
    // read data
    int step_rst = sqlite3_step(stmt);
    // read name
    int col_num = 0;
    int sql_type = sqlite3_column_type(stmt, col_num);
    std::vector<std::string> type_js;
    std::vector<std::string> col_name;
    while (sql_type != SQLITE_NULL) {
        std::string one_col_name = sqlite3_column_name(stmt, col_num);
        col_name.push_back(one_col_name);
        if (sql_type == SQLITE_INTEGER) {
            type_js.push_back(INT_TYPE);
        } else if (sql_type == SQLITE3_TEXT) {
            type_js.push_back(TEXT_TYPE);
        }
        ++col_num;
        sql_type = sqlite3_column_type(stmt, col_num);
    }
    // read items
    nlohmann::json items_json;
    while (step_rst == SQLITE_ROW) {
        nlohmann::json item_json;
        for (int i = 0; i < col_num; i++) {
            std::string read_type = type_js.at(i);
            std::string read_col_name = col_name.at(i);
            if (!read_type.compare(INT_TYPE)) {
                item_json[read_col_name] = sqlite3_column_int(stmt, i);
            } else if (!read_type.compare(TEXT_TYPE)) {
                item_json[read_col_name] = (char *)sqlite3_column_text(stmt, i);
            }
        }
        items_json.push_back(item_json);
        step_rst = sqlite3_step(stmt);
    }
    // end db search
    sqlite3_finalize(stmt);
    // integration
    nlohmann::json table_json;
    return items_json;
}

int db_sql(char *sql)
{
    // char *zErrMsg = 0;
    sqlite3_stmt *stmt = NULL;
    // sqlite3_exec(db, (char *)sql.c_str(), &zErrMsg);
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    int ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return ret;
}

int db_init(char *file)
{
    int rc;
    char *zErrMsg = 0;
    rc = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, NULL);
    // rc = sqlite3_open(file, &db);
    assert(!rc);
    return 0;
}

void db_deinit(void)
{
    if (db)
        sqlite3_close_v2(db);
    db = NULL;
}

float attempt_get_float(nlohmann::json attempt_json, std::string attempt_key, float default_return) {
    nlohmann::json fake_json = attempt_json;
    if (fake_json[attempt_key].empty()) {
        return default_return;
    } else {
        float rst = fake_json[attempt_key];
        return rst;
    }
}

// if attempt get is number, return it. only when attempt get is string , attempt_key is used.
float attempt_get_dynamic(nlohmann::json attempt_json, nlohmann::json row_data, std::string attempt_key, std::string attempt_rate, float default_return) {
    nlohmann::json fake_para = attempt_json;
    nlohmann::json fake_data = row_data;
    if (fake_para[attempt_key].empty()) {
        return default_return;
    }
    std::string row_key;
    float raw_num;
    if (fake_para.at(attempt_key).is_number()) {
        raw_num = fake_para.at(attempt_key);
        return raw_num;
    } else {
        row_key = fake_para.at(attempt_key);
        if (fake_data[row_key].empty()) {
            std::string err = "GetDynamicError: " + row_key + " is not exits!";
            throw err;
        } else {
            if (fake_data.at(row_key).is_number()) {
                raw_num = fake_data.at(row_key);
            } else {
                std::string raw_num_string = fake_data.at(row_key);
                raw_num = advance_string2number(raw_num_string);
                if (raw_num < 0) {
                    std::string err = "GetDynamicError: " + row_key + " is not number!";
                    throw err;
                }
            }
        }
    }
    // rate
    float num_rate;
    if (fake_para[attempt_rate].empty()) {
        num_rate = 1.0;
    } else {
        num_rate = fake_para.at(attempt_rate);
    }
    return raw_num * num_rate;
}

float advance_string2number(std::string raw_data) {
    float rst = -1.0;
    if (raw_data.find("/") != std::string::npos) {
        if (raw_data.find_first_of("/") == raw_data.find_last_of("/")) {
            int slice_num = raw_data.find_first_of("/");
            int len = raw_data.size();
            std::string pat0 = raw_data.substr(0, slice_num - 1);
            std::string pat1 = raw_data.substr(slice_num, len - 1);
            int up_num = atoi(pat0.c_str());
            int below_num = atoi(pat1.c_str());
            if ((below_num != 0) && ((up_num != 0) || !pat0.compare("0"))) {
                rst = (float)up_num / below_num;
            }
        }
    } else {
        int get_num = atoi(raw_data.c_str());
        if ((get_num != 0) || !raw_data.compare("0")) {
            rst = float(get_num);
        }
    }
    return rst;
}

char* readFile(const char* fileName) {
    std::ifstream file(fileName);
    char* content;
    int temp = file.tellg();
    file.seekg(0, std::ios_base::end);
    int file_length = file.tellg();
    if (file_length == 0) {
        return NULL;
    }
    file.seekg(temp);
    content = (char*)malloc(sizeof(char) * (file_length + 1));
    if (content) {
        char buff[1024];
        int pt = 0;
        while (file.getline(buff, 1024)) {
            for (int i = 0; i < 1024; i++) {
                char tmp = buff[i];
                if (tmp == '\0') {
                    content[pt] = '\n';
                    pt++;
                    break;
                }
                content[pt] = tmp;
                pt++;
            }
        }
        content[pt] = '\0';
        char* result = (char*)malloc(sizeof(char) * (++pt));
        if (!result) {
            free(content);
            return NULL;
        }
        for (int i = 0; i < pt; i++) {
            result[i] = content[i];
        }
        free(content);
        return result;
    }
    return NULL;
}

int is_file_exitst(const char* fileName) {
    std::ifstream file(fileName);
    if (file) {
        return 1;
    } else {
        return 0;
    }
}

void write_string_to_file(std::string &file_address, std::string content) {
    std::ofstream OsWrite(file_address);
    OsWrite << content;
    OsWrite << std::endl;
    OsWrite.close();
}
