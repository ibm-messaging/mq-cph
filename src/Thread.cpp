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

#include <assert.h>

#include "Thread.hpp"
#include "cphTrace.h"
#include "cphUtil.h"

#ifdef CPH_IBMI
#include <sys/select.h>
#endif

#define MAX_QUICKSLEEP_DURATION 1000

namespace cph {

Thread::Thread(CPH_CONFIG* pConfig) : shutdown(false), pConfig(pConfig), id(0), alive(false), shutdownLock() {}
Thread::Thread(CPH_CONFIG* pConfig, bool const attach) :
    shutdown(false), pConfig(pConfig), id(attach ? getCurrentThreadId() : 0), alive(attach), shutdownLock() {}
Thread::~Thread(){}

/*
 * Method: isAlive
 * ---------------
 *
 * Determines if this Thread is still running,
 * except where the thread has terminated due to an uncaught signal.
 */
bool Thread::isAlive() const {
  return alive;
}

/*
 * Method: getId
 * -------------
 *
 * Returns the OS-assigned unique id of this thread once started.
 * Its behaviour is undetermined before start() has been called.
 */
uint64_t Thread::getId() const {
  return id;
}

/*
 * Method: signalShutdown
 * ----------------------
 *
 * Signal to this thread that it should cease execution.
 * It is up to the extending implementation to honour this request.
 */
void Thread::signalShutdown(){
  CPHTRACEENTRY(pConfig->pTrc)
  shutdownLock.lock();
  shutdown = true;
  shutdownLock.notify();
  shutdownLock.unlock();
  CPHTRACEEXIT(pConfig->pTrc)
}

/*
 * Method: checkShutdown
 * ---------------------
 *
 * Throws a ShutdownException if this Thread
 * has been signalled to shutdown.
 */
void Thread::checkShutdown() const {
  if(shutdown)
    throw ShutdownException(this);
}

CPH_THREAD_RUN _thread_run(void* t) {
  Thread* th = (Thread*) t;
  CPHTRACEENTRY(th->pConfig->pTrc)
  th->alive = true;
  try {
    th->run();
    th->alive = false;
    CPHTRACEEXIT(th->pConfig->pTrc)
    return 0;
  } catch (...) {
    th->alive = false;
    throw;
  }
}

/*
 * Method: start
 * -------------
 *
 * Start this Thread, calling its run() implementation
 * in a new thread of execution.
 */
bool Thread::start(){
  CPHTRACEENTRY(pConfig->pTrc)
  bool rc = true;
  int ret = 0;

  if(!shutdown && id==0){
#ifdef CPH_WINDOWS
    DWORD tid;
    if(NULL == CreateThread(NULL, 0, _thread_run, this, 0, &tid)){
#else //#elif defined(CPH_UNIX)
    pthread_t tid;
    if ((ret = pthread_create(&tid, NULL, _thread_run, this)) != 0) {
#endif
      CPHTRACEMSG(pConfig->pTrc, (char*) "Failed to spawn new thread.")
	  if (ret) {
		  CPHTRACEMSG(pConfig->pTrc, (char*) "Return value from creating thread: %d", ret)
	  }
	  rc = false;
    }
#ifdef CPH_OSX
    id = (uint64_t) pthread_mach_thread_np(tid);
#elif defined(CPH_IBMI)
    id = ((uint64_t) tid.reservedHiId << (sizeof(unsigned int) << 3)) + tid.reservedLoId;
#else
    id = (uint64_t) tid;
#endif
  } else {
    CPHTRACEMSG(pConfig->pTrc, (char*) "Cannot spawn new thread. Either thread already started, or signalShutdown called.")
    rc = false;
  }
  CPHTRACEEXIT(pConfig->pTrc)
  return rc;
}

void Thread::sleep(int millis) const {
  CPHTRACEENTRY(pConfig->pTrc);
  assert(id==getCurrentThreadId());
  if(millis<=0) return;
  if(millis<=MAX_QUICKSLEEP_DURATION)
    cphUtilSleep(millis);
  else {
    lockAndWait(shutdownLock, millis, !shutdown)
  }
  CPHTRACEEXIT(pConfig->pTrc)
  checkShutdown();
}

Thread::ShutdownException::ShutdownException(Thread const * const thread) throw():
    std::exception(), thread(thread) {
  snprintf(msg, 80, "Thread [id=%" PRIu64 "] received a shutdown request.", thread->id);
}

Thread::ShutdownException::~ShutdownException() throw() {}

char const * Thread::ShutdownException::what() const throw() {
  return msg;
}

uint64_t Thread::getCurrentThreadId(){
#if defined(CPH_WINDOWS)
  return (uint64_t) GetCurrentThreadId();
#elif defined(CPH_IBMI)
  pthread_t tid = pthread_self();
  return ((uint64_t) tid.reservedHiId << (sizeof(unsigned int) << 3)) + tid.reservedLoId;
#elif defined(CPH_OSX)
  return (uint64_t) pthread_mach_thread_np(pthread_self());
#elif defined(CPH_UNIX)
  return (uint64_t) pthread_self();
#else
  #error "Don't know how to find threadId on this platform"
#endif
}

/*
 * Static Method: yield
 *
 * Causes the calling thread to voluntarily give up the processor,
 * so that other runnable threads may run.
 */
void Thread::yield() {
#if defined(WIN32) || defined(WIN64)
  YieldProcessor();
#elif defined(CPH_UNIX)
  sched_yield();
#endif
}

}
