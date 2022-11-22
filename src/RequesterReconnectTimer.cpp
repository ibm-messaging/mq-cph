/*<copyright notice="lm-source" pids="" years="2014,2022">*/
/*******************************************************************************
 * Copyright (c) 2014,2022 IBM Corp.
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
#include "RequesterReconnectTimer.hpp"
#include "cphLog.h"
#include <limits.h>

namespace cph {

bool RequesterReconnectTimer::useCorrelId = true;
bool RequesterReconnectTimer::useSelector = false;
bool RequesterReconnectTimer::usingPrimaryQM = true;
bool RequesterReconnectTimer::useCustomSelector = false;
char RequesterReconnectTimer::customSelector[MQ_SELECTOR_LENGTH];
long RequesterReconnectTimer::maxReconnectTime_ms;
long RequesterReconnectTimer::minReconnectTime_ms;
int RequesterReconnectTimer::numThreadsReconnected;
int RequesterReconnectTimer::totalThreads;
char RequesterReconnectTimer::secondaryHostName[80];              //Name of machine hosting queue manager
unsigned int RequesterReconnectTimer::secondaryPortNumber;
Lock RequesterReconnectTimer::rtGate;

int RequesterReconnectTimer::dqChannels = 1;
char RequesterReconnectTimer::iqPrefix[MQ_Q_NAME_LENGTH];
char RequesterReconnectTimer::oqPrefix[MQ_Q_NAME_LENGTH];

MQWTCONSTRUCTOR(RequesterReconnectTimer, true, true, true) {
  CPHTRACEENTRY(pConfig->pTrc)

  if(threadNum==0){

    if(pOpts->commitFrequency>1)
      configError(pConfig, "(cc) A commit-count greater than one wont work for RequesterReconnectTimer. Duh!");

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
      if (CPHTRUE != cphConfigGetString(pConfig, (char *) &customSelector, sizeof(customSelector), "gs"))
        configError(pConfig, "(gs) Cannot determine whether to use generic selector.");
      useCustomSelector = (0 != strcmp(customSelector, "") ? true : false);
      CPHTRACEMSG(pConfig->pTrc, "Use generic selector: %s", useCustomSelector ? "yes" : "no")
    }

    //Secondary Host name
	  if (CPHTRUE != cphConfigGetString(pConfig, secondaryHostName, sizeof(secondaryHostName), "h2"))
	    configError(pConfig, "(h2) Default secondary host name cannot be retrieved.");
	  CPHTRACEMSG(pConfig->pTrc, "Default secondary host name: %s", secondaryHostName)
		
	  //Secondary Port number
	  if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &secondaryPortNumber, "p2"))
	    configError(pConfig, "(p2) Default secondary port number cannot be retrieved.");
	  if(secondaryPortNumber==0)secondaryPortNumber=pOpts->portNumber;
	  CPHTRACEMSG(pConfig->pTrc, "Default secondary port number: %d", secondaryPortNumber)
	
	  printf("Secondary port number: %u\n",secondaryPortNumber);

	  maxReconnectTime_ms=0;
	  minReconnectTime_ms=LONG_MAX;
	  numThreadsReconnected=0;

    if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &totalThreads, "nt"))
      configError(pConfig, "(nt) Could not determine number of worker threads.");   

    // Input (request) queue.
    if(CPHTRUE != cphConfigGetString(pConfig, (char*) &iqPrefix, sizeof(iqPrefix), "iq"))
      configError(pConfig, "(iq) Cannot determine input (request) queue prefix.");
    CPHTRACEMSG(pConfig->pTrc, "Input (request) queue prefix: %s", iqPrefix)

    // Output (reply) queue.
    if(CPHTRUE != cphConfigGetString(pConfig, (char*) &oqPrefix, sizeof(oqPrefix), "oq"))
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
RequesterReconnectTimer::~RequesterReconnectTimer() {}

void RequesterReconnectTimer::openDestination(){
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

  //set correlId of message descriptor so RequesterReconnectTimer can select message from reply queue
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

void RequesterReconnectTimer::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  delete pInQueue;
  pInQueue = NULL;

  delete pOutQueue;
  pOutQueue = NULL;
  CPHTRACEEXIT(pConfig->pTrc)
}

static bool firstReconnectAttempt=true;

void RequesterReconnectTimer::reconnect(){
	CPHTRACEENTRY(pConfig->pTrc)
	//ToDo: Test here for valid codes we're prepared to attempt reconnection for?
	rtGate.lock();
	   if(firstReconnectAttempt ){
		  if(!pOpts->useChannelTable){ 
	         if(usingPrimaryQM){
	            pOpts->resetConnectionDef(secondaryHostName,secondaryPortNumber);
   	            printf("[%s] MQI call failed, attempting to reconnect all threads to queue manager %s on host %s:\n", name.data(),pOpts->QMName,secondaryHostName);
             } else {
   	            pOpts->resetConnectionDef(pOpts->hostName,pOpts->portNumber);
   	            printf("[%s] MQI call failed, attempting to reconnect all threads to queue manager %s on host %s:\n", name.data(),pOpts->QMName,pOpts->hostName);

             }
		 } else {
		 	printf("[%s] MQI call failed, attempting to reconnect all threads to next channel in CCDT\n", name.data());
		 }
	      firstReconnectAttempt=false;
	   }
   	rtGate.notify();	
	rtGate.unlock();
	reconnectStart = cphUtilGetNow();	
	delete pConnection;
	pConnection = new MQIConnection(this, true);
	reconnectTime_ms = cphUtilGetTimeDifference(cphUtilGetNow(),reconnectStart);
	
	if (reconnectTime_ms > maxReconnectTime_ms)maxReconnectTime_ms = reconnectTime_ms;
	if (reconnectTime_ms < minReconnectTime_ms)minReconnectTime_ms = reconnectTime_ms;
	
    sprintf(messageString, "[%s]: Time to connect to secondary host is %ld ms (min: %ld ms  max: %ld ms)",name.data(),reconnectTime_ms,minReconnectTime_ms,maxReconnectTime_ms);
    cphLogPrintLn(pConfig->pLog, LOG_INFO, messageString);
	
	numThreadsReconnected++;
	if(numThreadsReconnected == totalThreads){
       char chTime[80];
       cphUtilGetTraceTime(chTime);
	   sprintf(messageString, "[%s]: All threads reconnected at %s",name.data(),chTime);
	   cphLogPrintLn(pConfig->pLog, LOG_INFO, messageString);
	   if(usingPrimaryQM){
	     usingPrimaryQM=false;	
	   } else {
		 usingPrimaryQM=true;
	   }
	   numThreadsReconnected=0;
	   firstReconnectAttempt=true;
   	   maxReconnectTime_ms=0;
   	   minReconnectTime_ms=LONG_MAX;
    }
	openDestination();		
	CPHTRACEEXIT(pConfig->pTrc)
}

void RequesterReconnectTimer::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  // Put request
  int rc=0;
  rc = pInQueue->put_try(putMessage, putMD, pmo);
  if (rc !=0){	  
     reconnect();
     return;
  }	  
  //pInQueue->put(putMessage, putMD, pmo);

  // Commit transaction if necessary,
  // otherwise we won't be able to get our reply.
  if(pOpts->txSet) {
     rc = pConnection->commitTransaction_try();
     //printf("MQCMIT Reason code : %i\n",rc);
     if (rc !=0){	  
	    reconnect();
	    return;
     }
  }
  //if(pOpts->commitPGPut) pConnection->commitTransaction();

  // Get reply
  MQMD getMD = pOpts->getGetMD();
  if(useCorrelId && !useSelector){
    memcpy(getMD.CorrelId, putMD.MsgId, sizeof(MQBYTE24));
    cphTraceId(pConfig->pTrc, "Correlation ID", getMD.CorrelId);
  }

  rc = pOutQueue->get_try(getMessage, getMD, gmo);
  //printf("MQGET Reason code : %i\n",rc);
  if (rc !=0){	  
	  reconnect();
	  return;
  }
  //pOutQueue->get(getMessage, getMD, gmo);

  if(pOpts->txSet) {  //We're registered as a reconnector class so we do all commits ourselves
  	rc = pConnection->commitTransaction_try();
	//printf("MQCMIT Reason code : %i\n",rc);
    if (rc !=0){	  
	  reconnect();
	  return;
    }
  } 

  CPHTRACEEXIT(pConfig->pTrc)
}

}
