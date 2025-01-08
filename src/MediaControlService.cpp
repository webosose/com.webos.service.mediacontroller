// Copyright (c) 2020-2025 LG Electronics, Inc.
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

/*-----------------------------------------------------------------------------
 #include
 (File Inclusions)
 -- ----------------------------------------------------------------------------*/
#include "MediaControlService.h"
#include "Lsutils.h"

#include <string>
#include <iostream>
#include "Utils.h"

const std::string cstrMediaControlService = "com.webos.service.mediacontroller";
const std::string cstrBTAdapterGetStatus = "luna://com.webos.service.bluetooth2/adapter/getStatus";
const std::string cstrBTDeviceGetStatus = "luna://com.webos.service.bluetooth2/device/getStatus";
const std::string cstrBTAvrcpGetStatus = "luna://com.webos.service.bluetooth2/avrcp/getStatus";
const std::string cstrBTAvrcpReceivePassThroughCommand = "luna://com.webos.service.bluetooth2/avrcp/receivePassThroughCommand";
const std::string cstrSubscribe = "{\"subscribe\":true}";
const std::string cstrBTNotifyMediaPlayStatus = "luna://com.webos.service.bluetooth2/avrcp/notifyMediaPlayStatus";
const std::string cstrGetSessions = "luna://com.webos.service.account/getSessions";
const std::string MEDIA_SESSION_FOLDER = "/media/internal/.media-session";

bool BTConnected_ = false;

const short dispId = 0;

static void sendErrorResponse(int &errorCode, LS::Message &msgRequest) {
  PMLOG_ERROR(CONST_MODULE_MCS, "API fails with error %s", getErrorTextFromErrorCode(errorCode).c_str());
  std::string response = createJsonReplyString(false, errorCode, getErrorTextFromErrorCode(errorCode));
  msgRequest.respond(response.c_str());
}

MediaControlService::MediaControlService()
  : LS::Handle(LS::registerService(cstrMediaControlService.c_str())),
    lsHandle_(this->get()),
    ptrMediaSessionMgr_(&MediaSessionManager::getInstance()),
    ptrMediaControlPrivate_(&MediaControlPrivate::getInstance()) {
  PMLOG_INFO(CONST_MODULE_MCS,"%s IN", __FUNCTION__);
  LS_CATEGORY_BEGIN(MediaControlService, "/")
  LS_CATEGORY_METHOD(registerMediaSession)
  LS_CATEGORY_METHOD(unregisterMediaSession)
  LS_CATEGORY_METHOD(activateMediaSession)
  LS_CATEGORY_METHOD(deactivateMediaSession)
  LS_CATEGORY_METHOD(getMediaMetaData)
  LS_CATEGORY_METHOD(getMediaPlayStatus)
  LS_CATEGORY_METHOD(getMediaSessionInfo)
  LS_CATEGORY_METHOD(getMediaSessionId)
  LS_CATEGORY_METHOD(getActiveMediaSessions)
  LS_CATEGORY_METHOD(getMediaCoverArtPath)
  LS_CATEGORY_METHOD(setMediaMetaData)
  LS_CATEGORY_METHOD(setMediaPlayStatus)
  LS_CATEGORY_METHOD(setMediaMuteStatus)
  LS_CATEGORY_METHOD(setMediaPlayPosition)
  LS_CATEGORY_METHOD(receiveMediaPlaybackInfo)
  LS_CATEGORY_METHOD(injectMediaKeyEvent)
  LS_CATEGORY_METHOD(setMediaCoverArt)
  LS_CATEGORY_METHOD(setSupportedActions)
#if USE_TEST_METHOD
  LS_CATEGORY_METHOD(testKeyEvent)
#endif
  LS_CATEGORY_END;

  // attach to mainloop and run it
  attachToLoop(main_loop_ptr_.get());

  pbnjson::JValue payload = pbnjson::Object();
  payload.put("serviceName", "com.webos.service.bluetooth2");
  // check BT server status
  LS::Call status = callMultiReply("luna://com.webos.service.bus/signal/registerServerStatus",
                                   payload.stringify().c_str(),
                                   &MediaControlService::onBTServerStatusCb, this);

  int rev = directoryExists(MEDIA_SESSION_FOLDER);
  if(!rev) {
    // Create the directory with 755 permissions
    if (mkdir(MEDIA_SESSION_FOLDER.c_str(), 0755) == -1) {
      PMLOG_ERROR(CONST_MODULE_MCS,"%s failed to create %s", __FUNCTION__, MEDIA_SESSION_FOLDER.c_str());
    }
  } else {
    if (deleteAllFilesInDirectory(MEDIA_SESSION_FOLDER)) {
      PMLOG_INFO(CONST_MODULE_MCS,"%s All files deleted successfully.", __FUNCTION__);
    } else {
      PMLOG_INFO(CONST_MODULE_MCS,"%s Failed to delete all files.", __FUNCTION__);
    }
  }

  // run the gmainloop
  g_main_loop_run(main_loop_ptr_.get());
}

bool MediaControlService::onBTServerStatusCb(LSHandle *lshandle, LSMessage *message, void *ctx) {
  PMLOG_INFO(CONST_MODULE_MCS,"%s IN", __FUNCTION__);

  LS::Message response(message);
  pbnjson::JValue payload = pbnjson::JDomParser::fromString(response.getPayload());
  if(payload.isNull())
    return true;

  bool connected = payload["connected"].asBool();
  if(connected) {
    PMLOG_INFO(CONST_MODULE_MCS, "%s BT service running", __FUNCTION__);
    MediaControlService *ptrService = static_cast<MediaControlService*>(ctx);
    if(ptrService) {
      // get BT to get connected devices
      ptrService->subscribeToBTAdapterGetStatus();
    }
  }
  return true;
}

void MediaControlService::subscribeToBTAdapterGetStatus() {
  PMLOG_INFO(CONST_MODULE_MCS,"%s IN", __FUNCTION__);
  CLSError lserror;

  if(!LSCall(lsHandle_,
             cstrBTAdapterGetStatus.c_str(),
             cstrSubscribe.c_str(),
             &MediaControlService::onBTAdapterQueryCb,
             this, NULL, &lserror))
    PMLOG_ERROR(CONST_MODULE_MCS,"%s LS Call error for BT adapter/GetStatus", __FUNCTION__);
}

bool MediaControlService::onBTAdapterQueryCb(LSHandle *lshandle, LSMessage *message, void *ctx) {
  LSMessageJsonParser msg(message,SCHEMA_5(REQUIRED(subscribed, boolean), \
  REQUIRED(adapters, array),REQUIRED(returnValue, boolean),REQUIRED \
  (errorCode,integer),REQUIRED(errorText, string)));

  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS,"%s parse failure", __FUNCTION__);
    return true;
  }
  bool returnValue = false;
  msg.get("returnValue", returnValue);
  if (returnValue) {
    std::string deviceSetId;
    pbnjson::JValue adapterStatus = msg.get();
    pbnjson::JValue adapters = adapterStatus["adapters"];
    if (!adapters.isArray()) {
      PMLOG_ERROR(CONST_MODULE_MCS,"%s no adapters available", __FUNCTION__);
      return true;
    }

    if (0 == adapters.arraySize()) {
      PMLOG_ERROR(CONST_MODULE_MCS,"%s adapters empty array",__FUNCTION__);
      return true;
    }

    CLSError lserror;
    MediaControlService *obj = static_cast<MediaControlService *>(ctx);
    for (int i = 0; i < adapters.arraySize(); i++) {
      std::string adapterName = adapters[i]["name"].asString();
      std::string adapterAddress = adapters[i]["adapterAddress"].asString();
      PMLOG_INFO(CONST_MODULE_MCS,"%s : adapterName : %s, adapterAddress : %s",
                                   __FUNCTION__, adapterName.c_str(), adapterAddress.c_str());
      deviceSetId = "RSE-L";
      std::string payloadL = "{\"adapterAddress\":\"" + adapterAddress + "\",\"subscribe\":true}";
      PMLOG_INFO(CONST_MODULE_MCS,"%s : payload = %s for first adapter",__FUNCTION__, payloadL.c_str());
      if(obj) {
        if (!LSCall(obj->lsHandle_,
                    cstrBTDeviceGetStatus.c_str(),
                    payloadL.c_str(),
                    &onBTDeviceGetStatusCbWrapper,
                    ctx, NULL, &lserror))
          PMLOG_ERROR(CONST_MODULE_MCS,"%s : LSCall failed for BT device/getStatus", __FUNCTION__);
      }
#if defined(FEATURE_DUAL_DISPLAY)
      deviceSetId = "RSE-R";
      std::string payloadR = "{\"adapterAddress\":\"" + adapterAddress + "\",\"subscribe\":true}";
      PMLOG_INFO(CONST_MODULE_MCS,"%s : payload = %s for second adapter",__FUNCTION__, payloadR.c_str());
      if(obj) {
        if (!LSCall(obj->lsHandle_,
                    cstrBTDeviceGetStatus.c_str(),
                    payloadR.c_str(),
                    &MediaControlService::onBTDeviceGetStatusCb,
                    ctx, NULL, &lserror))
          PMLOG_ERROR(CONST_MODULE_MCS,"%s : LSCall failed for BT device/getStatus", __FUNCTION__);
      }
#endif
    PMLOG_INFO(CONST_MODULE_MCS,"%s : adapterAddress = %s deviceSetId = %s [%d]",__FUNCTION__, adapterAddress.c_str(), deviceSetId.c_str(), __LINE__);
    if (obj && !deviceSetId.empty() && !adapterAddress.empty())
      obj->ptrMediaControlPrivate_->setBTAdapterInfo(deviceSetId, adapterAddress);
    }
  }
  return true;
}

bool MediaControlService::onBTDeviceGetStatusCb(LSHandle *lshandle, LSMessage *message, void *ctx) {
  LSMessageJsonParser msg(message,SCHEMA_6(REQUIRED(subscribed, boolean), \
  REQUIRED(adapterAddress, string),REQUIRED(returnValue, boolean),REQUIRED \
  (devices,array),REQUIRED(errorCode, integer),REQUIRED(errorText, string)));

  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    return true;
  }

  bool returnValue = false;
  msg.get("returnValue", returnValue);

  if(returnValue) {
    pbnjson::JValue deviceStatus = msg.get();
    pbnjson::JValue devices = deviceStatus["devices"];
    if (!devices.isArray()) {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s Empty device array", __FUNCTION__);
      return true;
    }
    if (0 == devices.arraySize()) {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s devices arraySize 0", __FUNCTION__);
      return true;
    }
    for(int i = 0; i < devices.arraySize(); i++) {
      pbnjson::JValue connectedProfiles = devices[i]["connectedProfiles"];
      if (0 == connectedProfiles.arraySize()) continue;
      for (int j = 0; j < connectedProfiles.arraySize(); j++) {
        std::string profile = connectedProfiles[j].asString();
        PMLOG_INFO(CONST_MODULE_MCS, "%s profile : %s", __FUNCTION__, profile.c_str());
        if ("avrcp" == profile ) {
          std::string address = devices[i]["address"].asString();
          std::string adapterAddress = devices[i]["adapterAddress"].asString();
          std::string payload = "{\"adapterAddress\":\"" + adapterAddress + "\",\"address\":\"" + address + "\",\"subscribe\":true}";
          PMLOG_INFO(CONST_MODULE_MCS, "%s payload : %s", __FUNCTION__, payload.c_str());
          CLSError lserror;
          MediaControlService *obj = static_cast<MediaControlService *>(ctx);
          if(obj) {
            if (!LSCall(obj->lsHandle_,
                        cstrBTAvrcpGetStatus.c_str(),
                        payload.c_str(),
                        &MediaControlService::onBTAvrcpGetStatusCb,
                        ctx, NULL, &lserror))
              PMLOG_ERROR(CONST_MODULE_MCS, "%s LSCall failed to avrcp/getStatus", __FUNCTION__);
            break;
          }
        }
      }
    }
  }
  return true;
}

bool MediaControlService::onBTAvrcpGetStatusCb(LSHandle *lshandle, LSMessage *message, void *ctx) {
  LSMessageJsonParser msg(message,SCHEMA_9(REQUIRED(subscribed, boolean), \
  REQUIRED(adapterAddress, string),REQUIRED(returnValue, boolean),REQUIRED \
  (connecting, boolean),REQUIRED(connected, boolean),REQUIRED(playing, boolean), \
  REQUIRED(address, string),REQUIRED(errorCode, integer),REQUIRED(errorText, string)));

  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s parsing failed", __FUNCTION__);
    return true;
  }

  bool returnValue = false;
  msg.get("returnValue", returnValue);
  if (returnValue) {
    std::string address;
    msg.get("address", address);
    if(nullptr == address.c_str()) {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s Device MAC address field not found", __FUNCTION__);
      return true;
    }
    bool connected = false;
    msg.get("connected", connected);
    std::string adapterAddress;
    msg.get("adapterAddress", adapterAddress);
    PMLOG_INFO(CONST_MODULE_MCS, "%s Device MAC address %s Connection Status %d adapter address %s",\
                                  __FUNCTION__, address.c_str(), connected, adapterAddress.c_str());
    BTConnected_ = connected;
    if (connected) {
      //subscribe to bt key events
      MediaControlService *obj = static_cast<MediaControlService *>(ctx);
      if(obj) {

        if(!(obj->ptrMediaControlPrivate_->isDeviceRegistered(address, adapterAddress))) {
          int displayId = obj->ptrMediaControlPrivate_->getDisplayIdForBT(adapterAddress);
#if !defined(FEATURE_DUAL_DISPLAY)
          displayId = 0;
#endif
          BTDeviceInfo objDevInfo(address, adapterAddress, "", displayId);
          PMLOG_ERROR(CONST_MODULE_MCS, "%s Device MAC address field not found", __FUNCTION__);

          //save the details of BT device connected
          obj->ptrMediaControlPrivate_->setBTDeviceInfo(objDevInfo);

          std::string payload = "{\"address\":\"" + address + "\",\"adapterAddress\":\"" + adapterAddress + "\",\"subscribe\":true}";
          PMLOG_INFO(CONST_MODULE_MCS, "%s payload : %s", __FUNCTION__, payload.c_str());
          CLSError lserror;
          if (!LSCall(obj->lsHandle_,
                      cstrBTAvrcpReceivePassThroughCommand.c_str(),
                      payload.c_str(),
                      &MediaControlService::onBTAvrcpKeyEventsCb,
                      ctx, NULL, &lserror))
           PMLOG_ERROR(CONST_MODULE_MCS,"%s LSCall failed to avrcp/receivePassThroughCommand", __FUNCTION__);
        }
      }
    }
  }
  return true;
}

bool MediaControlService::onGetSessionsInfoCb(LSHandle *lshandle, LSMessage *message, void *ctx) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s ", __FUNCTION__);
  LSMessageJsonParser msg(message, SCHEMA_ANY);
  if (!msg.parse(__func__))
        return false;
  bool returnValue = false;
  msg.get("returnValue", returnValue);
  if (returnValue) {
    pbnjson::JValue payload = msg.get();
    pbnjson::JValue sessionInfo = payload["sessions"];
    if (!sessionInfo.isArray()) {
      PMLOG_ERROR(CONST_MODULE_MCS,"%s LSCall failed to onGetSessionsInfoCb", __FUNCTION__);
      return false;
    } else {
        MediaControlService *obj = static_cast<MediaControlService *>(ctx);
        for (const auto& items : sessionInfo.items()) {
          if (items.hasKey("deviceSetInfo")) {
            int displayId;
            std::string deviceSetId;
            pbnjson::JValue deviceSetInfo = items["deviceSetInfo"];
            displayId = deviceSetInfo["displayId"].asNumber<int>();
            deviceSetId = deviceSetInfo["deviceSetId"].asString();
            BTDeviceInfo btInfo("", "", std::move(deviceSetId), displayId);
            obj->ptrMediaControlPrivate_->setSessionListInfo(btInfo);
          }
        }
      }
    }
  return true;
}

bool MediaControlService::onBTAvrcpKeyEventsCb(LSHandle *lshandle, LSMessage *message, void *ctx) {
  LSMessageJsonParser msg(message,SCHEMA_8(REQUIRED(subscribed, boolean), \
  REQUIRED(adapterAddress, string),REQUIRED(returnValue, boolean),REQUIRED \
  (keyCode, string),REQUIRED(keyStatus, string),REQUIRED(address, string), \
  REQUIRED(errorCode, integer),REQUIRED(errorText, string)));

  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s parsing failed", __FUNCTION__);
    return true;
  }

  bool returnValue = false;
  msg.get("returnValue", returnValue);

  if(returnValue) {
    std::string address;
    msg.get("address", address);
    std::string adapterAddress;
    msg.get("adapterAddress", adapterAddress);
    std::string keyCode;
    msg.get("keyCode", keyCode);
    std::string keyStatus;
    msg.get("keyStatus",keyStatus);
    PMLOG_INFO(CONST_MODULE_MCS, "%s address = %s, adapterAddress = %s, keyCode = %s, keyStatus = %s",
                                  __FUNCTION__, address.c_str(), adapterAddress.c_str(), keyCode.c_str(), keyStatus.c_str());

    if(keyStatus == "released") {
      MediaControlService *obj = static_cast<MediaControlService *>(ctx);
      if(obj) {
        //get latest client from media session manager
        int displayIdForBT = obj->ptrMediaControlPrivate_->getDisplayIdForBT(adapterAddress);
        std::string mediaId = obj->ptrMediaControlPrivate_->getMediaId(address);
        int displayIdForMedia = obj->ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
        //ToDo : Below platform check to be removed once multi intsnace support available in chromium for OSE
#if !defined(FEATURE_DUAL_DISPLAY)
        displayIdForBT = displayIdForMedia = 0;
#endif
        PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId for sending BT key event : %s", __FUNCTION__, mediaId.c_str());
        PMLOG_INFO(CONST_MODULE_MCS, "%s displayIdForBT = %d displayIdForMedia = %d", __FUNCTION__, displayIdForBT, displayIdForMedia);
        //post key event to application
        if(!mediaId.empty() && (displayIdForBT == displayIdForMedia)) {
          pbnjson::JValue responseObj = pbnjson::Object();
          responseObj.put("keyEvent", keyCode);
          responseObj.put("mediaId", mediaId);
          responseObj.put("returnValue", true);
          responseObj.put("subscribed", true);
          PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responseObj.stringify().c_str());
          CLSError lserror;
          if (!LSSubscriptionReply(obj->lsHandle_,"registerMediaSession" , responseObj.stringify().c_str(), &lserror))
            PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
        }
      }
    }
  }
  return true;
}

bool MediaControlService::registerMediaSession(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_3(REQUIRED(mediaId, string),REQUIRED( \
  appId, string),REQUIRED(subscribe, boolean)) REQUIRED_3(mediaId, appId, subscribe)));

  LS::Message request(&message);
  bool returnValue = false;
  bool subscribed = false;
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  pbnjson::JObject responseObj;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    responseObj.put("subscribed", subscribed);
    responseObj.put("returnValue", returnValue);
    responseObj.put("errorCode", errorCode);
    responseObj.put("errorText", errorText);
    request.respond(responseObj.stringify().c_str());
    return true;
  }

  pbnjson::JValue payload = msg.get();
  subscribed = payload["subscribe"].asBool();
  std::string mediaId = payload["mediaId"].asString();
  std::string appId = payload["appId"].asString();
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s appId : %s subscribed : %d",
                                __FUNCTION__, mediaId.c_str(), appId.c_str(), subscribed);

  CLSError lserror;
  if (LSMessageIsSubscription(&message)) {
    if (!LSSubscriptionAdd(lsHandle_, "registerMediaSession", &message, &lserror)) {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s LSSubscriptionAdd failed ",__FUNCTION__);
      subscribed = false;
      errorCode = MCS_ERROR_REGISTERSESSION_FAILED;
      errorText = CSTR_REGISTERSESSION_FAILED;
      goto reply;
    }
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s Subscribed successfully, add Media Session", __FUNCTION__);
  if(ptrMediaSessionMgr_) {
    errorCode = ptrMediaSessionMgr_->addMediaSession(mediaId, appId);
    if(MCS_ERROR_NO_ERROR == errorCode)
      returnValue = true;
  }

  reply:
  responseObj.put("subscribed", subscribed);
  responseObj.put("returnValue", returnValue);
  if(!returnValue) {
    errorText = getErrorTextFromErrorCode(errorCode);
    responseObj.put("errorCode", errorCode);
    responseObj.put("errorText", errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, responseObj.stringify().c_str());
  request.respond(responseObj.stringify().c_str());

  return true;
}

bool MediaControlService::unregisterMediaSession(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_1(REQUIRED(mediaId, string)) REQUIRED_1(mediaId)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  std::string mediaId;
  msg.get("mediaId", mediaId);
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->removeMediaSession(mediaId);

  if(MCS_ERROR_NO_ERROR == errorCode)
    response = createJsonReplyString(true);
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::activateMediaSession(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_1(REQUIRED(mediaId, string)) REQUIRED_1(mediaId)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  std::string mediaId;
  msg.get("mediaId", mediaId);
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->activateMediaSession(mediaId);

  if(MCS_ERROR_NO_ERROR == errorCode)
    response = createJsonReplyString(true);
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::deactivateMediaSession(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_1(REQUIRED(mediaId, string)) REQUIRED_1(mediaId)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  std::string mediaId;
  msg.get("mediaId", mediaId);
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->deactivateMediaSession(mediaId);

  if(MCS_ERROR_NO_ERROR == errorCode)
    response = createJsonReplyString(true);
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::getMediaMetaData(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_1(REQUIRED(mediaId, string)) REQUIRED_1(mediaId)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  std::string mediaId;
  msg.get("mediaId", mediaId);
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  mediaMetaData objMetaData;
  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->getMediaMetaData(mediaId, objMetaData);

  if(MCS_ERROR_NO_ERROR == errorCode) {
    pbnjson::JObject metaDataObj;
    metaDataObj.put("title", objMetaData.getTitle());
    metaDataObj.put("artist", objMetaData.getArtist());
    metaDataObj.put("totalDuration", objMetaData.getDuration());
    metaDataObj.put("album", objMetaData.getAlbum());
    metaDataObj.put("genre", objMetaData.getGenre());
    metaDataObj.put("trackNumber", objMetaData.getTrackNumber());
    metaDataObj.put("volume", objMetaData.getVolume());

    pbnjson::JObject responseObj;
    responseObj.put("returnValue", true);
    responseObj.put("metaData", metaDataObj);
    response = responseObj.stringify();
  }
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::setSupportedActions(LSMessage& message) {
  PMLOG_DEBUG("%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(mediaId, string), \
  REQUIRED(supportedActions, array)) REQUIRED_2(mediaId, supportedActions)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;

  if (!msg.parse(__FUNCTION__)) {
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return false;
  }

  pbnjson::JValue payload = msg.get();
  std::string mediaId = payload["mediaId"].asString();
  pbnjson::JValue actions = payload["supportedActions"];

  std::vector<std::string> enableActions;

  if (!actions.isArray() || actions.arraySize() <= 0) {
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return false;
  }

  for (int i = 0; i < actions.arraySize(); i++) {
     if(!actions[i].isString() || actions[i].asString() == "") {
       errorCode = MCS_ERROR_PARSING_FAILED;
       sendErrorResponse(errorCode, request);
       return false;
     }
     enableActions.push_back(actions[i].asString());
  }

  if (ptrMediaSessionMgr_ == nullptr) {
    errorCode = MCS_ERROR_NO_ACTIVE_SESSION;
    sendErrorResponse(errorCode, request);
    return true;
  }

  errorCode = ptrMediaSessionMgr_->setMediaAction(mediaId, enableActions);
  if (MCS_ERROR_NO_ERROR != errorCode) {
    sendErrorResponse(errorCode, request);
    return true;
  }

  if (ptrMediaControlPrivate_->enableMediaAction_) {
    // Create response to share cover art details to subscribed client
    pbnjson::JValue responsePayload = pbnjson::Object();
    responsePayload.put("displayId", dispId);

    mediaSession objMediaSession;
    errorCode = ptrMediaSessionMgr_->getMediaSessionInfo(mediaId, objMediaSession);
    if (errorCode != MCS_ERROR_NO_ERROR) {
      sendErrorResponse(errorCode, request);
      return true;
    }

    responsePayload.put("mediaId", mediaId);
    responsePayload.put("appId", objMediaSession.getAppId());

    responsePayload.put("supportedActions", actions);
    responsePayload.put("returnValue", true);

    PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
    /*Reply coverArt details to receiveMediaPlaybackInfo*/
    CLSError lserror;
    if (!LSSubscriptionReply(lsHandle_, "receiveMediaPlaybackInfo", responsePayload.stringify().c_str(), &lserror)) {
      errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
      sendErrorResponse(errorCode, request);
      return true;
    }
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, createJsonReplyString(true).c_str());
  request.respond(createJsonReplyString(true).c_str());

  return true;
}

bool MediaControlService::getMediaPlayStatus(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_1(REQUIRED(mediaId, string)) REQUIRED_1(mediaId)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  std::string mediaId;
  msg.get("mediaId", mediaId);
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  std::string playStatus;
  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->getMediaPlayStatus(mediaId, playStatus);

  if(MCS_ERROR_NO_ERROR == errorCode) {
    pbnjson::JObject responseObj;
    responseObj.put("returnValue", true);
    responseObj.put("playStatus", playStatus);
    response = responseObj.stringify();
  }
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, MCS_ERROR_INVALID_MEDIAID, CSTR_INVALID_MEDIAID);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::getMediaSessionInfo(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_1(REQUIRED(mediaId, string)) REQUIRED_1(mediaId)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  std::string mediaId;
  msg.get("mediaId", mediaId);
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  mediaSession objMediaSession;
  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->getMediaSessionInfo(mediaId, objMediaSession);

  if(MCS_ERROR_NO_ERROR == errorCode) {
    mediaMetaData objMetaData = objMediaSession.getMediaMetaDataObj();
    pbnjson::JObject metaDataObj;
    metaDataObj.put("title", objMetaData.getTitle());
    metaDataObj.put("artist", objMetaData.getArtist());
    metaDataObj.put("totalDuration", objMetaData.getDuration());
    metaDataObj.put("album", objMetaData.getAlbum());
    metaDataObj.put("genre", objMetaData.getGenre());
    metaDataObj.put("trackNumber", objMetaData.getTrackNumber());
    metaDataObj.put("volume", objMetaData.getVolume());

    pbnjson::JObject sessionInfoObj;
    sessionInfoObj.put("mediaId", objMediaSession.getMediaId());
    sessionInfoObj.put("appId", objMediaSession.getAppId());
    sessionInfoObj.put("metaData", metaDataObj);

    pbnjson::JObject responseObj;
    responseObj.put("returnValue", true);
    responseObj.put("sessionInfo", sessionInfoObj);
    response = responseObj.stringify();
  }
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());
  return true;
}

bool MediaControlService::getMediaSessionId(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_1(REQUIRED(appId, string)) REQUIRED_1(appId)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  std::string appId;
  msg.get("appId", appId);
  PMLOG_INFO(CONST_MODULE_MCS, "%s appId : %s", __FUNCTION__, appId.c_str());
  //get media list from MSM
  std::vector<std::string> strMediaSessionIdList;
  if(ptrMediaSessionMgr_)
    strMediaSessionIdList = ptrMediaSessionMgr_->getMediaSessionList(appId);

  bool returnValue = false;
  if(strMediaSessionIdList.size())
    returnValue = true;

  if(returnValue) {
    pbnjson::JArray mediaSessionIdArray;
    for(const auto& itr : strMediaSessionIdList)
      mediaSessionIdArray.append(itr);

    pbnjson::JObject responseObj;
    responseObj.put("returnValue", returnValue);
    responseObj.put("mediaId", mediaSessionIdArray);
    response = responseObj.stringify();
  }
  else
    response = createJsonReplyString(returnValue, MCS_ERROR_INVALID_APPID, CSTR_INVALID_APPID);

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::getActiveMediaSessions(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,SCHEMA_0);

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }
  //get active sessions list from MSM
  std::vector<std::string> strActiveMediaSessionList;
  if(ptrMediaSessionMgr_)
    strActiveMediaSessionList = ptrMediaSessionMgr_->getActiveMediaSessionList();

  bool returnValue = false;
  if(strActiveMediaSessionList.size())
    returnValue = true;

  if(returnValue) {
    pbnjson::JArray activeMediaSessionArray;
    for(const auto& itr : strActiveMediaSessionList)
      activeMediaSessionArray.append(itr);

    pbnjson::JObject responseObj;
    responseObj.put("returnValue", returnValue);
    responseObj.put("sessionList", activeMediaSessionArray);
    response = responseObj.stringify();
  }
  else
    response = createJsonReplyString(returnValue, MCS_ERROR_NO_ACTIVE_SESSION, CSTR_NO_ACTIVE_SESSION);

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::getMediaCoverArtPath(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_3(REQUIRED(displayId, integer), \
    OPTIONAL(subscribe, boolean),REQUIRED(src, array)) REQUIRED_2(displayId, src)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;

  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return false;
  }

  response = createJsonReplyString(true);
  bool subscribed = false;
  pbnjson::JValue payload = msg.get();
  int displayId  = payload["displayId"].asNumber<int>();
  pbnjson::JValue srcs = payload["src"];
  if(payload.hasKey("subscribe"))
    subscribed = payload["subscribe"].asBool();

  std::vector<std::string> sources;

  if (!srcs.isArray() || srcs.arraySize() <= 0) {
    PMLOG_ERROR(CONST_MODULE_MCS,"%s coverArt source is empty", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response  = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return false;
  }

  for (int i = 0; i < srcs.arraySize(); i++) {
     if(!srcs[i].isString() || srcs[i].asString() == "") {
       PMLOG_ERROR(CONST_MODULE_MCS,"%s coverArt source is not correct", __FUNCTION__);
       errorCode = MCS_ERROR_PARSING_FAILED;
       errorText = CSTR_PARSING_ERROR;
       response  = createJsonReplyString(false, errorCode, errorText);
       request.respond(response.c_str());
       return false;
     }
     sources.push_back(srcs[i].asString());
  }

#if !defined(FEATURE_DUAL_DISPLAY)
      displayId = 0;
#endif

  CLSError lserror;
  if (LSMessageIsSubscription(&message)) {
    if (!LSSubscriptionAdd(lsHandle_, "getMediaCoverArtPath", &message, &lserror)) {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s LSSubscriptionAdd failed ",__FUNCTION__);
      errorCode = MCS_ERROR_REGISTERSESSION_FAILED;
      errorText = CSTR_REGISTERSESSION_FAILED;
      response = createJsonReplyString(false, errorCode, errorText);
      request.respond(response.c_str());
      return false;
    }
  }

  std::string target = "";

  std::string mediaId = ptrMediaSessionMgr_->getMediaIdFromDisplayId(displayId);

  ptrMediaSessionMgr_->setLSHandle(lsHandle_);
  errorCode = MCS_ERROR_NO_ERROR;
  errorCode = ptrMediaSessionMgr_->coverArtDownload(mediaId, std::move(sources));

  if(errorCode != MCS_ERROR_NO_ERROR)
  {
    pbnjson::JObject responseObj;
    errorText = CSTR_NO_ACTIVE_SESSION;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return false;
  }

  pbnjson::JValue coverArtArray = pbnjson::Array();
  for(int i = 0; i < srcs.arraySize(); i++) {
    pbnjson::JValue coverArtSrcInfo = pbnjson::Object();
    coverArtSrcInfo.put("src", srcs[i].asString());

    std::string targetPath = COVERART_FILE_PATH + extractFilenameFromUrl(srcs[i].asString());
    PMLOG_INFO(CONST_MODULE_MCS, "tagetPath : %s", targetPath.c_str());
    coverArtSrcInfo.put("srcPath", targetPath);
    coverArtArray.append(coverArtSrcInfo);
  }

  pbnjson::JObject responseObj;
  responseObj.put("coverArtPathInfo", coverArtArray);
  responseObj.put("subscribed", subscribed);
  responseObj.put("returnValue", true);

  response = responseObj.stringify();
  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::setMediaMetaData(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(OBJECT(mediaMetaData, OBJSCHEMA_7( \
  OPTIONAL(title, string),OPTIONAL(artist, string),OPTIONAL(totalDuration, string),OPTIONAL( \
  album, string),OPTIONAL(genre, string),OPTIONAL(trackNumber, integer),OPTIONAL(volume, integer) \
  )),REQUIRED(mediaId, string)) REQUIRED_2(mediaMetaData, mediaId)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;

  if (!msg.parse(__FUNCTION__)) {
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return false;
  }

  pbnjson::JValue payload = msg.get();
  std::string mediaId = payload["mediaId"].asString();
 /* extract mediaMetaData info */
  pbnjson::JValue metaData = payload["mediaMetaData"];
  std::string title        = metaData["title"].asString();
  std::string artist       = metaData["artist"].asString();
  std::string duration     = metaData["totalDuration"].asString();
  std::string album        = metaData["album"].asString();
  std::string genre        = metaData["genre"].asString();
  int trackNumber          =  metaData["trackNumber"].asNumber<int>();
  int volume               = metaData["volume"].asNumber<int>();
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s title : %s artist : %s duration : %s album : %s \
                                genre : %s trackNumber : %d volume : %d", __FUNCTION__, mediaId.c_str(),
                                title.c_str(), artist.c_str(), duration.c_str(), album.c_str(), genre.c_str(),
                                trackNumber, volume);

  mediaMetaData objMetaData(title, artist, duration, album, genre, trackNumber, volume);

  if (ptrMediaSessionMgr_ == nullptr) {
    errorCode = MCS_ERROR_NO_ACTIVE_SESSION;
    sendErrorResponse(errorCode, request);
    return true;
  }


  errorCode = ptrMediaSessionMgr_->setMediaMetaData(mediaId, objMetaData);
  if (MCS_ERROR_NO_ERROR != errorCode) {
    sendErrorResponse(errorCode, request);
    return true;
  }

  if (ptrMediaControlPrivate_->mediaMetaData_) {
    /*Get display ID from media ID*/
    int displayId = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
    //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if !defined(FEATURE_DUAL_DISPLAY)
    displayId = 0;
#endif
    pbnjson::JObject metaDataObj;
    errorCode = updateMetaDataResponse(mediaId, metaDataObj);
    pbnjson::JValue responsePayload = pbnjson::Object();
    responsePayload.put("mediaMetaData", metaDataObj);
    responsePayload.put("displayId", displayId);
    responsePayload.put("eventType", "mediaMetaData");

    mediaSession objMediaSession;
    errorCode = ptrMediaSessionMgr_->getMediaSessionInfo(mediaId, objMediaSession);
    if (errorCode != MCS_ERROR_NO_ERROR) {
      sendErrorResponse(errorCode, request);
      return true;
    }

    responsePayload.put("mediaId", mediaId);
    responsePayload.put("appId", objMediaSession.getAppId());

    responsePayload.put("returnValue", true);
    responsePayload.put("subscribed", true);
    PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
    /*Add subscription for receiveMediaPlaybackInfo*/
    CLSError lserror;
    if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)){
      errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
      sendErrorResponse(errorCode, request);
      return true;
    }
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, createJsonReplyString(true).c_str());
  request.respond(createJsonReplyString(true).c_str());

  return true;
}

bool MediaControlService::setMediaPlayStatus(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(mediaId, string), \
  REQUIRED(playStatus, string)) REQUIRED_2(mediaId, playStatus)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;

  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return false;
  }

  pbnjson::JValue payload = msg.get();
  std::string mediaId = payload["mediaId"].asString();
  std::string playStatus = payload["playStatus"].asString();

  if (ptrMediaSessionMgr_ == nullptr) {
    errorCode = MCS_ERROR_NO_ACTIVE_SESSION;
    sendErrorResponse(errorCode, request);
    return true;
  }

  /*  set play status in MSM  */
  errorCode = ptrMediaSessionMgr_->setMediaPlayStatus(mediaId, playStatus);
  if (MCS_ERROR_NO_ERROR != errorCode) {
    sendErrorResponse(errorCode, request);
    return true;
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s BTConnected_ value  = [%d]", __FUNCTION__, BTConnected_);
  /*The below code under if() condition will be removed once BT service will subscribe to MCS API
    for recieving the mediaPlayback status information. This is a temporary fix, until BT
    service implements the subscription call */
  if (BTConnected_) {
    std::string sendPlaybackStatus = "error";

    std::map<std::string, std::string> playStatusMap = {
      {"PLAYSTATE_STOPPED", "stopped"},
      {"PLAYSTATE_PAUSED", "paused"},
      {"PLAYSTATE_PLAYING", "playing"},
      {"PLAYSTATE_FAST_FORWARDING", "fwd_seek"},
      {"PLAYSTATE_REWINDING", "rev_seek"},
      {"PLAYSTATE_ERROR", "error"}
    };

    auto it = playStatusMap.find(playStatus);
    if (it != playStatusMap.end()) {
      sendPlaybackStatus = it->second;
    } else {
      PMLOG_INFO(CONST_MODULE_MCS, "%s Invalid PlaybackStatus", __FUNCTION__);
    }

    int displayIdForMedia = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
#if !defined(FEATURE_DUAL_DISPLAY)
    displayIdForMedia = 0;
#endif
    BTDeviceInfo objDevInfo;
    if (ptrMediaControlPrivate_->getBTDeviceInfo(displayIdForMedia, &objDevInfo)) {
      std::string adapterAddress;
      std::string address;
      adapterAddress = objDevInfo.adapterAddress_;
      address = objDevInfo.deviceAddress_;
      CLSError lserror;
      PMLOG_INFO(CONST_MODULE_MCS, "%s displayId = %d adapterAddress = %s, address = %s sendPlaybackStatus = %s",
                                 __FUNCTION__,displayIdForMedia, adapterAddress.c_str(), address.c_str(), sendPlaybackStatus.c_str());

      pbnjson::JObject playStatusObj;
      playStatusObj.put("duration",0);
      playStatusObj.put("position",0);
      playStatusObj.put("status",sendPlaybackStatus);

      pbnjson::JObject responseObj;
      responseObj.put("adapterAddress", adapterAddress);
      responseObj.put("address", address);
      responseObj.put("playbackStatus", playStatusObj);
      std::string payloadData;
      payloadData = responseObj.stringify();

      if (!LSCallOneReply(lsHandle_, cstrBTNotifyMediaPlayStatus.c_str(), payloadData.c_str(), NULL, NULL, NULL, &lserror)) {
        PMLOG_ERROR(CONST_MODULE_MCS,"%s LSCall failed to avrcp/notifyMediaPlayStatus", __FUNCTION__);
      }
    }
  }

  if (ptrMediaControlPrivate_->playStatus_) {
    /*Get display ID from media ID*/
    int displayId = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
    //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if !defined(FEATURE_DUAL_DISPLAY)
    displayId = 0;
#endif
    pbnjson::JValue responsePayload = pbnjson::Object();
    responsePayload.put("playStatus", playStatus);
    responsePayload.put("displayId", displayId);

    mediaSession objMediaSession;
    errorCode = ptrMediaSessionMgr_->getMediaSessionInfo(mediaId, objMediaSession);
    if (errorCode != MCS_ERROR_NO_ERROR) {
      sendErrorResponse(errorCode, request);
      return true;
    }
    responsePayload.put("mediaId", mediaId);
    responsePayload.put("appId", objMediaSession.getAppId());

    responsePayload.put("eventType", "playStatus");
    responsePayload.put("returnValue", true);
    responsePayload.put("subscribed", true);
    PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
    /*Add subscription for receiveMediaPlaybackInfo*/
    CLSError lserror;
    if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)) {
      PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
      errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
      sendErrorResponse(errorCode, request);
      return true;
    }
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, createJsonReplyString(true).c_str());
  request.respond(createJsonReplyString(true).c_str());

  return true;
}

bool MediaControlService::setMediaMuteStatus (LSMessage & message){
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(mediaId, string), \
  REQUIRED(muteStatus, string)) REQUIRED_2(mediaId, muteStatus)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;

  if (!msg.parse(__FUNCTION__)) {
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return false;
  }

  pbnjson::JValue payload = msg.get();
  std::string mediaId = payload["mediaId"].asString();
  std::string muteStatus = payload["muteStatus"].asString();
  if(muteStatus == "MUTE")
    muteStatus = "mute";
  else if(muteStatus == "UNMUTE")
    muteStatus = "unmute";

  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s muteStatus : %s ",
                                __FUNCTION__, mediaId.c_str(), muteStatus.c_str());

  if (ptrMediaSessionMgr_ == nullptr) {
    errorCode = MCS_ERROR_NO_ACTIVE_SESSION;
    sendErrorResponse(errorCode, request);
    return true;
  }

  /*validate the MediaId*/
  errorCode = ptrMediaSessionMgr_->setMediaMuteStatus(mediaId, muteStatus);
  if (MCS_ERROR_NO_ERROR != errorCode) {
    sendErrorResponse(errorCode, request);
    return true;
  }

  if (ptrMediaControlPrivate_->muteStatus_) {
    /*Get display ID from media ID*/
    int displayId = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
    //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if !defined(FEATURE_DUAL_DISPLAY)
    displayId = 0;
#endif
    pbnjson::JValue responsePayload = pbnjson::Object();
    responsePayload.put("muteStatus", muteStatus);
    responsePayload.put("displayId", displayId);
    responsePayload.put("eventType", "muteStatus");

    mediaSession objMediaSession;
    errorCode = ptrMediaSessionMgr_->getMediaSessionInfo(mediaId, objMediaSession);
    if (errorCode != MCS_ERROR_NO_ERROR) {
      sendErrorResponse(errorCode, request);
      return true;
    }
    responsePayload.put("mediaId", mediaId);
    responsePayload.put("appId", objMediaSession.getAppId());

    responsePayload.put("returnValue", true);
    responsePayload.put("subscribed", true);
    PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
    /*LSSubscriptionReply for receiveMediaPlaybackInfo*/
    CLSError lserror;
    if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)) {
      errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
      sendErrorResponse(errorCode, request);
      return true;
    }
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, createJsonReplyString(true).c_str());
  request.respond(createJsonReplyString(true).c_str());

  return true;
}

bool MediaControlService::setMediaPlayPosition (LSMessage & message){
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(mediaId, string), \
  REQUIRED(playPosition, string)) REQUIRED_2(mediaId, playPosition)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;

  if (!msg.parse(__FUNCTION__)) {
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return false;
  }

  pbnjson::JValue payload = msg.get();
  std::string mediaId = payload["mediaId"].asString();
  std::string playPosition = payload["playPosition"].asString();
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s playPosition : %s ",
                                __FUNCTION__, mediaId.c_str(), playPosition.c_str());

  if (ptrMediaSessionMgr_ == nullptr) {
    errorCode = MCS_ERROR_NO_ACTIVE_SESSION;
    sendErrorResponse(errorCode, request);
    return true;
  }

  /*validate the MediaId*/
  errorCode = ptrMediaSessionMgr_->setMediaPlayPosition(mediaId, playPosition);
  if (MCS_ERROR_NO_ERROR != errorCode) {
    sendErrorResponse(errorCode, request);
    return true;
  }

  if (ptrMediaControlPrivate_->playPosition_) {
    /*Get display ID from media ID*/
    int displayId = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
    //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if !defined(FEATURE_DUAL_DISPLAY)
    displayId = 0;
#endif
    pbnjson::JValue responsePayload = pbnjson::Object();
    responsePayload.put("playPosition", playPosition);
    responsePayload.put("displayId", displayId);
    responsePayload.put("eventType", "playPosition");

    mediaSession objMediaSession;
    errorCode = ptrMediaSessionMgr_->getMediaSessionInfo(mediaId, objMediaSession);
    if (errorCode != MCS_ERROR_NO_ERROR) {
      sendErrorResponse(errorCode, request);
      return true;
    }
    responsePayload.put("mediaId", mediaId);
    responsePayload.put("appId", objMediaSession.getAppId());

    responsePayload.put("returnValue", true);
    responsePayload.put("subscribed", true);
    PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
    /*LSSubscriptionReply for receiveMediaPlaybackInfo*/
    CLSError lserror;
    if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)) {
      PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
      errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
      sendErrorResponse(errorCode, request);
      return true;
    }
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, createJsonReplyString(true).c_str());
  request.respond(createJsonReplyString(true).c_str());

  return true;
}

bool MediaControlService::receiveMediaPlaybackInfo (LSMessage & message){
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_3(REQUIRED(displayId, integer), \
    OPTIONAL(eventType, string),REQUIRED(subscribe, boolean)) REQUIRED_2(displayId, subscribe)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;

  if (!msg.parse(__FUNCTION__)) {
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return true;
  }

  pbnjson::JValue payload = msg.get();
  int displayId = payload["displayId"].asNumber<int>();

  if(displayId != 0 && displayId != 1)
  {
    errorCode = MCS_ERROR_INVALID_DISPLAYID;
    sendErrorResponse(errorCode, request);
    return true;
  }
  std::string eventType = payload["eventType"].asString();
  if(strcmp(eventType.c_str(), "playStatus") && strcmp(eventType.c_str(), "muteStatus")
     && strcmp(eventType.c_str(), "playPosition") && strcmp(eventType.c_str(), "mediaMetaData")
     && strcmp(eventType.c_str(), "coverArt") && strcmp(eventType.c_str(), "supportedActions")
     && !eventType.empty())
  {
    errorCode = MCS_ERROR_INVALID_EVENT;
    sendErrorResponse(errorCode, request);
    return true;
  }
      //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if !defined(FEATURE_DUAL_DISPLAY)
      displayId = 0;
#endif
  bool subscribed = payload["subscribe"].asBool();

  if(!strcmp(eventType.c_str(), "playStatus")){
    ptrMediaControlPrivate_->playStatus_ = true;
  }else if (!strcmp(eventType.c_str(), "muteStatus")){
    ptrMediaControlPrivate_->muteStatus_ = true;
  }else if (!strcmp(eventType.c_str(), "playPosition")){
    ptrMediaControlPrivate_->playPosition_ = true;
  }else if (!strcmp(eventType.c_str(), "mediaMetaData")){
    ptrMediaControlPrivate_->mediaMetaData_ = true;
  }else if (!strcmp(eventType.c_str(), "coverArt")){
    ptrMediaControlPrivate_->coverArt_ = true;
  }else if (!strcmp(eventType.c_str(), "supportedActions")){
    ptrMediaControlPrivate_->enableMediaAction_ = true;
  }else if (!strcmp(eventType.c_str(), CSTR_EMPTY.c_str())){
    ptrMediaControlPrivate_->playStatus_ = true;
    ptrMediaControlPrivate_->muteStatus_ = true;
    ptrMediaControlPrivate_->playPosition_ = true;
    ptrMediaControlPrivate_->mediaMetaData_ = true;
    ptrMediaControlPrivate_->coverArt_ = true;
    ptrMediaControlPrivate_->enableMediaAction_ = true;
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s displayId : %d subscribe : %d  eventType : %s",__FUNCTION__, displayId, subscribed, eventType.c_str());
  /*LSSubscriptionAdd for sendMediaMetaData*/
  if (LSMessageIsSubscription(&message)) {
    CLSError lserror;
    if (!LSSubscriptionAdd(lsHandle_, "receiveMediaPlaybackInfo", &message, &lserror)) {
        errorCode = MCS_ERROR_SUBSCRIPTION_FAILED;
        sendErrorResponse(errorCode, request);
        return true;
    }
  }

  if (ptrMediaSessionMgr_ == nullptr) {
    errorCode = MCS_ERROR_NO_ACTIVE_SESSION;
    sendErrorResponse(errorCode, request);
    return true;
  }
  /*get mediaId from displayId*/
  std::string mediaId = ptrMediaSessionMgr_->getMediaIdFromDisplayId(displayId);

  pbnjson::JValue responsePayload = pbnjson::Object();
  if(eventType == "playPosition" || eventType.empty()) {
    std::string playPosition;
    ptrMediaSessionMgr_->getMediaPlayPosition(mediaId, playPosition);
    responsePayload.put("playPosition", playPosition);
  }
  if(eventType == "playStatus" ||eventType.empty()) {
    std::string playStatus;
    ptrMediaSessionMgr_->getMediaPlayStatus(mediaId, playStatus);
    responsePayload.put("playStatus", playStatus);
  }
  if(eventType == "muteStatus" || eventType.empty()){
    std::string muteStatus;
    ptrMediaSessionMgr_->getMediaMuteStatus(mediaId, muteStatus);
    responsePayload.put("muteStatus", muteStatus);
  }
  if(eventType == "mediaMetaData" || eventType.empty()){
    pbnjson::JObject metaDataObj;
    updateMetaDataResponse(mediaId, metaDataObj);
    responsePayload.put("mediaMetaData", metaDataObj);
  }
  if(eventType == "coverArt" || eventType.empty()) {
    std::vector<mediaCoverArt> objCoverArt;
    pbnjson::JValue coverArtArray = pbnjson::Array();

    ptrMediaSessionMgr_->getMediaCoverArt(mediaId, objCoverArt);

    for (auto &element : objCoverArt) {
      pbnjson::JValue coverArtItem = pbnjson::Object();

      coverArtItem.put("src", element.getSource());
      coverArtItem.put("type", element.getType());

      std::vector<coverArtSize> sizes = element.getSize();

      pbnjson::JValue coverArtSizes = pbnjson::Array();
      for(auto &size : sizes) {
        pbnjson::JValue sizesObj = pbnjson::Object();

        sizesObj.put("width", size.width);
        sizesObj.put("height", size.height);
        coverArtSizes.append(sizesObj);
      }
      coverArtItem.put("sizes", coverArtSizes);
      coverArtArray.append(coverArtItem);
    }
    responsePayload.put("coverArt", coverArtArray);
  }
  if (eventType == "supportedActions" || eventType.empty()) {
    std::vector<std::string> objActionList;
    pbnjson::JValue actionListArray = pbnjson::Array();

    ptrMediaSessionMgr_->getActionList(mediaId, objActionList);

    for (auto &element : objActionList) {
      actionListArray.append(pbnjson::JValue(element));
    }
    responsePayload.put("supportedActions", actionListArray);
  }
  if(!eventType.empty())
    responsePayload.put("eventType", eventType);

  mediaSession objMediaSession;
  errorCode = ptrMediaSessionMgr_->getMediaSessionInfo(mediaId, objMediaSession);
  if(errorCode != MCS_ERROR_NO_ERROR) {
    PMLOG_INFO(CONST_MODULE_MCS, "SessionInfo not found for mediaId : %s", mediaId.c_str());
  }

  responsePayload.put("mediaId", mediaId);
  responsePayload.put("appId", objMediaSession.getAppId());

  responsePayload.put("displayId", displayId);
  responsePayload.put("returnValue", true);
  responsePayload.put("subscribed", true);

  PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
  /*LSSubscriptionReply for receiveMediaPlaybackInfo*/
  CLSError lserror;
  if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)){
    PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
    errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
    sendErrorResponse(errorCode, request);
    return true;
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, createJsonReplyString(true).c_str());
  request.respond(createJsonReplyString(true).c_str());

  return true;
}

bool MediaControlService::injectMediaKeyEvent(LSMessage &message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(displayId, integer), \
  REQUIRED(keyEvent, string)) REQUIRED_2(displayId, keyEvent)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  pbnjson::JValue payload = msg.get();
  int displayId  = payload["displayId"].asNumber<int>();
      //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if !defined(FEATURE_DUAL_DISPLAY)
      displayId = 0;
#endif
  std::string keyEvent = payload["keyEvent"].asString();
  PMLOG_INFO(CONST_MODULE_MCS, "%s displayId : %d keyEvent : %s ",
                                __FUNCTION__, displayId, keyEvent.c_str());

  pbnjson::JValue responsePayload = pbnjson::Object();
  /*get mediaId from displayId*/
  std::string mediaId = ptrMediaSessionMgr_->getMediaIdFromDisplayId(displayId);
  if(!mediaId.c_str()) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s get mediaId from displayId is invalid ", __FUNCTION__);
    errorCode = MCS_ERROR_NO_ACTIVE_SESSION;
    errorText = CSTR_NO_ACTIVE_SESSION;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }
  responsePayload.put("mediaId", mediaId);
  responsePayload.put("keyEvent", keyEvent);
  responsePayload.put("returnValue", true);
  responsePayload.put("subscribed", true);
  PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
  response = createJsonReplyString(true);
  /*LSSubscriptionReply for registerMediaSession*/
  CLSError lserror;
  if (!LSSubscriptionReply(lsHandle_,"registerMediaSession" , responsePayload.stringify().c_str(), &lserror)){
    PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
    errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
    errorText = CSTR_SUBSCRIPTION_REPLY_FAILED;
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::setMediaCoverArt(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(mediaId, string), \
  REQUIRED(coverArt, array)) REQUIRED_2(mediaId, coverArt)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;

  if (!msg.parse(__FUNCTION__)) {
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return false;
  }

  pbnjson::JValue payload = msg.get();
  std::string mediaId = payload["mediaId"].asString();

  PMLOG_INFO(CONST_MODULE_MCS,"mediaId : %s", mediaId.c_str());

  pbnjson::JValue coverArt = payload["coverArt"];
  if (!coverArt.isArray() || coverArt.arraySize() <= 0) {
    errorCode = MCS_ERROR_PARSING_FAILED;
    sendErrorResponse(errorCode, request);
    return false;
  }
  std::vector<mediaCoverArt> coverArtData;
  for (int i = 0; i < coverArt.arraySize(); i++) {
    if(!coverArt[i].hasKey("src") || !coverArt[i]["src"].isString())
    {
      errorCode = MCS_ERROR_PARSING_FAILED;
      sendErrorResponse(errorCode, request);
      return false;
    }
    std::string coverArtSrc  = coverArt[i]["src"].asString();

    if(coverArt[i].hasKey("type") && !coverArt[i]["type"].isString())
    {
      errorCode = MCS_ERROR_PARSING_FAILED;
      sendErrorResponse(errorCode, request);
      return false;
    }
    std::string coverArtType = coverArt[i]["type"].asString();

    std::vector<coverArtSize> sizes;
    pbnjson::JValue coverArtSizes = coverArt[i]["sizes"];
    if (coverArt[i].hasKey("sizes") && !coverArtSizes.isArray()) {
      errorCode = MCS_ERROR_PARSING_FAILED;
      sendErrorResponse(errorCode, request);
      return false;
    }
    for(int j = 0; j < coverArtSizes.arraySize(); j++) {
      if((coverArtSizes[j].hasKey("width") && !coverArtSizes[j]["width"].isNumber())
         || (coverArtSizes[j].hasKey("height") && !coverArtSizes[j]["height"].isNumber()))
      {
        errorCode = MCS_ERROR_PARSING_FAILED;
        sendErrorResponse(errorCode, request);
        return false;
      }
      coverArtSize size;
      size.width  = coverArtSizes[j]["width"].asNumber<int>();
      size.height = coverArtSizes[j]["height"].asNumber<int>();
      sizes.push_back(size);
    }

    PMLOG_INFO(CONST_MODULE_MCS, "%s Cover Art src : %s, type : %s", __FUNCTION__, coverArtSrc.c_str(), coverArtType.c_str());

    mediaCoverArt objCoverArt(coverArtSrc, coverArtType, std::move(sizes));
    coverArtData.push_back(std::move(objCoverArt));
  }

  if (ptrMediaSessionMgr_ == nullptr) {
    errorCode = MCS_ERROR_NO_ACTIVE_SESSION;
    sendErrorResponse(errorCode, request);
    return true;
  }

  errorCode = ptrMediaSessionMgr_->setMediaCoverArt(mediaId, coverArtData);
  if (MCS_ERROR_NO_ERROR != errorCode) {
    sendErrorResponse(errorCode, request);
    return true;
  }

  if (ptrMediaControlPrivate_->coverArt_) {
    //Create response to share cover art details to subscribed client
    pbnjson::JValue responsePayload = pbnjson::Object();
    responsePayload.put("displayId", 0);
    responsePayload.put("subscribed", true);
    responsePayload.put("coverArt", coverArt);
    responsePayload.put("eventType", "coverArt");

    mediaSession objMediaSession;
    errorCode = ptrMediaSessionMgr_->getMediaSessionInfo(mediaId, objMediaSession);
    if (errorCode != MCS_ERROR_NO_ERROR) {
      sendErrorResponse(errorCode, request);
      return true;
    }
    responsePayload.put("mediaId", mediaId);
    responsePayload.put("appId", objMediaSession.getAppId());

    responsePayload.put("returnValue", true);

    PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());

    /*Reply coverArt details to receiveMediaPlaybackInfo*/
    CLSError lserror;
    if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)) {
      errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
      sendErrorResponse(errorCode, request);
      return true;
    }
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, createJsonReplyString(true).c_str());
  request.respond(createJsonReplyString(true).c_str());

  return true;
}

#if USE_TEST_METHOD
bool MediaControlService::testKeyEvent(LSMessage &message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,SCHEMA_2(REQUIRED(mediaId, string), \
  REQUIRED(keyEvent, string)));

  LS::Message request(&message);
  int errorCode = MCS_ERROR_NO_ERROR;
  std::string errorText;
  std::string response;
  if (!msg.parse(__FUNCTION__)) {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    errorCode = MCS_ERROR_PARSING_FAILED;
    errorText = CSTR_PARSING_ERROR;
    response = createJsonReplyString(false, errorCode, errorText);
    request.respond(response.c_str());
    return true;
  }

  std::string keyCode;
  std::string mediaId;
  msg.get("keyEvent", keyCode);
  PMLOG_INFO(CONST_MODULE_MCS, "%s keyEvent : %s", __FUNCTION__, keyCode.c_str());
  msg.get("mediaId", mediaId);
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s", __FUNCTION__, mediaId.c_str());

  pbnjson::JValue responseObj = pbnjson::Object();

  responseObj.put("keyEvent", keyCode);
  responseObj.put("mediaId", mediaId);
  responseObj.put("returnValue", true);
  responseObj.put("subscribed", true);
  PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responseObj.stringify().c_str());
  CLSError lserror;
  if (!LSSubscriptionReply(lsHandle_, "registerMediaSession",
      responseObj.stringify().c_str(), &lserror))
    PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);

  pbnjson::JValue responseObj1 = pbnjson::Object();
  responseObj1.put("returnValue", true);
  request.respond(responseObj1.stringify().c_str());
  return true;
}
#endif

int  MediaControlService:: updateMetaDataResponse (const std::string &mediaId,  pbnjson::JObject &metaDataObj) {
  int errorCode = MCS_ERROR_NO_ERROR;
  if(ptrMediaSessionMgr_){
    mediaMetaData objMetaData;
    errorCode = ptrMediaSessionMgr_->getMediaMetaData(mediaId, objMetaData);
    if (errorCode == MCS_ERROR_NO_ERROR){
      std::string title = objMetaData.getTitle();
      if(!title.empty())
        metaDataObj.put("title", title);

      std::string artist = objMetaData.getArtist();
      if(!artist.empty())
        metaDataObj.put("artist", artist);

      std::string totalDuration = objMetaData.getDuration();
      if(!totalDuration.empty())
        metaDataObj.put("totalDuration", totalDuration);

      std::string album = objMetaData.getAlbum();
      if(!album.empty())
        metaDataObj.put("album", album);

      std::string genre = objMetaData.getGenre();
      if(!genre.empty())
        metaDataObj.put("genre", genre);

      int trackNumber = objMetaData.getTrackNumber();
      if(trackNumber)
        metaDataObj.put("trackNumber", trackNumber);

      int volume = objMetaData.getVolume();
      if(volume)
        metaDataObj.put("volume", volume);
    }
  }
  return errorCode;
}

int main(int argc, char *argv[]) {
  try {
    MediaControlService mediacontrolsrv;
  }
  catch (LS::Error &err) {
    LSErrorPrint(err, stdout);
    return 1;
  }
  return 0;
}
