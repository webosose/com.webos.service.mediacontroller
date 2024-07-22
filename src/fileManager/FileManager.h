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

#include <string>
#include <unordered_map>
#include <list>
#include "CacheManager.h"

class FileManager {
public:
    std::string getURI(const std::string& uri, const std::string& outputPath);
private:
    CacheManager cacheManager;
    bool validateURI(const std::string& uri);
    bool urlExists(const std::string& uri);
};
