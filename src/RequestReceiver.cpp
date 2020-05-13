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

RequestReceiver::RequestReceiver() : clientListInfo_() {

}

void RequestReceiver::addClient (const std::string &mediaId) 
{
  PMLOG_INFO(CONST_MODULE_RR, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  requestReciever clients(mediaId, false);
  clientListInfo_.push_front(clients);
}

bool RequestReceiver::removeClient (const std::string &mediaId)
{
  PMLOG_INFO(CONST_MODULE_RR, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  if (clientListInfo_.empty()) {
    PMLOG_INFO(CONST_MODULE_RR, "%s clientListInfo_ is empty", __FUNCTION__);
    return false;
  }

  for (auto itr = clientListInfo_.begin(); itr != clientListInfo_.end(); ) {
    if(itr->mediaId_ == mediaId) {
      itr=clientListInfo_.erase(itr);
      return true;
    } else {
        ++itr;
    }
  }
  return false;
}

std::string RequestReceiver::getLatestClient ()
{
  PMLOG_INFO(CONST_MODULE_RR, "%s ", __FUNCTION__);
  requestReciever latestClient;
  std::string activeMediaId;

  if (clientListInfo_.empty()) {
    PMLOG_INFO(CONST_MODULE_RR, "%s clientListInfo_ is empty", __FUNCTION__);
    return "";
  }

  latestClient = clientListInfo_.front();
  if(latestClient.setPriority_ == true)
    activeMediaId = latestClient.mediaId_;
  return activeMediaId;
}

bool RequestReceiver::setPriorityClient (const std::string& mediaId, bool priorityFlag)
{ 
  PMLOG_INFO(CONST_MODULE_RR, "%s mediaId : %s %d ", __FUNCTION__, mediaId.c_str(), priorityFlag);
  std::string currMediaId;
  bool currPriorityVal;
  requestReciever frontClient;

  if (clientListInfo_.empty()) {
    PMLOG_INFO(CONST_MODULE_RR, "%s clientListInfo_ is empty", __FUNCTION__);
    return false;
  }

  //set PriorityFlag for recieved mediaId
  for(auto& itr : clientListInfo_) {
    if(itr.mediaId_ == mediaId)
      itr.setPriority_ = priorityFlag;
  }

  //check whether recieved element  is in the front of list and if not set it to front only when priority
  //flag set true
  if(priorityFlag) {
  frontClient = clientListInfo_.front();
  if(frontClient.mediaId_ == mediaId) {
    PMLOG_INFO(CONST_MODULE_RR, "%s : client is already at front", __FUNCTION__);
    return true;
  } else {
      std::list<requestReciever>::iterator current = clientListInfo_.begin();
        for (; current != clientListInfo_.end(); current++) {
          if (current->mediaId_ == mediaId) {
            currMediaId = current->mediaId_;
            currPriorityVal = current->setPriority_;
            break;
          }
        }
      current->mediaId_=frontClient.mediaId_;
      current->setPriority_=frontClient.setPriority_;

      std::list<requestReciever>::iterator update = clientListInfo_.begin();
      update->mediaId_=currMediaId;
      update->setPriority_=currPriorityVal;
      PMLOG_INFO(CONST_MODULE_RR, "%s : client set at front", __FUNCTION__);
      return true;
    } 
  }
  return false;
}