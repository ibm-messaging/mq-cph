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

#include <cstring>
#include <sstream>
#include <cstdlib>

#include "MQI.hpp"

using namespace std;

namespace cph {
bool MQIException::printDetails = true;

/*
 * ----------------------------------------
 * Method implementations for MQIConnection
 * ----------------------------------------
 */

MQCMHO const CrtMHOpts = {MQCMHO_DEFAULT};
MQBMHO const BufMHOpts = {MQBMHO_DEFAULT};
MQDMHO const DltMHOpts = {MQDMHO_DEFAULT};

MQIConnection::MQIConnection(MQIWorkerThread * const pOwner, bool reconnect) :
    pCurrentThread(pOwner),
    pTrc(pOwner->pConfig->pTrc),
    pLog(pOwner->pConfig->pLog),
    pOpts(pOwner->pOpts),
    name(pOwner->name.data()) {
  CPHTRACEENTRY(pTrc)

  snprintf(msg, CPH_MQC_MSG_LEN, "[%s] Connecting to QM: %s", name, pOpts->QMName);
  
  MQCNO cno;
  //char procId[80];

  CPH_TIME reconnectStart;
  //long reconnectTime_ms=0;
  
  cno = pOpts->getCNO();
  
  if(pOpts->connType==REMOTE && !pOpts->useChannelTable){
	   snprintf(msg+strlen(msg), CPH_MQC_MSG_LEN-strlen(msg), " (connection: %s; channel: %s)\n",
        ((MQCD*) cno.ClientConnPtr)->ConnectionName, ((MQCD*) cno.ClientConnPtr)->ChannelName);
       cphLogPrintLn(pLog, LOG_VERBOSE, msg);
  }

  if(!reconnect){
	  CPHCALLMQ(pTrc, MQCONNX, (PMQCHAR) pOpts->QMName, &cno, &hConn)
	  ownsConnection = mqrc!=MQRC_ALREADY_CONNECTED;	
  } else {
	  int cc;
	  int rc;
	  do{
		 if(pOwner->threadNum==0){ 
		    reconnectStart = cphUtilGetNow();	
		 }
		 cc = 0;
		 rc = 0;
	     try{
	         CPHCALLMQ(pTrc, MQCONNX, (PMQCHAR) pOpts->QMName, &cno, &hConn)	
	     } catch (cph::MQIException e){
	       cc = e.compCode;
		   rc = e.reasonCode;
         } 
		 //Code to see the results of reconnect attempts for first thread)
		 //reconnectTime_ms = cphUtilGetTimeDifference(cphUtilGetNow(),reconnectStart);
   	     //if(pOwner->threadNum==0){
		 //   cphConfigGetString(pOwner->pConfig, procId, "id");
   	     //   printf("[%s][%s]====cc: %i;  rc: %i===== Response time: %ld\n",procId,name,cc,rc,reconnectTime_ms);	
		 //}
	  } while(cc != 0);
  }

  CPHTRACEEXIT(pTrc)
}

MQIConnection::~MQIConnection() {
  CPHTRACEENTRY(pTrc)
  if(ownsConnection){
    snprintf(msg, CPH_MQC_MSG_LEN, "[%s] Disconnecting from QM: %s", name, pOpts->QMName);
    cphLogPrintLn(pLog, LOG_VERBOSE, msg);
    try {
      CPHCALLMQ(pTrc, MQDISC, &hConn)
    } catch (cph::MQIException) {
      CPHTRACEMSG(pTrc, "OOPS! MQIException in ~MQIConnection()!")
    }
  }
  CPHTRACEEXIT(pTrc)
}

/*
 * Method: createGetMessageHandle
 * ------------------------------
 *
 * Create a new message handle (MQHMSG), for associating with an MQGMO.
 */
MQHMSG MQIConnection::createGetMessageHandle() const {
  CPHTRACEENTRY(pTrc)
  MQHMSG rc = MQHM_NONE;
  CPHCALLMQ(pTrc, MQCRTMH, hConn, (PMQVOID) &CrtMHOpts, &rc)
  CPHTRACEEXIT(pTrc)
  return rc;
}

/*
 * Method: createPutMessageHandle
 * ------------------------------
 *
 * Create a new message handle (MQHMSG), for associating with an MQPMO.
 * The given RFH2 properties buffer will be loaded into the handle's properties in MQ.
 */
MQHMSG MQIConnection::createPutMessageHandle(MQMD* md, char* propsBuffer, MQLONG bufferLength) const {
  CPHTRACEENTRY(pTrc)
  MQHMSG rc = createGetMessageHandle();
  MQLONG dataLen;
  CPHCALLMQ(pTrc, MQBUFMH, hConn, rc, (PMQVOID) &BufMHOpts, md, bufferLength, propsBuffer, &dataLen)
  CPHTRACEEXIT(pTrc)
  return rc;
}

/*
 * Method: destroyMessageHandle
 * ----------------------------
 *
 * Destroy a previously created MQHMSG.
 */
void MQIConnection::destroyMessageHandle(MQHMSG& handle) const {
  CPHTRACEENTRY(pTrc)
  CPHCALLMQ(pTrc, MQDLTMH, hConn, &handle, (PMQVOID) &DltMHOpts)
  handle = MQHM_NONE;
  CPHTRACEEXIT(pTrc)
}

/*
 * Method: commitTransaction
 * -------------------------
 *
 * Commit any previous operations on objects associated with this HCONN
 * that were made under syncpoint.
 */
void MQIConnection::commitTransaction() const{
  CPHTRACEENTRY(pTrc)
  CPHCALLMQ(pTrc, MQCMIT, hConn)
  CPHTRACEEXIT(pTrc)
}

/*
 * Method: commitTransaction_try
 * -------------------------
 *
 * Commit any previous operations on objects associated with this HCONN
 * that were made under syncpoint.
 */
MQLONG MQIConnection::commitTransaction_try() const{
  CPHTRACEENTRY(pTrc)
	  MQLONG rc =0;
  try{	
     CPHCALLMQ(pTrc, MQCMIT, hConn)
  } catch (cph::MQIException e){
   rc = e.reasonCode;
  }
  CPHTRACEEXIT(pTrc)
	  return rc;
}

/*
 * Method: rollbackTransaction
 * ---------------------------
 *
 * Roll back any previous operations on objects associated with this HCONN
 * that were made under syncpoint.
 */
void MQIConnection::rollbackTransaction() const {
  CPHTRACEENTRY(pTrc)
  CPHCALLMQ(pTrc, MQBACK, hConn)
  CPHTRACEEXIT(pTrc)
}

/*
 * -------------------------------------
 * Method implementations for MQIMessage
 * -------------------------------------
 */

inline void freeMessage(MQIMessage* message){
  if(message->buffer != NULL)
      free(message->buffer);
}

/*
 * Constructor: MQIMessage
 * ----------------------
 *
 * Create a new empty MQIMessage.
 *
 * Input parameters:
 *    size - The size of the message buffer to create.
 */
MQIMessage::MQIMessage(size_t size) :
    buffer((MQBYTE*) malloc(size)),
    bufferLen((MQLONG)size), messageLen(0) {
  if(buffer==NULL)
    throw runtime_error("Could not allocate message buffer.");
}

/*
 * Constructor: MQIMessage
 * ----------------------
 *
 * Create a new MQIMessage.
 *
 * Input parameters:
 *    empty - If false, generates a message string (payload) and puts it in the buffer.
 *    pOpts - MQ messaging options. Used to determine message size and whether to generate an RFH2 header.
 */
MQIMessage::MQIMessage(MQIOpts const * const pOpts, bool empty) {
  if(empty){
    bufferLen = pOpts->receiveSize;
    if(NULL == (buffer = (MQBYTE*) malloc(bufferLen)))
      throw runtime_error("Could not allocate message buffer.");
  } else {
    size_t rfh2size = 0;
    buffer = (MQBYTE*) (pOpts->useRFH2 ? cphUtilMakeBigStringWithRFH2(pOpts->messageSize, &rfh2size) : cphUtilMakeBigString(pOpts->messageSize, pOpts->isRandom));
    if(buffer==NULL) throw runtime_error("Could not allocate message buffer.");
    bufferLen = messageLen = pOpts->messageSize + (MQLONG) rfh2size;
  }
}

/*
 * Method: resize
 * --------------
 *
 * Resize this message's buffer.
 */
void MQIMessage::resize(MQLONG newSize){
  bufferLen = newSize;
  void* tmp = realloc(buffer, bufferLen);
  if (NULL == tmp) {
    free(buffer);
    buffer = NULL;
    throw runtime_error("Could not reallocate message buffer.");
  } else {
    buffer = (MQBYTE*)tmp;
  }
  if(messageLen>bufferLen)
    messageLen = bufferLen;
}

MQIMessage::~MQIMessage(){
  freeMessage(this);
}

/*
 * ---------------------------------------
 * Method implementations for MQIException
 * ---------------------------------------
 */

inline string getErrorMsg(string functionName, MQLONG compCode, MQLONG reasonCode, MQBYTE24 id, const MQCHAR48 qName, bool printDetails){
  stringstream ss;
  char errorMsg[512] ="";

  if (id != NULL){ //supply the correl id/queue name of the message that was not retrieved
    //Hex encoded is going to be twice as long as the bytes array
    char newId[MQ_CORREL_ID_LENGTH * 2 + 1];
    uint8_t * castedId = (uint8_t *)id;

    memset(newId, 0, MQ_CORREL_ID_LENGTH * 2 + 1);
    int len = 0;
    for (int i = 0; i < 24; ++i){
      len += snprintf(newId + len, MQ_CORREL_ID_LENGTH, "%02X", static_cast<unsigned>(castedId[i]));
    }
    //ss << "Call to " << functionName << " failed [Completion code: " << compCode << "; Reason code: " << reasonCode << "]" << " -correlId: " << newId << " QName: " << qName;
    snprintf(errorMsg, 512, "Call to %s failed [Completion code: %ld; Reason code: %ld] CorrelId:%s QName:%s", &functionName[0], compCode, reasonCode, newId, qName);

  }
  else{
    //ss << "Call to " << functionName << " failed [Completion code: " << compCode << "; Reason code: " << reasonCode << "]";
    snprintf(errorMsg, 512, "Call to %s failed [Completion code: %ld; Reason code: %ld]\n]", &functionName[0], compCode, reasonCode);
  }
  if(printDetails){
	  printf("Created Error message to pass to runtime_error()\n");
	  printf(errorMsg);

	  fprintf(stderr, "Sending errorMsg to STDERR\n");
	  fprintf(stderr, errorMsg);
  }
  return errorMsg;
}

MQIException::MQIException(string functionName, MQLONG compCode, MQLONG reasonCode, MQBYTE24 id, const MQCHAR48 qName):
    runtime_error(getErrorMsg(functionName, compCode, reasonCode, id, qName, printDetails)),
    function(functionName),
    compCode(compCode),
    reasonCode(reasonCode) {}

MQIException::~MQIException() throw() {}

}
