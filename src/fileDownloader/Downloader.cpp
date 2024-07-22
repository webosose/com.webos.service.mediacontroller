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
#include "Downloader.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <curl/curl.h>
#include <sys/stat.h>
#include "PmLogLib.h"
#include "MediaControlTypes.h"
#include "Utils.h"

void print_tls_version()
{
  curl_version_info_data *version_info = curl_version_info(CURLVERSION_NOW);
  if (version_info->ssl_version) {
    PMLOG_INFO(CONST_MODULE_MCD, "Current SSL/TLS version: %s", version_info->ssl_version);;
  } else
  {
     PMLOG_INFO(CONST_MODULE_MCD, "No SSL/TLS library detected.");
  }
}

size_t Downloader::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    size_t totalSize = size * nmemb;
    file->write(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void Downloader::downloadFile(const std::string& url, const std::string& outputPath) {
    std::string finalOutputPath = determineFinalOutputPath(url, outputPath);

    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        std::ofstream file(finalOutputPath, std::ios::binary);
        if (!file) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("Could not open file for writing: " + finalOutputPath);
        }

        if ((CURLE_OK != curl_easy_setopt(curl, CURLOPT_URL, url.c_str()))
            || (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback))
            || (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file))
            || (CURLE_OK != curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))) {
            curl_easy_cleanup(curl);
            file.close();
            throw std::runtime_error("curl_easy_setopt() failed!!");
        }

        print_tls_version();
        // Enable strict SSL/TLS verification
        if ((CURLE_OK != curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L))
            || (CURLE_OK != curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L))) {
            curl_easy_cleanup(curl);
            file.close();
            throw std::runtime_error("curl_easy_setopt() failed!!");
        }

        // Specify the minimum SSL/TLS version (TLS 1.2)
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2)) {
            curl_easy_cleanup(curl);
            file.close();
            throw std::runtime_error("curl_easy_setopt() failed!!");
        }

        print_tls_version();

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            file.close();
            throw std::runtime_error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
        }
        curl_easy_cleanup(curl);
        file.close();
        PMLOG_INFO(CONST_MODULE_MCD, "%s %s download sucess!!", __FUNCTION__, finalOutputPath.c_str());
    } else {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

std::string Downloader::determineFinalOutputPath(const std::string& url, const std::string& outputPath) {
    struct stat info;
    if (stat(outputPath.c_str(), &info) != 0) {
        // Path does not exist or is not accessible
        if (outputPath.back() == '/' || outputPath.back() == '\\') {
            // If outputPath ends with a slash, it's intended to be a directory
            throw std::runtime_error("Directory does not exist: " + outputPath);
        }
        // Otherwise, it's a file path
        return outputPath;
    } else if (info.st_mode & S_IFDIR) {
        // If outputPath is a directory
        std::string filename = extractFilenameFromUrl(url);
        return outputPath + "/" + filename;
    } else {
        // It's a file path
        return outputPath;
    }
}

void HttpDownloader::download(const std::string& url, const std::string& outputPath) {
    downloadFile(url, outputPath);
}

void HttpsDownloader::download(const std::string& url, const std::string& outputPath) {
    downloadFile(url, outputPath);
}

