// Copyright (c) 2020 LG Electronics, Inc.
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

#ifndef REQUEST_RECEIVER_H_
#define REQUEST_RECEIVER_H_

/*-----------------------------------------------------------------------------
    (File Inclusions)
------------------------------------------------------------------------------*/
#include <string>
#include <map>
#include <iostream>
#include "MediaControlTypes.h"
#include <list>
#include <iostream>
#include <string>

class RequestReceiver
{
private:
  std::list<requestReciever> clientListInfo_;

public:
  RequestReceiver();
  void addClient(const std::string& mediaId);
  bool removeClient(const std::string& mediaId);
  std::string getLatestClient();
  bool setPriorityClient(const std::string& mediaId, const bool priorityFlag);
};

#endif /*REQUEST_RECEIVER_H_*/
