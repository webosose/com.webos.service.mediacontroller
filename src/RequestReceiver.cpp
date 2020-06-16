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

/*-----------------------------------------------------------------------------*/
#include "RequestReceiver.h"

RequestReceiver::RequestReceiver() : clientListInfo_()
{
}

void RequestReceiver::addClient (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_RR, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  //check if element is already in list and at front
  if(mediaId == clientListInfo_.front())
    return;
  //find the mediaId and move to top
  for(auto itr = clientListInfo_.begin(); itr != clientListInfo_.end();) {
    if(*itr == mediaId) {
      itr = clientListInfo_.erase(itr);
      clientListInfo_.push_front(mediaId);
      return;
    }
    else
      ++itr;
  }
  //no element in list, add new to front
  clientListInfo_.push_front(mediaId);
  return;
}

void RequestReceiver::removeClient (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_RR, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  if (clientListInfo_.empty()) {
    PMLOG_ERROR(CONST_MODULE_RR, "%s clientListInfo_ is empty", __FUNCTION__);
    return;
  }

  for (auto itr = clientListInfo_.begin(); itr != clientListInfo_.end(); ) {
    if(*itr == mediaId) {
      itr = clientListInfo_.erase(itr);
      return;
    } else {
        ++itr;
    }
  }
  return;
}

std::string RequestReceiver::getLastActiveClient () {
  PMLOG_INFO(CONST_MODULE_RR, "%s ", __FUNCTION__);

  if (clientListInfo_.empty()) {
    PMLOG_ERROR(CONST_MODULE_RR, "%s clientListInfo_ is empty", __FUNCTION__);
    return CSTR_EMPTY;
  }

  return clientListInfo_.front();
}
