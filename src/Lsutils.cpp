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
#include "Lsutils.h"
#include "MediaControlTypes.h"

LSMessageJsonParser::LSMessageJsonParser(LSMessage * message,
                                         const char * schema) :
                                         mMessage_(message),
                                         mSchemaText_(schema),
                                         mSchema_(schema) {
}

bool LSMessageJsonParser::parse(const char * callerFunction,
                                LSHandle * lssender,
                                ELogOption logOption) {
  const char * payload = getPayload();
  const char * context = 0;
  if (logOption == eLogOption_LogMessageWithCategory)
    context = LSMessageGetCategory(mMessage_);
  else if (logOption == eLogOption_LogMessageWithMethod)
    context = LSMessageGetMethod(mMessage_);
  if (context == 0)
    context = "";

  if (logOption != eLogOption_DontLogMessage)
    PMLOG_ERROR(CONST_MODULE_MCS, "%s %s: got '%s'", callerFunction, context, payload);
  if (!mParser_.parse(payload, mSchema_)) {
    const char *    sender = LSMessageGetSenderServiceName(mMessage_);
    if (sender == 0 || *sender == 0)
      sender = LSMessageGetSender(mMessage_);
    if (sender == 0)
      sender = "";
    std::string errorText = "Could not validate json message against schema";
    bool notJson = true;
    if (strcmp(mSchemaText_, SCHEMA_ANY) != 0) {
      pbnjson::JSchemaFragment    genericSchema(SCHEMA_ANY);
      notJson = !mParser_.parse(payload, genericSchema);
    }
    if (notJson) {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s %s: The message '%s' sent by '%s' is not a valid  \
                json message.", callerFunction, context, payload, sender);
      errorText = "Not a valid json message";
    }
    else {
      PMLOG_ERROR(CONST_MODULE_MCS, "%s %s: Could not validate json message '%s' sent by   \
                 '%s' against schema '%s'.",
                 callerFunction, context,
                 payload, sender, mSchemaText_);
     }
     if (sender) {
       std::string reply = createJsonReplyString(false, 1, errorText);
       CLSError lserror;
       if (!LSMessageReply(lssender, mMessage_, reply.c_str(), &lserror))
          PMLOG_ERROR(CONST_MODULE_MCS, "%s LSMessageReply failed",callerFunction);
     }
     return false;
   }
 return true;
}

std::string createJsonReplyString(bool returnValue,
                                  int errorCode,
                                  const std::string& errorText) {
  pbnjson::JValue responseObj = pbnjson::Object();
  if (returnValue)
    responseObj.put("returnValue", returnValue);
  else {
    responseObj.put("returnValue", false);
    responseObj.put("errorCode", errorCode);
    responseObj.put("errorText", errorText);
  }

  return responseObj.stringify();
}
