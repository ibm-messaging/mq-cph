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

#ifndef REQUESTER_HPP_
#define REQUESTER_HPP_

#include "MQIWorkerThread.hpp"
#include <cmqc.h>

namespace cph {

/*
 * Class: Requester
 * ----------------
 *
 * Extends: MQIWorkerThread
 *
 * Puts a message to a queue, then gets a response back from another queue.
 */
MQWTCLASSDEF(Requester,
  static bool useCorrelId;
  static bool useSelector, useCustomSelector;
  static int dqChannels;
  static char iqPrefix[MQ_Q_NAME_LENGTH];
  static char oqPrefix[MQ_Q_NAME_LENGTH];
  static char customSelector[MQ_SELECTOR_LENGTH];

  /*The queue to put to.*/
  MQIObject * pInQueue;
  /*The queue to get from.*/
  MQIObject * pOutQueue;
)

}

#endif /* REQUESTER_HPP_ */
