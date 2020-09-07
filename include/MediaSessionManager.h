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
  RequestReceiver objRequestRcvr_;

public:
  static MediaSessionManager &getInstance();
  int addMediaSession (const std::string& mediaId,
                        const std::string& appId);
  int activateMediaSession (const std::string& mediaId);
  int deactivateMediaSession (const std::string& mediaId);
  int removeMediaSession (const std::string& mediaId);
  int getMediaMetaData(const std::string& mediaId,
                       mediaMetaData& objMetaData);
  int getMediaSessionInfo(const std::string& mediaId,
                          mediaSession& objMediaSession);
  int getMediaPlayStatus(const std::string& mediaId,
                         std::string& playStatus);
  int getMediaMuteStatus(const std::string& mediaId,
                         std::string& muteStatus);
  int getMediaPlayPosition(const std::string& mediaId,
                         std::string& playPosition);
  int setMediaMetaData(const std::string& mediaId,
                       const mediaMetaData& objMetaData);
  int setMediaPlayStatus(const std::string& mediaId,
                         const std::string& playStatus);
  int setMediaMuteStatus(const std::string& mediaId,
                         const std::string& muteStatus);
  int setMediaPlayPosition(const std::string& mediaId,
                         const std::string& playPosition);
  std::vector<std::string> getMediaSessionList(const std::string& appId);
  std::vector<std::string> getActiveMediaSessionList();
  std::string getCurrentActiveSession();
  bool validatePlayStatus(const std::string& playStatus);
  int getDisplayIdForMedia(const std::string& mediaId);
  std::string getMediaIdFromDisplayId(const int& displayId);
};

#endif /*MEDIA_SESSION_MANAGER_H_*/
