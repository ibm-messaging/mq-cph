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

#ifndef WORKERTHREAD_H_
#define WORKERTHREAD_H_

#include <string>
#include <stdexcept>

#include "cphdefs.h"
#include "Thread.hpp"
#include "ControlThread.hpp"
#include "cphUtil.h"
#include "cphConfig.h"
#include "cphTrace.h"

#ifdef tr1_inc
#include tr1_inc(unordered_map)
#define hashMap tr1(unordered_map)
#else
#include <map>
#define hashMap std::map
#endif

namespace cph {
class ControlThread;
class WorkerThread;

/*
 * Various different WorkerThread state values.
 * These can be combined bitwise to produce compound states.
 */
#define S_CREATED 1
#define S_OPENING 2
#define S_RUNNING 4
#define S_CLOSING 8
#define S_ERROR 16
#define S_ENDED 32
#define S_OPEN 64
#define S_STARTED 128

/*
 * Typedef: WTFactory
 * ------------------
 *
 * A pointer to a function that takes a pointer to a ControlThread
 * as its only argument, and returns a pointer to a WorkerThread.
 *
 * Used for constructing new instances of a WorkerThread implementation.
 */
typedef WorkerThread* (*WTFactory)(ControlThread*);

/*
 * Typedef: WTRegister
 * -------------------
 *
 * A mapping between the names of final WorkerThread implementations
 * and a WTFactory capable of creating instance of that implementation.
 */
typedef hashMap<std::string, WTFactory> WTRegister;

/*
 * Class: WorkerThread
 * -------------------
 *
 * Extends: Thread
 *
 * Handles a single thread of workload-execution for CPH.
 * This class handles basic pacing, session-management, and progress logging;
 * it must be extended with implementations of openSession(), closeSession(),
 * and oneIteration() which execute the simulated work itself.
 */
class WorkerThread: public Thread {
private:
  static WTFactory implementation;

public:
  static bool registerWTImpl(std::string const name, WTFactory const factory);
  static void setImplementation(CPH_TRACE * const pTrc, std::string const className);
  static WorkerThread * create(ControlThread * pControlthread);

private:
  static unsigned int seq;
  static unsigned int count;

  /*A code representing the current state of this WorkerThread.*/
  unsigned int state;
  /*The total number of iterations completed so far by this WorkerThread.*/
  unsigned int iterations;
  /*The time when the thread first starts running iterations, after opening the initial session.*/
  CPH_TIME startTime;
  /*The time when the thread completes execution.*/
  CPH_TIME endTime;
  /*A pointer to the control thread that created this WorkerThread.*/
  ControlThread * const pControlThread;

  void pace();
  virtual void run();

  inline void _openSession();
  inline void _closeSession();
  inline bool doOneIteration(unsigned int& its);

protected:
  // Configuration values - see WorkerThread.properties
  static unsigned int messages;
  static float rate;
  static unsigned int yieldRate;
  static unsigned int rampTime;
  static unsigned int sessions;
  static unsigned int sessionInterval;

  /*The name of the class providing the final implementation of this WorkerThread.*/
  std::string const className;
  /*A unique identifier of this WorkerThread among others - the sequence number of object creation.*/
  unsigned int const threadNum;
  /*
   * An integer that can be used to identify a numeric name-part for the destination object(s)
   * that the final implementation of this WorkerThread messages to.
   *
   * This value is set initially by a call to cphDestinationFactoryGenerateDestinationIndex,
   * but extending classes are free to alter this value if desired.
   */
  int destinationIndex;

  /*
   * Abstract Method: openSession
   * ----------------------------
   *
   * Perform the necessary operations to open a new iteration session,
   * such that repeated calls to oneIteration will succeed.
   */
  virtual void openSession() = 0;

  /*
   * Abstract Method: closeSession
   * -----------------------------
   *
   * Perform the necessary operations to close an open iteration session,
   * cleaning up any objects created or opened by a previous call to openSession.
   */
  virtual void closeSession() = 0;

  /*
   * Abstract Method: oneIteration
   * -----------------------------
   *
   * Perform a single paced iteration task, such as sending/receiving a message.
   */
  virtual void oneIteration() = 0;

  static int getWorkerCount();

public:
  std::string const name;

  WorkerThread(ControlThread *pControlThread, std::string className);
  virtual ~WorkerThread();
  unsigned int getState() const;
  unsigned int getIterations() const;
  CPH_TIME getStartTime() const;
  CPH_TIME getEndTime() const;
};

static inline void configError(CPH_CONFIG* pConfig, std::string msg){
  cphConfigInvalidParameter(pConfig, (char*) msg.data());
  throw std::runtime_error(msg);
}

/*
 * Worker Thread Implementations
 * -----------------------------
 *
 * WorkerThread implementations register themselves automatically by calling the REGISTER_WORKER_IMPL
 * macro from their header file. This macro is null unless called from the ControlThread source file,
 * as indicated by the presence of the CPH_CONTROLTHREAD_CPP symbol.
 *
 * The result is that all that must be done to be able to use a new WorkerThread implementation
 * is to #include its header in the ControlThread source file.
 */
#ifdef CPH_CONTROLTHREAD_CPP

// Factory function to create worker threads
template<typename W> WorkerThread* createWorker(ControlThread * pControlThread) {
  return new W(pControlThread);
}

// Macro to register worker thread implementations from header files
#define REGISTER_WORKER_IMPL(CLASS) \
  UNUSED bool const reg_##CLASS = cph::WorkerThread::registerWTImpl(#CLASS, &createWorker<CLASS>);

#else //#ifdef CPH_CONTROLTHREAD_CPP
#define REGISTER_WORKER_IMPL(CLASS)
#endif

}

#endif /* WORKERTHREAD_H_ */
