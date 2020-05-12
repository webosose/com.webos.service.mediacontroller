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

/*-----------------------------------------------------------------------------
 #include
 (File Inclusions)
 -- ----------------------------------------------------------------------------*/
#include "MediaControlService.h"
#include "MediaSessionManager.h"
#include "MediaControlPrivate.h"
#include "Lsutils.h"

#include <string>

const std::string cstrMediaControlService = "com.webos.service.mediacontroller";
const std::string cstrBTAdapterGetStatus = "luna://com.webos.service.bluetooth2/adapter/getStatus";
const std::string cstrBTDeviceGetStatus = "luna://com.webos.service.bluetooth2/device/getStatus";
const std::string cstrBTAvrcpGetStatus = "luna://com.webos.service.bluetooth2/avrcp/getStatus";
const std::string cstrBTAvrcpReceivePassThroughCommand = "luna://com.webos.service.bluetooth2/avrcp/receivePassThroughCommand";
const std::string cstrSubscribe = "{\"subscribe\":true}";

std::string defaultAdapterAddress_;
std::string secondAdapterAddress_;

MediaControlService::MediaControlService()
  : LS::Handle(LS::registerService(cstrMediaControlService.c_str()))
{
  PMLOG_INFO(CONST_MODULE_MCS,"%s IN", __FUNCTION__);
  LS_CATEGORY_BEGIN(MediaControlService, "/")
  LS_CATEGORY_METHOD(registerMediaSession)
  LS_CATEGORY_METHOD(unregisterMediaSession)
  LS_CATEGORY_METHOD(activateMediaSession)
  LS_CATEGORY_METHOD(deactivateMediaSession)
  LS_CATEGORY_END;

  // attach to mainloop and run it
  attachToLoop(main_loop_ptr_.get());

  // get BT to get connected devices
  subscribeToBTAdapterGetStatus();

  // run the gmainloop
  g_main_loop_run(main_loop_ptr_.get());
}

void MediaControlService::subscribeToBTAdapterGetStatus()
{
  PMLOG_INFO(CONST_MODULE_MCS,"%s IN", __FUNCTION__);
  CLSError lserror;

  if(!LSCall(this->get(),
             cstrBTAdapterGetStatus.c_str(),
             cstrSubscribe.c_str(),
             &MediaControlService::onBTAdapterQueryCb,
             this, NULL, &lserror))
  {
    PMLOG_ERROR(CONST_MODULE_MCS,"%s LS Call error for BT adapter/GetStatus", __FUNCTION__);
  }
}

bool MediaControlService::onBTAdapterQueryCb(LSHandle *lshandle, LSMessage *message, void *ctx)
{
  LSMessageJsonParser msg(message,SCHEMA_5(REQUIRED(subscribed, boolean), \
  REQUIRED(adapters, array),REQUIRED(returnValue, boolean),REQUIRED \
  (errorCode,integer),REQUIRED(errorText, string)));

  if (!msg.parse(__FUNCTION__))
  {
    PMLOG_ERROR(CONST_MODULE_MCS,"%s parse failure", __FUNCTION__);
    return true;
  }
  bool returnValue = false;
  msg.get("returnValue", returnValue);
  if (returnValue)
  {
    pbnjson::JValue adapterStatus = msg.get();
    pbnjson::JValue adapters = adapterStatus["adapters"];
    if (!adapters.isArray())
    {
      PMLOG_ERROR(CONST_MODULE_MCS,"%s no adapters available", __FUNCTION__);
      return true;
    }

    if (0 == adapters.arraySize())
    {
      PMLOG_ERROR(CONST_MODULE_MCS,"%s adapters empty array",__FUNCTION__);
      return true;
    }

    CLSError lserror;
    for (int i = 0; i < adapters.arraySize(); i++)
    {
      std::string adapterName = adapters[i]["name"].asString();
      std::string adapterAddress = adapters[i]["adapterAddress"].asString();
      PMLOG_INFO(CONST_MODULE_MCS,"%s : adapterName : %s, adapterAddress : %s",
                                   __FUNCTION__, adapterName.c_str(), adapterAddress.c_str());
      if ("sa8155 Bluetooth hci0" == adapterName)
      {
        bool defaultValue = adapters[i]["default"].asBool();
        if (defaultAdapterAddress_ != adapterAddress)
        {
          defaultAdapterAddress_ = adapterAddress;
          std::string payload = "{\"adapterAddress\":" + adapterAddress + "\"subscribe\":true}";
          PMLOG_INFO(CONST_MODULE_MCS,"%s : payload = %s for first adapter",__FUNCTION__, payload.c_str());
          if (!LSCall(lshandle,
                      cstrBTDeviceGetStatus.c_str(),
                      payload.c_str(),
                      &MediaControlService::onBTDeviceGetStatusCb,
                      ctx, NULL, &lserror))
            PMLOG_ERROR(CONST_MODULE_MCS,"%s : LSCall failed for BT device/getStatus", __FUNCTION__);
        }
        else
          PMLOG_ERROR(CONST_MODULE_MCS,"%s: Already subscribe for adapter: %s", __FUNCTION__, adapterAddress.c_str());
      }
      else if ("sa8155 Bluetooth hci1" == adapterName)
      {
        if (secondAdapterAddress_ != adapterAddress)
        {
          secondAdapterAddress_ = adapterAddress;
          std::string payload = "{\"adapterAddress\":" + adapterAddress + "\"subscribe\":true}";
          PMLOG_INFO(CONST_MODULE_MCS,"%s : payload = %s for second adapter",__FUNCTION__, payload.c_str());
          if (!LSCall(lshandle,
                      cstrBTDeviceGetStatus.c_str(),
                      payload.c_str(),
                      &MediaControlService::onBTDeviceGetStatusCb,
                      ctx, NULL, &lserror))
            PMLOG_ERROR(CONST_MODULE_MCS,"%s : LSCall failed for BT device/getStatus", __FUNCTION__);
        }
        else
          PMLOG_ERROR("%s: Already subscribe for adapter: %s", __FUNCTION__, adapterAddress.c_str());
      }
      else
        PMLOG_ERROR("%s: Already subscribe for adapter: %s", __FUNCTION__, adapterAddress.c_str());
    }
  }
  return true;
}

bool MediaControlService::onBTDeviceGetStatusCb(LSHandle *lshandle, LSMessage *message, void *ctx)
{
  LSMessageJsonParser msg(message,SCHEMA_6(REQUIRED(subscribed, boolean), \
  REQUIRED(adapterAddress, string),REQUIRED(returnValue, boolean),REQUIRED \
  (devices,array),REQUIRED(errorCode, integer),REQUIRED(errorText, string)));

  if (!msg.parse(__FUNCTION__))
  {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s Parsing failed", __FUNCTION__);
    return true;
  }

  bool returnValue = false;
  msg.get("returnValue", returnValue);

  if(returnValue)
  {
    pbnjson::JValue deviceStatus = msg.get();
    pbnjson::JValue devices = deviceStatus["devices"];
    if (!devices.isArray())
    {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s Empty device array", __FUNCTION__);
      return true;
    }
    if (0 == devices.arraySize())
    {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s devices arraySize 0", __FUNCTION__);
      return true;
    }
    for(int i = 0; i < devices.arraySize(); i++)
    {
      pbnjson::JValue connectedProfiles = devices[i]["connectedProfiles"];
      if (0 == connectedProfiles.arraySize()) continue;
      for (int j = 0; j < connectedProfiles.arraySize(); j++)
      {
        std::string profile = connectedProfiles[j].asString();
        PMLOG_INFO(CONST_MODULE_MCS, "%s profile : %s", __FUNCTION__, profile.c_str());
        if ("avrcp" == profile )
        {
          std::string address = devices[i]["address"].asString();
          std::string adapterAddress = devices[i]["adapterAddress"].asString();
          std::string payload = "{\"adapterAddress\":" + adapterAddress + "\"address\":" + address + "\"subscribe\":true}";
          PMLOG_INFO(CONST_MODULE_MCS, "%s payload : %s", __FUNCTION__, payload.c_str());
          CLSError lserror;
          if (!LSCall(lshandle,
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
  return true;
}

bool MediaControlService::onBTAvrcpGetStatusCb(LSHandle *lshandle, LSMessage *message, void *ctx)
{
  LSMessageJsonParser msg(message,SCHEMA_9(REQUIRED(subscribed, boolean), \
  REQUIRED(adapterAddress, string),REQUIRED(returnValue, boolean),REQUIRED \
  (connecting, boolean),REQUIRED(connected, boolean),REQUIRED(playing, boolean), \
  REQUIRED(address, string),REQUIRED(errorCode, integer),REQUIRED(errorText, string)));

  if (!msg.parse(__FUNCTION__))
  {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s parsing failed", __FUNCTION__);
    return true;
  }

  bool returnValue = false;
  msg.get("returnValue", returnValue);
  if (returnValue)
  {
    std::string address;
    msg.get("address", address);
    if(nullptr == address.c_str())
    {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s Device MAC address field not found", __FUNCTION__);
      return true;
    }
    bool connected = false;
    msg.get("connected", connected);
    std::string adapterAddress;
    msg.get("adapterAddress", adapterAddress);
    PMLOG_INFO(CONST_MODULE_MCS, "%s Device MAC address %s Connection Status %d adapter address %s",\
                                  __FUNCTION__, address.c_str(), connected, adapterAddress.c_str());
    if (connected)
    {
      int displayId = (defaultAdapterAddress_ == adapterAddress) ? 1 : 2;

      BTDeviceInfo objDevInfo(address, adapterAddress, displayId);
      //save the details of BT device connected
      MediaControlPrivate::getInstance().setBTDeviceInfo(objDevInfo);

      std::string payload = "{\"address\":" + address + "\"subscribe\":true}";
      PMLOG_INFO(CONST_MODULE_MCS, "%s payload : %s", __FUNCTION__, payload.c_str());
      //subscribe to bt key events
      CLSError lserror;
      if (!LSCall(lshandle,
                  cstrBTAvrcpReceivePassThroughCommand.c_str(),
                  payload.c_str(),
                  &MediaControlService::onBTAvrcpKeyEventsCb,
                  ctx, NULL, &lserror))
        PMLOG_ERROR(CONST_MODULE_MCS,"%s LSCall failed to avrcp/receivePassThroughCommand", __FUNCTION__);
    }
  }
  return true;
}

bool MediaControlService::onBTAvrcpKeyEventsCb(LSHandle *lshandle, LSMessage *message, void *ctx)
{
  LSMessageJsonParser msg(message,SCHEMA_8(REQUIRED(subscribed, boolean), \
  REQUIRED(adapterAddress, string),REQUIRED(returnValue, boolean),REQUIRED \
  (keyCode, string),REQUIRED(keyStatus, string),REQUIRED(address, string), \
  REQUIRED(errorCode, integer),REQUIRED(errorText, string)));

  if (!msg.parse(__FUNCTION__))
  {
    PMLOG_ERROR(CONST_MODULE_MCS, "%s parsing failed", __FUNCTION__);
    return true;
  }

  bool returnValue = false;
  msg.get("returnValue", returnValue);

  if(returnValue)
  {
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

    if(keyStatus == "released")
    {
      //get latest client from media session manager
      std::string mediaId;
      //post key event to application
      pbnjson::JValue responseObj = pbnjson::Object();
      responseObj.put("keyCode", keyCode);
      responseObj.put("mediaId", mediaId);
      std::string reply = responseObj.stringify() + createJsonReplyString("true");
      CLSError lserror;
      if (!LSSubscriptionReply(lshandle,"/registerMediaSession" , reply.c_str(), &lserror))
        PMLOG_ERROR(CONST_MODULE_MCS,"%s LSSubscriptionReply failed", __FUNCTION__);
    }
  }
  return true;
}

bool MediaControlService::registerMediaSession(LSMessage& message)
{
  g_debug("registerMediaSession API invoked");
  bool subscribed = false;
  std::string mediaId;
  std::string appId;
  int displayId;
  CLSError lserror;

  LSMessageJsonParser msg(&message,SCHEMA_4(REQUIRED(mediaId, string), \
  REQUIRED(appId, string),REQUIRED(displayId, integer),REQUIRED(subscribe, boolean)));
  
  if (!msg.parse(__FUNCTION__))
    return true;

  msg.get("mediaId", mediaId);
  msg.get("appId", appId);
  msg.get("displayId", displayId);
  msg.get("subscribe", subscribed);
  
  if (LSMessageIsSubscription(&message))
  {
    if (!LSSubscriptionAdd(this->get(), "registerMediaSession", &message, &lserror))
    {
      lserror.Print(__FUNCTION__, __LINE__);
      g_debug("LSSubscriptionProcess failed");
    }
  }

  MediaSessionManager::getInstance().addMediaSession(mediaId, appId, displayId);

  pbnjson::JValue responseObj = pbnjson::Object();
  responseObj.put("subscribed", subscribed);
  responseObj.put("returnValue", true);
  responseObj.put("mediaId", mediaId);
  responseObj.put("displayId", displayId);
  std::string responseOut = responseObj.stringify();
  LS::Message request(&message);
  request.respond(responseOut.c_str());
  return true;
}

bool MediaControlService::unregisterMediaSession(LSMessage& message)
{
  std::string mediaId;

  LSMessageJsonParser msg(&message,SCHEMA_1(REQUIRED(mediaId, string)));

  if (!msg.parse(__FUNCTION__))
    return true;

  msg.get("mediaId", mediaId);

  MediaSessionManager::getInstance().removeMediaSession(mediaId);
  
  pbnjson::JValue responseObj = pbnjson::Object();
  responseObj.put("returnValue", true);
  std::string responseOut = responseObj.stringify();
  return true;
}

bool MediaControlService::activateMediaSession(LSMessage& message)
{
  std::string mediaId;

  LSMessageJsonParser msg(&message,SCHEMA_1(REQUIRED(mediaId, string)));

  if (!msg.parse(__FUNCTION__))
    return true;

  msg.get("mediaId", mediaId);

  MediaSessionManager::getInstance().activateMediaSession(mediaId);

  pbnjson::JValue responseObj = pbnjson::Object();
  responseObj.put("returnValue", true);
  std::string responseOut = responseObj.stringify();

  return true;
}

bool MediaControlService::deactivateMediaSession(LSMessage& message)
{
  std::string mediaId;

  LSMessageJsonParser msg(&message,SCHEMA_1(REQUIRED(mediaId, string)));

  if (!msg.parse(__FUNCTION__))
    return true;

  msg.get("mediaId", mediaId);

  MediaSessionManager::getInstance().deactivateMediaSession(mediaId);
  
  pbnjson::JValue responseObj = pbnjson::Object();
  responseObj.put("returnValue", true);
  std::string responseOut = responseObj.stringify();
  return true;
}

int main(int argc, char *argv[])
{
  try
  {
    MediaControlService mediacontrolsrv;
  }
  catch (LS::Error &err)
  {
    LSErrorPrint(err, stdout);
    return 1;
  }
  return 0;
}
