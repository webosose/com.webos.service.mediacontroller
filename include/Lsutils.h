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

#ifndef LSUTILS_H_
#define LSUTILS_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include <string>
#include <cstring>
#include <iostream>
#include <pbnjson.hpp>
#include <luna-service2/lunaservice.hpp>

#define STANDARD_JSON_SUCCESS "{\"returnValue\":true}"
#define SYSTEM_PARAMETERS "\"$activity\":{\"type\":\"object\",\"optional\":true}"

// Build a schema as a const char * string without any execution overhead
#define SCHEMA_ANY "{}"
#define SCHEMA_0 "{\"type\":\"object\",\"properties\":{" SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_1(param) "{\"type\":\"object\",\"properties\":{" param "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_2(p1, p2) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_3(p1, p2, p3) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_4(p1, p2, p3, p4) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_5(p1, p2, p3, p4, p5) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_6(p1, p2, p3, p4, p5, p6) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_7(p1, p2, p3, p4, p5, p6, p7) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," p7 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_8(p1, p2, p3, p4, p5, p6, p7, p8) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," p7 "," p8 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define SCHEMA_9(p1, p2, p3, p4, p5, p6, p7, p8, p9) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "," p7 "," p8 "," p9 "," SYSTEM_PARAMETERS "},\"additionalProperties\":false}"
#define OBJECT(name, objschema) "\"" #name "\":" objschema
#define OBJSCHEMA_4(p1, p2, p3, p4) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "}}"
#define OBJSCHEMA_6(p1, p2, p3, p4, p5, p6) "{\"type\":\"object\",\"properties\":{" p1 "," p2 "," p3 "," p4 "," p5 "," p6 "}}"
#define PROPS_2(p1, p2) ",\"properties\":{" p1 "," p2 "}"
#define STRICT_SCHEMA(attributes) "{\"type\":\"object\"" attributes ",\"additionalProperties\":false}"
#define REQUIRED_2(p1, p2) ",\"required\":[\"" #p1 "\",\"" #p2 "\"]"

// Macros to use in place of the parameters in the SCHEMA_xxx macros above
#define REQUIRED(name, type) "\"" #name "\":{\"type\":\"" #type "\"}"
#define OPTIONAL(name, type) "\"" #name "\":{\"type\":\"" #type "\",\"optional\":true}"

/*
* Small wrapper around LSError. User is responsible for calling Print or Free after the * error has been set.
*/
struct CLSError : public LSError
{
  CLSError()
  {
    LSErrorInit(this);
  }
  void Free()
  {
    LSErrorFree(this);
  }
};

// LSMessageJson::parse can log the message received, or not, with more or less parameters...
enum ELogOption
{
  eLogOption_DontLogMessage = 0,
  eLogOption_LogMessage,
  eLogOption_LogMessageWithCategory,
  eLogOption_LogMessageWithMethod
};

class LSMessageJsonParser
{
public:
  // Default no using any specific schema. Will simply validate that the message is a valid json message.
  LSMessageJsonParser(LSMessage * message, const char * schema);

  //Parse the message using the schema passed in constructor
  //If 'sender' is specified, automatically reply in case of bad syntax using standard format
  //Option to log the text of the message by default
  bool parse(const char *func, LSHandle *sender = 0, ELogOption logOption = eLogOption_LogMessage);
  pbnjson::JValue get() { return mParser_.getDom(); }
  const char * getPayload() { return LSMessageGetPayload(mMessage_); }

  // convenience functions to get a parameter directly.
  bool get(const char * name, std::string & str){ return get()[name].asString(str) == CONV_OK; }
  bool get(const char * name, bool & boolean) { return get()[name].asBool(boolean) == CONV_OK; }
  template <class T> bool get(const char *name, T &number) { return get().asNumber<T>(number) == CONV_OK; }

private:
  LSMessage *mMessage_;
  const char *mSchemaText_;
  pbnjson::JSchemaFragment mSchema_;
  pbnjson::JDomParser mParser_;
};

// build a standard json reply string without the overhead of using json schema
std::string createJsonReplyString(bool returnValue = true, int errorCode = 0, const std::string& errorText = "" );

#endif /*MEDIA_CONTROL_SERVICE_H_*/
