#ifndef __COMMONDB_H__
#define __COMMONDB_H__

#include <assert.h>
#include "thirdparty/nlohmann/json.hpp"
extern "C" {
    #include "thirdparty/sqlite3/sqlite3.h"
}

nlohmann::json db_search_normal(std::string sql);
nlohmann::json db_search_sys(std::string sql);

int db_sql(char *sql);
int db_init(char *file);
void db_deinit(void);

float attempt_get_float(nlohmann::json attempt_json, std::string attempt_key, float default_return);
float attempt_get_dynamic(nlohmann::json attempt_json, nlohmann::json row_data, std::string attempt_key, std::string attempt_rate, float default_return);
float advance_string2number(std::string raw_data);

char* readFile(const char* fileName);
int is_file_exitst(const char* fileName);
void write_string_to_file(std::string &file_address, std::string content);

#endif
