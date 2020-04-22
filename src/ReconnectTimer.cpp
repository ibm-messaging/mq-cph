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

#include "ReconnectTimer.hpp"
#include <string.h>
#include "cphConfig.h"
#include "cphLog.h"
#include "MQI.hpp"
#include <limits.h>

namespace cph {

/*Whether or not to associate a correlId with the message being put/got.*/
bool ReconnectTimer::useCorrelId = true;
bool ReconnectTimer::useSelector = false;
bool ReconnectTimer::usingPrimaryQM = true;
bool ReconnectTimer::useCustomSelector = false;
char ReconnectTimer::customSelector[MQ_SELECTOR_LENGTH];
long ReconnectTimer::maxReconnectTime_ms;
long ReconnectTimer::minReconnectTime_ms;
int ReconnectTimer::numThreadsReconnected;
int ReconnectTimer::totalThreads;
char ReconnectTimer::secondaryHostName[80];              //Name of machine hosting queue manager
unsigned int ReconnectTimer::secondaryPortNumber;
Lock ReconnectTimer::rtGate;

MQWTCONSTRUCTOR(ReconnectTimer, true, true, true) {
  CPHTRACEENTRY(pConfig->pTrc)
  if(threadNum==0){
	MQIException::printDetails = false;  //turn off detailed MQIException messages to avoid flooding the console, when primary QM is switched/killed.
    if(pOpts->commitFrequency>1)
      configError(pConfig, "(cc) A commit-count greater than one wont work for ReconnectTimer as it's a PutGet variant");

    int temp = 0;
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "co"))
      configError(pConfig, "(co) Cannot determine whether to use correlIDs.");
    useCorrelId = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use correlIDs: %s", useCorrelId ? "yes" : "no")

    //using selectors?
    temp = 0;
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cs"))
      configError(pConfig, "(cs) Cannot determine whether to use message selectors.");
    useSelector = temp == CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use message selectors: %s", useSelector ? "yes" : "no")

    //Use generic selector? (not selecting on correlId)
    if (useSelector){
      int temp = cphConfigGetString(pConfig, (char *)&customSelector, "gs") == CPHTRUE;
      useCustomSelector = (temp && (0 != strcmp(customSelector, "")) ? true : false);
      if (!useCustomSelector)
        configError(pConfig, "(gs) Cannot determine whether to use generic selector.");
      CPHTRACEMSG(pConfig->pTrc, "Use generic selector: %s", useCustomSelector ? customSelector : "no")
    }
	
	//Secondary Host name
	if (CPHTRUE != cphConfigGetString(pConfig, secondaryHostName, "h2"))
	  configError(pConfig, "(h2) Default secondary host name cannot be retrieved.");
	CPHTRACEMSG(pConfig->pTrc, "Default secondary host name: %s", secondaryHostName)
		
	//Secondary Port number
	if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &secondaryPortNumber, "p2"))
	  configError(pConfig, "(p2) Default secondary port number cannot be retrieved.");
	if(secondaryPortNumber==0)secondaryPortNumber=pOpts->portNumber;
	CPHTRACEMSG(pConfig->pTrc, "Default secondary port number: %d", secondaryPortNumber)
	
	printf("Secondary port number: %i\n",secondaryPortNumber);
	
	maxReconnectTime_ms=0;
	minReconnectTime_ms=LONG_MAX;
	numThreadsReconnected=0;
	
    if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &totalThreads, "nt"))
      configError(pConfig, "(nt) Could not determine number of worker threads.");    
  }

  if(useCorrelId)
    generateCorrelID(correlId, pControlThread->procId);

  CPHTRACEEXIT(pConfig->pTrc)
	  
  
}
ReconnectTimer::~ReconnectTimer() {}

void ReconnectTimer::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Open the put/get queue
  pQueue = new MQIQueue(pConnection, true, true);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pQueue->setName, pOpts->destinationPrefix, destinationIndex)

    //create message selector if required
  if (useSelector){
    pQueue->createSelector(pConfig->pTrc, correlId, useCustomSelector ? customSelector : NULL);
  }

  pQueue->open(true);

  if(useCorrelId){
    memcpy(putMD.CorrelId, correlId, sizeof(MQBYTE24));
    pmo.Options &= ~MQPMO_NEW_CORREL_ID;
  }
  CPHTRACEEXIT(pConfig->pTrc)
}

void ReconnectTimer::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  // Close the put/get queue
  delete pQueue;
  CPHTRACEEXIT(pConfig->pTrc)
}

static bool firstReconnectAttempt=true;

void ReconnectTimer::reconnect(){
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

void ReconnectTimer::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)

  //Put message
  int rc=0;
  rc = pQueue->put_try(putMessage, putMD, pmo);
  if (rc !=0){	  
	  reconnect();
	  return;
  }
  //printf("MQPUT Reason code : %i\n",rc);
  
  /*
   * We commit the transaction now,
   * otherwise we won't be able to get the message back again.
   * This means we're doing 2 commits per iteration.
   */
  if(pOpts->txSet) {
	  rc = pConnection->commitTransaction_try();
	  //printf("MQCMIT Reason code : %i\n",rc);
	  if (rc !=0){	  
		  reconnect();
		  return;
	  }

  } 

  //Get it back
  MQMD getMD = pOpts->getGetMD();
  if(useCorrelId && !useSelector) memcpy(getMD.CorrelId, correlId, sizeof(MQBYTE24));
  rc = pQueue->get_try(getMessage, getMD, gmo);
  //printf("MQGET Reason code : %i\n",rc);
  if (rc !=0){	  
	  reconnect();
	  return;
  }
  
  if(pOpts->txSet) {
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
