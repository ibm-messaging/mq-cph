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

#define CPH_CONTROLTHREAD_CPP
#include "ControlThread.hpp"

//Include WorkerThread implementation headers in order to register them
#include "Dummy.hpp"
#include "SndRcv.hpp"
#include "PubSub.hpp"
#include "PubSubV6.hpp"
#include "PutGet.hpp"
#include "Requester.hpp"
#include "Responder.hpp"
#include "Forwarder.hpp"
#include "DLQMon.hpp"

#include <string.h>
#include <sstream>
#include <time.h>
#include <exception>
#include <csignal>

#ifdef CPH_WINDOWS
#include "msint.h"
#else
#include <inttypes.h>
#endif

#include "Lock.hpp"

/******************************************************************************/
/* Global variables                                                           */
/******************************************************************************/
extern "C" {
  /* This static variable is used by cphUtilSigInt when a control C is invoked */
  volatile sig_atomic_t cphControlCInvoked = 0;
}

namespace cph {

/*
 * Class: ShutdownException
 *
 * Used to indicate that a signal has been intercepted that implies cph
 * should shut down.
 */
class ShutdownException : public std::exception {
  char msg[80];
public:
  ShutdownException(): std::exception(){
    sprintf(msg, "Shutdown signal detected: %d", cphControlCInvoked);
  }
  virtual ~ShutdownException() throw() {}
  virtual char const * what() const throw(){return msg;}
};
////////////////////////
// END ShutdownException
////////////////////////

/*
 * Class: StatsThread
 * Extends: cph::Thread
 *
 * Periodically outputs statistics to the log.
 */
class StatsThread : public Thread {
  ControlThread const * const pControlThread;
  unsigned int const statsInterval;
  bool statsPerThread;

public:

  StatsThread(ControlThread const * const pControlThread, unsigned int const interval) :
      Thread(pControlThread->pConfig),
      pControlThread(pControlThread), statsInterval(interval) {

    CPHTRACEENTRY(pControlThread->pConfig->pTrc)

    int temp;
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "sp"))
      configError(pConfig, "(sp) Could not determine whether to display per-thread performance data.");
    statsPerThread = temp==CPHTRUE;
    CPHTRACEMSG(pControlThread->pConfig->pTrc, "Display per-thread performance data %s.", statsPerThread ? "yes" : "no")

    CPHTRACEEXIT(pControlThread->pConfig->pTrc)
  }

  virtual ~StatsThread(){}

protected:
  virtual void run(){
    CPHTRACEREF(pTrc, pConfig->pTrc)
    CPHTRACEENTRY(pTrc)

    CPH_LOG *pLog = pConfig->pLog;

    std::vector<unsigned int> * prev = new std::vector<unsigned int>();
    std::vector<unsigned int> * curr = new std::vector<unsigned int>();
    std::vector<unsigned int> * temp;

    std::stringstream ss;

    if(0 < strlen(pControlThread->procId))
      ss << "id=" << pControlThread->procId << ",";
    std::string const stem = ss.str();

    /* Get the initial start time */
    CPH_TIME startTime = cphUtilGetNow();  /* start of sleep time       */

    try {
      while(!shutdown) {
        unsigned int total, j, running;
        size_t shortest;
        long elapsedTime;                            /* elapsed time in milli-sec */
        CPH_TIME endTime;                            /* end of sleep time         */

        sleep(statsInterval * 1000);

        temp = prev;
        prev = curr;
        curr = temp;
        running = pControlThread->getThreadStats(*curr);
        endTime = cphUtilGetNow();

        total=0;
        std::stringstream ss2;
        ss2 << stem;
        if(statsPerThread) ss2 << " (";

        shortest = prev->size();
        if(curr->size() < shortest) shortest = curr->size();

        for (j = 0; j < shortest; j++) {
          unsigned int diff = (*curr)[j] - (*prev)[j];
          if(statsPerThread) ss2 << diff << "\t";
          total += diff;
        }
        /* Add on new threads */
        for (j = (int)shortest; j<curr->size(); j++ ) {
          if(statsPerThread) ss2 << (*curr)[j];
          total += (*curr)[j];
        }

        if(statsPerThread) ss2 << ") ";

        /* Get the elapsed time */
        elapsedTime = cphUtilGetTimeDifference(endTime, startTime);
        /* Set the start time to the end time, for the next sleep */
        cphCopyTime(&startTime, &endTime);

        char buff[80];
        sprintf(buff, "rate=%.2f,threads=%u", (double) total / (elapsedTime / 1000), running);
        ss2 << buff;

        cphLogPrintLn(pLog, LOGINFO, ss2.str().data());

      }
    } catch (ShutdownException) {
      CPHTRACEMSG(pTrc, "Stats thread shutdown received.");
    }

    delete prev;
    delete curr;

    CPHTRACEEXIT(pTrc)
  }
};

//////////////////
// END StatsThread
//////////////////

/*
 * Constructor: ControlThread
 *
 */
ControlThread::ControlThread(CPH_CONFIG * const pConfig) :
    pConfig(pConfig),
    pDestinationFactory(NULL),
    threadId(Thread::getCurrentThreadId()),
    workers(),
    pStatsThread(NULL),
    shutdown(false),
    runningWorkers(0),
    threadCountLock() {
  cphDestinationFactoryIni(&pDestinationFactory, pConfig);
}

/*
 * Destructor: ControlThread
 *
 */
ControlThread::~ControlThread(){
  CPH_TRACE *pTrc = pConfig->pTrc;
  CPHTRACEENTRY(pTrc)

  bool traceIsOn = cphTraceIsOn(pTrc)==CPHTRUE;

  if(traceIsOn){
    fprintf(pTrc->tFp, "\n\nSUMMARY OF THREAD ACTIVITY.\n");
    fprintf(pTrc->tFp,     "===========================\n\n\n");

    fprintf(pTrc->tFp, "ThreadId: %" PRIu64 "\t\t\tControl thread.\n", threadId);
    if(pStatsThread!=NULL)
      fprintf(pTrc->tFp, "ThreadId: %" PRIu64 "\t\t\tStats thread.\n", pStatsThread->getId());
  }

  for(std::vector<WorkerThread *>::const_iterator it = workers.begin(); it != workers.end(); ++it){
    if(traceIsOn)
      fprintf(pTrc->tFp, "Thread Id: %" PRIu64 "\t\t\tThread Name: %s.\n", (*it)->getId(), (*it)->name.data());

    delete *it;
  }

  if(traceIsOn) fprintf(pTrc->tFp, "\n\n");

  workers.clear();
  delete pStatsThread;
  cphDestinationFactoryFree(&pDestinationFactory);

  CPHTRACEEXIT(pTrc)
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4297)
#endif

/*
** Method: run
**
** This is the main driving method for the control thread.
** Starts worker threads, then waits for an appropriate condition to bet met,
** indicating we should shut down, then shuts down the worker threads.
*/
void ControlThread::run() {

  CPHTRACEREF(pTrc, pConfig->pTrc)
  CPHTRACEENTRY(pTrc)

  CPH_LOG *pLog = pConfig->pLog;

  /* Options Variables */
  bool doFinalSummary, reportTlf = false;
  //int threadStackSize;
  unsigned int numWorkers, threadStartInterval, threadStartTimeout, runLength;

  /* Temporary/transient variables */
  int temp;
  char tempStr[512];

  cphLogPrintLn(pLog, LOGVERBOSE, "controlThread START");
  CPH_TIME threadStartTime = cphUtilGetNow();

  try {

    if(CPHTRUE != cphConfigGetString(pConfig, tempStr, "tc"))
      configError(pConfig, "(tc) Could not determine worker thread type class.");
    cph::WorkerThread::setImplementation(pConfig->pTrc, std::string(tempStr));

    /* Get the set duration option */
    if (CPHTRUE != cphConfigGetString(pConfig, tempStr, "sd")) exit(1);
    if (0 == strcmp(tempStr, "normal"))
      reportTlf = false;
    else if (0 == strcmp(tempStr, "tlf"))
      reportTlf = true;
    else
      configError(pConfig, (char*) "-sd must be one of {normal,tlf}");

    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "sh"))
      configError(pConfig,"(sh) Could not determine whether to use a shutdown signal handler.");

    if (CPHTRUE == temp){
      signal(SIGINT, cphUtilSigInt);
      signal(SIGTERM, cphUtilSigInt);
    }

    if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &numWorkers, "nt"))
      configError(pConfig, "(nt) Could not determine number of worker threads.");
    CPHTRACEMSG(pTrc, "Num worker threads: %d.", numWorkers)

    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "su"))
      configError(pConfig, "(su) Could not determine whether to print final summary.");
    doFinalSummary = temp==CPHTRUE;
    CPHTRACEMSG(pTrc, "Display final summary: %s.", doFinalSummary ? "yes" : "no")

    if (CPHTRUE != cphConfigGetString(pConfig, procId, "id"))
      configError(pConfig, "(id) Could not determine process identifier.");
    CPHTRACEMSG(pTrc, "Process Id: %s.", procId)

    /* Start the stats thread if the specified interval is greater than zero */
    if (CPHTRUE != cphConfigGetInt(pConfig, (int *) &temp, "ss"))
      configError(pConfig, "(ss) Could not determine stats interval.");
    CPHTRACEMSG(pTrc, "Stats interval: %d.", temp)
    // Create stats thread if needed
    if(temp>0)
      pStatsThread = new StatsThread(this, temp);

#ifdef DISABLED
    if (CPHTRUE != cphConfigGetInt(pConfig, &threadStackSize, "ts"))
      status = CPHFALSE;
    else {
      CPHTRACEMSG(pTrc, "Thread stack size command line option: %d.", threadStackSize)
                if (threadStackSize > 0)
                {
#ifdef CPH_UNIX
                  CPHTRACEMSG(pTrc, "Setting per thread stack allocation to: %d.\n", 1024 * threadStackSize)
                   cphThreadSetStackSize(&pControlThread->threadAttr, 1024 * threadStackSize);
#else
                  CPHTRACEMSG(pTrc, "-ts option is only supported on Unix.\n")
                  cphConfigInvalidParameter(pConfig, (char*) "-ts is only supported on Unix");
#endif
                }
    }
#endif

    if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &threadStartInterval, "wi"))
      configError(pConfig, "(wi) Could not determine worker thread start interval.");
    CPHTRACEMSG(pTrc, "Worker Thread start interval is: %u.", threadStartInterval)

    if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &threadStartTimeout, "wt"))
      configError(pConfig, "(wt) Could not determine worker thread start timeout.");
    CPHTRACEMSG(pTrc, "Number of seconds to wait for worker thread to start: %u.", threadStartTimeout)

      if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &runLength, "rl"))
        configError(pConfig, "(rl) Could not determine run length");
      CPHTRACEMSG(pTrc, "Run length: %us.", runLength)

    // Create worker threads (but don't start them).
    workers.reserve(numWorkers);
    for(unsigned int i=0; i<numWorkers; ++i)
      workers.push_back(WorkerThread::create(this));

    if(cphConfigIsInvalid(pConfig)==CPHTRUE)
      throw std::runtime_error("Configuration is invalid.");

  } catch (std::runtime_error &e) {
    /* If the configuration is invalid at this point, print a message and exit before starting any worker threads */
    if (CPHTRUE == cphConfigIsInvalid(pConfig))
      cphLogPrintLn( pLog, LOGERROR, "The current configuration is not valid. Use -h to see available options." );
    throw e;
  }

  if (cphControlCInvoked != 0) throw ShutdownException();

  CPH_TIME startTime;
  std::exception ex;
  bool exceptionCaught = false;

  try {
    if(pStatsThread != NULL) pStatsThread->start();
    startWorkers(threadStartInterval, threadStartTimeout);

    startTime = cphUtilGetNow();
    runLength *= 1000;
    long remaining = runLength;
    unsigned int duration = (runLength>0 && remaining<2000) ? remaining : 2000;

    while(duration>0 && runningWorkers>0){
      CPHTRACEMSG(pTrc, "Sleeping for %ums...", duration)
      cphUtilSleep(duration);
      if (cphControlCInvoked != 0) {
    	  sprintf(tempStr, "cphControlCInvoked flag triggered: %d - Signal detected", cphControlCInvoked);
          cphLogPrintLn(pLog, LOGERROR, tempStr);
          CPHTRACEMSG(pTrc, tempStr)
    	  throw ShutdownException();
      }

      remaining = (long) runLength - cphUtilGetTimeDifference(cphUtilGetNow(), startTime);
      if(remaining < 0) remaining = 0;
      CPHTRACEMSG(pTrc, "Remaining time: %lums.", remaining)
      duration = (runLength>0 && remaining<2000) ? remaining : 2000;
    }

    if(runningWorkers>0)
      cphLogPrintLn(pLog, LOGVERBOSE, "Timer : Runlength expired" );

  } catch (ShutdownException) {
    cphLogPrintLn(pLog, LOGERROR, "Caught shutdown exception." );
    CPHTRACEMSG(pTrc, "Caught shutdown exception.")
  } catch (std::exception &e) {
    sprintf(tempStr, "[ControlThread] Caught exception: %s", e.what());
    cphLogPrintLn(pLog, LOGERROR, tempStr);
    ex = e;
    exceptionCaught = true;
  } catch (...) {
    cphLogPrintLn(pLog, LOGERROR, "[ControlThread] Caught unknown object.");
  }

  CPH_TIME approxEndTime = cphUtilGetNow();

  shutdownWorkers();
  if(pStatsThread!=NULL){
    pStatsThread->signalShutdown();
    while(pStatsThread->isAlive())
      Thread::yield();
  }

  CPH_TIME wtTime, endTime;
  cphUtilTimeIni(&endTime);
  unsigned int iterations = 0;

  for(std::vector<WorkerThread *>::iterator it = workers.begin(); it != workers.end(); ++it){

    if(!reportTlf){
      wtTime = (*it)->getStartTime();
      if(cphUtilTimeIsZero(wtTime)!=CPHTRUE && cphUtilTimeCompare(wtTime, startTime) < 0)
        startTime = wtTime;
    }

    if(doFinalSummary)
      iterations += (*it)->getIterations();

    wtTime = (*it)->getEndTime();
    if (cphUtilTimeCompare(wtTime, endTime) > 0)
      endTime = wtTime;

  }

  /* Override the above for different timing modes */
  if (reportTlf) {
    CPHTRACEMSG(pTrc, "(TLF) Using THREAD start time")
    startTime = threadStartTime;
  }

  /* If no one has a valid end time, use our rough value */
  if(cphUtilTimeIsZero(endTime)) {
    CPHTRACEMSG(pTrc, "Using approximate value of end time")
    endTime = approxEndTime;
  }

  if(doFinalSummary) {
    double duration = cphUtilGetDoubleDuration(startTime, endTime);

    sprintf(tempStr,
        "totalIterations=%u,totalSeconds=%.2f,avgRate=%.2f",
        iterations, duration, (double)iterations/duration);
    cphLogPrintLn(pLog, LOGWARNING, tempStr);
  }
  cphLogPrintLn(pLog, LOGVERBOSE, "controlThread STOP");

  if(exceptionCaught)
    throw ex;

  CPHTRACEEXIT(pTrc)
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/*
** Method: getThreadStats
**
** This method gets an iteration count for each worker thread and puts
** them in the given vector. Any pre-existing contents of the vector
** are erased.
*/
unsigned int ControlThread::getThreadStats(std::vector<unsigned int> &stats) const {
  CPHTRACEENTRY(pConfig->pTrc)
  stats.clear();
  stats.reserve(workers.size());

  for(std::vector<WorkerThread *>::const_iterator it = workers.begin(); it != workers.end(); ++it)
    stats.push_back((*it)->getIterations());

  CPHTRACEEXIT(pConfig->pTrc)
  return runningWorkers;
}

/*
** Method: startWorkers
**
** This method starts the worker threads whose control blocks have been initialised and inserted into the
** "workers" vector. For each worker thread that is started, the method waits up to the time specified by the
** "wt" parameter to check that the thread has started. Between starting threads, the method will sleep for
** the number of milliseconds specified by the "interval" parameter. This controls the pause between starting multiple threads.
**
** Input Parameters:
**     interval - the number of milliseconds to wait between starting threads
**     timeout  - the max number of seconds to wait for each thread to start
**
*/
void ControlThread::startWorkers(unsigned int interval, unsigned int timeout) {
  int state;
  char errorString[512];
  WorkerThread* pWorker;

    CPHTRACEREF(pTrc, pConfig->pTrc)
  CPHTRACEENTRY(pTrc)

  //for(std::vector<WorkerThread *>::iterator it = workers.begin(); it != workers.end(); ++it){
  for(size_t i = 0; i<workers.size();){ // i is incremented below
    pWorker = workers[i];
    /* Skip any already running threads */
    if (pWorker->getState() & S_STARTED) continue;

    CPHTRACEMSG(pTrc, "Starting worker thread.")
    pWorker->start();

    // Wait for thread to get up and running
    if(timeout>0){
      lockAndWait(threadCountLock, timeout*1000, (pWorker->getState() & (S_RUNNING|S_ENDED|S_ERROR))==0)

      if (cphControlCInvoked != 0) throw ShutdownException();

      state = pWorker->getState();
      if(state & S_ERROR){
        sprintf(errorString, "%s: State ERROR set.", pWorker->name.data());
        CPHTRACEMSG(pTrc, errorString)
        throw std::runtime_error(errorString);
      } else if (state & S_ENDED) {
        CPHTRACEMSG(pTrc, "   State ended set.")
      } else if (state & S_RUNNING) {
        CPHTRACEMSG(pTrc, "   State running is set.")
      } else {
        sprintf(errorString, "Timed out waiting for thread '%s' to start.", pWorker->name.data());
        cphLogPrintLn(pConfig->pLog, LOGERROR, errorString);
        throw std::runtime_error(errorString);
      }
    }

    if(++i<workers.size()){
      CPHTRACEMSG(pTrc, "Sleeping for wi(%u) ms.", interval)
      cphUtilSleep(interval);
    }

    // If we didn't wait for the thread to get up and running,
    // check now for errors.
    if(timeout<=0){
      if(S_ERROR & pWorker->getState()){
        sprintf(errorString, "%s: State ERROR set.", pWorker->name.data());
        CPHTRACEMSG(pTrc, errorString)
        throw std::runtime_error(errorString);
      }
    }
  }

  CPHTRACEEXIT(pTrc)
}

/*
** Method: incRunners
**
** Increment the number of running workers in the control thread control block
*/
void ControlThread::incRunners() {
  CPHTRACEENTRY(pConfig->pTrc)
  threadCountLock.lock();
  ++runningWorkers;
  threadCountLock.notify();
  threadCountLock.unlock();
  CPHTRACEMSG(pConfig->pTrc, "Num of running workers is now: %d.", runningWorkers)
  CPHTRACEEXIT(pConfig->pTrc)
}

/*
** Method: cphControlThreadDecRunners
**
** Decrement the number of running workers in the control thread control block
*/
void ControlThread::decRunners() {
  CPHTRACEENTRY(pConfig->pTrc)
  bool err = false;
  threadCountLock.lock();
  if(runningWorkers==0)
    err = true;
  else
    --runningWorkers;
  threadCountLock.notify();
  threadCountLock.unlock();
  if(err)
    throw std::logic_error("Tried to decrement runningWorkers when it's already 0.");
  CPHTRACEMSG(pConfig->pTrc, "Num of running workers is now: %d.", runningWorkers)
  CPHTRACEEXIT(pConfig->pTrc)
}

/*
** Method: shutdownWorkers
**
** This method requests all worker threads to shutdown and waits for them to finish.
** It displays a list every 10 seconds that gives information about the threads that
** it is still waiting for.
*/
void ControlThread::shutdownWorkers() {
  CPHTRACEENTRY(pConfig->pTrc)

  for(std::vector<WorkerThread *>::iterator it=workers.begin(); it!=workers.end(); ++it)
    (*it)->signalShutdown();

  while(runningWorkers>0){

    lockAndWait(threadCountLock, 10000, runningWorkers>0)

    if(runningWorkers>0){
      std::stringstream ss;
      ss << "[ControlThread] Waiting for: (";
      for(std::vector<WorkerThread *>::iterator it=workers.begin(); it!=workers.end(); ++it)
        if((*it)->isAlive())
          ss << (*it)->name << " ";
      ss << ")";
      cphLogPrintLn(pConfig->pLog, LOGWARNING, ss.str().data());
    }
  }

  CPHTRACEEXIT(pConfig->pTrc)
}

}

