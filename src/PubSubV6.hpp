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

#ifndef PUBSUBV6_HPP_
#define PUBSUBV6_HPP_

#include "MQIWorkerThread.hpp"
#include "cphDestinationFactory.h"
#include <cmqpsc.h>   /* WMQ V6 MQI Publish/Subscribe */

namespace cph {

MQWTCLASSDEF(PublisherV6,
  static bool topicPerMsg;
  static char streamQName[MQ_Q_NAME_LENGTH];
  CPH_DESTINATIONFACTORY * const pDestFactory;

  MQIQueue * pStreamQueue;
  MQLONG headerLen;

  inline void buildMQRFHeader(PMQRFH pStart, PMQLONG pDataLength);
)

MQWTCLASSDEF(SubscriberV6,
  static bool unsubscribe;
static char streamQName[MQ_Q_NAME_LENGTH];

  MQBYTE24 correlId;
  MQIQueue * pControlQueue;
  MQIQueue * pSubscriberQueue;

  void buildMQRFHeader(PMQRFH pRFHeader, PMQLONG pDataLength, MQCHAR const * const pCommand, MQLONG regOptions, MQLONG pubOptions);
  void psCommand(MQCHAR const * const Command, MQLONG regOptions);
  void checkForResponse(PMQMD pMd, MQIMessage * pMessage);
)

}

#endif /* PUBSUBV6_HPP_ */
