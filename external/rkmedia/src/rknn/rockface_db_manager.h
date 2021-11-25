// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mutex>
#include <rockface/rockface.h>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace easymedia {

typedef struct FaceDb {
  rockface_feature_t feature;
  int user_id;
} FaceDb;

class FaceDBManager {
public:
  FaceDBManager(std::string path);
  virtual ~FaceDBManager();

  int AddUser(rockface_feature_t *feature);
  int DeleteUser(int user_id);

  std::vector<FaceDb> GetAllFaceDb(void);
  void ClearDb(void);

#define SQ_BUFFER_LEN (1024)

protected:
  void CreateTable(void);
  int GetRecordCount(void);

  int GetMaxUserId(void);
  int InsertFaceDb(FaceDb *face_db);

private:
  std::string path_;
  std::mutex mutex_;
  sqlite3 *sqlite_;
};

} // namespace easymedia
