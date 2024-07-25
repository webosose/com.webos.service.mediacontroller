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

#include "CacheManager.h"
#include "FileSystem.h"
#include <iostream>
#include "PmLogLib.h"
#include "MediaControlTypes.h"

void CacheManager::addFile(const std::string &uri, const std::string &filePath)
{
    PMLOG_INFO(CONST_MODULE_MCFM, "%s uri : %s, filePath : %s", __FUNCTION__, uri.c_str(), filePath.c_str());

    lruList.push_front(uri);
    cache[uri] = filePath;
    currentSize += FileSystem::getFileSize(filePath);
    evictLRUFiles();
}

std::string CacheManager::getFile(const std::string &uri)
{
    if (cache.find(uri) != cache.end())
    {
        PMLOG_INFO(CONST_MODULE_MCFM, "%s cache[uri]: %s", __FUNCTION__, cache[uri].c_str());
        return cache[uri];
    }
    return "";
}

void CacheManager::updateAccessTime(const std::string &uri)
{
    lruList.remove(uri);
    lruList.push_front(uri);
}

void CacheManager::evictLRUFiles()
{
    PMLOG_INFO(CONST_MODULE_MCFM, "%s currentSize: %lu, MAX_SIZE : %lu", __FUNCTION__, currentSize, MAX_SIZE);

    // Evict files until the total size is within the limit
    while (currentSize > MAX_SIZE)
    {
        std::string lru = lruList.back();
        lruList.pop_back();
        currentSize -= FileSystem::getFileSize(cache[lru]);
        FileSystem::deleteFile(cache[lru]);
        cache.erase(lru);
    }
}

void CacheManager::removeFile(const std::string &uri)
{
    PMLOG_INFO(CONST_MODULE_MCFM, "%s uri : %s", __FUNCTION__, uri.c_str());
    auto it = cache.find(uri);
    if (it != cache.end())
    {
        currentSize -= FileSystem::getFileSize(it->second);
        lruList.remove(uri);
        cache.erase(it);
    }
}
