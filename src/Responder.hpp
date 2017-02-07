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

#ifndef RESPONDER_HPP_
#define RESPONDER_HPP_

#include "cphdefs.h"
#include "MQIWorkerThread.hpp"

#ifdef tr1_inc
#include tr1_inc(unordered_set)
#else
#include <set>
#endif

#include <cmqc.h>

namespace cph {

#ifdef tr1

/*
 * Typedef: MQIQueueHash
 * ---------------------
 *
 * A pointer to a function that generates a hash code from a pointer
 * to an MQIQueue.
 */
typedef size_t (*MQIQueueHash)(MQIQueue * const);

/*
 * Typedef: MQIQueueEq
 * -------------------
 *
 * A pointer to a function that determines whether two given pointers
 * to MQIQueues point to equivalent structures.
 */
typedef bool (*MQIQueueEq)(MQIQueue * const, MQIQueue * const);

/*
 * Typedef: ReplyQCache
 * --------------------
 *
 * An unordered set of pointers to MQIQueues.
 * Used to cache open reply-to-queues.
 */
typedef tr1(unordered_set)<MQIQueue *, MQIQueueHash, MQIQueueEq> ReplyQCache;

#else

/*
 * Typedef: MQIQueueCmp
 * --------------------
 *
 * A pointer to a function that determines the relative ordering
 * of two queues referred to by given pointers.
 */
typedef bool (*MQIQueueCmp)(MQIQueue * const, MQIQueue * const);

/*
 * Typedef: ReplyQCache
 * --------------------
 *
 * A set of pointers to MQIQueues.
 * Used to cache open reply-to-queues.
 */
typedef std::set<MQIQueue *, MQIQueueCmp> ReplyQCache;
#endif

/*
 * Class: Responder
 * ----------------
 *
 * Extends: MQIWorkerThread
 *
 * Gets a request message from a queue, then sends a reply to the specified reply queue.
 */
MQWTCLASSDEF(Responder,
  static bool useCorrelId;
  static bool useSelector;
  static bool copyRequest;
  static char iqPrefix[MQ_Q_NAME_LENGTH];
  static bool commitBetween;

  /*The queue to get requests from.*/
  MQIObject * pInQueue;
  /*Cache of reply-to-queues already opened*/
  ReplyQCache oqCache;

  /*Dummy object used for querying outCache.*/
  mutable MQIQueue * dummyOut;

  inline MQIQueue * getReplyQueue(MQMD const & md);
)

}

#endif /* RESPONDER_HPP_ */
