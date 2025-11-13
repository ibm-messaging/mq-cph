/*<copyright notice="lm-source" pids="" years="2014,2025">*/
/*******************************************************************************
 * Copyright (c) 2014,2025 IBM Corp.
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

#include "WorkerThread.hpp"
#include <math.h>
#include <sstream>
#include <iostream>
#include "cphDestinationFactory.h"
#include "cphLog.h"
#include <limits.h>

#define WINDOW_SIZE 4

#ifdef _MSC_VER
#include "msio.h"
#define MIN_PERIOD_BETWEEN_SLEEPS 0.1
#else
#define MIN_PERIOD_BETWEEN_SLEEPS 0.05
#endif

#if (defined(_MSC_VER) && (_MSC_VER < 1800)) || defined(CPH_IBMI) //support for C++11 was added in VS2013
static inline double round(double in){
  return floor(in + 0.5);
}
#endif

using namespace std;

namespace cph {

static inline std::string buildName(std::string className, int threadNum){
  std::stringstream ss;
  ss << className << threadNum;
  return ss.str();
}

unsigned int WorkerThread::count = 0;
unsigned int WorkerThread::seq = 0;

/*The number of messages per session.*/
unsigned int WorkerThread::messages = 0;
/*Desired rate (iterations/second).*/
float WorkerThread::rate = 0;
/*How frequently to yield the thread.*/
unsigned int WorkerThread::yieldRate = 0;
/*Time period (s) to ramp up to the full rate.*/
unsigned int WorkerThread::rampTime = 0;
/*Number of sessions to run.*/
unsigned int WorkerThread::sessions = 1;
/*Interval between sessions (milliseconds).*/
unsigned int WorkerThread::sessionInterval = 0;

/**
 * Constructor: WorkerThread
 *
 * This method creates and initialises a worker thread control block
 *
 * Input parameters: pControlThread - a pointer to the control thread control structure
 *                   className - The name of the final implementation of the WorkerThread being created
 *
 */
WorkerThread::WorkerThread(ControlThread *pControlThread, std::string className):
    Thread(pControlThread->pConfig),
    state(0),
    iterations(0),
    pControlThread(pControlThread),
    className(className),
    threadNum(seq++),
    destinationIndex(cphDestinationFactoryGenerateDestinationIndex(pControlThread->pDestinationFactory)),
    name(buildName(className, threadNum)){
  CPHTRACEREF(pTrc, pConfig->pTrc)
  CPHTRACEENTRY(pTrc)
  count++;

  /* Initialise the worker thread start and end times */
  cphUtilTimeIni(&startTime);
  cphUtilTimeIni(&endTime);

  collectLatencyStats = false;

  // Initialise static configuration variables (only if we're the first WorkerThread to be created).
  if(threadNum==0){

    if (CPHTRUE != cphConfigGetFloat(pConfig, &rate, "rt"))
      configError(pConfig, "Could not determine target rate (rt).");
    CPHTRACEMSG(pTrc, "Msg Rate: %g", rate)

    if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &yieldRate, "yd"))
      configError(pConfig, "Could not determine yield frequency (yd).");
    CPHTRACEMSG(pTrc, "Frequency to call yield: %d.", yieldRate)

    if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &messages, "mg"))
      configError(pConfig, "Could not determine number of iterations to run (mg).");
    CPHTRACEMSG(pTrc, "Max Iterations: %d.", messages)

    if(rate>0){
      if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &rampTime, "rp"))
        configError(pConfig, "Could not determine ramp time (rp).");
      CPHTRACEMSG(pTrc, "Ramptime: %d.", rampTime)
    }

    if(messages>0){
      if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &sessions, "sn"))
        configError(pConfig, "Could not determine number of sessions (sn).");
      CPHTRACEMSG(pTrc, "Max Sessions: %d.", sessions)

      if(sessions!=1){
        if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &sessionInterval, "si"))
          configError(pConfig, "Could not determine interval between sessions (si).");
        CPHTRACEMSG(pTrc, "Session interval: %dms.", sessionInterval)
      }
    }
  }

  state = S_CREATED;
  CPHTRACEEXIT(pTrc)
}

WorkerThread::~WorkerThread(){
  CPHTRACEENTRY(pConfig->pTrc)
  count--;
  CPHTRACEEXIT(pConfig->pTrc)
}

/*
 * Static Method: getWorkerCount
 * -----------------------------
 *
 * Returns the number of WorkerThread instances that have currently been created,
 * but not destroyed.
 */
int WorkerThread::getWorkerCount(){
  return count;
}

inline void WorkerThread::_openSession(){
  CPHTRACEMSG(pConfig->pTrc, "[%s] Opening new session.", name.data())
  state |= S_OPENING;
  openSession();
  state &= ~S_OPENING;
  state |= S_OPEN;
}

inline void WorkerThread::_closeSession(){
  CPHTRACEMSG(pConfig->pTrc, "[%s] Closing session.", name.data())
  state &= ~S_OPEN;
  state |= S_CLOSING;
  closeSession();
  state &= ~S_CLOSING;
}

/*
 * Method: run
 * -----------
 *
 * Inherited from class: Thread
 *
 * The over-arching method executed by the WorkerThread.
 * Repeatedly (if necessary) open sessions, calls pace(),
 * then closes the session again.
 */
void WorkerThread::run(){
  CPHTRACEENTRY(pConfig->pTrc)
  char msg[512];
  minLatency=LONG_MAX;
  maxLatency=0;
  latencyIter=0;

  state |= S_STARTED;
  snprintf(msg, 512, "[%s] START", name.data());
  cphLogPrintLn(pConfig->pLog, LOG_INFO, msg);

  try{
    _openSession();
    snprintf(msg, 512, "[%s] First session open - entering RUNNING state.", name.data());
    cphLogPrintLn(pConfig->pLog, LOG_INFO, msg);
    state |= S_RUNNING;
    pControlThread->incRunners();
    startTime = cphUtilGetNow();

    pace();

    unsigned long sessionCount = 0;

    while(!shutdown && ++sessionCount!=sessions){
      _closeSession();

      if(sessionInterval>0){
        snprintf(msg, 512, "[%s] Session %lu complete - sleeping for %ums.", name.data(), sessionCount, sessionInterval);
        cphLogPrintLn(pConfig->pLog, LOG_VERBOSE, msg);
        this->sleep(sessionInterval);
      }

      if(!shutdown){
        snprintf(msg, 512, "[%s] Starting session %lu.", name.data(), sessionCount+1);
        cphLogPrintLn(pConfig->pLog, LOG_VERBOSE, msg);
        _openSession();
        pace();
      }
    }
  } catch (ShutdownException &e){
    (void)e;
    /*Ignore - closeSession & exit*/
    CPHTRACEMSG(pConfig->pTrc, "Caught Thread::ShutdownException.")

  } catch (runtime_error &e) {

    /*
     * An error has occurred!!!!!
     *
     * Shut down the process (consistent with JMSPerfHarness)
     */

    snprintf(msg, 512, "[%s] Caught exception: %s", name.data(), e.what());
    cphLogPrintLn(pConfig->pLog, LOG_ERROR, msg);
    state |= S_ERROR;
    state &= ~(S_OPENING | S_CLOSING);

    //Current shutdown flow is waiting for the controller to notice that an error has occured (parsing output for mqrc)
    //and killing remote clients. We could end this process more speedily by calling
    //pControlThread->shutdownWorkers();

  }

  if(state & S_OPEN)
    _closeSession();

  state |= S_ENDED;
  if(state & S_RUNNING){
    state &= ~S_RUNNING;
    pControlThread->decRunners();
    endTime = cphUtilGetNow();
  }
  snprintf(msg, 512, "[%s] STOP", name.data());
  cphLogPrintLn(pConfig->pLog, LOG_INFO, msg);
  CPHTRACEEXIT(pConfig->pTrc)
}

inline bool WorkerThread::doOneIteration(unsigned int& its){
  if(shutdown) return false;

  if(collectLatencyStats) latencyStartTime = cphUtilGetNow();
  oneIteration();

  if(collectLatencyStats) {
     latencyStopTime = cphUtilGetNow();
     latency=cphUtilGetUsTimeDifference(latencyStopTime,latencyStartTime);
     //printf("Latency: %d\n",latency);
     if(latency > maxLatency)maxLatency= latency;
     if(latency < minLatency)minLatency= latency;
     if(latencyIter == 0){
	    avgLatency = latency;
     } else {
	    avgLatency = ((avgLatency * latencyIter) + latency) / (latencyIter+1);
     }
     latencyIter++;
  }
  iterations++;
  if(++its==messages) return false;
  if (yieldRate!=0 && its%yieldRate==0)
    yield();
  return true;
}

/**
 * Method: pace
 *
 * The main method for this worker thread. Repeatedly calls oneIteration at the desired rate.
 *
 * Returns: true on successful execution, false otherwise
 */
void WorkerThread::pace() {
  CPHTRACEREF(pTrc, pConfig->pTrc)
  CPHTRACEENTRY(pTrc)

  unsigned int its = 0;

  if(rate==0) // We are not trying to fix the rate
    while(doOneIteration(its));
  else { // We are trying to fix the rate

    //Nominally, CPH_SLEEP_GRANULARITY has [Dimension: 1/t | Units: 1/seconds]

    /**
     * period [Dimension: time | Units: seconds]
     * ------
     * The intended delay between subsequent iteration starts.
     */
    const double period = (double)1/rate;
    CPHTRACEMSG(pTrc, (char*) "Target iteration duration: %f", period)

    /*
     * If our period (the time between consecutive iteration starts)
     * is smaller than the minimum sleep time supported by the system (the 'sleep granularity'),
     * then there's no point calculating how long we need to sleep for.
     * So, we calculate the minimum number of iterations that must be sent
     * before the total maximum amount of sleep time exceeds the sleep granularity.
     */

    /**
     * itsBeforeCheck [Dimension: count | Units: iterations]
     * --------------
     * The number of iterations between the last time we checked the time
     * to see if we could sleep, and the next time we plan on doing it.
     */
    int itsBeforeCheck;

    /**
     * nextCheck [Dimension: count | Units: iterations]
     * ---------
     * The number of the next iteration when we'll check to see if we should sleep.
     */
    unsigned int nextCheck;

    /**
     * fallbackCheckFrequency [Dimension: count | Units: iterations]
     * ----------------------
     * The number of iteration between checking to see if we can sleep
     * when we currently don't appear to be able to keep up the desired rate.
     *
     * This is generally expected to be a large value (when needed),
     * and is intended to reduce the number of system calls made
     * (querying the time) to a bare minimum.
     */
    int const fallbackCheckFrequency = (int) ceil(rate);
    CPHTRACEMSG(pTrc, (char*) "Fallback frequency: %d.", fallbackCheckFrequency)

    /*
     * windowSleep [Dimension: none | Units: none]
     * -----------
     * The amount of sleeping we should have done so far during this window,
     * given in the units passed to cphUtilSleep
     */
    int windowSleep = 0;

    /**
     * windowStart [Dimension: time | Units: unspecified]
     * -----------
     * The time when the current window started.
     */
    CPH_TIME windowStart;

    // Ramping Period
    if(rampTime>0){
      CPHTRACEMSG(pTrc, (char*) "Starting ramping period: %ds.", rampTime)
      //Preemptively do some calculations to speed things up while messaging
      const double minimumPeriod = (double)1/CPH_SLEEP_GRANULARITY;
      const double integral = (double)(2*rampTime)/rate;

      itsBeforeCheck = (int) ceil(rate/(rampTime*2*CPH_SLEEP_GRANULARITY*CPH_SLEEP_GRANULARITY));
      itsBeforeCheck = (int) ceil(MIN_PERIOD_BETWEEN_SLEEPS*CPH_SLEEP_GRANULARITY<1
          ? rate/(rampTime*2*CPH_SLEEP_GRANULARITY*CPH_SLEEP_GRANULARITY)
          : MIN_PERIOD_BETWEEN_SLEEPS*MIN_PERIOD_BETWEEN_SLEEPS/integral);
      CPHTRACEMSG(pTrc, (char*) "Execute %d iterations before checking if we need to sleep.", itsBeforeCheck)
      nextCheck = itsBeforeCheck;

      double windowPosition = 0;

      windowStart = cphUtilGetNow();

      while(windowPosition<rampTime && doOneIteration(its)) {
        if(its==nextCheck){
          windowPosition = sqrt(integral*nextCheck);
          CPHTRACEMSG(pTrc, (char*) "Ramping period: %d iterations in %f seconds", nextCheck, windowPosition)

          double now = cphUtilGetDoubleDuration(windowStart, cphUtilGetNow());
          CPHTRACEMSG(pTrc, (char*) "Elapsed time: %fs", now)

          /*
           * sleep [Dimension: none | Units: none]
           * -----
           * The amount of sleeping we'd like to do
           * (in an ideal world containing a time-travel-capable, arbitrarily precise cphUtilSleep),
           * given in the units passed to cphUtilSleep
           */
          double sleep = (windowPosition-now)*CPH_SLEEP_GRANULARITY;
          double denom = (integral-now+sleep+windowSleep)*CPH_SLEEP_GRANULARITY;

          if (sleep > 0.5) {
            CPHTRACEMSG(pTrc, (char*) "Sleeping for %f seconds.", sleep/CPH_SLEEP_GRANULARITY)
            int sleepInt = (int) round(sleep);
            this->sleep(sleepInt);
            windowSleep += sleepInt;
          }

          if(denom<=0) break; // We can't keep up the required rate, so exit the ramping code as it's no longer doing anything for us
          itsBeforeCheck = (int)((minimumPeriod+now*2-1)/denom) + 1; //The '-1' and ' + 1' here ensures rounding up instead of down.
          int minCheckFrequency = (int) round((windowPosition*2+MIN_PERIOD_BETWEEN_SLEEPS)*MIN_PERIOD_BETWEEN_SLEEPS/integral);
          if(itsBeforeCheck<minCheckFrequency) itsBeforeCheck = minCheckFrequency;
          CPHTRACEMSG(pTrc, (char*) "Execute %d iterations before sleeping.", itsBeforeCheck)
          nextCheck = its + itsBeforeCheck;
        }
      }

      CPHTRACEMSG(pTrc, (char*) "Ramping period over.")
      windowSleep = 0;
    } // End of ramping period

    itsBeforeCheck = (int) ceil(MIN_PERIOD_BETWEEN_SLEEPS*CPH_SLEEP_GRANULARITY<1
        ? rate/CPH_SLEEP_GRANULARITY
        : rate*MIN_PERIOD_BETWEEN_SLEEPS);
    if(itsBeforeCheck>fallbackCheckFrequency) itsBeforeCheck = fallbackCheckFrequency;
    CPHTRACEMSG(pTrc, (char*) "Execute %d iterations before sleeping.", itsBeforeCheck)
    nextCheck = its + itsBeforeCheck;

    /**
     * windowCount [Dimension: count | Units: iterations]
     * -----------
     * The number of iterations performed so far during this window.
     */
    int windowCount = 0;

    /**
     * windowPosition [Dimension: time | Units: seconds]
     * --------------
     * The time between the start of the window, and the intended end of the current iteration.
     */
    double windowPosition = 0;

    windowStart = cphUtilGetNow();

    int const minCheckFrequency = (int) round(rate * MIN_PERIOD_BETWEEN_SLEEPS);

    while(doOneIteration(its)) {
      if(its==nextCheck){
        windowPosition += period * itsBeforeCheck;
        windowCount += itsBeforeCheck;
        CPHTRACEMSG(pTrc, (char*) "This window: %d iterations in %lf seconds", windowCount, windowPosition)

        double now = cphUtilGetDoubleDuration(windowStart, cphUtilGetNow());

        /*
         * sleep [Dimension: none | Units: none]
         * -----
         * The amount of sleeping we'd like to do
         * (in an ideal world containing a time-travel-capable, arbitrarily precise cphUtilSleep),
         * given in the units passed to cphUtilSleep.
         */
        double sleep = (windowPosition - now) * CPH_SLEEP_GRANULARITY;
        CPHTRACEMSG(pTrc, (char*) "Target sleep: %fs", sleep / CPH_SLEEP_GRANULARITY)

        if(windowSleep<=-sleep){
          CPHTRACEMSG(pTrc, (char*) "Struggling to keep up desired rate.")
            itsBeforeCheck = fallbackCheckFrequency;
        } else {
          if(sleep>0.5){
            int sleepInt = (int) round(sleep);
            CPHTRACEMSG(pTrc, (char*) "Sleeping for %f seconds.", (float) sleepInt / CPH_SLEEP_GRANULARITY)
            this->sleep(sleepInt);
            windowSleep += sleepInt;
            CPHTRACEMSG(pTrc, (char*) "Total accrued sleep time this window: %fs", (double) windowSleep / CPH_SLEEP_GRANULARITY)
            sleep -= sleepInt;
            CPHTRACEMSG(pTrc, (char*) "Remaining sleep deficit: %fs", sleep / CPH_SLEEP_GRANULARITY)
          } else {
            CPHTRACEMSG(pTrc, (char*) "Not sleeping as sleep deficit is too short: %fs", sleep / CPH_SLEEP_GRANULARITY)
          }
          /*
           * If we're here, then we should be able to assert that 2 conditions are true,
           * which together guarantee that the following calculation yields a value >0:
           *
           * windowSleep > -sleep :- Assured by else block we're inside; guarantees denominator >0.
           * sleep <= 0.5 :- Assured by the above subtraction of any integer part; guarantees numerator >0.
           */
          itsBeforeCheck = (int) ceil(( (-sleep + 1) * windowCount) / (sleep + windowSleep));
          if (itsBeforeCheck > fallbackCheckFrequency) itsBeforeCheck = fallbackCheckFrequency;
          else if (itsBeforeCheck < minCheckFrequency) itsBeforeCheck = minCheckFrequency;
        }
        if(itsBeforeCheck <= 0){
          stringstream ss;
          ss << "PACING ERROR: Iterations before next time check is <=0 [" << itsBeforeCheck << "]. Terminating session.";
          throw runtime_error(ss.str());
        }
        CPHTRACEMSG(pTrc, (char*) "Execute %d iterations before checking again.", itsBeforeCheck)
        nextCheck = its + itsBeforeCheck;

        if(windowPosition>WINDOW_SIZE){
          CPHTRACEMSG(pTrc, (char*) "Starting new pacing window.")
          CPHTRACEMSG(pTrc, (char*) "Current sleep deficit: %fs", sleep / CPH_SLEEP_GRANULARITY)
          //If we let the sleep deficit build too much, we can then run too fast whilst trying to correct our pacing producing odd throughput rates
          //Lets limit the deficit to the current window size
          if (sleep < (WINDOW_SIZE * CPH_SLEEP_GRANULARITY * -1)) {
            CPHTRACEMSG(pTrc, (char*) "Sleep deficit too large, limiting to -WINDOW SIZE: %fs", WINDOW_SIZE * -1)
            sleep = WINDOW_SIZE * CPH_SLEEP_GRANULARITY * -1;
          }
          windowPosition = sleep / CPH_SLEEP_GRANULARITY;
          windowStart = cphUtilGetNow();
          windowSleep = windowCount = 0;
        }
      }
    }
  }

  CPHTRACEEXIT(pTrc)
}

/**
 * Method: getState
 *
 * Return the current "state" of the worker thread. See WorkerThread.hpp for the different states
 * the worker thread can be in.
 *
 * Returns: the worker thread state (int)
 */
unsigned int WorkerThread::getState() const {
  return state;
}

/**
 * Method: cphWorkerThreadGetIterations
 *
 * This method returns the current number of iterations executed by this worker thread.
 *
 * Returns: the number of iterations executed by this worker thread
 */
unsigned int WorkerThread::getIterations() const {
  return iterations;
}

/**
 * Method: setCollectLatencyStats
 *
 * This method toggles the collection of latency timestamps
 *
 */
void WorkerThread::setCollectLatencyStats(bool flag) {
  collectLatencyStats = flag;
}

/**
 * Method: getLatencyStats
 *
 * This method returns the current values for avgLatency, maxLatency and minLatency
 * then resets them.
 *
 */
void WorkerThread::getLatencyStats(long statsArray[]) {
  statsArray[0] = avgLatency;
  statsArray[1] = maxLatency;
  statsArray[2] = minLatency;

  minLatency=LONG_MAX;
  maxLatency=0;
  latencyIter=0;
}

/**
 * Method: getStartTime
 *
 * This method returns the start time of the worker thread. This is set at the beginning of the
 * pace method. (cphWorkerThreadPace).
 *
 * Returns: the start time of the worker thread as a CPH_TIME value
 */
CPH_TIME WorkerThread::getStartTime() const {
  return startTime;
}

/**
 * Method: cphWorkerThreadGetEndTime
 *
 * This method returns the end time of the worker thread. This is set at the end of the
 * pace method. (cphWorkerThreadPace).
 *
 * Returns: the end time of the worker thread as a CPH_TIME value
 */
CPH_TIME WorkerThread::getEndTime() const {
  return endTime;
}

static WTRegister & getRegister(){
  static WTRegister reg = WTRegister();
  return reg;
}

/*
 * Static Method: registerWTImpl
 * -----------------------------
 *
 * Associates the given name - which should be the name of a class extending WorkerThread,
 * providing a full implementation - with a WTFactory for creating instances of that class.
 */
bool WorkerThread::registerWTImpl(string const name, WTFactory const factory){
  getRegister()[name]=factory;
  return true;
}

WTFactory WorkerThread::implementation;

/*
 * Static Method: setImplementation
 * --------------------------------
 *
 * Sets the implementation of the WorkerThread to use,
 * so that subsequent calls to create() will create
 * instance of that implementation.
 */
void WorkerThread::setImplementation(CPH_TRACE* const pTrc, string const className){
  CPHTRACEENTRY(pTrc)
  WTRegister reg = getRegister();
  stringstream impList;
  impList << "Registered WorkerThread implementations:";
  for(WTRegister::iterator it = reg.begin(); it!=reg.end(); it++)
    impList << " " << it->first << ",";
  char last;
  impList >> last;
  CPHTRACEMSG(pTrc, impList.str().data());
  WTRegister::iterator f = reg.find(className);
  if(f==reg.end()){
    throw runtime_error("No known worker-thread implementation named '" + className + "'.\n" + impList.str());
  } else {
    implementation = (*f).second;
    CPHTRACEMSG(pTrc, "Implementation set to %s.", (*f).first.data())
  }
  CPHTRACEEXIT(pTrc)
}

/*
 * Static Method: create
 * ---------------------
 *
 * Create a new instance of a WorkerThread using the implementation
 * provided by the previous call to setImplementation(),
 * and the associated WTFactory provided when the implementation
 * was previously registered with registerWTImpl.
 */
WorkerThread * WorkerThread::create(ControlThread * pControlThread){
  CPHTRACEENTRY(pControlThread->pConfig->pTrc)
  if(implementation==NULL)
    throw logic_error("WorkerThread implementation has not yet been set.");
  CPHTRACEEXIT(pControlThread->pConfig->pTrc)
  return (*implementation)(pControlThread);
}

}

