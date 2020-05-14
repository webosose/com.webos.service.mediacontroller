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
#include "MediaSessionManager.h"
#include "MediaControlTypes.h"

MediaSessionManager::MediaSessionManager() :
  mapMediaSessionInfo_() {
}

MediaSessionManager& MediaSessionManager::getInstance() {
  static MediaSessionManager objMediaSessionMgr;
  return objMediaSessionMgr;
}

bool MediaSessionManager::activateMediaSession (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      objRequestRcvr_.setClientPriority(mediaId, SET);
      return true;
    }
  }
  return false;
}

void MediaSessionManager::addMediaSession (const std::string& mediaId, const std::string& appId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s , appId : %s", __FUNCTION__, mediaId.c_str(), appId.c_str());
  //create mediaSession object : todo : get displayId from ums
  mediaSession objMediaSession(mediaId, appId, false);
  mapMediaSessionInfo_[mediaId] = objMediaSession;
  //add mediaId to receiver stack
  objRequestRcvr_.addClient(mediaId);
}

bool MediaSessionManager::deactivateMediaSession (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      //reset priority of media client in receiver stack
      objRequestRcvr_.setClientPriority(mediaId, RESET);
      return true;
    }
  }
  return false;
}

bool MediaSessionManager::removeMediaSession (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      mapMediaSessionInfo_.erase(itr.first);
      //delete media client from receiver stack
      objRequestRcvr_.removeClient(mediaId);
      return true;
    }
  }
  return false;
}

std::string MediaSessionManager::getActiveSessionbyDisplayId (const int& displayId) {
  return objRequestRcvr_.getLastActiveClient();
}
