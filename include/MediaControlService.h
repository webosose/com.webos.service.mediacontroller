// Copyright (c) 2020-2024 LG Electronics, Inc.
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

#ifndef MEDIA_CONTROL_SERVICE_H_
#define MEDIA_CONTROL_SERVICE_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include <luna-service2/lunaservice.hpp>
#include <glib.h>

#include "MediaSessionManager.h"
#include "MediaControlPrivate.h"

class MediaControlService : public LS::Handle
{
private:
  using mainloop = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>;
  mainloop main_loop_ptr_ = {g_main_loop_new(nullptr, false), g_main_loop_unref};

public:
  MediaControlService();

  MediaControlService(MediaControlService const &) = delete;
  MediaControlService(MediaControlService &&) = delete;
  MediaControlService &operator=(MediaControlService const &) = delete;
  MediaControlService &operator=(MediaControlService &&) = delete;

  bool registerMediaSession(LSMessage &);
  bool unregisterMediaSession(LSMessage &);
  bool activateMediaSession(LSMessage &);
  bool deactivateMediaSession(LSMessage &);
  bool getMediaMetaData(LSMessage &);
  bool getMediaPlayStatus(LSMessage &);
  bool getMediaSessionInfo(LSMessage &);
  bool getMediaSessionId(LSMessage &);
  bool getActiveMediaSessions(LSMessage &);
  bool getMediaCoverArtPath(LSMessage &);
  bool setMediaMetaData(LSMessage &);
  bool setMediaPlayStatus(LSMessage &);
  bool setMediaMuteStatus (LSMessage &);
  bool setMediaPlayPosition (LSMessage &);
  bool setMediaCoverArt (LSMessage &);
  bool receiveMediaPlaybackInfo (LSMessage &);
  bool injectMediaKeyEvent (LSMessage &);
  static bool onBTServerStatusCb(LSHandle *lshandle, LSMessage *message, void *ctx);
  static bool onBTAdapterQueryCb(LSHandle *lshandle, LSMessage *message, void *ctx);
  static bool onBTDeviceGetStatusCb(LSHandle *lshandle, LSMessage *message, void *ctx);
  static bool onBTAvrcpGetStatusCb(LSHandle *lshandle, LSMessage *message, void *ctx);
  static bool onBTAvrcpKeyEventsCb(LSHandle *lshandle, LSMessage *message, void *ctx);
  static bool onGetSessionsInfoCb(LSHandle *lshandle, LSMessage *message, void *ctx);
  int updateMetaDataResponse(const std::string &,  pbnjson::JObject &);

private:
  void subscribeToBTAdapterGetStatus();
#if USE_TEST_METHOD
  bool testKeyEvent(LSMessage &);
#endif

  LSHandle *lsHandle_;
  MediaSessionManager *ptrMediaSessionMgr_;
  MediaControlPrivate *ptrMediaControlPrivate_;
};

#endif /*MEDIA_CONTROL_SERVICE_H_*/

extern "C" bool onBTDeviceGetStatusCbWrapper(LSHandle *lshandle, LSMessage *message, void *ctx) {
    return MediaControlService::onBTDeviceGetStatusCb(lshandle, message, ctx);
}
