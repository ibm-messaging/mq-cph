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

#include "SndRcv.hpp"
#include "cphLog.h"
#include <sstream>
#include <cstring>


namespace cph {

/*Whether or not to associate a correlId with the message being put/got.*/
bool Sender::useCorrelId = false;
bool Sender::useSelector = false;
bool Sender::useCustomSelector = false;
char Sender::customSelector[MQ_SELECTOR_LENGTH];

bool Receiver::useCorrelId = false;
bool Receiver::useSelector = false;
bool Receiver::useCustomSelector = false;
char Receiver::customSelector[MQ_SELECTOR_LENGTH];

MQWTCONSTRUCTOR(Sender, true, false, false) {
  CPHTRACEENTRY(pConfig->pTrc)
  Sender::pQueue = NULL;
  if (threadNum==0) {

    int temp = 0;
    // To support correlation IDs with Sender Receiver class, we are only going to use our generated correlationIDs so that we can control
    // the uniqueness. Using MQ generated MsgIds would be unique per message.
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "co")) {
      configError(pConfig, "(co) Cannot determine whether to use correlIDs.");
    }
    useCorrelId = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use correlIDs: %s", useCorrelId ? "yes" : "no")

    //using selectors?
    temp = 0;
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cs")) {
      configError(pConfig, "(cs) Cannot determine whether to use message selectors.");
    }
    useSelector = temp == CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use message selectors: %s", useSelector ? "yes" : "no")
  }

  if(useCorrelId || useSelector) {
    generateCorrelID(correlId, pControlThread->procId);
  }

  CPHTRACEEXIT(pConfig->pTrc)
}

Sender::~Sender() {}

void Sender::openDestination() {
  CPHTRACEENTRY(pConfig->pTrc)
  pQueue = new MQIQueue(pConnection, true, false);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pQueue->setName, pOpts->destinationPrefix, destinationIndex)
  pQueue->open(true);
  if(useCorrelId || useSelector){
    memcpy(putMD.CorrelId, correlId, sizeof(MQBYTE24));
    //printf("Setting correlId in putMD: %*s\n", (int) sizeof(MQBYTE24), putMD.CorrelId);
    pmo.Options &= ~MQPMO_NEW_CORREL_ID;
  }

  CPHTRACEEXIT(pConfig->pTrc)
}

void Sender::closeDestination() {
  CPHTRACEENTRY(pConfig->pTrc)
  delete pQueue;
  pQueue = NULL;
  CPHTRACEEXIT(pConfig->pTrc)
}

void Sender::msgOneIteration() {
  CPHTRACEENTRY(pConfig->pTrc)
  pQueue->put(putMessage, putMD, pmo);
  CPHTRACEEXIT(pConfig->pTrc)
}


/* Receiver Class */
MQWTCONSTRUCTOR(Receiver, false, true, false) {
  CPHTRACEENTRY(pConfig->pTrc)
  Receiver::pQueue = NULL;

  if (threadNum==0) {

    int temp = 0;
    // To support correlation IDs with Sender Receiver class, we are only going to use our generated correlationIDs so that we can control
    // the uniqueness. Using MQ generated MsgIds would be unique per message.
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "co")) {
      configError(pConfig, "(co) Cannot determine whether to use correlIDs.");
    }
    useCorrelId = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use correlIDs: %s", useCorrelId ? "yes" : "no")

    //Using selectors by defining selection string in object definition?
    temp = 0;
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cs")) {
      configError(pConfig, "(cs) Cannot determine whether to use message selectors.");
    }
    useSelector = temp == CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use message selectors: %s", useSelector ? "yes" : "no")

	  //Use custom generic selector
    temp = 0;
    if (useSelector){
      temp = cphConfigGetString(pConfig, (char *)&customSelector, sizeof(customSelector), "gs") == CPHTRUE;
      useCustomSelector = (temp && (0 != strcmp(customSelector, "")) ? true : false);
      CPHTRACEMSG(pConfig->pTrc, "Use generic selector: %s", useCustomSelector ? customSelector : "no")
    }
  }

  if(useCorrelId || useSelector) {
    // Need to replace Receiver threadname with Sender so that the messages can be retrieved that were sent with Sender class
    // Format: <id>-Sender0 or 977df480-Sender0, padded to 8 and 15 chars respectively with spaces
    std::stringstream ss;
    ss << "Sender" << threadNum;
    adjustedClassName = ss.str();
    generateCorrelID(correlId, pControlThread->procId, &adjustedClassName);
  }

  CPHTRACEEXIT(pConfig->pTrc)
}

Receiver::~Receiver() {}

void Receiver::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  pQueue = new MQIQueue(pConnection, false, true);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pQueue->setName, pOpts->destinationPrefix, destinationIndex)

  //create message selector if required
  if (useSelector){
    if(pOpts->useMessageHandle) {
      pQueue->createMsgHandleSelector(pConfig->pTrc, correlId);
    } else {
      pQueue->createSelector(pConfig->pTrc, correlId, useCustomSelector ? customSelector : NULL);
    }
  }
  pQueue->open(true);

  CPHTRACEEXIT(pConfig->pTrc)
}

void Receiver::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  delete pQueue;
  pQueue = NULL;
  CPHTRACEEXIT(pConfig->pTrc)
}

void Receiver::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  //Receive a message using correlation ID. This will only work if -id is set on both sender and receiver as this will
  //statically set the first part of the correlationID. i.e. 1-Sender0  rather than 977df480-Sender0
  //If using selectors, weve already created message selector of this queue
  MQMD getMD = pOpts->getGetMD();
  if (useCorrelId && !useSelector) {
    memcpy(getMD.CorrelId, correlId, sizeof(MQBYTE24));
  }
  pQueue->get(getMessage, getMD, gmo);
  CPHTRACEEXIT(pConfig->pTrc)
}

}
