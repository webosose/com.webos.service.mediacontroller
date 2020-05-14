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

MediaSessionManager::MediaSessionManager() : activeMediaId_ (""),
inactiveMediaId_(""), mapMediaSessionInfo_() {

}

MediaSessionManager& MediaSessionManager::getInstance()
{
  static MediaSessionManager objMediaSessionMgr;
  return objMediaSessionMgr;
}

bool MediaSessionManager::activateMediaSession (const std::string &mediaId)
{
    activeMediaId_ = mediaId;
    return true;
}

bool MediaSessionManager::deactivateMediaSession (const std::string &mediaId)
{
    inactiveMediaId_ = mediaId;
    return true;
}

bool MediaSessionManager::addMediaSession (const std::string &mediaId, const std::string &appId, int displayId)
{
   //names will be changed later
   mediaSession id(mediaId, appId);
   mapMediaSessionInfo_[id.mediaId_]=id;

   //logs will be added
   for (auto itr : mapMediaSessionInfo_) {
     std::cout << "value key"<< itr.first << std::endl;
     std::cout << "value element:1"<< itr.second.appId_ << std::endl;
  }
  std::cout<<"size:"<<mapMediaSessionInfo_.size()<<std::endl;
  return true;
}

bool MediaSessionManager::removeMediaSession (const std::string &mediaId)
{
   const auto cMediaId = mapMediaSessionInfo_.find(mediaId)->first;
   for (auto itr : mapMediaSessionInfo_) {
     if(itr.first == cMediaId) {
       mapMediaSessionInfo_.erase(itr.first);
       break;
     }
   }
   return true;
}
