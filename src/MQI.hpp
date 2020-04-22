/*<copyright notice="lm-source" pids="" years="2014,2020">*/
/*******************************************************************************
 * Copyright (c) 2014,2020 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Contributors:
 *    Rowan Lonsdale - Initial implementation
 *    Various members of the WebSphere MQ Performance Team at IBM Hursley UK
 *******************************************************************************/
/*</copyright>*/
/*******************************************************************************/
/*                                                                             */
/* Performance Harness for IBM MQ C-MQI interface                              */
/*                                                                             */
/*******************************************************************************/

namespace cph {
class MQIConnection;
class MQIObject;
class MQIQueue;
class MQITopic;
class MQISubscription;
class MQIMessage;
class MQIException;
}

#ifndef MQOBJECT_H_
#define MQOBJECT_H_

#include <stdexcept>
#include <string>

#include "MQIOpts.hpp"
#include "MQIWorkerThread.hpp"
#include <cmqc.h>
#include "cphTrace.h"
#include "cphLog.h"

#define CPH_MQC_MSG_LEN 256

namespace cph {

/*
 * Class: MQIConnection
 * -------------------
 *
 * Represents a connection to MQ, backed by an MQHCONN.
 */
class MQIConnection {
friend class MQIObject;
friend class MQIQueue;
friend class MQITopic;
friend class MQISubscription;

private:
  Thread const * const pCurrentThread;
  CPH_TRACE * const pTrc;
  CPH_LOG * const pLog;
  MQIOpts const * const pOpts;
  char const * const name;

  MQHCONN hConn;
  bool ownsConnection;

  mutable char msg[CPH_MQC_MSG_LEN];

public:
  MQIConnection(MQIWorkerThread * const pOwner, bool reconnect);

  MQHMSG createPutMessageHandle(MQMD* md, char* propsBuffer, MQLONG bufferLength) const;
  MQHMSG createGetMessageHandle() const;
  void destroyMessageHandle(MQHMSG& handle) const;

  void commitTransaction() const;
  MQLONG commitTransaction_try() const;
  void rollbackTransaction() const;

  virtual ~MQIConnection();
};

/*
 * Class: MQIMessage
 * ----------------
 *
 * A wrapper around a byte buffer, with meta-data
 * recording the size of the buffer and the length of the message on the buffer,
 */
class MQIMessage {
public:
  /*The message buffer.*/
  MQBYTE * buffer;
  /*The size of the message buffer.*/
  MQLONG bufferLen;
  /*The length of the message on the buffer.*/
  MQLONG messageLen;

  MQIMessage(size_t size);
  MQIMessage(MQIOpts const * const pOpts, bool empty);
  void resize(MQLONG newSize);
  ~MQIMessage();
};

/*
 * Class: MQIException
 * ------------------
 *
 * Used to indicate an MQI call failed.
 * Records the completion code (MQCC)
 * and reason code (MQRC).
 */
class MQIException: public std::runtime_error {
public:
  /*The MQI function that failed.*/
  std::string const function;
  /*The completion code (MQCC) of the error.*/
  MQLONG const compCode;
  /*The reason code (MQRC) of the error.*/
  MQLONG const reasonCode;
  static bool printDetails;

  MQIException(std::string functionName, MQLONG compCode, MQLONG reasonCode, MQBYTE24 id = NULL, const MQCHAR48 qName = NULL);

  virtual ~MQIException() throw();
};

/*
 * Class: MQIObject
 * ----------------
 *
 * A wrapper around an MQHOBJ.
 */
class MQIObject {
  friend class MQIConnection;
  char * selectionString;
protected:
  /*The MQHCONN used to create this object.*/
  MQIConnection const * const pConn;

  /*The associated MQHOBJ.*/
  MQHOBJ hObj;
  MQOD od;

  /*Whether the object was opened to allow putting.*/
  bool const canPut;
  /*Whether the object was opened to allow getting.*/
  bool const canGet;

  inline void checkOpen() const;
  inline void checkNotOpen() const;

public:
  char const * const desc;

  MQIObject(MQIConnection const * const pConnection, MQLONG type, bool put, bool get, char const * const desc);
  virtual ~MQIObject();
  virtual void open(bool log);

  virtual char const * getName() const = 0;
  char const * getQMName() const;
  virtual void setName(char const * const format, ...) = 0;
  void setQMName(char const * const format, ...);
  void setQMName(MQCHAR48 const * qmName);

  void put(MQIMessage const * const msg, MQMD& md, MQPMO& pmo);  
  MQLONG put_try(MQIMessage const * const msg, MQMD& md, MQPMO& pmo);
  void put1(MQIMessage const * const msg, MQMD& md, MQPMO& pmo);
  void get(MQIMessage * const msg, MQMD& md, MQGMO& gmo) const;  
  MQLONG get_try(MQIMessage * const msg, MQMD& md, MQGMO& gmo) const;

  void createSelector(CPH_TRACE * pTrc, MQBYTE24 correlId, char * customSelector);

};

/*
 * Class: MQIQueue
 * ---------------
 *
 * Extends: MQIObject
 *
 * A wrapper around a queue MQHOBJ.
 */
class MQIQueue : public MQIObject {
public:
  MQIQueue(MQIConnection const * const pConnection, bool put, bool get);
  virtual ~MQIQueue();

  virtual char const * getName() const;
  virtual void setName(char const * const format, ...);
  void setName(MQCHAR48 const * name);
};

/*
 * Class: MQITopic
 * ---------------
 *
 * Extends: MQIObject
 *
 * A wrapper around a topic MQHOBJ.
 */
class MQITopic : public MQIObject {
public:
  MQITopic(MQIConnection const * const pConnection);
  virtual ~MQITopic();

  virtual char const * getName() const;
  virtual void setName(char const * const format, ...);
};

/*
 * Class: MQISubscription
 * ----------------------
 *
 * Extends: MQIObject
 *
 * An extension of MQIObject for handling managed subscriptions to topic strings.
 */
class MQISubscription : public MQIObject {
  friend class MQIConnection;

private:
  MQHOBJ hSub;
  MQSD sd;
  bool unsubscribeOnClose;

public:
  MQISubscription(MQIConnection const * const pConnection);
  virtual ~MQISubscription();
  virtual void open(bool log);

  virtual char const * getName() const;
  virtual void setName(char const * const format, ...);

  char const * getTopicString() const;
  void setTopicString(char const * const format, ...);

  virtual char const * getSubName() const;
  virtual void setSubName(char const * const format, ...);

  virtual void setDurable(bool unsubscribeOnClose, char const * const subNameFormat, ...);
};

/*
 * Function: bpstrncmp
 * -------------------
 *
 * Equivalent to strncmp, but treat blank-padding
 * and null-termination as equivalent.
 */
inline int bpstrncmp(char const * a, char const * b, size_t len){
  for(size_t i=0; i<len; i++){
    if(a!=NULL && a[i]=='\0') a = NULL;
    if(b!=NULL && b[i]=='\0') b = NULL;
    if(a==NULL && b==NULL) return 0;
    int result = (a==NULL ? ' ' : a[i]) - (b==NULL ? ' ' : b[i]);
    if(result!=0) return result;
  }
  return 0;
}

/*
 * Function: bpstrnlen
 * -------------------
 *
 * Finds the length of a possibly blank-padded string,
 * which is defined as to be one greater than the index of the last
 * non-blank character in the buffer that does not come after a null character,
 * or zero if no such character exists.
 */
inline size_t bpstrnlen(char const * str, size_t bufferLen){
  size_t rc = 0;
  for(size_t i=0; i<bufferLen; i++){
    if(str[i]=='\0') break;
    if(str[i]!=' ') rc = i+1;
  }
  return rc;
}

// printf format specifier for a fixed-width, 32-bit, hexidecimal number
#ifdef CPH_DOTRACE
#define f_id4 "%02X"
#endif

/*
 * Function: cphTraceId
 * --------------------
 *
 * Write an MQBYTE24 (used for message IDs and correlation IDs)
 * to the trace, split into 6 4-byte (8-digit) hexidecimal words.
 */
inline void cphTraceId(CPH_TRACE * const pTrc, char const * const label, MQBYTE24 const id){
#ifdef CPH_DOTRACE
  char convertedId[49] = "";
  int len = 0;

  unsigned char * idd = (unsigned char *)id;
  for (int i = 0; i < 24; ++i){
    len += sprintf(convertedId + len, f_id4, idd[i]);
  }

  CPHTRACEMSG(pTrc, "%s: Bytes: %s", label, convertedId)
#endif
}

/*
 * Macro: CPHCALLMQ
 * ----------------
 *
 * Calls an MQI function, logging a trace message before-hand,
 * and throwing an exception in the event of a failure.
 */
#define CPHCALLMQ(T, F, ...) \
  MQLONG mqcc=0, mqrc=0;\
  CPHTRACEMSG(T, (char*) "About to call %s.", #F)\
  F(__VA_ARGS__, &mqcc, &mqrc);\
  if(mqrc!=MQRC_NONE) {\
    CPHTRACEMSG(T, (char*) "Exception from call: Comp Code:%ld ;Reason: %ld", mqcc, mqrc)\
    throw cph::MQIException(#F, mqcc, mqrc);\
  }\

} /* Namespace */

#endif /* MQOBJECT_H_ */
