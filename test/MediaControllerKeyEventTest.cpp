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
#include <iostream>
#include <sstream>
#include <string>

const std::string serviceUri = "luna-send -n 1 -f luna://com.webos.service.mediacontroller/";

std::string getCommandOutput(std::string cmd) {
  FILE *fp = nullptr;
  std::string str = "";
  char path[1035];
  fp = popen(cmd.c_str(), "r");
  if (fp == NULL)
  {
    std::cout << "Failed to run command" << std::endl;
    exit(1);
  }
  while (fgets(path, sizeof(path) - 1, fp) != NULL)
  {
    str += path;
  }
  pclose(fp);
  return str;
}

int main(int argc, char const *argv[]) {
    int choice = -1;
    bool flag = true;

    while (flag) {
    std::cout << "Select key event : " << std::endl;
    std::cout << "1. Play" << std::endl << "2. Pause" << std::endl;
    std::cout << "3. Next" << std::endl << "4. Previous" << std::endl;
    std::cout << "5. VolumeUp" << std::endl << "6. VolumeDown" << std::endl;
    std::cout << "7. Mute" << std::endl << "8. Unmute" << std::endl;
    std::cout << "9. PlayInfo" << std::endl << "10. Exit" << std::endl;
    std::cin >> choice;
    switch (choice) {
    case 1: {
      std::string keyEventPlay = serviceUri + "/testKeyEvent '{\"mediaId\":\"xDFNUI\", \"keyEvent\":\"play\"}'";
      std::cout << keyEventPlay << std::endl;
      std::string output = getCommandOutput(keyEventPlay);
      std::cout << output << std::endl;
      break;
    }
    case 2: {
      std::string keyEventPause = serviceUri + "/testKeyEvent '{\"mediaId\":\"xDFNUI\", \"keyEvent\":\"pause\"}'";
      std::cout << keyEventPause << std::endl;
      std::string output = getCommandOutput(keyEventPause);
      std::cout << output << std::endl;
      break;
    }
    case 3: {
      std::string keyEventNext = serviceUri + "/testKeyEvent '{\"mediaId\":\"xDFNUI\", \"keyEvent\":\"next\"}'";
      std::cout << keyEventNext << std::endl;
      std::string output = getCommandOutput(keyEventNext);
      std::cout << output << std::endl;
      break;
    }
    case 4: {
      std::string keyEventPrevious = serviceUri + "/testKeyEvent '{\"mediaId\":\"xDFNUI\", \"keyEvent\":\"previous\"}'";
      std::cout << keyEventPrevious << std::endl;
      std::string output = getCommandOutput(keyEventPrevious);
      std::cout << output << std::endl;
      break;
    }
    case 5: {
      std::string keyEventVolumeUp = serviceUri + "/testKeyEvent '{\"mediaId\":\"xDFNUI\", \"keyEvent\":\"volumeUp\"}'";
      std::cout << keyEventVolumeUp << std::endl;
      std::string output = getCommandOutput(keyEventVolumeUp);
      std::cout << output << std::endl;
      break;
    }
    case 6: {
      std::string keyEventVolumeDown = serviceUri + "/testKeyEvent '{\"mediaId\":\"xDFNUI\", \"keyEvent\":\"volumeDown\"}'";
      std::cout << keyEventVolumeDown << std::endl;
      std::string output = getCommandOutput(keyEventVolumeDown);
      std::cout << output << std::endl;
      break;
    }
    case 7: {
      std::string keyEventMute = serviceUri + "/testKeyEvent '{\"mediaId\":\"xDFNUI\", \"keyEvent\":\"mute\"}'";
      std::cout << keyEventMute << std::endl;
      std::string output = getCommandOutput(keyEventMute);
      std::cout << output << std::endl;
      break;
    }
    case 8: {
      std::string keyEventUnmute = serviceUri + "/testKeyEvent '{\"mediaId\":\"xDFNUI\", \"keyEvent\":\"unmute\"}'";
      std::cout << keyEventUnmute << std::endl;
      std::string output = getCommandOutput(keyEventUnmute);
      std::cout << output << std::endl;
      break;
    }
    case 9: {
      std::string keyEventMetaData = serviceUri + "/receiveMediaPlaybackInfo '{\"displayId\":0, \"subscribe\":true}'";
      std::cout << keyEventMetaData << std::endl;
      std::string output = getCommandOutput(keyEventMetaData);
      std::cout << output << std::endl;
      break;
    }
    case 10: {
      flag = false;
      break;
    }
    default: {
      std::cout << "Wrong choice" << std::endl;
      break;
    }
    }
  }
    return 0;
}
