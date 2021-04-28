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

#include "PubSubV6.hpp"
#include <stdexcept>
#include <string.h>

#include "cphConfig.h"

/* This is the initial length of the buffer used for assembing the RFH2 before we add the message data */
#define CPH_PUBLISHERV6_INITIAL_BUFFER_LENGTH 1024

using namespace std;

namespace cph {

bool PublisherV6::topicPerMsg;
char PublisherV6::streamQName[MQ_Q_NAME_LENGTH];

MQWTCONSTRUCTOR(PublisherV6, true, false, false),
    pDestFactory(pControlThread->pDestinationFactory),
    headerLen(CPH_PUBLISHERV6_INITIAL_BUFFER_LENGTH) {
  CPHTRACEENTRY(pConfig->pTrc)

  if(threadNum==0){

    if(pOpts->useMessageHandle)
      configError(pConfig, "(mh) Use of message handle not supported in V6 publisher.");
    if(pOpts->useRFH2)
      configError(pConfig, "(rf) Use of superfluous RFH2 headers not supported in V6 publisher.");


    if(CPHTRUE != cphConfigGetString(pConfig, streamQName, "jq"))
      configError(pConfig, "(jq) Could not determine the stream queue to use.");
    CPHTRACEMSG(pConfig->pTrc, (char*) "Stream queue: %s", streamQName)

    int temp;
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "tp"))
      configError(pConfig, "(tp) Could not determine whether to use one topic per publisher.");
    topicPerMsg = temp!=CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Publish to a new topic every iteration: %s", topicPerMsg ? "yes" : "no")
  }

  /*
   * Build RFH message header into message buffer
   */
  // Resize message buffer to accommodate at least the RFH header
  if(putMessage->bufferLen<headerLen)
    putMessage->resize(headerLen);

  // Build RFH header in message buffer
  memset(putMessage->buffer, 0, putMessage->bufferLen);
  buildMQRFHeader((PMQRFH)putMessage->buffer, &headerLen);

  // Increase the size of the message buffer to accommodate payload as well
  putMessage->resize(headerLen + pOpts->messageSize + 1);

  // Generate payload, and append it to the buffer
  MQBYTE * buffer = (MQBYTE*) cphUtilMakeBigString(pOpts->messageSize, pOpts->isRandom);
  strcpy((char*) putMessage->buffer + headerLen, (char*) buffer);
  putMessage->messageLen = putMessage->bufferLen;

  CPHTRACEEXIT(pConfig->pTrc)
}
PublisherV6::~PublisherV6(){}

/*********************************************************************/
/*                                                                   */
/* Function Name : BuildMQRFHeader                                   */
/*                                                                   */
/* Description   : Build the MQRFH header and accompaning            */
/*                 NameValueString.                                  */
/*                                                                   */
/* Flow          :                                                   */
/*                                                                   */
/*  Initialise the message block to nulls                            */
/*  Define the start of the message as an MQRFH                      */
/*  Set the default values of the MQRFH                              */
/*  Set the format of the user data in the MQFRH                     */
/*  Set the CCSID of the user data in the MQRFH                      */
/*  Define the NameValueString that follows the MQRFH                */
/*   Add the command                                                 */
/*   Add publication options                                         */
/*   Add topic                                                       */
/*  Pad the NameValueString to a 16 byte boundary                    */
/*  Set the StrucLength in the MQRFH to the total length so far      */
/*                                                                   */
/* Input Parms   : PMQBYTE pStart                                    */
/*                  Start of message block                           */
/*                 MQCHAR  TopicType[]                               */
/*                  Topic name suffix string                         */
/*                                                                   */
/* Input/Output  : PMQLONG pDataLength                               */
/*                  Size of message block on entry and amount of     */
/*                  block used on exit                               */
/*                                                                   */
/*********************************************************************/
inline void PublisherV6::buildMQRFHeader(PMQRFH pRFHeader, PMQLONG pDataLength){
  MQRFH DefaultMQRFH = {MQRFH_DEFAULT};

  PMQCHAR  pNameValueString;

  CPHTRACEENTRY(pConfig->pTrc)

  /*******************************************************************/
  /* Clear the buffer before we start (initialise to nulls).         */
  /*******************************************************************/
  memset((PMQBYTE)pRFHeader, 0, *pDataLength);

  /*******************************************************************/
  /* Copy the MQRFH default values into the start of the buffer.     */
  /*******************************************************************/
  memcpy( pRFHeader, &DefaultMQRFH, (size_t)MQRFH_STRUC_LENGTH_FIXED);

  /*******************************************************************/
  /* Set the format of the user data to be MQFMT_STRING, even though */
  /* some of the publications use a structure to pass user data the  */
  /* data within this structure is entirely MQCHAR and can be        */
  /* treated as MQFMT_STRING by the data conversion routines.        */
  /*******************************************************************/
  memcpy( pRFHeader->Format, MQFMT_STRING, (size_t)MQ_FORMAT_LENGTH);

  /*******************************************************************/
  /* As we have user data following the MQRFH we must set the CCSID  */
  /* of the user data in the MQRFH for data conversion to be able to */
  /* be performed by the queue manager. As we do not currently know  */
  /* the CCSID that we are running in we can tell MQSeries that the  */
  /* data that follows the MQRFH is in the same CCSID as the MQRFH.  */
  /* The MQRFH will default to the CCSID of the queue manager        */
  /* (MQCCSI_Q_MGR), so the user data will also inherit this CCSID.  */
  /*******************************************************************/
  pRFHeader->CodedCharSetId = MQCCSI_INHERIT;

  /*******************************************************************/
  /* Start the NameValueString directly after the MQRFH structure.   */
  /*******************************************************************/
  pNameValueString = (MQCHAR *)pRFHeader + MQRFH_STRUC_LENGTH_FIXED;

  /*******************************************************************/
  /* Add the command to the start of the NameValueString, this must  */
  /* always be the first MQPS name token in the string.              */
  /*******************************************************************/
  strcpy(pNameValueString, MQPS_COMMAND_B);
  strcat(pNameValueString, MQPS_PUBLISH);

  /*******************************************************************/
  /* Add the publication options and topic to the NameValueString.   */
  /* We specify 'no registration' because neither sample application */
  /* is concerned with who is currently publishing, it also allows   */
  /* us not to specify an identity queue (we are also publishing     */
  /* datagrams so no replies will be sent either) which means that   */
  /* we do not have to define a queue for this sample to use.        */
  /*******************************************************************/
  strcat(pNameValueString, MQPS_PUBLICATION_OPTIONS_B);
  strcat(pNameValueString, MQPS_NO_REGISTRATION);

  strcat(pNameValueString, MQPS_TOPIC_B);
  strcat(pNameValueString, pOpts->destinationPrefix);
  if(destinationIndex>=0)
    sprintf(pNameValueString+strlen(pNameValueString), "%d", destinationIndex);

  /*******************************************************************/
  /* Any user data that follows the NameValueString should start on  */
  /* a word boundary, to ensure all platforms are satisfied we align */
  /* to a 16 byte boundary.                                          */
  /* As the NameValueString has been null terminated (by using       */
  /* strcat) any characters between the end of the string and the    */
  /* next 16 byte boundary will be ignored by the broker, but if the */
  /* message is to be data converted we advise any extra characters  */
  /* are set to nulls ('\0') or blanks (' '). In this sample we have */
  /* initialised the whole message block to nulls before we started  */
  /* so all extra characters will be nulls by default.               */
  /*******************************************************************/
  *pDataLength = (MQLONG)(MQRFH_STRUC_LENGTH_FIXED
      + ((strlen(pNameValueString)+15)/16)*16);
  pRFHeader->StrucLength = *pDataLength;

  CPHTRACEEXIT(pConfig->pTrc)
}

void PublisherV6::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  pStreamQueue = new MQIQueue(pConnection, true, false);
  pStreamQueue->setName(streamQName);
  pStreamQueue->open(true);

  strncpy(putMD.Format, MQFMT_RF_HEADER, (size_t)MQ_FORMAT_LENGTH);
  CPHTRACEEXIT(pConfig->pTrc)
}

void PublisherV6::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)
  delete pStreamQueue;
  pStreamQueue = NULL;
  CPHTRACEEXIT(pConfig->pTrc)
}

void PublisherV6::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)
  if(topicPerMsg){
    MQLONG newHeaderLen = headerLen;
    buildMQRFHeader((PMQRFH)putMessage->buffer, &newHeaderLen);
    if(newHeaderLen!=headerLen)
      throw runtime_error("Length of new RFH header does not match space originally allocated.");
  }

  pStreamQueue->put(putMessage, putMD, pmo);

  if(topicPerMsg)
    destinationIndex = cphDestinationFactoryGenerateDestinationIndex(pDestFactory);

  CPHTRACEEXIT(pConfig->pTrc)
}

}
