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
#include <vector>
#include "MediaControlTypes.h"
#include "RequestReceiver.h"

class MediaSessionManager
{
private:
  MediaSessionManager();
  std::map<std::string, mediaSession> mapMediaSessionInfo_;
  RequestReceiver objRequestRcvr_[2];

public:
  static MediaSessionManager &getInstance();
  void addMediaSession (const std::string& mediaId,
                        const std::string& appId);
  bool activateMediaSession (const std::string& mediaId);
  bool deactivateMediaSession (const std::string& mediaId);
  bool removeMediaSession (const std::string& mediaId);
  bool getMediaMetaData(const std::string& mediaId,
                        mediaMetaData& objMetaData);
  bool getMediaSessionInfo(const std::string& mediaId,
                           mediaSession& objMediaSession);
  bool getMediaPlayStatus(const std::string& mediaId,
                          std::string& playStatus);
  bool setMediaMetaData(const std::string& mediaId,
                        const mediaMetaData& objMetaData);
  bool setMediaPlayStatus(const std::string& mediaId,
                          const std::string& playStatus);
  std::vector<std::string> getMediaSessionId(const std::string& appId);
  std::vector<std::string> getActiveMediaSessions();
  std::string getActiveSessionbyDisplayId (const int& displayId);
};

#endif /*MEDIA_SESSION_MANAGER_H_*/
