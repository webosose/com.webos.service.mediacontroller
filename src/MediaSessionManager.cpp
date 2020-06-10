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

int MediaSessionManager::addMediaSession (const std::string& mediaId,
                                           const std::string& appId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s , appId : %s", __FUNCTION__, mediaId.c_str(), appId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      PMLOG_ERROR(CONST_MODULE_MSM, "%s mediaId already regsitered", __FUNCTION__);
      return MCS_ERROR_SESSION_ALREADY_REGISTERED;
    }
  }
  mediaSession objMediaSession(mediaId, appId);
  mapMediaSessionInfo_[mediaId] = objMediaSession;
  //add mediaId to receiver stack
  objRequestRcvr_.addClient(mediaId);
  return MCS_ERROR_NO_ERROR;
}

int MediaSessionManager::activateMediaSession (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      if(SET == objRequestRcvr_.getClientPriority(mediaId)) {
        PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId Already SET", __FUNCTION__);
        return MCS_ERROR_SESSION_ALREADY_ACTIVE;
      }
      else {
        objRequestRcvr_.setClientPriority(mediaId, SET);
        return MCS_ERROR_NO_ERROR;
      }
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return MCS_ERROR_INVALID_MEDIAID;
}

int MediaSessionManager::deactivateMediaSession (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      if(RESET == objRequestRcvr_.getClientPriority(mediaId)) {
        PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId Already RESET", __FUNCTION__);
        return MCS_ERROR_SESSION_ALREADY_DEACTIVE;
      }
      else {
        objRequestRcvr_.setClientPriority(mediaId, RESET);
        return MCS_ERROR_NO_ERROR;
      }
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return MCS_ERROR_INVALID_MEDIAID;
}

int MediaSessionManager::removeMediaSession (const std::string& mediaId) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for (const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      mapMediaSessionInfo_.erase(itr.first);
      //delete media client from receiver stack
      objRequestRcvr_.removeClient(mediaId);
      return MCS_ERROR_NO_ERROR;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return MCS_ERROR_INVALID_MEDIAID;
}

int MediaSessionManager::getMediaMetaData(const std::string& mediaId,
                                           mediaMetaData& objMetaData) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      objMetaData = itr.second.getMediaMetaDataObj();
      return MCS_ERROR_NO_ERROR;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return MCS_ERROR_INVALID_MEDIAID;
}

int MediaSessionManager::getMediaSessionInfo(const std::string& mediaId,
                                              mediaSession& objMediaSession) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      objMediaSession.setMediaId(itr.first);
      objMediaSession.setAppId(itr.second.getAppId());
      objMediaSession.setPlayStatus(itr.second.getPlayStatus());
      objMediaSession.setMetaData(itr.second.getMediaMetaDataObj());
      return MCS_ERROR_NO_ERROR;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return MCS_ERROR_INVALID_MEDIAID;
}

int MediaSessionManager::getMediaPlayStatus(const std::string& mediaId,
                                             std::string& playStatus) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  for(const auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      playStatus = itr.second.getPlayStatus();
      return MCS_ERROR_NO_ERROR;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return MCS_ERROR_INVALID_MEDIAID;
}

int MediaSessionManager::setMediaMetaData(const std::string& mediaId,
                                           const mediaMetaData& objMetaData) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());
  for(auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      //save metadata info
      itr.second.setMetaData(objMetaData);
      return MCS_ERROR_NO_ERROR;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return MCS_ERROR_INVALID_MEDIAID;
}

int MediaSessionManager::setMediaPlayStatus(const std::string& mediaId,
                                             const std::string& playStatus) {
  PMLOG_INFO(CONST_MODULE_MSM, "%s mediaId : %s playStatus : %s", __FUNCTION__,
                                mediaId.c_str(), playStatus.c_str());

  for(auto& itr : mapMediaSessionInfo_) {
    if(itr.first == mediaId) {
      itr.second.setPlayStatus(playStatus);
      return MCS_ERROR_NO_ERROR;
    }
  }
  PMLOG_ERROR(CONST_MODULE_MSM, "%s MediaId doesnt exists", __FUNCTION__);
  return MCS_ERROR_INVALID_MEDIAID;
}

std::vector<std::string> MediaSessionManager::getActiveMediaSessionList() {
  PMLOG_INFO(CONST_MODULE_MSM, "%s", __FUNCTION__);
  std::vector<std::string> activeMediaId;

  for (const auto& itr : objRequestRcvr_.getClientList()) {
       if(itr.priority_ == SET)
         activeMediaId.push_back(itr.mediaId_);
  }

  return activeMediaId;
}

std::vector<std::string> MediaSessionManager::getMediaSessionList(const std::string& appId) {
  std::vector<std::string> mediaSessionId;
  for (const auto& itr : mapMediaSessionInfo_) {
    if(appId == itr.second.getAppId()) {
      mediaSessionId.push_back(itr.first);
    }
  }
  return mediaSessionId;
}

std::string MediaSessionManager::getCurrentActiveSession() {
  return objRequestRcvr_.getLastActiveClient();
}
