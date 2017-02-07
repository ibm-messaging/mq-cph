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

#include <cstring>
#include "DLQMon.hpp"
#include "cphLog.h"

namespace cph {

MQWTCONSTRUCTOR(DLQMon, false, true) {}
DLQMon::~DLQMon() {}

void DLQMon::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  pQueue = new MQIQueue(pConnection, false, true);
  pQueue->setName("SYSTEM.DEAD.LETTER.QUEUE");
  pQueue->open(true);

  gmo.WaitInterval = MQWI_UNLIMITED;
  CPHTRACEEXIT(pConfig->pTrc)
}

void DLQMon::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  delete pQueue;
  pQueue = NULL;
  CPHTRACEEXIT(pConfig->pTrc)
}

void DLQMon::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  MQMD md = {MQMD_DEFAULT};
  pQueue->get(getMessage, md, gmo);

  // Check we have a Dead Letter Header at the beginning of our message
  MQDLH *dlh = (MQDLH*) getMessage->buffer;
  if(
      getMessage->messageLen < MQDLH_LENGTH_1 ||
      !dlh || std::memcmp(MQDLH_STRUC_ID, dlh->StrucId, 4)
  ) throw std::runtime_error("Message retrieved from dead letter queue does not begin with an MQDLH.");

#ifdef AMQ_PC
  _snprintf(msg, DL_DETECT_MSG_LEN,
      "DEAD LETTER received:\n"
      "\tReason: %d\n"
      "\tDestination Queue: [%*.*s]\n"
      "\tDestination QM: [%*.*s]",
      dlh->Reason,
      MQ_Q_NAME_LENGTH, MQ_Q_NAME_LENGTH, dlh->DestQName,
      MQ_Q_MGR_NAME_LENGTH, MQ_Q_MGR_NAME_LENGTH, dlh->DestQMgrName);
#else
  snprintf(msg, DL_DETECT_MSG_LEN,
      "DEAD LETTER received:\n"
      "\tReason: %d\n"
      "\tDestination Queue: [%*.*s]\n"
      "\tDestination QM: [%*.*s]",
      dlh->Reason,
      MQ_Q_NAME_LENGTH, MQ_Q_NAME_LENGTH, dlh->DestQName,
      MQ_Q_MGR_NAME_LENGTH, MQ_Q_MGR_NAME_LENGTH, dlh->DestQMgrName);
#endif

  cphLogPrintLn(pConfig->pLog, LOGWARNING, msg);

  CPHTRACEEXIT(pConfig->pTrc)
}

}
