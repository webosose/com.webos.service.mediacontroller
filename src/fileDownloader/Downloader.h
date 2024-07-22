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

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include <string>
#include <iostream>

class Downloader {
public:
    virtual void download(const std::string& url, const std::string& outputPath) = 0;
    virtual ~Downloader() {}

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    void downloadFile(const std::string& url, const std::string& outputPath);
    std::string determineFinalOutputPath(const std::string& url, const std::string& outputPath);
};

class HttpDownloader : public Downloader {
public:
    void download(const std::string& url, const std::string& outputPath) override;
};

class HttpsDownloader : public Downloader {
public:
    void download(const std::string& url, const std::string& outputPath) override;
};

#endif /*DOWNLOADER_H*/
