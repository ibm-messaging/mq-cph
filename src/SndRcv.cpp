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

/*
 * Macro: SROC
 * -----------
 *
 * Defines the openDestination and closeDestination methods
 * of the Sender or Receiver class,
 * whose implementations of these methods are virtually identical.
 */
#define SROC(CLASS, PUT)\
void CLASS::openDestination(){\
  CPHTRACEENTRY(pConfig->pTrc)\
  pQueue = new MQIQueue(pConnection, PUT, ! PUT);\
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pQueue->setName, pOpts->destinationPrefix, destinationIndex)\
  pQueue->open(true);\
  CPHTRACEEXIT(pConfig->pTrc)\
}\
\
void CLASS::closeDestination(){\
  CPHTRACEENTRY(pConfig->pTrc)\
  delete pQueue;\
  pQueue = NULL;\
  CPHTRACEEXIT(pConfig->pTrc)\
}

namespace cph {

MQWTCONSTRUCTOR(Sender, true, false, false) {}
Sender::~Sender() {}
SROC(Sender, true)

void Sender::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  pQueue->put(putMessage, putMD, pmo);
  CPHTRACEEXIT(pConfig->pTrc)
}

MQWTCONSTRUCTOR(Receiver, false, true, false) {}
Receiver::~Receiver() {}
SROC(Receiver, false)

void Receiver::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  MQMD md = {MQMD_DEFAULT};
  pQueue->get(getMessage, md, gmo);
  CPHTRACEEXIT(pConfig->pTrc)
}

}
