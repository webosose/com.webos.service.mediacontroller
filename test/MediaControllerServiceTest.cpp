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
//SPDX-License-Identifier: Apache-2.0

/*-----------------------------------------------------------------------------*/
#include <iostream>
#include <sstream>
#include <string>

const std::string serviceUri = "luna-send -n 1 -f luna://com.webos.service.mediacontroller/";

std::string executeCommand(std::string cmd) {
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

void printOutput(std::string uri, int &test_count){
  std::cout << uri << std::endl;
  std::string output = executeCommand(uri);
    std::cout << output << std::endl;
  test_count++;
}

/* This function will cover both valid and invalid cases and TC count for each API*/
void test_registerMediaSession() {
  int test_count = 0;
  std::string api = "registerMediaSession ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\","
                      "\"appId\":\"com.webos.app.test.youtube\",\"subscribe\":true}'";
  printOutput(uri, test_count);
  /* invalid case same media ID registering again*/
  uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\","
    "\"appId\":\"com.webos.app.test.youtube\",\"subscribe\":true}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754,"
    "\"appId\":\"com.webos.app.test.youtube\",\"subscribe\":true}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_activateMediaSession() {
  int test_count = 0;
  std::string api = "activateMediaSession ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\"}'";
  printOutput(uri, test_count);
  /*invalid Media ID */
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\"}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_getMediaMetaData() {
  int test_count = 0;
  std::string api = "getMediaMetaData ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\"}'";
  printOutput(uri, test_count);
  /*invalid Media ID */
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\"}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_getMediaPlayStatus() {
  int test_count = 0;
  std::string api = "getMediaPlayStatus ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\"}'";
  printOutput(uri, test_count);
  /*invalid Media ID */
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\"}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_getMediaSessionInfo() {
  int test_count = 0;
  std::string api = "getMediaSessionInfo ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\"}'";
  printOutput(uri, test_count);
  /*invalid Media ID */
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\"}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_getMediaSessionId() {
  int test_count = 0;
  std::string api = "getMediaSessionId ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"appId\":\"com.webos.app.test.youtube\"}'";
  printOutput(uri, test_count);
  /*invalid appId*/
  uri = serviceUri + api + "'{\"appId\":\"com.webos.app.test.youte\"}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"appId\":68754}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_getActiveMediaSessions() {
  int test_count = 0;
  std::string api = "getActiveMediaSessions ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{}'";
  printOutput(uri, test_count);
  /*invalid test case passing extra argument*/
  uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\"}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_setMediaMetaData() {
  int test_count = 0;
  std::string api = "setMediaMetaData ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\","
    "\"mediaMetaData\": {\"title\":\"BigBangTheory\",\"artist\":\"BigBang\",\"totalDuration\":\"10.10\","
    "\"album\":\"BBT\",\"genre\":\"drama\",\"trackNumber\":2,\"volume\":100 }}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\","
    "\"mediaMetaData\": {\"title\":\"BigBangTheory\",\"artist\":\"BigBang\",\"totalDuration\":\"10.10\","
    "\"album\":\"BBT\",\"genre\":23,\"trackNumber\":2,\"volume\":100 }}'";
  printOutput(uri, test_count);
  /*invalid Media ID */
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\","
    "\"mediaMetaData\": {\"title\":\"BigBangTheory\",\"artist\":\"BigBang\",\"totalDuration\":\"10.10\","
    "\"album\":\"BBT\",\"genre\":\"drama\",\"trackNumber\":2,\"volume\":100 }}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754,"
    "\"mediaMetaData\": {\"title\":\"BigBangTheory\",\"artist\":\"BigBang\",\"totalDuration\":\"10.10\","
    "\"album\":\"BBT\",\"genre\":\"drama\",\"trackNumber\":2,\"volume\":100 }}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_setMediaPlayStatus() {
  int test_count = 0;
  std::string api = "setMediaPlayStatus ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\",\"playStatus\":\"PLAYSTATE_PLAYING\"}'";
  printOutput(uri, test_count);
  /*invalid Media ID */
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\",\"playStatus\":\"PLAYSTATE_PLAYING\"}'";
  printOutput(uri, test_count);
  /*invalid play state*/
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\",\"playStatus\":\"PLAY\"}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754,\"playStatus\":\"PLAYSTATE_PLAYING\"}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_deactivateMediaSession() {
  int test_count = 0;
  std::string api = "deactivateMediaSession ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\"}'";
  printOutput(uri, test_count);
  /*invalid Media ID */
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\"}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

void test_unregisterMediaSession() {
  int test_count = 0;
  std::string api = "unregisterMediaSession ";
  /*Valid test case*/
  std::string uri = serviceUri + api + "'{\"mediaId\":\"xDFNUI\"}'";
  printOutput(uri, test_count);
  /*invalid Media ID */
  uri = serviceUri + api + "'{\"mediaId\":\"xDLNR\"}'";
  printOutput(uri, test_count);
  /*invalid parsing expecting string put passing integer*/
  uri = serviceUri + api + "'{\"mediaId\":68754}'";
  printOutput(uri, test_count);

  std::cout << test_count << " cases executed." << std::endl;
}

int main(int argc, char const *argv[]) {
    int choice = -1;
    bool flag=true;
    while (flag) {
        std::cout <<  std::endl << "List of test cases : " << std::endl;
        std::cout << "1. registerMediaSession" << std::endl << "2. activateMediaSession" << std::endl;
        std::cout << "3. setMediaMetaData" << std::endl << "4. setMediaPlayStatus" << std::endl;
        std::cout << "5. getMediaMetaData" << std::endl << "6. getMediaPlayStatus" << std::endl;
        std::cout << "7. getMediaSessionInfo" << std::endl << "8. getMediaSessionId" << std::endl;
        std::cout << "9. getActiveMediaSessions" << std::endl << "10. deactivateMediaSession" << std::endl;
        std::cout << "11. unregisterMediaSession" << std::endl << "12.Execute all test case"<< std::endl;
        std::cout << "13.Exit" << std::endl;
    std::cin >> choice;
    switch (choice) {
    case 1:
      test_registerMediaSession();
      break;
    case 2:
      test_activateMediaSession();
      break;
    case 3:
      test_setMediaMetaData();
      break;
    case 4:
      test_setMediaPlayStatus();
      break;
    case 5:
      test_getMediaMetaData();
      break;
    case 6:
      test_getMediaPlayStatus();
      break;
    case 7:
      test_getMediaSessionInfo();
      break;
    case 8:
      test_getMediaSessionId();
      break;
    case 9:
      test_getActiveMediaSessions();
      break;
    case 10:
      test_deactivateMediaSession();
      break;
    case 11:
      test_unregisterMediaSession();
      break;
    case 12:
      test_registerMediaSession();
      test_activateMediaSession();
      test_setMediaMetaData();
      test_setMediaPlayStatus();
      test_getMediaMetaData();
      test_getMediaPlayStatus();
      test_getMediaSessionInfo();
      test_getMediaSessionId();
      test_getActiveMediaSessions();
      test_deactivateMediaSession();
      test_unregisterMediaSession();
      break;
    case 13:
      flag = false;
      break;
    default:
      std::cout << "Wrong choice" << std::endl;
    }
  }
  return 0;
}
