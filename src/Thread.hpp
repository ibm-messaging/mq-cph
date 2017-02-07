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

#ifndef THREAD_H_
#define THREAD_H_

#include "Lock.hpp"

#ifdef CPH_WINDOWS
#include <Windows.h>
#include <winbase.h>
#include "msio.h"
#include "msint.h"
#define CPH_THREAD_RUN DWORD WINAPI
#else
#include <pthread.h>
#include <inttypes.h>
#define CPH_THREAD_RUN void*
#endif

#include <exception>

#include "cphConfig.h"

namespace cph {

/*
 * Class: Thread
 * -------------
 *
 * Provides an object-wrapper around a thread of execution.
 */
class Thread {
public:

  /*
   * Class: Thread::ShutdownException
   * --------------------------------
   *
   * Thrown to indicate that the Thread should terminate execution,
   * as it has been signalled to shut down.
   */
  class ShutdownException: public std::exception {
    char msg[80];
  public:
    Thread const * const thread;

    ShutdownException(Thread const * const thread) throw();
    virtual ~ShutdownException() throw();
    virtual char const * what() const throw();
  };

  Thread(CPH_CONFIG* pConfig);
  virtual ~Thread();
  bool start();
  void signalShutdown();
  uint64_t getId() const;
  bool isAlive() const;
  void checkShutdown() const;

  static uint64_t getCurrentThreadId();
  static void yield();

protected:
  /*
   * A value of true indicates that this thread should cease execution.
   * Implementations of the run() method should check this value regularly,
   * and respond accordingly.
   */
  bool shutdown;
  /*A pointer to the current configuration structure of the CPH process.*/
  CPH_CONFIG* pConfig;

  /*
   * Abstract Method: run
   * --------------------
   *
   * The 'main' method for this thread.
   * Will be called when the thread is started.
   */
  virtual void run() = 0;

  void sleep(int millis) const;

private:
  uint64_t id;
  bool alive;
  mutable Lock shutdownLock;

  friend CPH_THREAD_RUN _thread_run(void* t);
  friend class ThreadAttach;

  /**
   * Create a new Thread object, and associate it with the current thread of execution.
   */
  Thread(CPH_CONFIG* pConfig, bool const attach);
};

class ThreadAttach : Thread {

public:
  ~ThreadAttach(){}

protected:
  ThreadAttach(CPH_CONFIG * const pConfig) : Thread(pConfig, true) {}
  void run(){}

};

}

#endif /* THREAD_H_ */
