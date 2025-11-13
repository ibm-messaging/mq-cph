/*<copyright notice="lm-source" pids="" years="2014,2021">*/
/*******************************************************************************
 * Copyright (c) 2014,2021 IBM Corp.
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

#ifdef MQWORKERTHREAD_H_
namespace cph {
  class MQIWorkerThread;
}
#else
#define MQWORKERTHREAD_H_

#include "cphdefs.h"

#include <string>

#include "ControlThread.hpp"
#include "WorkerThread.hpp"
#include "MQIOpts.hpp"
#include "MQI.hpp"

extern "C"{
  #include "cphConfig.h"
  #include <cmqc.h>
}

namespace cph {

/*
 * Class: MQIWorkerThread
 * ---------------------
 *
 * Extends: WorkerThread
 *
 * Provides a partial WorkerThread implementation with objects, methods,
 * and configuration options that will commonly be used by all WorkerThread implementations
 * that primarily test the MQI interface.
 */
class MQIWorkerThread : public WorkerThread {
  friend class MQIConnection;
private:
  bool const putter;
  bool const getter;
  bool const reconnector;

protected:
  /*Command line configuration options, and tools to create derived MQI data structures.*/
  static MQIOpts * pOpts;
  /*A pointer to an MQIConnection object, that can be used to make MQI calls.*/
  MQIConnection * pConnection;

  //Only used by putters...

  /*An MQ message descriptor that can be used for putting messages.*/
  MQMD putMD;
  /*
   * An MQ message handle that can be used for a put message.
   * Once created (at session open time), this will be referenced
   * by the pmo MQPMO object.
   */
  MQHMSG putMsgHandle;
  /*A message buffer containing meaningless data that can be put.*/
  MQIMessage * putMessage;
  /*Put message options.*/
  MQPMO pmo;

  //Only used by getters...

  /*
   * An MQ message handle that can be used for a got message.
   * Once created (at session open time), this will be referenced
   * by the gmo MQGMO object.
   */
  MQHMSG getMsgHandle;
  /*An empty message buffer to get messages into.*/
  MQIMessage * getMessage;
  /*Get message options.*/
  MQGMO gmo;

  /*The correlId to associate with messages.*/
  MQBYTE24 correlId;

  void generateCorrelID(MQBYTE24 & genCorrelId, char const * const procId);

  virtual void openSession();
  virtual void closeSession();
  virtual void oneIteration();

  /*
   * Abstract Method: openDestination
   * --------------------------------
   *
   * Create and open any objects (including the destination MQ queue/topic)
   * that are required to run a session of iterations.
   *
   * It can be assumed that any objects and handles declared in MQIWorkerThread
   * will be created, connected, and opened according to the relevant configuration
   * before this method is called.
   */
  virtual void openDestination() = 0;

  /*
   * Abstract Method: closeDestination
   * ---------------------------------
   *
   * Close and clean up any objects created in a previous call to openDestination.
   *
   * It can be assumed that any objects and handles declared in MQIWorkerThread
   * will still be in existence, connected, and open according to the relevant configuration
   * when this method is called.
   */
  virtual void closeDestination() = 0;

  /*
   * Abstract Method: msgOneIteration
   * --------------------------------
   *
   * Run a single iteration's-worth of messaging.
   *
   * After this method returns, if the configuration requires it,
   * a call to MQCMIT will be made on pConnection,
   */
  virtual void msgOneIteration() = 0;

public:

  MQIWorkerThread(ControlThread* pControlThread, std::string className, bool putter, bool getter, bool reconnector);
  virtual ~MQIWorkerThread();
};

#define MQWTCLASSDEF(CLASS, ...) \
class CLASS : public MQIWorkerThread {\
public:\
  CLASS(cph::ControlThread* pControlThread);\
  virtual ~CLASS();\
protected:\
  virtual void openDestination();\
  virtual void closeDestination();\
  virtual void msgOneIteration();\
private:\
__VA_ARGS__\
};\
REGISTER_WORKER_IMPL(CLASS)

#define MQWTCONSTRUCTOR(CLASS, PUTTER, GETTER, RECONNECTOR) \
CLASS::CLASS(cph::ControlThread* pControlThread) :\
    MQIWorkerThread(pControlThread, #CLASS, PUTTER, GETTER, RECONNECTOR)

}
#endif /* MQIWORKERTHREAD_H_ */
