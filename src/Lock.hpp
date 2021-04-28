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

#ifndef LOCK_HPP_
#define LOCK_HPP_

#include "cphdefs.h"

#ifdef ISUPPORT_CPP11
#include <mutex>
#include <condition_variable>
#include <chrono>
#else
#include <pthread.h>
#include <time.h>
#endif

#ifdef ISUPPORT_CPP11
typedef std::chrono::time_point<std::chrono::system_clock> absTime;
#else
typedef struct timespec absTime;
#endif

/*
 * Macro: lockAndWait
 * ------------------
 *
 * Using the given Lock for wait/notify,
 * wait for either the given condition to become false,
 * or the given number of milliseconds to expire.
 */
#define lockAndWait(LOCK, MILLIS, COND)\
  absTime until = cph::durationToAbs(MILLIS);\
  if(COND){\
    LOCK.lock();\
    while((COND) && !LOCK.wait(until));\
    LOCK.unlock();\
  }

namespace cph {

/*
 * Class: Lock
 * -----------
 *
 * Provides similar functionality java Object monitors:
 *   1. Exclusive access to critical sections of code.
 *   2. Wait/Notify
 */
class Lock {
public:
  Lock();
  virtual ~Lock();

  void lock();
  void unlock();
  void wait();
  bool wait(absTime const &until);
  void notify();

private:

#ifdef ISUPPORT_CPP11
  std::mutex mutex;
  std::condition_variable cv;
#else
  pthread_mutex_t mutex;
  pthread_cond_t cv;
#endif

};

absTime durationToAbs(int millis);

}

#endif /* LOCK_HPP_ */
