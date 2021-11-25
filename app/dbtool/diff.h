#ifndef __DIFF_H__
#define __DIFF_H__

#include "commondb.h"
void get_diff(std::string raw_json_path, std::string compare_json_path);
void patch_diff(std::string raw_json_path, std::string diff_path, std::string get_file_name);
nlohmann::json array2dict(nlohmann::json raw_data);
nlohmann::json dict2array(nlohmann::json raw_data);

#endif