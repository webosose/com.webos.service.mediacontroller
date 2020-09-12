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
  static MediaControlPrivate &getInstance();

  void setBTDeviceInfo(const BTDeviceInfo& objDevInfo);
  BTDeviceInfo getBTDeviceInfo();
  std::string getMediaId(const std::string& deviceAddress);

private:
  MediaControlPrivate();
  std::map<std::string, BTDeviceInfo> mapDeviceInfo_;
};

#endif /*MEDIA_CONTROL_SERVICE_H_*/
