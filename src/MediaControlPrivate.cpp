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
#include "MediaControlPrivate.h"
#include "MediaSessionManager.h"

MediaControlPrivate::MediaControlPrivate() :
  mapDeviceInfo_(), playStatus_(false), muteStatus_(false),
  playPosition_(false), mediaMetaData_(false) {
}

MediaControlPrivate& MediaControlPrivate::getInstance() {
  static MediaControlPrivate objMediaCtrlPrivate;
  return objMediaCtrlPrivate;
}

void MediaControlPrivate::setBTDeviceInfo(const BTDeviceInfo& objDevInfo) {
  PMLOG_INFO(CONST_MODULE_MCP, "%s Connected device", __FUNCTION__);
  //add Device to the map
  std::string deviceAddress = objDevInfo.deviceAddress_;
  PMLOG_INFO(CONST_MODULE_MCP, "%s Connected device device Address : %s", __FUNCTION__, deviceAddress.c_str());
  mapDeviceInfo_[deviceAddress] = objDevInfo;
}

std::string MediaControlPrivate::getMediaId(const std::string& deviceAddress) {
  PMLOG_INFO(CONST_MODULE_MCP, "%s for device Address : %s", __FUNCTION__, deviceAddress.c_str());
  std::string mediaId;
  for(const auto& itr : mapDeviceInfo_) {
    if(itr.first == deviceAddress) {
      mediaId = MediaSessionManager::getInstance().getCurrentActiveSession();
      return mediaId;
    }
  }
  return mediaId;
}
