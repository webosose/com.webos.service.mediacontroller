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
      objRequestRcvr_[itr.second.getDisplayId()].setClientPriority(mediaId, SET);
      return true;
    }
  }
  return false;
}

void MediaSessionManager::addMediaSession (const std::string& mediaId,
                                           const std::string& appId,
                                           const mediaMetaData& objMetaData) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s , appId : %s", __FUNCTION__, mediaId.c_str(), appId.c_str());
  int displayId;
  //create mediaSession object : todo : get displayId from ums
  mediaSession objMediaSession(mediaId, appId, false);
  //save metadata info
  objMediaSession.setMetaData(objMetaData);
  mapMediaSessionInfo_[mediaId] = objMediaSession;
  //add mediaId to receiver stack
  objRequestRcvr_[displayId].addClient(mediaId);
}

bool MediaSessionManager::deactivateMediaSession (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      //reset priority of media client in receiver stack
      objRequestRcvr_[itr.second.getDisplayId()].setClientPriority(mediaId, RESET);
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
      objRequestRcvr_[itr.second.getDisplayId()].removeClient(mediaId);
      return true;
    }
  }
  return false;
}

mediaMetaData MediaSessionManager::getMediaMetaData(const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId);

  mediaMetaData objMetaData;
  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      objMetaData = itr.second.getMediaMetaDataObj();
      break;
    }
  }
  return objMetaData;
}

std::string MediaSessionManager::getMediaPlayStatus(const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId);

  std::string playStatus;
  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      playStatus = itr.second.getPlayStatus();
      break;
    }
  }
  return playStatus;
}

std::string MediaSessionManager::getActiveSessionbyDisplayId (const int& displayId) {
  return objRequestRcvr_[displayId].getLastActiveClient();
}

std::vector<std::string> MediaSessionManager::getActiveMediaSessions() {
  std::vector<std::string> activeMediaId;
  for (auto itr = objRequestRcvr_[0].getClientList().begin(); itr != objRequestRcvr_[0].getClientList().end(); itr++ ) {
       if(itr->priority_ == SET)
         activeMediaId.push_back(itr->mediaId_);
  }

  for (auto itr = objRequestRcvr_[1].getClientList().begin(); itr != objRequestRcvr_[1].getClientList().end(); itr++) {
       if(itr->priority_ == SET)
         activeMediaId.push_back(itr->mediaId_);
  }

  return activeMediaId;
}

std::vector<std::string> MediaSessionManager::getMediaSessionId(const std::string& appId) {
  std::vector<std::string> mediaSessionId;
  for (const auto& itr : mapMediaSessionInfo_) {
    if(appId == itr.second.getAppId()) {
      mediaSessionId.push_back(itr.first);
    }
  }
  return mediaSessionId;
}

mediaSession MediaSessionManager::getMediaSessionInfo(const std::string& mediaId) {
  mediaSession objMediaSession;
  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      objMediaSession.setMediaId(itr.first);
      objMediaSession.setAppId(itr.second.getAppId());
      objMediaSession.setPlayStatus(itr.second.getPlayStatus());
      objMediaSession.setDisplayId(itr.second.getDisplayId());
      objMediaSession.setVolume(itr.second.getVolume());
      objMediaSession.setMetaData(itr.second.getMediaMetaDataObj());
    }
  }
  return objMediaSession;
}