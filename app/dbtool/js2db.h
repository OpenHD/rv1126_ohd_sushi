#ifndef __JS2DB_H__
#define __JS2DB_H__

#include "commondb.h"

void js2db(std::string db_path, std::string json_path);
void first_cycle(nlohmann::json js);
void cycle_register(nlohmann::json js);
void register_sys_para(nlohmann::json js);
bool is_upper(char ch);
std::string get_create_sql(nlohmann::json js_part);
int exec_insert_sql(std::string table_name, nlohmann::json js_part);
int exec_insert_sys_para(std::string table_name, nlohmann::json js_part);
void register_func_normal(std::string table_name, nlohmann::json table_data);
int check_json(nlohmann::json &json_file);
nlohmann::json check_json_func(nlohmann::json para_json, nlohmann::json &data_json, nlohmann::json para_bank, std::string table_name);
int get_customer_check(std::string table_name, nlohmann::json row_data, std::string col);
nlohmann::json get_refer_item(nlohmann::json refer_list, int refer_order, nlohmann::json refer_json);
nlohmann::json check4type(nlohmann::json para, nlohmann::json row_data, std::string key_name, std::string table_name);
void run_auto_check();

#endif // __JS2DB_H__