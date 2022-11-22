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

#include <string.h>
#include "Forwarder.hpp"
#include "cphLog.h"

using namespace std;

namespace cph {

/*Whether to copy the input's message ID to the output*/
bool Forwarder::copyMD = true;
/*Whether or not to send the input message on, or generate a new one*/
bool Forwarder::copyMsg = true;
/*The prefix of the input queue name, with the suffix to be provided by destinationIndex*/
char Forwarder::iqPrefix[MQ_Q_NAME_LENGTH];
/*The prefix of the output queue name, with the suffix to be provided by destinationIndex*/
char Forwarder::oqPrefix[MQ_Q_NAME_LENGTH];
/*Whether or not to commit transactions between getting a message and putting one.*/
bool Forwarder::commitBetween = false;

MQWTCONSTRUCTOR(Forwarder, true, true, false), pInQueue(NULL), pOutQueue(NULL) {
  CPHTRACEENTRY(pConfig->pTrc)

  if(threadNum==0){

    if(pOpts->timeout!=-1)
      cphLogPrintLn(pConfig->pLog, LOG_WARNING,
          "\t******************************************************************************\n"
          "\t* WARNING - (to) Setting the MQGET timeout to something other than -1        *\n"
          "\t*           (i.e. wait indefinitely) for Forwarders is probably not what you *\n"
          "\t*           intend.                                                          *\n"
          "\t******************************************************************************");

    int temp = 0;

    // Copy MD?
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cm"))
      configError(pConfig, "(cm) Cannot determine whether to copy MDs.");
    copyMD = temp == CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Copy MDs: %s", copyMD ? "yes" : "no")

    // Copy message?
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cr"))
      configError(pConfig, "(cr) Cannot determine whether to copy input message to output.");
    copyMsg = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Copy input message to output: %s", copyMsg ? "yes" : "no")

    // Input queue.
    if(CPHTRUE != cphConfigGetString(pConfig, (char*) &iqPrefix, sizeof(iqPrefix), "iq")){
      cphLogPrintLn(pConfig->pLog, LOG_INFO,
          "No input queue prefix parameter (-iq) detected. Falling back on destination prefix (-d).");
      strncpy(iqPrefix, pOpts->destinationPrefix, MQ_Q_NAME_LENGTH);
    }
    CPHTRACEMSG(pConfig->pTrc, "Input queue prefix: %s", iqPrefix)

    // Output queue.
    if(CPHTRUE != cphConfigGetString(pConfig, (char*) &oqPrefix, sizeof(oqPrefix), "oq")){
      cphLogPrintLn(pConfig->pLog, LOG_INFO,
          "No output queue prefix parameter (-oq) detected. Falling back on destination prefix (-d).");
      strncpy(oqPrefix, pOpts->destinationPrefix, MQ_Q_NAME_LENGTH);
    }
    CPHTRACEMSG(pConfig->pTrc, "Output queue prefix: %s", oqPrefix)

    // Commit between?
    if(pOpts->commitFrequency){
      if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cb"))
        configError(pConfig, "(cb) Cannot determine whether to commit between getting the input and putting the output.");
      commitBetween = temp==CPHTRUE;
      CPHTRACEMSG(pConfig->pTrc, "Commit between getting the input and putting the output: %s", commitBetween ? "yes" : "no")
    }
  }

  CPHTRACEEXIT(pConfig->pTrc)
}
Forwarder::~Forwarder() {}

void Forwarder::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Open input queue
  pInQueue = new MQIQueue(pConnection, false, true);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pInQueue->setName, iqPrefix, destinationIndex)
  pInQueue->open(true);

  // Open output queue
  pOutQueue = new MQIQueue(pConnection, true, false);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pOutQueue->setName, oqPrefix, destinationIndex)
  pOutQueue->open(true);

  // Don't generate new correl IDs if we're correlating
  if(copyMD)
    pmo.Options &= ~MQPMO_NEW_CORREL_ID;

  CPHTRACEEXIT(pConfig->pTrc)
}

void Forwarder::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Close input queue
  delete pInQueue;
  pInQueue = NULL;

  // Close output queue
  delete pOutQueue;
  pOutQueue = NULL;

  CPHTRACEEXIT(pConfig->pTrc)
}

void Forwarder::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Get request message
  MQMD getMD = pOpts->getGetMD();
  getMessage->messageLen = 0;

  pInQueue->get(getMessage, getMD, gmo);

  if(pOpts->commitFrequency>0 && commitBetween && (getIterations()+1)%pOpts->commitFrequency==0)
    pConnection->commitTransaction();

  // Put reply
  pOutQueue->put(copyMsg ? getMessage : putMessage, copyMD ? getMD : putMD, pmo);

  CPHTRACEEXIT(pConfig->pTrc)
}

}
