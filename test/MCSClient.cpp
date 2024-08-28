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

#include "MCSClient.h"
#include <string>
#include <iostream>
#include <cstdio>
#include <thread>

#include <pbnjson.hpp>

struct AutoLSError : LSError {
   AutoLSError() { LSErrorInit(this); }
  ~AutoLSError() { LSErrorFree(this); }
};

int displayId = 0;
std::string muteStatus[3];
std::string playStatus[3];
std::string playPosition[3];
std::string mediaMetaData[3];
std::string AVNDeviceType = "AVN";
std::string coverArtData;
std::vector<std::string> source;
std::string supportedActions;

bool isAvn = false;
LSHandle *gHandle = nullptr;

bool metaDataResponse(LSHandle* sh, LSMessage* reply, void* ctx) {
  LS::Message response(reply);
  pbnjson::JValue payload = pbnjson::JDomParser::fromString(response.getPayload());
  if(payload.isNull())
    return true;

  bool returnValue = payload["returnValue"].asBool();
  if(returnValue) {
    displayId = payload["displayId"].asNumber<int>();
    if((payload["playStatus"].asString() != "") && playStatus[displayId] != payload["playStatus"].asString())
      playStatus[displayId] = payload["playStatus"].asString();
    if((payload["muteStatus"].asString() != "") && muteStatus[displayId] != payload["muteStatus"].asString())
      muteStatus[displayId] = payload["muteStatus"].asString();
    if(payload["playPosition"].asString() != "")
      playPosition[displayId] = payload["playPosition"].asString();
    pbnjson::JValue metaData = payload["mediaMetaData"];
    if(metaData.stringify() != "null")
      mediaMetaData[displayId] = metaData.stringify();
    if(payload.hasKey("coverArt"))
    {
      source.clear();
      pbnjson::JValue coverArt = payload["coverArt"];
      if (coverArt.isArray() && coverArt.arraySize() > 0) {
        coverArtData = coverArt.stringify();
        for (int i = 0; i < coverArt.arraySize(); i++) {
          if(coverArt[i].hasKey("src") && coverArt[i]["src"].isString())
            source.push_back(coverArt[i]["src"].asString());
        }
      }
    }
    if(payload.hasKey("supportedActions"))
    {
      supportedActions.clear();
      pbnjson::JValue sActions = payload["supportedActions"];
      // Create a formatted string manually
      std::string formattedActions = "SupportedActions : [\n";
      for (size_t i = 0; i < sActions.arraySize(); ++i) {
          formattedActions += "    \"" + sActions[i].asString() + "\"";
          if (i < sActions.arraySize() - 1) {
              formattedActions += ",";
          }
          formattedActions += "\n";
      }
      formattedActions += "]";

      // Assign the formatted JSON string
      supportedActions = formattedActions;
    }
  }
  return true;
}

MCSClient::MCSClient()
{
  std::string serviceName = "com.webos.service.mediacontrollertest";
  AutoLSError error;
  context = nullptr;
  if(LSRegister(serviceName.c_str(), &handle, &error)) {
    context = g_main_context_ref(g_main_context_default());
    LSGmainContextAttach(handle, context, &error);
  }
}

MCSClient::~MCSClient() {
  AutoLSError error;
  LSUnregister(handle, &error);
  g_main_context_unref(context);
}

void runSubscriptionThread() {
  GMainLoop* mainLoop = g_main_loop_new(nullptr, false);
  LSError lserror;
  if(!LSGmainAttach(gHandle, mainLoop, &lserror)) {
    std::cout << "Unable to attach to service" << std::endl;
    return;
  }
  std::string subscriptionUri = "luna://com.webos.service.mediacontroller/receiveMediaPlaybackInfo";
  std::string payloadUri0;
  std::string payloadUri1;
  if(isAvn) {
    payloadUri0 = "{\"displayId\":1,\"subscribe\":true}";
    payloadUri1 = "{\"displayId\":2,\"subscribe\":true}";
  } else {
    payloadUri0 = "{\"displayId\":0,\"subscribe\":true}";
    payloadUri1 = "{\"displayId\":1,\"subscribe\":true}";
  }

  LSError error;
  if(!LSCall(gHandle, subscriptionUri.c_str(), payloadUri0.c_str(), metaDataResponse, NULL, NULL, &error)) {
    std::cout << "LSCall error for receiveMediaPlaybackInfo" << std::endl;
  }

  g_main_loop_run(mainLoop);
  g_main_loop_unref(mainLoop);
  mainLoop = nullptr;
}

std::string executeCommand(const char* cmd) {
    std::string result = "";
    char buffer[128];
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        std::cerr << "Error executing command\n";
        return "";
    }
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

void runMainMenuThread() {
  GMainLoop* mainLoop = g_main_loop_new(nullptr, false);
  LSError lserror;
  if(!LSGmainAttach(gHandle, mainLoop, &lserror)) {
    std::cout << "Unable to attach to service" << std::endl;
    return;
  }
  bool flag = true;
  while(flag) {
    std::cout << "Select Display ID" << std::endl;
    std::cout << "0. Display-0" << std::endl;
    std::cout << "1. Display-1" << std::endl;
    std::cout << "2. Exit" << std::endl;

    int input = -1;
    std::cin >> input;
    switch (input) {
      case 0:
      case 1:
      {
        bool flagMainMenu = true;
        input = isAvn ? (input + 1) : input;
        while(flagMainMenu) {
          std::cout << "Select widget event : " << std::endl;
          std::cout << "1.  Play" << std::endl;
          std::cout << "2.  Pause" << std::endl;
          std::cout << "3.  Mute" << std::endl;
          std::cout << "4.  Unmute" << std::endl;
          std::cout << "5.  Next" << std::endl;
          std::cout << "6.  Previous" << std::endl;
          std::cout << "7.  Get Play Position" << std::endl;
          std::cout << "8.  Get Mute Status" << std::endl;
          std::cout << "9.  Get Playback Status" << std::endl;
          std::cout << "10. Get Metadata" << std::endl;
          std::cout << "11. Main Menu" << std::endl;
          std::cout << "12. Toggle Mic" << std::endl;
          std::cout << "13. Toggle Camera" << std::endl;
          std::cout << "14. Hang Up" << std::endl;
          std::cout << "15. Get CoverArt Info" << std::endl;
          std::cout << "16. Get CoverArt Path" << std::endl;
          std::cout << "17. Get Supported Actions" << std::endl;

          int userChoice = -1;
          std::cin >> userChoice;
          switch (userChoice) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 12:
            case 13:
            case 14:
            {
              std::string eventCmd;
              if(userChoice == 1) eventCmd = "play";
              else if(userChoice == 2) eventCmd = "pause";
              else if(userChoice == 3) eventCmd = "mute";
              else if(userChoice == 4) eventCmd = "unmute";
              else if(userChoice == 5) eventCmd = "next";
              else if(userChoice == 6) eventCmd = "previous";
              else if(userChoice == 12) eventCmd = "toggleMic";
              else if(userChoice == 13) eventCmd = "toggleCamera";
              else if(userChoice == 14) eventCmd = "hangUp";
              std::string uri = "luna-send -n 1 -f luna://com.webos.service.mediacontroller/injectMediaKeyEvent ";
              std::string payload = "'{\"displayId\":" + std::to_string(input) + ",\"keyEvent\":\"" + eventCmd + "\"}'";
              std::string cmd = uri + payload;
              std::cout << "cmd = " << cmd << std::endl;
              system(cmd.c_str());
              break;
            }
            case 7:
            {
             while(1) {
                std::cout << "position : " << playPosition[input] << std::endl;
                char ch;
                if((ch = std::cin.get()) == 27)
                  break;
              }
             std::cout << "" << std::endl;
             break;
            }
            case 8:
            {
              std::cout << "MuteStatus : " << muteStatus[input] << std::endl;
              break;
            }
            case 9:
            {
              std::cout << "Playback status : " << playStatus[input] << std::endl;
              break;
            }
            case 10:
            {
              std::cout << "MetaData : " << mediaMetaData[input] << std::endl;
              break;
            }
            case 11:
            {
              flagMainMenu = false;
              break;
            }
            case 15:
            {
              std::cout << "CoverArt : " << coverArtData << std::endl;
              break;
            }
            case 16:
            {
              std::string uri = "luna-send -n 1 -f luna://com.webos.service.mediacontroller/getMediaCoverArtPath ";
              std::string src = "";
              for(int i = 0; i < source.size(); i++) {
                src += "\"" + source[i] + "\"";
                if(i+1 < source.size())
                  src += ",";
              }
              std::string payload = "'{\"displayId\":" + std::to_string(input) + ",\"src\":[" + src + "]}'";
              std::string cmd = uri + payload;
              std::cout << "cmd = " << cmd << std::endl;
              std::string response = executeCommand(cmd.c_str());

              std::cout << "Response:\n" << response << std::endl;
              break;
            }
            case 17:
            {
              std::cout << supportedActions << std::endl;
              break;
            }
            default:
            {
              std::cout << "Wrong Choice" << std::endl;
              break;
            }
          }
        }
        break;
      }
      case 2:
      {
        flag = false;
        break;
      }
      default:
      {
        std::cout << "Wrong choice" << std::endl;
        break;
      }
    }
  }
  g_main_loop_run(mainLoop);

  g_main_loop_unref(mainLoop);
  mainLoop = nullptr;
}

int main(int argc, char ** argv) {
  MCSClient obj;
  gHandle = obj.getHandle();

  std::thread subscriptionThread(runSubscriptionThread);
  std::thread menuThread(runMainMenuThread);
  subscriptionThread.join();
  menuThread.join();

  return 0;
}

