// Copyright (c) 2020-2022 LG Electronics, Inc.
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

const std::string cstrMediaControlService = "com.webos.service.mediacontroller";
const std::string cstrBTAdapterGetStatus = "luna://com.webos.service.bluetooth2/adapter/getStatus";
const std::string cstrBTDeviceGetStatus = "luna://com.webos.service.bluetooth2/device/getStatus";
const std::string cstrBTAvrcpGetStatus = "luna://com.webos.service.bluetooth2/avrcp/getStatus";
const std::string cstrBTAvrcpReceivePassThroughCommand = "luna://com.webos.service.bluetooth2/avrcp/receivePassThroughCommand";
const std::string cstrSubscribe = "{\"subscribe\":true}";
const std::string cstrBTNotifyMediaPlayStatus = "luna://com.webos.service.bluetooth2/avrcp/notifyMediaPlayStatus";
const std::string cstrGetSessions = "luna://com.webos.service.account/getSessions";

bool BTConnected_ = false;

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
  LS_CATEGORY_METHOD(setMediaMetaData)
  LS_CATEGORY_METHOD(setMediaPlayStatus)
  LS_CATEGORY_METHOD(setMediaMuteStatus)
  LS_CATEGORY_METHOD(setMediaPlayPosition)
  LS_CATEGORY_METHOD(receiveMediaPlaybackInfo)
  LS_CATEGORY_METHOD(injectMediaKeyEvent)
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

#if defined(PLATFORM_SA8155)
        std::string payloadData = "{\"subscribe\":true}";
        CLSError lserrorno;
        if (!LSCall(ptrService->lsHandle_,
                    cstrGetSessions.c_str(),
                    payloadData.c_str(),
                    &MediaControlService::onGetSessionsInfoCb,
                    ctx, NULL, &lserrorno))
          PMLOG_ERROR(CONST_MODULE_MCS,"%s LSCall failed to onGetSessionsInfoCb", __FUNCTION__);
#endif
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

      if(("raspberrypi4 #2" == adapterName) || ("raspberrypi4" == adapterName) ||
         ("raspberrypi4-64 #2" == adapterName) || ("raspberrypi4-64" == adapterName) ||
         ("qemux86-64 #2" == adapterName) || ("qemux86-64" == adapterName)) ||
         ("o22" == adapterName) || //After the Bluetooth service support is expanded, the adapter name will need to be updated
         ("sa8155 Bluetooth hci0" == adapterName) {

        deviceSetId = "RSE-L";
        std::string payload = "{\"adapterAddress\":\"" + adapterAddress + "\",\"subscribe\":true}";
        PMLOG_INFO(CONST_MODULE_MCS,"%s : payload = %s for first adapter",__FUNCTION__, payload.c_str());
        if(obj) {
          if (!LSCall(obj->lsHandle_,
                      cstrBTDeviceGetStatus.c_str(),
                      payload.c_str(),
                      &MediaControlService::onBTDeviceGetStatusCb,
                      ctx, NULL, &lserror))
            PMLOG_ERROR(CONST_MODULE_MCS,"%s : LSCall failed for BT device/getStatus", __FUNCTION__);
        }
      }
      else if ("sa8155 Bluetooth hci1" == adapterName) {
        deviceSetId = "RSE-R";
        std::string payload = "{\"adapterAddress\":\"" + adapterAddress + "\",\"subscribe\":true}";
        PMLOG_INFO(CONST_MODULE_MCS,"%s : payload = %s for second adapter",__FUNCTION__, payload.c_str());
        if(obj) {
          if (!LSCall(obj->lsHandle_,
                      cstrBTDeviceGetStatus.c_str(),
                      payload.c_str(),
                      &MediaControlService::onBTDeviceGetStatusCb,
                      ctx, NULL, &lserror))
            PMLOG_ERROR(CONST_MODULE_MCS,"%s : LSCall failed for BT device/getStatus", __FUNCTION__);
        }
      }
      else
        PMLOG_ERROR(CONST_MODULE_MCS,"%s: Already subscribe for adapter: %s", __FUNCTION__, adapterAddress.c_str());

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
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
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
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
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

bool MediaControlService::setMediaMetaData(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(OBJECT(mediaMetaData, OBJSCHEMA_7( \
  OPTIONAL(title, string),OPTIONAL(artist, string),OPTIONAL(totalDuration, string),OPTIONAL( \
  album, string),OPTIONAL(genre, string),OPTIONAL(trackNumber, integer),OPTIONAL(volume, integer) \
  )),REQUIRED(mediaId, string)) REQUIRED_2(mediaMetaData, mediaId)));

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
  std::string mediaId = payload["mediaId"].asString();
 /* extract mediaMetaData info */
  pbnjson::JValue metaData = payload["mediaMetaData"];
  std::string title = metaData["title"].asString();
  std::string artist = metaData["artist"].asString();
  std::string duration = metaData["totalDuration"].asString();
  std::string album = metaData["album"].asString();
  std::string genre = metaData["genre"].asString();
  int trackNumber =  metaData["trackNumber"].asNumber<int>();
  int volume = metaData["volume"].asNumber<int>();
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s title : %s artist : %s duration : %s album : %s \
                                genre : %s trackNumber : %d volume : %d", __FUNCTION__, mediaId.c_str(),
                                title.c_str(), artist.c_str(), duration.c_str(), album.c_str(), genre.c_str(),
                                trackNumber, volume);

  mediaMetaData objMetaData(title, artist, duration, album, genre, trackNumber, volume);

  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->setMediaMetaData(mediaId, objMetaData);

  if(MCS_ERROR_NO_ERROR == errorCode){
    response = createJsonReplyString(true);
    if(ptrMediaControlPrivate_->mediaMetaData_){
      /*Get display ID from media ID*/
      int displayId = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
      //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
      displayId = 0;
#endif
      pbnjson::JObject metaDataObj;
      errorCode = updateMetaDataResponse(mediaId, metaDataObj);
      pbnjson::JValue responsePayload = pbnjson::Object();
      responsePayload.put("mediaMetaData", metaDataObj);
      responsePayload.put("displayId", displayId);
      responsePayload.put("eventType", "mediaMetaData");
      responsePayload.put("returnValue", true);
      responsePayload.put("subscribed", true);
      PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
      /*Add subscription for receiveMediaPlaybackInfo*/
      CLSError lserror;
      if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)){
        PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
        errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
        errorText = CSTR_SUBSCRIPTION_REPLY_FAILED;
        response = createJsonReplyString(false, errorCode, errorText);
      }
    }
  }
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::setMediaPlayStatus(LSMessage& message) {
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(mediaId, string), \
  REQUIRED(playStatus, string)) REQUIRED_2(mediaId, playStatus)));

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
  std::string mediaId = payload["mediaId"].asString();
  std::string playStatus = payload["playStatus"].asString();
  /*  set play status in MSM  */
  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->setMediaPlayStatus(mediaId, playStatus);

  if(MCS_ERROR_NO_ERROR == errorCode){
    response = createJsonReplyString(true);

    PMLOG_INFO(CONST_MODULE_MCS, "%s BTConnected_ value  = [%d]", __FUNCTION__, BTConnected_);
    /*The below code under if() condition will be removed once BT service will subscribe to MCS API
      for recieving the mediaPlayback status information. This is a temporary fix, until BT
      service implements the subscription call */
    if(BTConnected_) {
      std::string sendPlaybackStatus;

      if(playStatus == "PLAYSTATE_STOPPED")
        sendPlaybackStatus = "stopped";
      else if(playStatus == "PLAYSTATE_PAUSED")
        sendPlaybackStatus = "paused";
      else if(playStatus == "PLAYSTATE_PLAYING")
        sendPlaybackStatus = "playing";
      else if(playStatus == "PLAYSTATE_FAST_FORWARDING")
        sendPlaybackStatus = "fwd_seek";
      else if(playStatus == "PLAYSTATE_REWINDING")
        sendPlaybackStatus = "rev_seek";
      else if(playStatus == "PLAYSTATE_ERROR")
        sendPlaybackStatus = "error";
      else
        PMLOG_INFO(CONST_MODULE_MCS, "%s Invalid PlaybackStatus", __FUNCTION__);

      int displayIdForMedia = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
      displayIdForMedia = 0;
#endif
      BTDeviceInfo objDevInfo;
      if(ptrMediaControlPrivate_->getBTDeviceInfo(displayIdForMedia, &objDevInfo)) {
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

        if(!LSCallOneReply(lsHandle_, cstrBTNotifyMediaPlayStatus.c_str(), payloadData.c_str(), NULL, NULL, NULL, &lserror)) {
          PMLOG_ERROR(CONST_MODULE_MCS,"%s LSCall failed to avrcp/notifyMediaPlayStatus", __FUNCTION__);
        }
      }
    }

    if(ptrMediaControlPrivate_->playStatus_){
      /*Get display ID from media ID*/
      int displayId = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
      //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
      displayId = 0;
#endif
      pbnjson::JValue responsePayload = pbnjson::Object();
      responsePayload.put("playStatus", playStatus);
      responsePayload.put("displayId", displayId);
      responsePayload.put("eventType", "playStatus");
      responsePayload.put("returnValue", true);
      responsePayload.put("subscribed", true);
      PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
      /*Add subscription for receiveMediaPlaybackInfo*/
      CLSError lserror;
      if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)){
        PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
        errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
        errorText = CSTR_SUBSCRIPTION_REPLY_FAILED;
        response = createJsonReplyString(false, errorCode, errorText);
      }
    }
  }
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::setMediaMuteStatus (LSMessage & message){
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(mediaId, string), \
  REQUIRED(muteStatus, string)) REQUIRED_2(mediaId, muteStatus)));

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
  std::string mediaId = payload["mediaId"].asString();
  std::string muteStatus = payload["muteStatus"].asString();
  if(muteStatus == "MUTE")
    muteStatus = "mute";
  else if(muteStatus == "UNMUTE")
    muteStatus = "unmute";

  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s muteStatus : %s ",
                                __FUNCTION__, mediaId.c_str(), muteStatus.c_str());
  /*validate the MediaId*/
  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->setMediaMuteStatus(mediaId, muteStatus);

  if(MCS_ERROR_NO_ERROR == errorCode){
    response = createJsonReplyString(true);
    if(ptrMediaControlPrivate_->muteStatus_){
      /*Get display ID from media ID*/
      int displayId = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
      //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
      displayId = 0;
#endif
      pbnjson::JValue responsePayload = pbnjson::Object();
      responsePayload.put("muteStatus", muteStatus);
      responsePayload.put("displayId", displayId);
      responsePayload.put("eventType", "muteStatus");
      responsePayload.put("returnValue", true);
      responsePayload.put("subscribed", true);
      PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
      /*LSSubscriptionReply for receiveMediaPlaybackInfo*/
      CLSError lserror;
      if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)){
        PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
        errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
        errorText = CSTR_SUBSCRIPTION_REPLY_FAILED;
        response = createJsonReplyString(false, errorCode, errorText);
      }
  }
  }
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::setMediaPlayPosition (LSMessage & message){
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_2(REQUIRED(mediaId, string), \
  REQUIRED(playPosition, string)) REQUIRED_2(mediaId, playPosition)));

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
  std::string mediaId = payload["mediaId"].asString();
  std::string playPosition = payload["playPosition"].asString();
  PMLOG_INFO(CONST_MODULE_MCS, "%s mediaId : %s playPosition : %s ",
                                __FUNCTION__, mediaId.c_str(), playPosition.c_str());
  /*validate the MediaId*/
  if(ptrMediaSessionMgr_)
    errorCode = ptrMediaSessionMgr_->setMediaPlayPosition(mediaId, playPosition);

  if(MCS_ERROR_NO_ERROR == errorCode){
    response = createJsonReplyString(true);
    if(ptrMediaControlPrivate_->playPosition_){
      /*Get display ID from media ID*/
      int displayId = ptrMediaSessionMgr_->getDisplayIdForMedia(mediaId);
      //ToDo : Below platform check to be removed once dual blueetooth support in OSE
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
      displayId = 0;
#endif
      pbnjson::JValue responsePayload = pbnjson::Object();
      responsePayload.put("playPosition", playPosition);
      responsePayload.put("displayId", displayId);
      responsePayload.put("eventType", "playPosition");
      responsePayload.put("returnValue", true);
      responsePayload.put("subscribed", true);
      PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
      /*LSSubscriptionReply for receiveMediaPlaybackInfo*/
      CLSError lserror;
      if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)){
        PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
        errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
        errorText = CSTR_SUBSCRIPTION_REPLY_FAILED;
        response = createJsonReplyString(false, errorCode, errorText);
      }
    }
  }
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

  return true;
}

bool MediaControlService::receiveMediaPlaybackInfo (LSMessage & message){
  PMLOG_INFO(CONST_MODULE_MCS, "%s IN", __FUNCTION__);

  LSMessageJsonParser msg(&message,STRICT_SCHEMA(PROPS_3(REQUIRED(displayId, integer), \
    OPTIONAL(eventType, string),REQUIRED(subscribe, boolean)) REQUIRED_2(displayId, subscribe)));

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
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
      displayId = 0;
#endif
  bool subscribed = payload["subscribe"].asBool();
  std::string eventType = payload["eventType"].asString();
  if(!strcmp(eventType.c_str(), "playStatus")){
    ptrMediaControlPrivate_->playStatus_ = true;
  }else if (!strcmp(eventType.c_str(), "muteStatus")){
    ptrMediaControlPrivate_->muteStatus_ = true;
  }else if (!strcmp(eventType.c_str(), "playPosition")){
    ptrMediaControlPrivate_->playPosition_ = true;
  }else if (!strcmp(eventType.c_str(), "mediaMetaData")){
    ptrMediaControlPrivate_->mediaMetaData_ = true;
  }else if (!strcmp(eventType.c_str(), CSTR_EMPTY.c_str())){
    ptrMediaControlPrivate_->playStatus_ = true;
    ptrMediaControlPrivate_->muteStatus_ = true;
    ptrMediaControlPrivate_->playPosition_ = true;
    ptrMediaControlPrivate_->mediaMetaData_ = true;
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s displayId : %d subscribe : %d  eventType : %s",__FUNCTION__, displayId, subscribed, eventType.c_str());
  response = createJsonReplyString(true);
  /*LSSubscriptionAdd for sendMediaMetaData*/
  if (LSMessageIsSubscription(&message)) {
    CLSError lserror;
    if (!LSSubscriptionAdd(lsHandle_, "receiveMediaPlaybackInfo", &message, &lserror)) {
        PMLOG_ERROR(CONST_MODULE_MCS, "%s LSSubscriptionAdd failed ",__FUNCTION__);
        errorCode = MCS_ERROR_SUBSCRIPTION_FAILED;
        errorText = CSTR_SUBSCRIPTION_FAILED;
        response = createJsonReplyString(false, errorCode, errorText);
    }
  }
  else {
    errorText = getErrorTextFromErrorCode(errorCode);
    response = createJsonReplyString(false, errorCode, errorText);
  }
  /*Create subscription reply*/
  /*get mediaId from displayId*/
  std::string mediaId = ptrMediaSessionMgr_->getMediaIdFromDisplayId(displayId);
  if(mediaId.c_str()){
    pbnjson::JValue responsePayload = pbnjson::Object();
    if(eventType == "playPosition" || eventType.empty()){
      std::string playPosition;
      if(ptrMediaSessionMgr_)
        errorCode = ptrMediaSessionMgr_->getMediaPlayPosition(mediaId, playPosition);
      responsePayload.put("playPosition", playPosition);

    }
    if(eventType == "playStatus" ||eventType.empty()){
      std::string playStatus;
      if(ptrMediaSessionMgr_)
        errorCode = ptrMediaSessionMgr_->getMediaPlayStatus(mediaId, playStatus);
      responsePayload.put("playStatus", playStatus);
    }
    if(eventType == "muteStatus" || eventType.empty()){
      std::string muteStatus;
      if(ptrMediaSessionMgr_)
        errorCode = ptrMediaSessionMgr_->getMediaMuteStatus(mediaId, muteStatus);
      responsePayload.put("muteStatus", muteStatus);
    }
    if(eventType == "mediaMetaData" || eventType.empty()){
      pbnjson::JObject metaDataObj;
      errorCode = updateMetaDataResponse(mediaId, metaDataObj);
      responsePayload.put("mediaMetaData", metaDataObj);
    }
    if(!eventType.empty())
      responsePayload.put("eventType", eventType);

    responsePayload.put("displayId", displayId);
    responsePayload.put("returnValue", true);
    responsePayload.put("subscribed", true);
    PMLOG_INFO(CONST_MODULE_MCS, "%s send subscription response :%s", __FUNCTION__, responsePayload.stringify().c_str());
    /*LSSubscriptionReply for receiveMediaPlaybackInfo*/
    CLSError lserror;
    if (!LSSubscriptionReply(lsHandle_,"receiveMediaPlaybackInfo" , responsePayload.stringify().c_str(), &lserror)){
      PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
      errorCode = MCS_ERROR_SUBSCRIPTION_REPLY_FAILED;
      errorText = CSTR_SUBSCRIPTION_REPLY_FAILED;
      response = createJsonReplyString(false, errorCode, errorText);
    }
  }

  PMLOG_INFO(CONST_MODULE_MCS, "%s response : %s", __FUNCTION__, response.c_str());
  request.respond(response.c_str());

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
#if defined(PLATFORM_RASPBERRYPI4) || defined(PLATFORM_O22)
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
