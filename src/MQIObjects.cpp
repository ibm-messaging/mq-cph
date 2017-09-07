/*<copyright notice="lm-source" pids="" years="2014,2017">*/
/*******************************************************************************
 * Copyright (c) 2014,2017 IBM Corp.
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

#include <cstdarg>
#include <cstring>
#include <assert.h>

#ifdef _MSC_VER
#include "msio.h"
#else
#include <cstdio>
#endif
#include "MQI.hpp"
#include "cphUtil.h"

/*
 * -------------
 * Helper macros
 * -------------
 */

/*
 * Macro: CPH_TIMEOUT_UNLIMITED
 * ----------------------------
 *
 * The number of milliseconds we'll set the MQGET wait interval to,
 * if an unlimited timeout (-1) is specified on the command line.
 *
 * We don't use MQWI_UNLIMITED, as this would make it difficult to
 * terminate the worker thread if a shutdown signal is received
 * during an MQGET. Instead, we set the WaitInterval to the number
 * of milliseconds specified here, and reissue the MQGET if we get
 * MQRC_NO_MSG_AVAILABLE.
 */
#define CPH_TIMEOUT_UNLIMITED 10000

/*
 * Macro: VSPRINTF_FIXED
 * ---------------------
 *
 * sprintf to a fixed-length string.
 *
 * Parameters:
 *   NAME - A description of the name being set.
 *   PTR  - A pointer to a char array to sprintf to.
 *   LEN  - The fixed length of the string buffer.
 *   FMT  - The name of the sprintf format parameter passed into the enclosing function.
 *
 * It is expected that any additional sprintf arguments are variadic arguments to the
 * enclosing function, declared immediately following FMT.
 */
#define VSPRINTF_FIXED(NAME, PTR, LEN, FMT)\
  va_list args;\
  va_start(args, FMT);\
  int reqLen = vsnprintf(PTR, LEN, FMT, args);\
  va_end(args);\
  if(reqLen>=LEN)\
    throw runtime_error("Cannot set " NAME ": too long");

/*
 * Macro: VSPRINTF_FIXED
 * ---------------------
 *
 * sprintf to a string, reallocating it to increase its length if necessary.
 *
 * Parameters:
 *   NAME - A description of the name being set.
 *   PTR  - A pointer to a char array to sprintf to.
 *   LEN  - An integer variable set to the current length of the string buffer,
 *          this will be set to the new length of the buffer is reallocated.
 *   FMT  - The name of the sprintf format parameter passed into the enclosing function.
 *
 * It is expected that any additional sprintf arguments are variadic arguments to the
 * enclosing function, declared immediately following FMT.
 */
#define VSPRINTF_VAR(NAME, PTR, LEN, FMT)\
  va_list args;\
  va_start(args, FMT);\
  int reqLen = vsnprintf(NULL, 0, FMT, args)+1;\
  va_end(args);\
  if(PTR==NULL){\
    PTR = malloc(sizeof(char)*reqLen);\
    LEN = reqLen;\
  } else if(LEN < reqLen) {\
    void *tmp = realloc(PTR, sizeof(char)*reqLen);\
    if (NULL == tmp) {\
      free(PTR);\
      PTR = NULL;\
    }\
    PTR = tmp;\
    LEN = reqLen;\
  }\
  if(PTR==NULL)\
    throw runtime_error("Cannot set " NAME ": could not allocate memory.");\
  va_start(args, FMT);\
  vsprintf((char*) PTR, FMT, args);\
  va_end(args);

/*
 * Macro: VS_GET_SET_NAME
 * ----------------------
 *
 * Define getters and setters for a string member of a class,
 * to be stored in an MQCHARV structure.
 *
 * Parameters:
 *   CLASS - The owning class.
 *   NAME - Define the methods 'get<X>' and 'set<X>', where X=NAME.
 *   PRETTY_NAME - A description of the name being set.
 *   CV - The MQCRAHV variable to get and set. The VSLength field
 *        should have previously been set to MQVS_NULL_TERMINATED.
 */
#define VS_GET_SET_NAME(CLASS, NAME, PRETTY_NAME, CV)\
  char const * CLASS::get##NAME() const {\
    return (char const * const) CV.VSPtr;\
  }\
  \
  void CLASS::set##NAME(char const * const format, ...) {\
    CPHTRACEENTRY(pConn->pTrc)\
    checkNotOpen();\
    VSPRINTF_VAR(PRETTY_NAME, CV.VSPtr, CV.VSBufSize, format)\
    CPHTRACEEXIT(pConn->pTrc)\
  }

using namespace std;

namespace cph {

/*
 * ------------------------------------
 * Method implementations for MQIObject
 * ------------------------------------
 */

MQOD const protoOD = {MQOD_DEFAULT};

MQIObject::MQIObject(MQIConnection const * const pConnection, MQLONG type, bool put, bool get, char const * const desc) :
    pConn(pConnection), hObj(MQHO_NONE), od(protoOD), canPut(put), canGet(get), desc(desc){
  CPHTRACEENTRY(pConn->pTrc)
  od.Version = MQOD_VERSION_4;
  od.ObjectType = type;
  selectionString = NULL;
  CPHTRACEEXIT(pConn->pTrc)
}

MQIObject::~MQIObject() {
  CPHTRACEENTRY(pConn->pTrc)
  if(hObj!=MQHO_NONE && hObj!=MQHO_UNUSABLE_HOBJ){
    try {
      CPHCALLMQ(pConn->pTrc, MQCLOSE, pConn->hConn, &hObj, MQCO_NONE)
    } catch (cph::MQIException) {
      CPHTRACEMSG(pConn->pTrc, "OOPS! MQIException in ~MQIObject()!")
    }
  }
  if (selectionString != NULL){
    free(selectionString);
    selectionString = NULL;
  }
  CPHTRACEEXIT(pConn->pTrc)
}

inline void MQIObject::checkOpen() const{
  if(hObj==MQHO_NONE)
      throw logic_error("Cannot use MQI object: not yet opened.");
}

inline void MQIObject::checkNotOpen() const{
  if(hObj!=MQHO_NONE)
      throw logic_error("Cannot alter MQI object properties: already opened.");
}

char const * MQIObject::getQMName() const {
  return od.ObjectQMgrName;
}

/*
 * Method: setQMName
 * -----------------
 *
 * Sets the name of the queue manager that the object is local to,
 * using a printf-style format.
 *
 * If the queue manager name is to be set from a blank-padded
 * (rather than null-terminated), fixed length string - such as
 * the ReplyToQMgr field of an incoming message descriptor -
 * then the setQMName(MQCHAR48 const *) overload should be used instead
 * by passing a pointer to the character array.
 */
void MQIObject::setQMName(char const * const format, ...){
  CPHTRACEENTRY(pConn->pTrc)
  checkNotOpen();
  VSPRINTF_FIXED("object QM name", od.ObjectQMgrName, MQ_Q_MGR_NAME_LENGTH, format)
  CPHTRACEEXIT(pConn->pTrc)
}

/*
 * Method: setQMName
 * -----------------
 *
 * Sets the name of the queue manager that the object is local to.
 *
 * The input string can be either blank padded or null-terminated.
 */
void MQIObject::setQMName(MQCHAR48 const * qmName){
  CPHTRACEENTRY(pConn->pTrc)
  checkNotOpen();
  strncpy(od.ObjectQMgrName, *qmName, MQ_Q_MGR_NAME_LENGTH);
  CPHTRACEEXIT(pConn->pTrc)
}

/*
 * Method: open
 * ------------
 *
 * Open a queue or topic.
 *
 * If the object is to be used only for putting, and the put1 option
 * was specified on the command line, then this method does nothing.
 */
void MQIObject::open(bool log) {
  CPHTRACEENTRY(pConn->pTrc)
  checkNotOpen();

  if(canGet || (canPut && !pConn->pOpts->put1)){
    MQLONG opts = MQOO_FAIL_IF_QUIESCING;
    if(canPut && !pConn->pOpts->put1) opts |= MQOO_OUTPUT;
    if(canGet) {
      opts |= MQOO_INPUT_SHARED;
      if(pConn->pOpts->readAhead) opts |= MQOO_READ_AHEAD;
    }

    if(bpstrnlen(getQMName(), MQ_Q_MGR_NAME_LENGTH)==0)
      setQMName(&(pConn->pOpts->QMName));

    if(log){
      snprintf(pConn->msg, CPH_MQC_MSG_LEN, "[%s] Opening %s: %.*s", pConn->name,
          desc, MQ_Q_NAME_LENGTH, getName());
      if(bpstrncmp(pConn->pOpts->QMName, getQMName(), MQ_Q_MGR_NAME_LENGTH))
        snprintf(pConn->msg+strlen(pConn->msg), CPH_MQC_MSG_LEN-strlen(pConn->msg),
            " (QM: %.*s)",
            MQ_Q_MGR_NAME_LENGTH, getQMName());
      cphLogPrintLn(pConn->pLog, LOGVERBOSE, pConn->msg);
    }

    CPHCALLMQ(pConn->pTrc, MQOPEN, pConn->hConn, &od, opts, &hObj)
  }
  CPHTRACEEXIT(pConn->pTrc)
}

/*
 * Method: put
 * -----------
 *
 * Put a message to this MQIObject.
 */
void MQIObject::put(MQIMessage const * const msg, MQMD& md, MQPMO& pmo) {
  CPHTRACEENTRY(pConn->pTrc)
  if(!canPut)
    throw logic_error("An attempt was made to put to an MQ object that was not open for output.");

  if(pConn->pOpts->put1)
    put1(msg, md, pmo);
  else if(hObj!=MQHO_NONE && hObj!=MQHO_UNUSABLE_HOBJ){
    CPHTRACEMSG(pConn->pTrc, "Putting message:\n[%s]", msg->buffer)
    CPHCALLMQ(pConn->pTrc, MQPUT, pConn->hConn, hObj, &md, &pmo, msg->messageLen, msg->buffer)
  } else
    throw logic_error("Cannot put to non-open object handle if put1 is not specified.");

  CPHTRACEEXIT(pConn->pTrc)
}

/*
 * Method: put1
 * ------------
 *
 * Put a single message to the queue described by the given MQOD.
 */
void MQIObject::put1(MQIMessage const * const msg, MQMD& md, MQPMO& pmo) {
  CPHTRACEENTRY(pConn->pTrc)
  CPHTRACEMSG(pConn->pTrc, "Put-1-ing message:\n[%s]", msg->buffer)
  CPHCALLMQ(pConn->pTrc, MQPUT1, pConn->hConn, &od, &md, &pmo, msg->messageLen, msg->buffer)
  CPHTRACEEXIT(pConn->pTrc)
}

/*
 * Method: get
 * -----------
 *
 * Get a message from this MQIObject.
 */
void MQIObject::get(MQIMessage * const msg, MQMD& md, MQGMO& gmo) const {
  CPHTRACEENTRY(pConn->pTrc)
  if(!canGet)
    throw logic_error("An attempt was made to get from an MQ object that was not open for input.");
  checkOpen();

  MQMD mdCopy;
  MQGMO gmoCopy;
  bool waitUnlimited = gmo.WaitInterval == MQWI_UNLIMITED;

  //if(!(gmo.Options&MQGMO_ACCEPT_TRUNCATED_MSG)){
    mdCopy = md;
    gmoCopy = gmo;
  //}

  MQLONG mqcc=0, mqrc=0;

  while(true){

	//Removing check here, which was likely inserted to speed up controlled shutdown (Ctrl-c)
    //It has the side affect of the requester stopping before obtaining its final message
    //pConn->pCurrentThread->checkShutdown();

    if(waitUnlimited) gmo.WaitInterval = CPH_TIMEOUT_UNLIMITED;
    CPHTRACEMSG(pConn->pTrc, "About to call MQGET.")
    MQGET(pConn->hConn, hObj, &md, &gmo, msg->bufferLen, msg->buffer, &msg->messageLen, &mqcc, &mqrc);
    // Reset WaitInterval in case we return.
    if(waitUnlimited) gmo.WaitInterval = MQWI_UNLIMITED;

    switch(mqrc){

    case MQRC_TRUNCATED_MSG_ACCEPTED:
      CPHTRACEMSG(pConn->pTrc, "Accepted truncated buffer. Growing buffer to %ld bytes.", msg->messageLen)
      mqcc = msg->bufferLen;
      msg->resize(msg->messageLen);
      msg->messageLen = mqcc;
      // v Keep going v

    case MQRC_NONE:
      CPHTRACEMSG(pConn->pTrc, "Got message:\n[%s]", msg->buffer)
      CPHTRACEEXIT(pConn->pTrc)
      return;

    case MQRC_TRUNCATED_MSG_FAILED:
      CPHTRACEMSG(pConn->pTrc, "Message buffer to small. Growing buffer to %ld bytes and retrying.", msg->messageLen)
      msg->resize(msg->messageLen);
      msg->messageLen = 0;

      md = mdCopy;
      gmo = gmoCopy;
      continue;

    case MQRC_NO_MSG_AVAILABLE:
      /*
       * If we get MQRC_NO_MSG_AVAILABLE,
       * and the thread has been signalled to shut down,
       * the likelihood is that this failure is because the
       * corresponding putter has already stopped,
       * in which case we should throw a ShutdownException instead
       * of an MQIException.
       */
      pConn->pCurrentThread->checkShutdown();
      if(waitUnlimited)
        continue;

      // v Keep going v
    default:
      printf("MQGET Return code caused error or not recognized; mqrc:%ld\n", mqrc);
      printf("MQGET Throwing exception; mqcc:%ld ;Name: %s\n", mqcc, getName());
      fprintf(stderr, "MQGET Return code caused error or not recognized; mqrc:%ld\n", mqrc);
      fprintf(stderr, "MQGET Throwing exception; mqcc:%ld ;Name: %s\n", mqcc, getName());
      throw MQIException("MQGET", mqcc, mqrc, md.CorrelId , getName());
    }
  }
}

#define f_id4 "%02X"

/*
* Method: createSelector
* ----------------------
*
* Sets the selection string in the object descriptor.
* Will select on correlID if no custom selector is specified.
*/
void MQIObject::createSelector(CPH_TRACE * pTrc, MQBYTE24 correlId, char * customSelector){

  //Assign selection string to custom selector or to memory
  selectionString = (customSelector == 0 ? (char *)malloc(MQ_SELECTOR_LENGTH) : customSelector);
  assert(NULL != selectionString);

  //Convert correlId to char if not using custom selector
  if (customSelector == 0){
    int len = sprintf(selectionString, "%s", "Root.MQMD.CorrelId = \"0x");
    char * convertedId = (char *)correlId;

    for (int i = 0; i < 24; ++i){
      len += sprintf(selectionString + len, f_id4, convertedId[i]);
    }
    strcat(selectionString, "\"");
  }

  CPHTRACEENTRY(pTrc)
    CPHTRACEMSG(pTrc, "Selection string: %s", selectionString);
  CPHTRACEEXIT(pTrc)

  od.SelectionString.VSPtr = selectionString;
  od.SelectionString.VSLength = (MQLONG) strlen(selectionString);
  od.SelectionString.VSBufSize = MQ_SELECTOR_LENGTH;
  od.SelectionString.VSOffset = 0;
  od.SelectionString.VSCCSID = MQCCSI_APPL;
}


/*
 * ------------------------------------
 * Method implementations for MQIQueue
 * ------------------------------------
 */

MQIQueue::MQIQueue(MQIConnection const * const pConnection, bool put, bool get):
    MQIObject(pConnection, MQOT_Q, put, get,
        put
        ? (get ? "input/output queue" : "output queue")
        : (get ? "input queue" : "queue")){}

MQIQueue::~MQIQueue(){}

char const * MQIQueue::getName() const {
  return od.ObjectName;
}

/*
 * Method: setName
 * ---------------
 *
 * Sets the name of the queue using a printf-style format.
 *
 * If the queue name is to be set from a blank-padded
 * (rather than null-terminated), fixed length string - such as
 * the ReplyToQ field of an incoming message descriptor -
 * then the setName(MQCHAR48 const *) overload should be used instead
 * by passing a pointer to the character array.
 */
void MQIQueue::setName(char const * const format, ...) {
  CPHTRACEENTRY(pConn->pTrc)
  checkNotOpen();
  VSPRINTF_FIXED("queue name", od.ObjectName, MQ_Q_NAME_LENGTH, format)
  CPHTRACEEXIT(pConn->pTrc)
}

/*
 * Method: setName
 * ---------------
 *
 * Sets the name of the queue.
 *
 * The input string can be either blank padded or null-terminated.
 */
void MQIQueue::setName(MQCHAR48 const * name){
  CPHTRACEENTRY(pConn->pTrc)
  checkNotOpen();
  strncpy(od.ObjectName, *name, MQ_Q_NAME_LENGTH);
  CPHTRACEEXIT(pConn->pTrc)
}



/*
 * -----------------------------------
 * Method implementations for MQITopic
 * -----------------------------------
 */

MQITopic::MQITopic(MQIConnection const * const pConnection):
    MQIObject(pConnection, MQOT_TOPIC, true, false, "topic") {
  CPHTRACEENTRY(pConn->pTrc)
  od.ObjectString.VSLength = MQVS_NULL_TERMINATED;
  CPHTRACEEXIT(pConn->pTrc)
}

MQITopic::~MQITopic(){
  CPHTRACEENTRY(pConn->pTrc)
  if(od.ObjectString.VSPtr!=NULL)
    free(od.ObjectString.VSPtr);
  CPHTRACEEXIT(pConn->pTrc)
}

VS_GET_SET_NAME(MQITopic, Name, "topic string", od.ObjectString)

/*
 * ------------------------------------------
 * Method implementations for MQISubscription
 * ------------------------------------------
 */

MQSD const protoSD = {MQSD_DEFAULT};

MQISubscription::MQISubscription(MQIConnection const * const pConnection) :
    MQIObject(pConnection, MQOT_NONE, false, true, "subscription to topic"),
    hSub(MQHO_NONE), unsubscribeOnClose(true) {
  CPHTRACEENTRY(pConn->pTrc)
  sd = protoSD;
  sd.Options = MQSO_CREATE | MQSO_FAIL_IF_QUIESCING | MQSO_MANAGED | MQSO_NON_DURABLE;
  if (pConn->pOpts->readAhead) sd.Options |= MQSO_READ_AHEAD;
  sd.ObjectString.VSLength = MQVS_NULL_TERMINATED;
  CPHTRACEEXIT(pConn->pTrc)
}

MQISubscription::~MQISubscription() {
  CPHTRACEENTRY(pConn->pTrc)
  if(hSub!=MQHO_NONE && hSub!=MQHO_UNUSABLE_HOBJ){
    try {
      CPHCALLMQ(pConn->pTrc, MQCLOSE, pConn->hConn, &hSub, unsubscribeOnClose ? MQCO_REMOVE_SUB : MQCO_NONE)
    } catch (cph::MQIException) {
      CPHTRACEMSG(pConn->pTrc, "OOPS! MQIException in ~MQISubscription()!")
    }
  }
  if(sd.ObjectString.VSPtr!=NULL) free(sd.ObjectString.VSPtr);
  if(sd.SubName.VSPtr!=NULL) free(sd.SubName.VSPtr);
  CPHTRACEEXIT(pConn->pTrc)
}

void MQISubscription::setDurable(bool unsubscribeOnClose, char const * const subNameFormat, ...){
  CPHTRACEENTRY(pConn->pTrc)
  checkNotOpen();
  sd.Options &= ~MQSO_NON_DURABLE;
  sd.Options |= MQSO_DURABLE | MQSO_RESUME;
  sd.SubName.VSLength = MQVS_NULL_TERMINATED;

  this->unsubscribeOnClose = unsubscribeOnClose;
  VSPRINTF_VAR("subscription name", sd.SubName.VSPtr, sd.SubName.VSBufSize, subNameFormat)

  CPHTRACEEXIT(pConn->pTrc)
}

VS_GET_SET_NAME(MQISubscription, Name, "topic string", sd.ObjectString)
VS_GET_SET_NAME(MQISubscription, TopicString, "topic string", sd.ObjectString)
VS_GET_SET_NAME(MQISubscription, SubName, "subscription name", sd.SubName)

void MQISubscription::open(bool log) {
  CPHTRACEENTRY(pConn->pTrc)
  if(log){
    snprintf(pConn->msg, CPH_MQC_MSG_LEN, "[%s] Subscribng to topic string: %s", pConn->name, getTopicString());
    cphLogPrintLn(pConn->pLog, LOGVERBOSE, pConn->msg);
  }
  CPHCALLMQ(pConn->pTrc, MQSUB, pConn->hConn, &sd, &hObj, &hSub)
  CPHTRACEEXIT(pConn->pTrc)
}

}
