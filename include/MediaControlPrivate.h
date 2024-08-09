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

#ifndef MEDIA_CONTROL_PRIVATE_H_
#define MEDIA_CONTROL_PRIVATE_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include <map>

#include "MediaControlTypes.h"

class MediaControlPrivate
{
public:
  bool playStatus_;
  bool muteStatus_;
  bool playPosition_;
  bool mediaMetaData_;
  bool coverArt_;
  static MediaControlPrivate &getInstance();

  void setBTDeviceInfo(const BTDeviceInfo& objDevInfo);
  bool getBTDeviceInfo(const int& displayId, BTDeviceInfo *objDevInfo);
  bool isDeviceRegistered(const std::string& address, const std::string& adapterAddress);
  std::string getMediaId(const std::string& deviceAddress);
  void setSessionListInfo(const BTDeviceInfo& btInfo);
  void setBTAdapterInfo(const std::string& deviceSetId, const std::string& adapterAddress);
  int getDisplayIdForBT(const std::string& adapterAddress);

private:
  MediaControlPrivate();
  std::map<std::string, BTDeviceInfo> mapDeviceInfo_;
  std::vector<BTDeviceInfo> btAdapterInfo_;
};

#endif /*MEDIA_CONTROL_SERVICE_H_*/
