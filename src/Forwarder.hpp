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

#ifndef FORWARDER_HPP_
#define FORWARDER_HPP_

#include "cphdefs.h"
#include "MQIWorkerThread.hpp"
#include <cmqc.h>

namespace cph {

/*
 * Class: Forwarder
 * ----------------
 *
 * Extends: MQIWorkerThread
 *
 * Gets a request message from a queue, then puts it on another.
 */
MQWTCLASSDEF(Forwarder,
  static bool copyMD;
  static bool copyMsg;
  static char iqPrefix[MQ_Q_NAME_LENGTH];
  static char oqPrefix[MQ_Q_NAME_LENGTH];
  static bool commitBetween;

  /*The queue to get from.*/
  MQIObject * pInQueue;
  /*The queue to put to.*/
  MQIObject * pOutQueue;
)

}

#endif /* FORWARDER_HPP_ */
