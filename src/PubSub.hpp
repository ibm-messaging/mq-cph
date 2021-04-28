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

#ifndef PUBSUB_HPP_
#define PUBSUB_HPP_

#include "MQIWorkerThread.hpp"
#include "cphDestinationFactory.h"

namespace cph {

MQWTCLASSDEF(Publisher,
  static bool topicPerMsg;
  CPH_DESTINATIONFACTORY * const pDestFactory;

  MQITopic * pTopic;
)

MQWTCLASSDEF(Subscriber,
  static bool durable;
  static bool unsubscribe;

  char subName[MQ_SUB_IDENTITY_LENGTH];
  MQISubscription * pSubscription;
)

}

#endif /* PUBSUB_HPP_ */
