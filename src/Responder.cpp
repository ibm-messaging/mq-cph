/*<copyright notice="lm-source" pids="" years="2014,2018">*/
/*******************************************************************************
 * Copyright (c) 2014,2018 IBM Corp.
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

#include "Responder.hpp"
#include <string.h>
#include "cphLog.h"

#define CPH_RESPONDER_OQ_CACHE_INITIAL_BUCKET_COUNT 10

using namespace std;

namespace cph {

#ifdef tr1
/*
 * Function: pobj_hash
 * -------------------
 *
 * Hash function for pointers to MQIObjects.
 * Hashes on the object and QM name in the OD.
 *
 * Used for locating MQIObject pointers in the ObjectCache hash set.
 */
static size_t pobj_hash(MQIQueue * const pObj){
  size_t rc = 5381;

  char const * name = pObj->getQMName();
  for(int i = 0; i<MQ_Q_MGR_NAME_LENGTH; i++){
    char c = name[i];
    if(c=='\0') break;
    rc = ((rc<<5) + rc) ^ name[i];
  }

  name = pObj->getName();
  for(int i = 0; i<MQ_Q_NAME_LENGTH; i++){
    char c = name[i];
    if(c=='\0') break;
    rc = ((rc<<5) + rc) ^ name[i];
  }

  return rc;
}

/*
 * Function: pobj_eq
 * -----------------
 *
 * Determines whether MQIQueues pointed to by the two given pointers
 * represent the same queue, as determined by the object name and QM name of the OD.
 *
 * Used for determining the equality of members of the ObjectCache hash set.
 */
static bool pobj_eq(MQIQueue * const a, MQIQueue * const b){
  return
      0==strncmp(a->getQMName(), b->getQMName(), MQ_Q_MGR_NAME_LENGTH) &&
      0==strncmp(a->getName(), b->getName(), MQ_Q_NAME_LENGTH);
}
#else

/*
 * Function: pobj_cmp
 * ------------------
 *
 * Determines whether an MQIQueue pointed at by the first given pointer
 * is lexicographically less than the one pointed at by the second given pointer.
 * Here, we define lexicographic ordering to be first by associated QM name,
 * and then by queue name.
 *
 * Used for determining the ordering and equality of members of the ObjectCache set.
 */
static bool pobj_cmp(MQIQueue * const a, MQIQueue * const b){
  if(strncmp(a->getQMName(), b->getQMName(), MQ_Q_MGR_NAME_LENGTH) < 0)
    return true;
  if(strncmp(a->getName(), b->getName(), MQ_Q_NAME_LENGTH) < 0)
    return true;
  return false;
}
#endif

/*Whether to copy the request's message ID to the reply's correlation ID*/
bool Responder::useCorrelId = true;
/*Whether or not to send the request message back as the reply*/
bool Responder::copyRequest = true;
/*The prefix of the request queue name, with the suffix to be provided by destinationIndex*/
char Responder::iqPrefix[MQ_Q_NAME_LENGTH];
/*Whether or not message selectors are used*/
bool Responder::useSelector = false;

/*
 * Whether or not to commit transactions between getting a request and putting a reply.
 * This option is mainly provided to enable legacy behaviour.
 */
bool Responder::commitBetween = false;

MQWTCONSTRUCTOR(Responder, true, true), pInQueue(NULL),
#ifdef tr1
    oqCache(CPH_RESPONDER_OQ_CACHE_INITIAL_BUCKET_COUNT, &pobj_hash, &pobj_eq),
#else
    oqCache(&pobj_cmp),
#endif
    dummyOut(NULL) {
  CPHTRACEENTRY(pConfig->pTrc)

  if(threadNum==0){

    if(pOpts->commitFrequency>1)
      cphLogPrintLn(pConfig->pLog, LOG_WARNING,
          "\t******************************************************************************\n"
          "\t* WARNING - (cc) Setting commit-count to greater than 1 for Responders could *\n"
          "\t*           cause your test to lock up if you don't have enough Requesters.  *\n"
          "\t*           I'll trust you know what you're doing.                           *\n"
          "\t******************************************************************************");

    if(pOpts->timeout!=-1)
      cphLogPrintLn(pConfig->pLog, LOG_WARNING,
          "\t******************************************************************************\n"
          "\t* WARNING - (to) Setting the MQGET timeout to something other than -1        *\n"
          "\t*           (i.e. wait indefinitely) for Responders is probably not what you *\n"
          "\t*           intend. If a Requester thread does not receive a message within  *\n"
          "\t*           the specified time, then it will throw an exception and cease    *\n"
          "\t*           execution. You have been warned!                                 *\n"
          "\t******************************************************************************");

    int temp = 0;

    // Use correlId?
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "co"))
      configError(pConfig, "(co) Cannot determine whether to use correlIDs.");
    useCorrelId = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use correlIDs: %s", useCorrelId ? "yes" : "no")

    // Use message selectors?
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cs"))
      configError(pConfig, "(cs) Cannot determine whether to use message selectors.");
    useSelector = temp == CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use message selectors: %s", useSelector ? "yes" : "no")

    // Copy request?
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cr"))
      configError(pConfig, "(cr) Cannot determine whether to copy request in reply.");
    copyRequest = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Copy request: %s", copyRequest ? "yes" : "no")

    // Input (request) queue.
    if(CPHTRUE != cphConfigGetString(pConfig, (char*) &iqPrefix, "iq")){
      cphLogPrintLn(pConfig->pLog, LOG_INFO,
          "No input (request) queue prefix parameter (-iq) detected. Falling back on destination prefix (-d).");
      strncpy(iqPrefix, pOpts->destinationPrefix, MQ_Q_NAME_LENGTH);
    }
    CPHTRACEMSG(pConfig->pTrc, "Input (request) queue prefix: %s", iqPrefix)

    // Commit between?
    if(pOpts->commitFrequency){
      if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "cb"))
        configError(pConfig, "(cb) Cannot determine whether to commit between getting the request and putting the reply.");
      commitBetween = temp==CPHTRUE;
      CPHTRACEMSG(pConfig->pTrc, "Commit between getting the request and putting the reply: %s", commitBetween ? "yes" : "no")
    }
  }

  CPHTRACEEXIT(pConfig->pTrc)
}
Responder::~Responder() {}

void Responder::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Open request queue
  pInQueue = new MQIQueue(pConnection, false, true);
  CPH_DESTINATIONFACTORY_CALL_PRINTF(pInQueue->setName, iqPrefix, destinationIndex)
  pInQueue->open(true);

  // Set message type to reply
  putMD.MsgType = MQMT_REPLY;

  // Don't generate new correl IDs if we're correlating
  if(useCorrelId)
    pmo.Options &= ~MQPMO_NEW_CORREL_ID;

  CPHTRACEEXIT(pConfig->pTrc)
}

void Responder::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Close input queue
  delete pInQueue;
  pInQueue = NULL;

  // Close cached output queues
  for(ReplyQCache::iterator it = oqCache.begin(); it!=oqCache.end(); it++)
    delete *it;
  oqCache.clear();

  // Delete dummy output queue object
  delete dummyOut;
  dummyOut = NULL;

  CPHTRACEEXIT(pConfig->pTrc)
}

inline MQIQueue * Responder::getReplyQueue(MQMD const & md){
  CPHTRACEENTRY(pConfig->pTrc)
  if(dummyOut==NULL)
    dummyOut = new MQIQueue(pConnection, true, false);

  CPHTRACEMSG(pConfig->pTrc, "Reply-to-Q-Mgr [%*.*s]", MQ_Q_MGR_NAME_LENGTH, MQ_Q_MGR_NAME_LENGTH, md.ReplyToQMgr)
  CPHTRACEMSG(pConfig->pTrc, "Reply-to-queue [%*.*s]", MQ_Q_NAME_LENGTH, MQ_Q_NAME_LENGTH, md.ReplyToQ)
  dummyOut->setQMName(&md.ReplyToQMgr);
  dummyOut->setName(&md.ReplyToQ);

  pair<ReplyQCache::iterator, bool> result = oqCache.insert(dummyOut);
  if(result.second){
    /*
     * If we successfully made an insertion,
     * then this is the first time we're replying to this queue,
     * so we need to open it.
     *
     * Also, nullify dummyOut to make sure we create a new instance next time.
     */
    dummyOut->open(true);
    dummyOut = NULL;
  }

  CPHTRACEEXIT(pConfig->pTrc)
  return *(result.first);
}

void Responder::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)

  // Get request message
  MQMD getMD = pOpts->getGetMD();
  getMessage->messageLen = 0;

  pInQueue->get(getMessage, getMD, gmo);

  if(pOpts->commitFrequency>0 && commitBetween && (getIterations()+1)%pOpts->commitFrequency==0)
    pConnection->commitTransaction();

  if(useCorrelId){
    memcpy(putMD.CorrelId, useSelector ? getMD.CorrelId : getMD.MsgId, sizeof(MQBYTE24));
    cphTraceId(pConfig->pTrc, "Correlation ID", putMD.CorrelId);
  }

  // Put reply
  getReplyQueue(getMD)->put(copyRequest ? getMessage : putMessage, putMD, pmo);

  CPHTRACEEXIT(pConfig->pTrc)
}

}
