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

#include "Lock.hpp"

#if (defined(CPH_IBMI) && !defined(ISUPPORT_CPP11)) || defined(CPH_OSX)
#include <sys/time.h>
#endif

#ifdef ISUPPORT_CPP11
using namespace std;
#else
#include <errno.h>
#endif

#include <assert.h>

namespace cph {

Lock::Lock()
#ifdef ISUPPORT_CPP11
    : mutex(), cv() {}
#else
{
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cv, NULL);
}
#endif


Lock::~Lock() {
#ifndef ISUPPORT_CPP11
  pthread_cond_destroy(&cv);
  pthread_mutex_destroy(&mutex);
#endif
}

/*
 * Method: lock
 * ------------
 *
 * Enter a critical section of code,
 * with access controlled by this Lock.
 */
void Lock::lock(){
#ifdef ISUPPORT_CPP11
  mutex.lock();
#else
  pthread_mutex_lock(&mutex);
#endif
}

/*
 * Method: unlock
 * --------------
 *
 * Leave a critical section of code that
 * has access controlled by this Lock.
 */
void Lock::unlock(){
#ifdef ISUPPORT_CPP11
  mutex.unlock();
#else
  pthread_mutex_unlock(&mutex);
#endif
}

/*
 * Method: wait
 * ------------
 *
 * Release this Lock, and wait for another thread
 * to call notify() on it before re-acquiring the Lock
 * and resuming execution.
 *
 * This method must only be called from within a critical
 * section of code controlled by this Lock.
 */
void Lock::wait(){
#ifdef ISUPPORT_CPP11
  unique_lock<std::mutex> ul(mutex, adopt_lock);
  cv.wait(ul);
  ul.release();
#else
  pthread_cond_wait(&cv, &mutex);
#endif
}

/*
 * Method: wait
 * ------------
 *
 * Release this Lock, and wait for another thread
 * to call notify() on it, or for the given absolute time-point
 * to be reached, before re-acquiring the Lock
 * and resuming execution.
 *
 * This method must only be called from within a critical
 * section of code controlled by this Lock.
 *
 * Returns: true if the given timeout expired, false otherwise.
 */
bool Lock::wait(absTime const &until){
#ifdef ISUPPORT_CPP11
  unique_lock<std::mutex> ul(mutex, adopt_lock);
  auto rc = cv.wait_until(ul, until);
  ul.release();
#ifdef __GXX_EXPERIMENTAL_CXX0X__
  return eqTypes(rc, true) ? !((bool)rc) : (bool) rc;
#else
  assert(eqTypes(rc,cv_status::timeout));
  return rc == cv_status::timeout;
#endif
#else
  return ETIMEDOUT == pthread_cond_timedwait(&cv, &mutex, &until);
#endif
}

/*
 * Method: notify
 * --------------
 *
 * Wake up another thread that's currently waiting on this Lock.
 */
void Lock::notify(){
#ifdef ISUPPORT_CPP11
  cv.notify_one();
#else
  pthread_cond_signal(&cv);
#endif
}

/*
 * Function: durationToAbs
 * -----------------------
 *
 * Convert a duration in milliseconds to the absolute time-point
 * that's aforementioned duration after the present.
 */
absTime durationToAbs(int millis){
#ifdef ISUPPORT_CPP11
  return chrono::system_clock::now() + chrono::milliseconds(millis);
#else
  absTime abs;
#if defined(CPH_IBMI) || defined(CPH_OSX)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  abs.tv_sec = tv.tv_sec;
  abs.tv_nsec = tv.tv_usec*1000;
#else //#ifdef CPH_IBMI
  clock_gettime(CLOCK_REALTIME, &abs);
#endif //#ifdef CPH_IBMI
  abs.tv_sec += millis/1000;
  abs.tv_nsec += (millis%1000)*1000000;
  if(abs.tv_nsec > 1000000000){
    abs.tv_sec++;
    abs.tv_nsec -= 1000000000;
  }
  return abs;
#endif
}

}
