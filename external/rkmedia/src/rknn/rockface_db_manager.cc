// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "rockface_db_manager.h"
#include "utils.h"

namespace easymedia {

FaceDBManager::FaceDBManager(std::string path) {
  int ret = sqlite3_open(path.c_str(), &sqlite_);
  if (ret) {
    RKMEDIA_LOGI("sqlite3_open %s failed.\n", path.c_str());
    return;
  }
  if (sqlite_)
    CreateTable();
}

FaceDBManager::~FaceDBManager() {
  if (sqlite_)
    sqlite3_close(sqlite_);
}

int FaceDBManager::AddUser(rockface_feature_t *feature) {
  std::lock_guard<std::mutex> lock(mutex_);
  FaceDb face_db;
  face_db.user_id = GetMaxUserId() + 1;
  memcpy(&face_db.feature, feature, sizeof(rockface_feature_t));
  int ret = InsertFaceDb(&face_db);
  if (ret < 0) {
    RKMEDIA_LOGI("insert feature failed.\n");
    return -1;
  }
  return face_db.user_id;
}

int FaceDBManager::DeleteUser(int user_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  char sq_buffer[SQ_BUFFER_LEN];
  snprintf(sq_buffer, SQ_BUFFER_LEN, "DELETE from FACE WHERE USER='%d';",
           user_id);

  char *error = nullptr;
  int ret = sqlite3_exec(sqlite_, sq_buffer, nullptr, nullptr, &error);
  if (ret != SQLITE_OK) {
    RKMEDIA_LOGI("sqlite3_exec failed, error = %s\n", error);
    return -1;
  }
  return 0;
}

void FaceDBManager::ClearDb(void) {
  std::lock_guard<std::mutex> lock(mutex_);
  char sq_buffer[SQ_BUFFER_LEN] = "DROP TABLE IF EXISTS FACE;";

  char *error = nullptr;
  int ret = sqlite3_exec(sqlite_, sq_buffer, nullptr, nullptr, &error);
  if (ret != SQLITE_OK) {
    RKMEDIA_LOGI("sqlite3_exec failed, error = %s\n", error);
    return;
  }
  if (sqlite_)
    CreateTable();
}

void FaceDBManager::CreateTable(void) {
  char *error = 0;
  char sq_buffer[SQ_BUFFER_LEN] =
      "CREATE TABLE IF NOT EXISTS FACE ("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
      "USER INT,"
      "FEATURE_VERSION INT,"
      "FEATURE_COUNT INT,"
      "FEATURE BLOB);";
  int ret = sqlite3_exec(sqlite_, sq_buffer, nullptr, 0, &error);
  if (ret != SQLITE_OK) {
    RKMEDIA_LOGI("sqlite3_exec error: %s\n", error);
    return;
  }
}

int FaceDBManager::GetRecordCount(void) {
  sqlite3_stmt *stmt;
  char sq_buffer[SQ_BUFFER_LEN] = "SELECT COUNT(*) FROM FACE;";

  int ret = sqlite3_prepare_v2(sqlite_, sq_buffer, -1, &stmt, nullptr);
  if (ret != SQLITE_OK) {
    RKMEDIA_LOGI("sqlite3_prepare_v2 failed, ret = %d\n", ret);
    return -1;
  }
  ret = sqlite3_step(stmt);
  if (ret != SQLITE_ROW) {
    RKMEDIA_LOGI("sqlite3_step failed. ret = %d\n", ret);
    return -1;
  }
  int count = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return count;
}

int FaceDBManager::GetMaxUserId(void) {
  sqlite3_stmt *stmt;
  char sq_buffer[SQ_BUFFER_LEN] = "SELECT * FROM FACE;";

  int max_user = -1;
  int ret = sqlite3_prepare_v2(sqlite_, sq_buffer, -1, &stmt, nullptr);
  if (ret != SQLITE_OK) {
    RKMEDIA_LOGI("sqlite3_prepare failed. ret = %d\n", ret);
    goto exit;
  }
  ret = sqlite3_step(stmt);
  while (ret == SQLITE_ROW) {
    int user_id = sqlite3_column_int(stmt, 1);
    if (user_id > max_user)
      max_user = user_id;
    ret = sqlite3_step(stmt);
  }
exit:
  sqlite3_finalize(stmt);
  return max_user;
}

std::vector<FaceDb> FaceDBManager::GetAllFaceDb(void) {
  std::lock_guard<std::mutex> lock(mutex_);

  sqlite3_stmt *stmt;
  std::vector<FaceDb> face_db;
  char sq_buffer[SQ_BUFFER_LEN] = "SELECT * FROM FACE;";

  int ret = sqlite3_prepare_v2(sqlite_, sq_buffer, -1, &stmt, nullptr);
  if (ret != SQLITE_OK) {
    RKMEDIA_LOGI("sqlite3_prepare failed. ret = %d\n", ret);
    goto exit;
  }
  ret = sqlite3_step(stmt);
  while (ret == SQLITE_ROW) {
    int user = sqlite3_column_int(stmt, 1);
    int feature_version = sqlite3_column_int(stmt, 2);
    int feature_len = sqlite3_column_int(stmt, 3);
    const void *feature = sqlite3_column_blob(stmt, 4);

    FaceDb face;
    face.user_id = user;
    face.feature.version = feature_version;
    face.feature.len = feature_len;
    memcpy(face.feature.feature, feature, feature_len);
    face_db.push_back(face);
    ret = sqlite3_step(stmt);
  }
exit:
  sqlite3_finalize(stmt);
  return std::move(face_db);
}

int FaceDBManager::InsertFaceDb(FaceDb *face_db) {
  if (!face_db)
    return -1;

  char sq_buffer[SQ_BUFFER_LEN] = {0};
  snprintf(sq_buffer, SQ_BUFFER_LEN,
           "INSERT INTO FACE (USER, FEATURE_VERSION, FEATURE_COUNT, FEATURE) "
           "VALUES (%d, %d, %d, ? );",
           face_db->user_id, face_db->feature.version, face_db->feature.len);

  sqlite3_stmt *stmt;
  int ret = sqlite3_prepare_v2(sqlite_, sq_buffer, -1, &stmt, nullptr);
  if (ret != SQLITE_OK) {
    RKMEDIA_LOGI("sqlite3_prepare_v2 failed. ret = %d\n", ret);
    goto error;
  }
  ret = sqlite3_bind_blob(stmt, 1, face_db->feature.feature,
                          face_db->feature.len, nullptr);
  if (ret != SQLITE_OK) {
    RKMEDIA_LOGI("sqlite3_bind_blob failed, ret = %d\n", ret);
    goto error;
  }
  sqlite3_step(stmt);
error:
  sqlite3_finalize(stmt);
  return ret;
}

} // namespace easymedia
