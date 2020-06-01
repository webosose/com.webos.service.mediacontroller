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

void MediaSessionManager::addMediaSession (const std::string& mediaId,
                                           const std::string& appId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s , appId : %s", __FUNCTION__, mediaId.c_str(), appId.c_str());
  int displayId = 0;
  //create mediaSession object : todo : get displayId from ums
  mediaSession objMediaSession(mediaId, appId, false);
  mapMediaSessionInfo_[mediaId] = objMediaSession;
  //add mediaId to receiver stack
  objRequestRcvr_[displayId].addClient(mediaId);
}

bool MediaSessionManager::activateMediaSession (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      objRequestRcvr_[itr.second.getDisplayId()].setClientPriority(mediaId, SET);
      return true;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return false;
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
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
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
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return false;
}

bool MediaSessionManager::getMediaMetaData(const std::string& mediaId,
                                           mediaMetaData& objMetaData) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      objMetaData = itr.second.getMediaMetaDataObj();
      return true;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return false;
}

bool MediaSessionManager::getMediaSessionInfo(const std::string& mediaId,
                                              mediaSession& objMediaSession) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      objMediaSession.setMediaId(itr.first);
      objMediaSession.setAppId(itr.second.getAppId());
      objMediaSession.setPlayStatus(itr.second.getPlayStatus());
      objMediaSession.setDisplayId(itr.second.getDisplayId());
      objMediaSession.setMetaData(itr.second.getMediaMetaDataObj());
      return true;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return false;
}

bool MediaSessionManager::getMediaPlayStatus(const std::string& mediaId,
                                             std::string& playStatus) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      playStatus = itr.second.getPlayStatus();
      return true;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return false;
}

bool MediaSessionManager::setMediaMetaData(const std::string& mediaId,
                                           const mediaMetaData& objMetaData) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for(auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      //save metadata info
      itr.second.setMetaData(objMetaData);
      return true;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return false;
}

bool MediaSessionManager::setMediaPlayStatus(const std::string& mediaId,
                                             const std::string& playStatus) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s playStatus", __FUNCTION__,
                                mediaId.c_str(), playStatus.c_str());

  for(auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      itr.second.setPlayStatus(playStatus);
      return true;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return false;
}

std::vector<std::string> MediaSessionManager::getActiveMediaSessions() {
  PMLOG_INFO(CONST_MODULE_MSM, "%s", __FUNCTION__);
  std::vector<std::string> activeMediaId;

  for(int displayId = 0; displayId < 2; displayId++) {
    for (const auto& itr : objRequestRcvr_[displayId].getClientList()) {
       PMLOG_INFO(CONST_MODULE_MSM, "%s", __FUNCTION__);
       if(itr.priority_ == SET)
         activeMediaId.push_back(itr.mediaId_);
    }
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

std::string MediaSessionManager::getActiveSessionbyDisplayId (const int& displayId) {
  return objRequestRcvr_[displayId].getLastActiveClient();
}
