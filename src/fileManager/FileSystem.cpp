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
//

#include "FileSystem.h"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include "PmLogLib.h"
#include "MediaControlTypes.h"

bool FileSystem::directoryExists(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool FileSystem::fileExists(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

size_t FileSystem::getFileSize(const std::string &path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0)
    {
        PMLOG_INFO(CONST_MODULE_MCFM, "file : %s, size : %d", path, buffer.st_size);
        return buffer.st_size;
    }
    PMLOG_INFO(CONST_MODULE_MCFM, "file : %s, size : %d", path, buffer.st_size);
    return 0;
}

void FileSystem::deleteFile(const std::string &path)
{
    PMLOG_INFO(CONST_MODULE_MCFM, "Deleting %s", path);
    if (!fileExists(path))
    {
        return;
    }
    if (remove(path.c_str()) != 0) {
       PMLOG_INFO(CONST_MODULE_MCFM, "remove failed : %s", path);
       return;
    }
}
