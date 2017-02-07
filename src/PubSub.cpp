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

#include "PubSub.hpp"
#include <stdexcept>

#include "cphConfig.h"

using namespace std;

namespace cph {

bool Publisher::topicPerMsg;

MQWTCONSTRUCTOR(Publisher, true, false), pDestFactory(pControlThread->pDestinationFactory) {
  CPHTRACEENTRY(pConfig->pTrc)
  if(threadNum==0){
    cphConfigRegisterModule(pConfig, (char*) className.data());

    int temp;
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, (char*) "tp"))
      configError(pConfig, "(tp) Could not determine whether to use one topic per publisher.");
    topicPerMsg = temp!=CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, (char*) "Publish to a new topic every iteration: %s", topicPerMsg ? "yes" : "no")
  }
  CPHTRACEEXIT(pConfig->pTrc)
}
Publisher::~Publisher(){}

void Publisher::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  if(!topicPerMsg){
    pTopic = new MQITopic(pConnection);
    CPH_DESTINATIONFACTORY_CALL_PRINTF(pTopic->setName, pOpts->destinationPrefix, destinationIndex)
    pTopic->open(true);
  }
  CPHTRACEEXIT(pConfig->pTrc)
}

void Publisher::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  delete pTopic;
  pTopic = NULL;
  CPHTRACEEXIT(pConfig->pTrc)
}

void Publisher::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  if(topicPerMsg){
    pTopic = new MQITopic(pConnection);
    CPH_DESTINATIONFACTORY_CALL_PRINTF(pTopic->setName, pOpts->destinationPrefix, destinationIndex)
    pTopic->open(false);
  }

  pTopic->put(putMessage, putMD, pmo);

  if(topicPerMsg){
    delete pTopic;
    pTopic = NULL;
    destinationIndex = cphDestinationFactoryGenerateDestinationIndex(pDestFactory);
  }
  CPHTRACEEXIT(pConfig->pTrc)
}

bool Subscriber::durable = false;
bool Subscriber::unsubscribe = true;

MQWTCONSTRUCTOR(Subscriber, false, true) {
  CPHTRACEENTRY(pConfig->pTrc)

  // Populate static fields
  if(threadNum==0){
    cphConfigRegisterModule(pConfig, (char*) className.data());

    int temp;
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, (char*) "du"))
      configError(pConfig, "(du) Could not determine whether to use durable subscriptions.");
    durable = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, (char*) "Durable subscriptions: %s", durable ? "yes" : "no")

    if(durable){
      if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, (char*) "un"))
        configError(pConfig, "(un) Could not determine whether to unsubscribe when closing durable subscriptions.");
      unsubscribe = temp==CPHTRUE;
      CPHTRACEMSG(pConfig->pTrc, (char*) "Unsubscribe when closing durable subscriptions: %s", unsubscribe ? "yes" : "no")
    }
  }

  if(durable)
    sprintf(subName, "%s_%s", pControlThread->procId, name.data());

  CPHTRACEEXIT(pConfig->pTrc)
}
Subscriber::~Subscriber(){}

void Subscriber::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  pSubscription = new MQISubscription(pConnection);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pSubscription->setTopicString, pOpts->destinationPrefix, destinationIndex)
  if(durable)
    pSubscription->setDurable(unsubscribe, subName);
  pSubscription->open(true);
  CPHTRACEEXIT(pConfig->pTrc)
}

void Subscriber::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  delete pSubscription;
  pSubscription = NULL;
  CPHTRACEEXIT(pConfig->pTrc)
}

void Subscriber::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  MQMD md = {MQMD_DEFAULT};
  pSubscription->get(getMessage, md, gmo);
  CPHTRACEEXIT(pConfig->pTrc)
}

}
