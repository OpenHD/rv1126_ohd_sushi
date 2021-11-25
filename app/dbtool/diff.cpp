#include <iostream>
#include <fstream>
#include "diff.h"
#include "help.h"
#include "commondb.h"

void get_diff(std::string raw_json_path, std::string compare_json_path) {
    char* raw_data = readFile((char *)raw_json_path.c_str());
    char* com_data = readFile((char *)compare_json_path.c_str());
    nlohmann::json raw_json = nlohmann::json::parse(raw_data);
    nlohmann::json raw_dict = array2dict(raw_json);
    nlohmann::json com_json = nlohmann::json::parse(com_data);
    nlohmann::json com_dict = array2dict(com_json);
    nlohmann::json diff_json = nlohmann::json::diff(raw_dict, com_dict);
    std::string diff_data = diff_json.dump(4);
    std::string diff_path = compare_json_path + ".diff";
    write_string_to_file(diff_path, diff_data);
}

void patch_diff(std::string raw_json_path, std::string diff_path, std::string get_file_name) {
    char* raw_data = readFile((char *)raw_json_path.c_str());
    char* diff_data = readFile((char *)diff_path.c_str());
    nlohmann::json raw_json = nlohmann::json::parse(raw_data);
    nlohmann::json raw_dict = array2dict(raw_json);
    nlohmann::json diff_json = nlohmann::json::parse(diff_data);
    nlohmann::json patch_json = raw_dict.patch(diff_json);
    nlohmann::json patch_array = dict2array(patch_json);
    std::string patch_data = patch_array.dump(4);
    std::string patch_path;
    if (get_file_name.empty()) {
        patch_path = raw_json_path + ".patch";
    } else {
        patch_path = get_file_name;
    }
    write_string_to_file(patch_path, patch_data);
}

nlohmann::json array2dict(nlohmann::json raw_data) {
    nlohmann::json dict_json;
    for (auto &x: nlohmann::json::iterator_wrapper(raw_data)) {
        std::string tb_name = x.value().at(TABLE_NAME);
        dict_json[tb_name] = x.value();
    }
    return dict_json;
}

nlohmann::json dict2array(nlohmann::json raw_data) {
    nlohmann::json array_json;
    array_json.push_back(0);
    for (auto &x: nlohmann::json::iterator_wrapper(raw_data)) {
        std::string tb_name = x.key();
        if (!tb_name.compare(SYS_PARA)) {
            array_json.at(0) = x.value();
        } else {
            array_json.push_back(x.value());
        }
    }
    return array_json;
}
