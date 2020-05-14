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

#ifndef MEDIA_CONTROL_TYPES_H_
#define MEDIA_CONTROL_TYPES_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include <string>
#include "PmLogLib.h"

const std::string CSTR_INVALID_MEDIAID = "Invalid mediaId";
const std::string CSTR_REGISTERSESSION_FAILED = "MediaId registeration failed";
const std::string CSTR_PARSING_ERROR = "Parsing Error";
const std::string CSTR_INVALID_APPID = "Invalid appId";
const std::string CSTR_NO_ACTIVE_SESSION = "No session is active";
const std::string CSTR_EMPTY = "";

#define CONST_MODULE_MCS "MediaControlService"
#define CONST_MODULE_MCP "MediaControlPrivate"
#define CONST_MODULE_MSM "MediaSessionManager"
#define CONST_MODULE_RR  "RequestReceiver"

#define SET 1
#define RESET 0

#define PMLOG_ERROR(module, args...) PmLogMsg(getLunaPmLogContext(), Error, module, 0, ##args)
#define PMLOG_INFO(module, args...) PmLogMsg(getLunaPmLogContext(), Info, module, 0, ##args)
#define PMLOG_DEBUG(args...) PmLogMsg(getLunaPmLogContext(), Debug, NULL, 0, ##args)

enum MCSErrorCode
{
  MCS_ERROR_INVALID_MEDIAID = 0,
  MCS_ERROR_REGISTERSESSION_FAILED,
  MCS_ERROR_PARSING_FAILED,
  MCS_ERROR_INVALID_APPID,
  MCS_ERROR_NO_ACTIVE_SESSION,
  MCS_ERROR_NO_ERROR
};

enum mediaPlayState
{
  PLAY_STATE_NONE = 0,
  PLAY_STATE_STOPPED,
  PLAY_STATE_PAUSED,
  PLAY_STATE_PLAYING,
  PLAY_STATE_FAST_FORWARDING,
  PLAY_STATE_REWINDING,
  PLAY_STATE_BUFFERING,
  PLAY_STATE_ERROR
};

struct requestReceiver
{
  std::string mediaId_;
  int priority_;
  requestReceiver(): mediaId_(CSTR_EMPTY), priority_(RESET) {}
  requestReceiver(const std::string& mediaId, const int& priority = RESET)
    : mediaId_(mediaId), priority_(priority)
  {}
};

struct mediaMetaData
{
  std::string title_;
  std::string artist_;
  std::string totalDuration_;
  std::string album_;
  std::string genre_;
  int trackNumber_;
  mediaMetaData() :
    title_(CSTR_EMPTY),
    artist_(CSTR_EMPTY),
    totalDuration_(CSTR_EMPTY),
    album_(CSTR_EMPTY),
    genre_(CSTR_EMPTY),
    trackNumber_(0) {}
};

struct mediaSession
{
  std::string mediaId_;
  std::string appId_;
  int displayId_;
  int volume_;
  mediaMetaData objMetaData_;
  mediaSession() :
    mediaId_(CSTR_EMPTY),
    appId_(CSTR_EMPTY),
    displayId_(0),
    volume_(0) {}
  mediaSession(const std::string& mediaId, const std::string& appId,
               const int displayId = 0, const int volume = 0)
    : mediaId_(mediaId),
      appId_(appId),
      displayId_(displayId),
      volume_(volume) {}
};

struct BTDeviceInfo
{
  std::string deviceAddress_;
  std::string adapterAddress_;
  int displayId_;
  BTDeviceInfo() :
    deviceAddress_(CSTR_EMPTY),
    adapterAddress_(CSTR_EMPTY),
    displayId_(0) {}
  BTDeviceInfo(const std::string& address, const std::string &adapterAddress,
               const int& displayId = 0)
    : deviceAddress_(address),
      adapterAddress_(adapterAddress),
      displayId_(displayId)
  {}
};

static PmLogContext getLunaPmLogContext()
{
  static PmLogContext logContext = 0;
  if (0 == logContext)
    PmLogGetContext("mediacontroller", &logContext);
  return logContext;
}

#endif /*MEDIA_CONTROL_SERVICE_H_*/
