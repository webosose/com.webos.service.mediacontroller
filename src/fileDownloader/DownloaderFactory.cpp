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

/*-----------------------------------------------------------------------------*/
#include "DownloaderFactory.h"
#include <memory>
#include <stdexcept>
#include "Downloader.h"
#include "PmLogLib.h"
#include "MediaControlTypes.h"

std::unique_ptr<Downloader> DownloaderFactory::createDownloader(const std::string& uri) {
    PMLOG_INFO(CONST_MODULE_MCD, "%s IN", __FUNCTION__);
    std::string scheme = parseScheme(uri);
    if (scheme == "http") {
        return std::unique_ptr<Downloader>(new HttpDownloader());
    } else if (scheme == "https") {
        return std::unique_ptr<Downloader>(new HttpsDownloader());
    }
    // Add more schemes as needed
    else {
        throw std::invalid_argument("Unsupported scheme: " + scheme);
    }
}

std::string DownloaderFactory::parseScheme(const std::string& uri) {
    PMLOG_INFO(CONST_MODULE_MCD, "%s IN", __FUNCTION__);
    size_t pos = uri.find("://");
    if (pos == std::string::npos) {
        throw std::invalid_argument("Invalid URI: " + uri);
    }
    return uri.substr(0, pos);
}
