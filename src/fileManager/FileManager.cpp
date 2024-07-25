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

#include "FileManager.h"
#include "DownloaderFactory.h"
#include "FileSystem.h"
#include <stdexcept>
#include <unistd.h>
#include "Utils.h"
#include "PmLogLib.h"
#include "MediaControlTypes.h"

std::string FileManager::getURI(const std::string &uri, const std::string &outputPath)
{
    if (cacheManager.getFile(uri) != "")
    {
        std::string filePath = cacheManager.getFile(uri);
        if (FileSystem::fileExists(filePath))
        {
            PMLOG_INFO(CONST_MODULE_MCFM, "fileExists: YES filePath: %s", filePath.c_str());
            cacheManager.updateAccessTime(uri);
            return filePath;
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_MCFM, "fileExists: NO");
            cacheManager.removeFile(uri);
        }
    }

    validateURI(uri);
    if (!urlExists(uri))
    {
        throw std::runtime_error("URL not found");
    }
    std::string targetPath = "";
    short int attempt = 1;
    while (attempt <= MAX_TRY) {
      try {
        std::unique_ptr<Downloader> downloader = DownloaderFactory::createDownloader(uri);
        downloader->download(uri, outputPath);
        targetPath = COVERART_FILE_PATH + extractFilenameFromUrl(uri);
        PMLOG_INFO(CONST_MODULE_MCFM, "Downloaded successfully done : %s", targetPath.c_str());
        break;
      } catch (...) {
        if (attempt == MAX_TRY)
        {
            throw std::runtime_error("Download failed");
        }
        else
            PMLOG_INFO(CONST_MODULE_MCFM, "Downloaded error trying again...");
        attempt++;
        sleep(2);
      }
    }
    if (FileSystem::fileExists(targetPath))
    {
        cacheManager.addFile(uri, targetPath);
        cacheManager.updateAccessTime(uri);
        return targetPath;
    }
    else
    {
        throw std::runtime_error("Download failed");
    }
}

bool FileManager::validateURI(const std::string &uri)
{
    // Implementation of URI validation
    // TODO
    return true;
}

bool FileManager::urlExists(const std::string &uri)
{
    // Implementation to check if URL exists
    // TODO
    return true;
}
