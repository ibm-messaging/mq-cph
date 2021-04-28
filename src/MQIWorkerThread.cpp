/*<copyright notice="lm-source" pids="" years="2014,2020">*/
/*******************************************************************************
 * Copyright (c) 2014,2020 IBM Corp.
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

#include "MQIWorkerThread.hpp"
#include <cstring>
#include <stdexcept>

#ifdef ISUPPORT_CPP11
#include <chrono>
#elif defined(CPH_IBMI) || defined(CPH_OSX)
#include <sys/time.h>
#else
#include <ctime>
#endif

#ifdef tr1_inc
#include tr1_inc(random)
#else
#include <cstdlib>
#endif

#include "cphMQSplitter.h"
#include "cphUtil.h"

using namespace std;

namespace cph {

MQIOpts * MQIWorkerThread::pOpts;

MQIWorkerThread::MQIWorkerThread(ControlThread* pControlThread, string className, bool putter, bool getter, bool reconnector) :
    WorkerThread(pControlThread, className),
    putter(putter), getter(getter), reconnector(reconnector), pConnection(NULL),
    putMsgHandle(MQHM_NONE), putMessage(NULL),
    getMsgHandle(MQHM_NONE), getMessage(NULL) {
  CPHTRACEENTRY(pConfig->pTrc)

  if(threadNum==0){

    pOpts = new MQIOpts(pConfig, putter, getter, reconnector);

    /* Load the server or client MQ library as appropriate */
    if (0 != cphMQSplitterCheckMQLoaded(pConfig->pLog, pOpts->connType==REMOTE ? 1 : 0))
      throw runtime_error("Failed to load MQ client dynamic library.");
  }

  if(putter){
    putMessage = new MQIMessage(pOpts, false);
    pmo = pOpts->getPMO();
  }

  if(getter){
    getMessage = new MQIMessage(pOpts, true);
    gmo = pOpts->getGMO();
  }

  CPHTRACEEXIT(pConfig->pTrc)
}

MQIWorkerThread::~MQIWorkerThread(){
  CPHTRACEENTRY(pConfig->pTrc)
  delete putMessage;
  delete getMessage;

  if(getWorkerCount()==1)
    delete pOpts;
  CPHTRACEEXIT(pConfig->pTrc)
}

void MQIWorkerThread::openSession(){
  CPHTRACEENTRY(pConfig->pTrc)

  pConnection = new MQIConnection(this, false);

  if(putter){
    putMD = pOpts->getPutMD();
    if(pOpts->useMessageHandle){
      MQLONG rfh2Len;
      char * rfh2Buf = cphBuildRFH2(&rfh2Len);
      putMsgHandle = pConnection->createPutMessageHandle(&putMD, rfh2Buf, rfh2Len);
      pmo.OriginalMsgHandle = putMsgHandle;
    }
  }

  if(getter && pOpts->useMessageHandle){
    getMsgHandle = pConnection->createGetMessageHandle();
    gmo.Version = MQGMO_VERSION_4;
    gmo.MsgHandle = getMsgHandle;
  }

  openDestination();

  CPHTRACEEXIT(pConfig->pTrc)
}

void MQIWorkerThread::closeSession(){
  CPHTRACEENTRY(pConfig->pTrc)

  if(pOpts->commitFrequency>0)
    pConnection->rollbackTransaction();

  if(putMsgHandle!=MQHM_NONE) pConnection->destroyMessageHandle(putMsgHandle);
  if(getMsgHandle!=MQHM_NONE) pConnection->destroyMessageHandle(getMsgHandle);

  closeDestination();

  delete pConnection;
  pConnection = NULL;

  CPHTRACEEXIT(pConfig->pTrc)
}

void MQIWorkerThread::oneIteration(){
  msgOneIteration();
  if(pOpts->commitFrequency>0 && (getIterations()+1)%pOpts->commitFrequency==0 && !reconnector)
    pConnection->commitTransaction();
}

static inline uint32_t getCorrelIdBase(CPH_TRACE * pTrc){
  CPHTRACEENTRY(pTrc)
#ifndef ISUPPORT_CPP11
#if defined(CPH_IBMI) || defined(CPH_OSX)
  timeval now = {0,0};
  gettimeofday(&now, NULL);
  uint32_t seed = now.tv_sec ^ now.tv_usec;
#else
  timespec now = {0,0};
  clock_gettime(CLOCK_REALTIME, &now);
  uint32_t seed = now.tv_sec ^ now.tv_nsec;
#endif
#else // Windows *should* always follow this path
  uint32_t seed = (uint32_t) std::chrono::system_clock::now().time_since_epoch().count();
#endif

#ifdef tr1
  uint32_t correlIdBase = tr1(mt19937)((uint32_t)seed)();
#else
  srand((unsigned int) seed);
  uint32_t correlIdBase = (uint32_t) rand();
#endif

  CPHTRACEMSG(pTrc, "Generated random number to use as correlID base: %x", correlIdBase)
  CPHTRACEEXIT(pTrc)
  return correlIdBase;
}

/*
 * Method: generateCorrelID
 * ------------------------
 *
 * Generate a unique CorrelId for this thread.
 *
 * In the previous version of cph, various classes used various techniques to generate correlation IDs.
 * I've also been reliably informed by A. Hickson that some of these methods had high likelihood of causing
 * collisions, particularly when multiple processes are accessing the same queue.
 * So, in an attempt to improve in the situation, we use one of 2 methods here:
 *
 *  > If a unique identifier has been provided to the process via the 'id' command line option,
 *    then we generate the correlId from this and the WorkerThread name.
 *    This is the preferred option.
 *
 *  > For compatibility with older test configurations, if no 'id' is provided,
 *    then we generate a single random id for the process, and append the WorkerThread name.
 */
void MQIWorkerThread::generateCorrelID(MQBYTE24 & id, char const * const procId){
  CPHTRACEENTRY(pConfig->pTrc)

  memset(id, 0, sizeof(MQBYTE24));
  char localproc[80];
  char procPart[24];
  size_t procIdLen = strlen(procId);

  strcpy(localproc, procId);
  if (procIdLen == 0){
    static uint32_t correlIdBase = getCorrelIdBase(pConfig->pTrc);
    if (name.length() > 14) {
       snprintf((char*)id, 24, "%x-%s", correlIdBase, name.substr(name.length() - 15, 14).data());
	} else {
        snprintf((char*)id, 24, "%x-%s", correlIdBase, name.data());
	}
  }
  else {
    string adjustedName;
    //Additional code added to avoid potentially dangling pointer of temporary value returned from substr
	if (name.length() > 18) { 
	   adjustedName = name.substr(name.length() - 19, 18);
	} else {
	   adjustedName = name;
	}
	char const * const threadPart = adjustedName.data();
	
	size_t remaining = 22 - strlen(threadPart);
	if (procIdLen > remaining) {
	  strncpy(procPart, &localproc[procIdLen - remaining], remaining);
	  procPart[remaining] = '\0';
	  sprintf((char*)id, "%s-%s", procPart, threadPart);
	} else {
      sprintf((char*)id, "%s-%s", procId, threadPart);
	}
  }
  CPHTRACEMSG(pConfig->pTrc, "CorrelID: %s", (char*)id)
  CPHTRACEEXIT(pConfig->pTrc)
}

}
