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

#ifndef MEDIA_SESSION_MANAGER_H_
#define MEDIA_SESSION_MANAGER_H_

/*-----------------------------------------------------------------------------
    (File Inclusions)
------------------------------------------------------------------------------*/
#include <map>
#include "MediaControlTypes.h"
#include "RequestReceiver.h"

class MediaSessionManager
{
private:
  MediaSessionManager();
  std::map<std::string, mediaSession> mapMediaSessionInfo_;
  RequestReceiver objRequestRcvr_;

public:
  static MediaSessionManager &getInstance();
  void addMediaSession (const std::string& mediaId,
                        const std::string& appId,
                        const mediaMetaData& obj);
  bool activateMediaSession (const std::string& mediaId);
  bool deactivateMediaSession (const std::string& mediaId);
  bool removeMediaSession (const std::string& mediaId);
  mediaMetaData getMediaMetaData(const std::string& mediaId);
  std::string getMediaPlayStatus(const std::string& mediaId);
  //todo : add session related API's in nxc ccc
  std::string getActiveSessionbyDisplayId (const int& displayId);
};

#endif /*MEDIA_SESSION_MANAGER_H_*/
