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
  requestReceiver clients(mediaId, false);
  clientListInfo_.push_front(clients);
}

bool RequestReceiver::removeClient (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_RR, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  if (clientListInfo_.empty()) {
    PMLOG_ERROR(CONST_MODULE_RR, "%s clientListInfo_ is empty", __FUNCTION__);
    return false;
  }

  for (auto itr = clientListInfo_.begin(); itr != clientListInfo_.end(); ) {
    if(itr->mediaId_ == mediaId) {
      itr = clientListInfo_.erase(itr);
      return true;
    } else {
        ++itr;
    }
  }
  return false;
}

std::string RequestReceiver::getLastActiveClient () {
  PMLOG_INFO(CONST_MODULE_RR, "%s ", __FUNCTION__);

  std::string activeMediaId;
  if (clientListInfo_.empty()) {
    PMLOG_ERROR(CONST_MODULE_RR, "%s clientListInfo_ is empty", __FUNCTION__);
    return activeMediaId;
  }

  //todo : get active client based on displayid
  requestReceiver latestClient = clientListInfo_.front();
   if(latestClient.priority_)
     activeMediaId = latestClient.mediaId_;

  return activeMediaId;
}

bool RequestReceiver::setClientPriority (const std::string& mediaId, const int& priority)
{
  PMLOG_INFO(CONST_MODULE_RR, "%s mediaId : %s %d ", __FUNCTION__, mediaId.c_str(), priority);

  if (clientListInfo_.empty()) {
    PMLOG_ERROR(CONST_MODULE_RR, "%s clientListInfo_ is empty", __FUNCTION__);
    return false;
  }

  //check if element is at front
  requestReceiver frontClient = clientListInfo_.front();
  if(frontClient.mediaId_ == mediaId) {
    PMLOG_INFO(CONST_MODULE_RR, "%s : client is already at front", __FUNCTION__);
    frontClient.priority_ = priority;
    return true;
  } else { //else swap the element with front
    std::list<requestReceiver>::iterator front = clientListInfo_.begin();
    std::list<requestReceiver>::iterator itr = clientListInfo_.begin();
    for (; itr != clientListInfo_.end(); itr++) {
      if (itr->mediaId_ == mediaId) {
        itr->priority_ = priority;
        if(priority) {
          std::string currMediaId = itr->mediaId_;
          int currPriority = itr->priority_;
          itr->mediaId_ = front->mediaId_;
          itr->priority_ = front->priority_;
          front->mediaId_ = currMediaId;
          front->priority_ = currPriority;
          PMLOG_INFO(CONST_MODULE_RR, "%s : client set at front", __FUNCTION__);
        }
        break;
      }
    }
    return true;
  }
  return false;
}
