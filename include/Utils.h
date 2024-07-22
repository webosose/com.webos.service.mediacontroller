// Copyright (c) 2024 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cstring>
#include <dirent.h>

const short int MAX_TRY = 3;
const std::string COVERART_FILE_PATH = "/media/internal/.media-session/";

static bool directoryExists(const std::string& path) {

  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    return false; // Directory does not exist
  }
  else if (info.st_mode & S_IFDIR) {
    return true; // Directory exists
  }

  return false; // Not a directory
}

static bool deleteAllFilesInDirectory(const std::string& path) {
  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) {
    return false;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string filePath = path + "/" + entry->d_name;

    // Skip the "." and ".." entries
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Check if it's a file
    struct stat info;
    if (stat(filePath.c_str(), &info) == 0 && S_ISREG(info.st_mode)) {
      if (remove(filePath.c_str()) != 0) {
        closedir(dir);
        return false;
      }
    }
  }

  closedir(dir);
  return true;
}

static int computeHashKey(const std::string& str) {
    std::hash<std::string> hasher;
    size_t hashValue = hasher(str); // Compute hash value

    // Convert hash value to a 3-digit number
    int hashKey = hashValue % 1000; // Ensure it's within 0 to 999

    return hashKey;
}

static std::string extractFilenameFromUrl(const std::string& url) {
    int hashValue = computeHashKey(url);
    // Find the position of the query string
    size_t query_pos = url.find('?');
    std::string clean_url = url.substr(0, query_pos);

    size_t pos = clean_url.find_last_of("/\\");
    std::string fileName = "";
    if (pos != std::string::npos) {
        fileName = clean_url.substr(pos + 1);

        size_t dot_pos = fileName.find_last_of('.');
        std::string newFileName = "";
        if (dot_pos != std::string::npos) {
            std::string name = fileName.substr(0,dot_pos);
            std::string fileType = fileName.substr(dot_pos);
            //create new file name with hashValue
            newFileName = name + "_" + std::to_string(hashValue) + fileType;
            return newFileName;
        }
        return fileName;
    }
    // Fallback to a default name if no '/' is found
    return "";
}
