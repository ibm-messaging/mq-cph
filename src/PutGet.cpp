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

#include "PutGet.hpp"
#include <string.h>
#include "cphConfig.h"

namespace cph {

/*Whether or not to associate a correlId with the message being put/got.*/
bool PutGet::useCorrelId = true;

bool PutGet::useSelector = false;
bool PutGet::useCustomSelector = false;
char PutGet::customSelector[MQ_SELECTOR_LENGTH];

MQWTCONSTRUCTOR(PutGet, true, true, false) {
  CPHTRACEENTRY(pConfig->pTrc)

  if(threadNum==0){

    if(pOpts->commitFrequency>1)
      configError(pConfig, "(cc) A commit-count greater than one wont work for PutGet. Duh!");

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
	  temp = 0;
    if (useSelector){
      temp = cphConfigGetString(pConfig, (char *)&customSelector, sizeof(customSelector), "gs") == CPHTRUE;
      useCustomSelector = (temp && (0 != strcmp(customSelector, "")) ? true : false);
      // We now support selectors with the use of message handles, so this is no longer an error
      //if (!useCustomSelector)
      //  configError(pConfig, "(gs) Cannot determine whether to use generic selector.");
      CPHTRACEMSG(pConfig->pTrc, "Use generic selector: %s", useCustomSelector ? customSelector : "no")
    }
  }

  if(useCorrelId || useSelector)
    generateCorrelID(correlId, pControlThread->procId);

  CPHTRACEEXIT(pConfig->pTrc)
}
PutGet::~PutGet() {}

void PutGet::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Open the put/get queue
  pQueue = new MQIQueue(pConnection, true, true);
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

  if(useCorrelId || useSelector){
    memcpy(putMD.CorrelId, correlId, sizeof(MQBYTE24));
    pmo.Options &= ~MQPMO_NEW_CORREL_ID;
  }
  CPHTRACEEXIT(pConfig->pTrc)
}

void PutGet::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  // Close the put/get queue
  delete pQueue;
  CPHTRACEEXIT(pConfig->pTrc)
}

void PutGet::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)

  //Put message
  pQueue->put(putMessage, putMD, pmo);
  /*
   * We commit the transaction now,
   * otherwise we won't be able to get the message back again.
   * This means we're doing 2 commits per iteration.
   */
  if(pOpts->commitPGPut) pConnection->commitTransaction();

  //Get it back
  MQMD getMD = pOpts->getGetMD();
  if(useCorrelId && !useSelector) memcpy(getMD.CorrelId, correlId, sizeof(MQBYTE24));
  pQueue->get(getMessage, getMD, gmo);

  CPHTRACEEXIT(pConfig->pTrc)
}

}
