#ifndef __DB2JS_H__
#define __DB2JS_H__

#include "commondb.h"

nlohmann::json get_db_tree(nlohmann::json db_struct_json);
void split(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters);
void db2js(std::string db_path, std::string json_path);
std::vector<std::string> get_favorite_table();
int is_in_favorite_table(std::vector<std::string> favorite_table, std::string check_table);
void set_string_mode();

#endif // __DB2JS_H__