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

#include <cstring>
#ifdef _MSC_VER
#include "msio.h"
#else
#include <cstdio>
#endif
#include "Requester.hpp"
#include "cphLog.h"

namespace cph {

bool Requester::useCorrelId = true;
bool Requester::useSelector = false;
bool Requester::useCustomSelector = false;
char Requester::customSelector[MQ_SELECTOR_LENGTH];

int Requester::dqChannels = 1;
char Requester::iqPrefix[MQ_Q_NAME_LENGTH];
char Requester::oqPrefix[MQ_Q_NAME_LENGTH];

MQWTCONSTRUCTOR(Requester, true, true) {
  CPHTRACEENTRY(pConfig->pTrc)

  if(threadNum==0){
    cphConfigRegisterModule(pConfig, (char*) className.data());

    if(pOpts->commitFrequency>1)
      configError(pConfig, "(cc) A commit-count greater than one wont work for Requester. Duh!");

    int temp = 0;

    // Use correlId?
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "co"))
      configError(pConfig, "(co) Cannot determine whether to use correlIDs.");
    useCorrelId = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use correlIDs: %s", useCorrelId ? "yes" : "no")

    // Use message selectors?
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cs"))
      configError(pConfig, "(cs) Cannot determine whether to use message selectors.");
    useSelector = temp == CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use message selectors: %s", useSelector ? "yes" : "no")

    //Use generic selector? (not selecting on correlId)
    if (useSelector){
      if (CPHTRUE != cphConfigGetString(pConfig, (char *) &customSelector, "gs"))
        configError(pConfig, "(gs) Cannot determine whether to use generic selector.");
      useCustomSelector = (0 != strcmp(customSelector, "") ? true : false);
      CPHTRACEMSG(pConfig->pTrc, "Use generic selector: %s", useCustomSelector ? "yes" : "no")
    }

    // Input (request) queue.
    if(CPHTRUE != cphConfigGetString(pConfig, (char*) &iqPrefix, "iq"))
      configError(pConfig, "(iq) Cannot determine input (request) queue prefix.");
    CPHTRACEMSG(pConfig->pTrc, "Input (request) queue prefix: %s", iqPrefix)

    // Output (reply) queue.
    if(CPHTRUE != cphConfigGetString(pConfig, (char*) &oqPrefix, "oq"))
      configError(pConfig, "(oq) Cannot determine output (reply) queue prefix.");
    CPHTRACEMSG(pConfig->pTrc, "Output (reply) queue prefix: %s", oqPrefix)

    // DQ Channels
    if(CPHTRUE != cphConfigGetInt(pConfig, (int*) &dqChannels, "dq"))
      configError(pConfig, "(dq) Cannot determine number of dq channels");
    CPHTRACEMSG(pConfig->pTrc, "Number of DQ channels to configure: %d", dqChannels)
  }

  if (useSelector)
    generateCorrelID(correlId, pControlThread->procId);

  CPHTRACEEXIT(pConfig->pTrc)
}
Requester::~Requester() {}

void Requester::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Open request queue
  pInQueue = new MQIQueue(pConnection, true, false);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pInQueue->setName, iqPrefix, destinationIndex)
  pInQueue->open(true);

  // Open reply queue
  pOutQueue = new MQIQueue(pConnection, false, true);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pOutQueue->setName, oqPrefix, destinationIndex)
  if (useSelector){
    pOutQueue->createSelector(pConfig->pTrc, correlId, useCustomSelector ? customSelector : NULL);
  }
  pOutQueue->open(true);

  // Set message type to request
  putMD.MsgType = MQMT_REQUEST;

  //set correlId of message descriptor so requester can select message from reply queue
  if (useSelector){
    memcpy(putMD.CorrelId, correlId, sizeof(MQBYTE24));
    pmo.Options &= ~MQPMO_NEW_CORREL_ID;
  }

  // Set ReplyTo QM
  // We should let the QM set the reply to QM as this will default to local QM, but will also allow us to override
  // the ReplyTo QM using queue alias if required. This is the mechanism used to support multiple return channels
  // strncpy(putMD.ReplyToQMgr, pOpts->QMName, MQ_Q_MGR_NAME_LENGTH);

  //If more than one DQ channel is in use, we need to fixup the replyToQ to a Q alias as defined on the client QM
  //We are using QMX.REPLYZ syntax, i.e. PERF1.REPLY1
  if (dqChannels > 1) {
    snprintf(putMD.ReplyToQ, MQ_Q_NAME_LENGTH, "%s.%s", pOpts->QMName, pOutQueue->getName());
  } else {
    // Set ReplyTo queue
    if(destinationIndex>=0)
      snprintf(putMD.ReplyToQ, MQ_Q_NAME_LENGTH, "%s%d", oqPrefix, destinationIndex);
    else
      strncpy(putMD.ReplyToQ, oqPrefix, MQ_Q_NAME_LENGTH);
  }

  CPHTRACEEXIT(pConfig->pTrc)
}

void Requester::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  delete pInQueue;
  pInQueue = NULL;

  delete pOutQueue;
  pOutQueue = NULL;
  CPHTRACEEXIT(pConfig->pTrc)
}

void Requester::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  // Put request
  pInQueue->put(putMessage, putMD, pmo);

  // Commit transaction if necessary,
  // otherwise we won't be able to get our reply.
  if(pOpts->commitFrequency) pConnection->commitTransaction();

  // Get reply
  MQMD getMD = pOpts->getGetMD();
  if(useCorrelId && !useSelector){
    memcpy(getMD.CorrelId, putMD.MsgId, sizeof(MQBYTE24));
    cphTraceId(pConfig->pTrc, "Correlation ID", getMD.CorrelId);
  }

  pOutQueue->get(getMessage, getMD, gmo);

  CPHTRACEEXIT(pConfig->pTrc)
}

}
