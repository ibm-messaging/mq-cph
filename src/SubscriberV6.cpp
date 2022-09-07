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

#include <stdexcept>
#include <string.h>

#include "PubSubV6.hpp"
#include "cphConfig.h"
#include <cmqxc.h>                 /* For MQCD definition           */
#include <cmqcfc.h>                /* MQAI                  */

/* This is the name of the broker control queue used to send pub/sub control commands to */
#define CPH_SUBSCRIBERV6_CONTROL_QUEUE      "SYSTEM.BROKER.CONTROL.QUEUE"

/* This is the name of the queue from which we receive responses to control commands and published data from the broker */
#define CPH_SUBSCRIBERV6_SUBSCRIBER_QUEUE   "CPH.BROKER.SUBSCRIBER.QUEUE"

/* Response wait time for a response to a message sent to the broker control queue*/
#define CPH_SUBSCRIBERV6_MAX_RESPONSE_TIME  10000

#define CPH_SUBSCRIBERV6_DEFAULT_MESSAGE_SIZE  512

using namespace std;

namespace cph {

bool SubscriberV6::unsubscribe = true;
char SubscriberV6::streamQName[MQ_Q_NAME_LENGTH];

MQWTCONSTRUCTOR(SubscriberV6, false, true, false){
  CPHTRACEENTRY(pConfig->pTrc)

  // Populate static fields
  if(threadNum==0){
    if(pOpts->useMessageHandle)
      configError(pConfig, "(mh) Use of message handle not supported in V6 subscriber.");
    if(pOpts->useRFH2)
      configError(pConfig, "(rf) Use of superfluous RFH2 headers not supported in V6 subscriber.");
    if(pOpts->readAhead)
      configError(pConfig, "(jy) Use of read-ahead not supported in V6 subscriber.");

    if(CPHTRUE != cphConfigGetString(pConfig, streamQName, "jq"))
      configError(pConfig, "(jq) Could not determine the stream queue to use.");
    CPHTRACEMSG(pConfig->pTrc, (char*) "Stream queue: %s", streamQName)

    int temp;
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &temp, "un"))
      configError(pConfig, "(un) Could not determine whether to unsubscribe when closing subscriptions.");
    unsubscribe = temp==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, (char*) "Unsubscribe when closing subscriptions: %s", unsubscribe ? "yes" : "no")
  }

  generateCorrelID(correlId, pControlThread->procId);

  CPHTRACEEXIT(pConfig->pTrc)
}
SubscriberV6::~SubscriberV6(){}

void SubscriberV6::openDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  pControlQueue = new MQIQueue(pConnection, true, false);
  pControlQueue->setName(CPH_SUBSCRIBERV6_CONTROL_QUEUE);
  pControlQueue->open(true);

  pSubscriberQueue = new MQIQueue(pConnection, false, true);
  pSubscriberQueue->setName(CPH_SUBSCRIBERV6_SUBSCRIBER_QUEUE);
  pSubscriberQueue->open(true);

  CPHTRACEMSG(pConfig->pTrc, "Registering subscriber to topic.")
  psCommand(MQPS_REGISTER_SUBSCRIBER, MQREGO_CORREL_ID_AS_IDENTITY);

  CPHTRACEEXIT(pConfig->pTrc)
}

void SubscriberV6::closeDestination(){
  CPHTRACEENTRY(pConfig->pTrc)

  if(unsubscribe){
    CPHTRACEMSG(pConfig->pTrc, "Unregistering subscriber")
    psCommand(MQPS_DEREGISTER_SUBSCRIBER, MQREGO_CORREL_ID_AS_IDENTITY);
  }
  delete pControlQueue;
  delete pSubscriberQueue;
  pControlQueue = pSubscriberQueue = NULL;

  CPHTRACEEXIT(pConfig->pTrc)
}

void SubscriberV6::msgOneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)

  MQMD md = {MQMD_DEFAULT};
  memcpy(md.CorrelId, correlId, MQ_CORREL_ID_LENGTH);
  pSubscriberQueue->get(getMessage, md, gmo);

  CPHTRACEEXIT(pConfig->pTrc)
}

/*********************************************************************/
/*                                                                   */
/* Function Name : BuildMQRFHeader                                   */
/*                                                                   */
/* Description   : Build an MQRFH header and the accompaning         */
/*                 NameValueString.                                  */
/*                                                                   */
/* Flow          :                                                   */
/*                                                                   */
/*  Initialise the message block to nulls                            */
/*  Define the start of the message as an MQRFH                      */
/*  Set the default values of the MQRFH                              */
/*  Define the NameValueString that follows the MQRFH                */
/*   Add the command                                                 */
/*   Add registration options if supplied                            */
/*   Add publication options if supplied                             */
/*   Add topic                                                       */
/*   Add stream name                                                 */
/*  Pad the NameValueString to a 16 byte boundary                    */
/*  Set the StrucLength in the MQRFH to the total length so far      */
/*                                                                   */
/*********************************************************************/
void SubscriberV6::buildMQRFHeader(PMQRFH pRFHeader, PMQLONG pDataLength, MQCHAR const * const pCommand, MQLONG regOptions, MQLONG pubOptions){
  MQRFH DefaultMQRFH = {MQRFH_DEFAULT};

  PMQCHAR  pNameValueString;

  CPHTRACEENTRY(pConfig->pTrc)

  /*******************************************************************/
  /* Clear the buffer before we start (initialise to nulls).         */
  /*******************************************************************/
  memset((PMQBYTE)pRFHeader, '\0', *pDataLength);

  /*******************************************************************/
  /* Copy the MQRFH default values into the start of the buffer.     */
  /*******************************************************************/
  memcpy( pRFHeader, &DefaultMQRFH, (size_t)MQRFH_STRUC_LENGTH_FIXED);

  /*******************************************************************/
  /* Start the NameValueString directly after the MQRFH structure.   */
  /*******************************************************************/
  pNameValueString = (MQCHAR *)pRFHeader + MQRFH_STRUC_LENGTH_FIXED;

  /*******************************************************************/
  /* Add the command to the start of the NameValueString, this must  */
  /* always be the first MQPS name token in the string.              */
  /*******************************************************************/
  strcpy(pNameValueString, MQPS_COMMAND_B);
  strcat(pNameValueString, pCommand);

  /*******************************************************************/
  /* If registration options were supplied add them to the string,   */
  /* for ease of implementation we insert the decimal representation */
  /* of the options into the string as opposed to the character      */
  /* strings supplied for each option.                               */
  /*******************************************************************/
  if( regOptions != 0 )
  {
    char regOptsStr[64];
    sprintf(regOptsStr, " %d", regOptions);
    strcat(pNameValueString, MQPS_REGISTRATION_OPTIONS_B);
    strcat(pNameValueString, regOptsStr);
  }

  /*******************************************************************/
  /* If publication options were supplied add them to the string,    */
  /* for ease of implementation we insert the decimal representation */
  /* of the options into the string as opposed to the character      */
  /* strings supplied for each option.                               */
  /*******************************************************************/
  if( pubOptions != 0 )
  {
    char pubOptsStr[64];
    sprintf(pubOptsStr, " %d", pubOptions);
    strcat(pNameValueString, MQPS_PUBLICATION_OPTIONS_B);
    strcat(pNameValueString, pubOptsStr);
  }

  /*******************************************************************/
  /* Add the stream name to the NameValueString (optional for        */
  /* publications).                                                  */
  /*******************************************************************/
  strcat(pNameValueString, MQPS_STREAM_NAME_B);
  strcat(pNameValueString, streamQName);

  /*******************************************************************/
  /* Add the topic to the NameValueString.                           */
  /*******************************************************************/
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
  *pDataLength = MQRFH_STRUC_LENGTH_FIXED
      + (((MQLONG)strlen(pNameValueString)+15)/16)*16;
  pRFHeader->StrucLength = *pDataLength;

  CPHTRACEMSG(pConfig->pTrc, "pNameValueString: %s", pNameValueString)

  CPHTRACEEXIT(pConfig->pTrc)

}

/*********************************************************************/
/*                                                                   */
/* Function Name : cphSubscriberV6PubSubCommand                      */
/*                                                                   */
/* Description   : Build the Publish/Subscribe command message       */
/*                 and send it to the broker.                        */
/*                                                                   */
/* Flow          :                                                   */
/*                                                                   */
/*  Build the MQRFH structure and NameValueString                    */
/*  MQPUT the command to the queue as a request                      */
/*   Wait for the response to arrive                                 */
/*                                                                   */
/*********************************************************************/

void SubscriberV6::psCommand(MQCHAR const * const Command, MQLONG regOptions){
  MQPMO   localPMO = { MQPMO_DEFAULT };
  MQMD    md       = { MQMD_DEFAULT };

  CPHTRACEENTRY(pConfig->pTrc)

  /*******************************************************************/
  /* Allocate a block of storage to hold the Command message.        */
  /*******************************************************************/
  MQIMessage * pMessage = new MQIMessage(CPH_SUBSCRIBERV6_DEFAULT_MESSAGE_SIZE);
  pMessage->messageLen = CPH_SUBSCRIBERV6_DEFAULT_MESSAGE_SIZE;
  /*****************************************************************/
  /* Define an MQRFH structure at the start of the allocated       */
  /* storage, fill in the required fields and generate the         */
  /* NameValueString that follows it.                              */
  /*****************************************************************/
  buildMQRFHeader((PMQRFH) pMessage->buffer, &pMessage->messageLen, Command, regOptions, MQPUBO_NONE);

  /*****************************************************************/
  /* Send the command as a request so that a reply is returned to  */
  /* us on completion at the broker.                               */
  /*****************************************************************/
  memcpy(md.Format, MQFMT_RF_HEADER, (size_t)MQ_FORMAT_LENGTH);
  md.MsgType = MQMT_REQUEST;
  /*****************************************************************/
  /* Specify the subscriber's queue in the ReplyToQ of the MD.     */
  /* We have not put the subscriber's queue in the MQRFH           */
  /* NameValueString so the one in the ReplyToQ of the MD will be  */
  /* used as the identity of the subscriber.                       */
  /*****************************************************************/
  memcpy(md.ReplyToQ, CPH_SUBSCRIBERV6_SUBSCRIBER_QUEUE, strlen(CPH_SUBSCRIBERV6_SUBSCRIBER_QUEUE) + 1);
  localPMO.Options |= MQPMO_NEW_MSG_ID;
  /*****************************************************************/
  /* All commands sent use the correlId as part of their identity. */
  /*****************************************************************/
  memcpy(md.CorrelId, correlId, MQ_CORREL_ID_LENGTH);

  /*****************************************************************/
  /* Put the command message to the broker control queue.          */
  /*****************************************************************/
  pControlQueue->put(pMessage, md, localPMO);

  /***************************************************************/
  /* The put was successful, now wait for a response from the    */
  /* broker to inform us if the command was accepted by the      */
  /* broker.                                                     */
  /* We use our command storage block to receive the response    */
  /* into to save on allocating extra storage.                   */
  /***************************************************************/
  checkForResponse(&md, pMessage);

  /*****************************************************************/
  /* Free the storage.                                             */
  /*****************************************************************/
  delete pMessage;

  CPHTRACEEXIT(pConfig->pTrc)
}

/*********************************************************************/
/*                                                                   */
/* Function Name : CheckForResponse                                  */
/*                                                                   */
/* Description   : Wait for a reply to arrive for a command sent     */
/*                 to the broker and check for any errors.           */
/*                                                                   */
/* Flow          :                                                   */
/*                                                                   */
/*  MQGET the response message                                       */
/*   Locate the NameValueString in the message                       */
/*    Check for an 'OK' nameValueString                              */
/*    If the response is not okay:                                   */
/*     Display the error                                             */
/*                                                                   */
/*********************************************************************/
void SubscriberV6::checkForResponse(PMQMD pMd, MQIMessage * pMessage){
  MQGMO    localGMO = { MQGMO_DEFAULT };
  PMQRFH   pMQRFHeader;
  PMQCHAR  pNameValueString;

  CPHTRACEENTRY(pConfig->pTrc)

  /*******************************************************************/
  /* Wait for a response message to arrive on our subscriber queue,  */
  /* the response's correlId will be the same as the messageId that  */
  /* the original message was sent with (returned in the md from the */
  /* MQPUT) so match against this.                                   */
  /*******************************************************************/
  localGMO.Options = MQGMO_WAIT + MQGMO_CONVERT;
  localGMO.WaitInterval = CPH_SUBSCRIBERV6_MAX_RESPONSE_TIME;
  localGMO.Version = MQGMO_VERSION_2;
  localGMO.MatchOptions = MQMO_MATCH_CORREL_ID;
  memcpy( pMd->CorrelId, pMd->MsgId, sizeof(MQBYTE24));
  memset( pMd->MsgId, '\0', sizeof(MQBYTE24));

  pSubscriberQueue->get(pMessage, *pMd, localGMO);

  /*****************************************************************/
  /* Check that the message is in the MQRFH format.                */
  /* If the message is not in the MQRFH format we have the wrong   */
  /* message.                                                      */
  /*****************************************************************/
  if( memcmp(pMd->Format, MQFMT_RF_HEADER, MQ_FORMAT_LENGTH) != 0 )
    throw runtime_error(string("Unexpected message format: ") + pMd->Format);
  /***************************************************************/
  /* Locate the start of the NameValueString and its length.     */
  /***************************************************************/
  pMQRFHeader = (PMQRFH)pMessage->buffer;
  pNameValueString = (PMQCHAR)(pMessage->buffer + MQRFH_STRUC_LENGTH_FIXED);

  /***************************************************************/
  /* The start of a response NameValueString is always in the    */
  /* same format:                                                */
  /*   MQPSCompCode x MQPSReason y MQPSReasonText string ...     */
  /* We can scan the start of the string to check the CompCode   */
  /* and reason of the reply.                                    */
  /***************************************************************/
  MQLONG mqcc, mqrc;
  sscanf(pNameValueString, "MQPSCompCode %d MQPSReason %d ", &mqcc, &mqrc);
  if( mqcc != MQCC_OK ) {
    /*************************************************************/
    /* One possible error is acceptable, MQRCCF_NO_RETAINED_MSG, */
    /* which is returned from a Request Update when there is no  */
    /* retained message on the broker. This is an allowable      */
    /* error so we can continue as before.                       */
    /*************************************************************/
    if( mqrc != MQRCCF_NO_RETAINED_MSG ){
      /***********************************************************/
      /* Otherwise, display the error message supplied with the  */
      /* user data that was returned, this will be the original  */
      /* commands NameValueString.                               */
      /*                                                         */
      /* A response NameValueString is ALWAYS NULL terminated,   */
      /* therefore, we can use printf to display it (as it is a  */
      /* string in the true sense of the word). We do not        */
      /* necessarily generate NULL terminated NameValueStrings   */
      /* so we have to be more careful when displaying           */
      /* the NameValueString returned with the message, if any   */
      /* (most error responses do return the original            */
      /* NameValueString as user data).                          */
      /***********************************************************/
      char messageString[1024];
      sprintf(messageString, "Error response returned :\n %s\n", pNameValueString);
      cphLogPrintLn(pConfig->pLog, LOG_ERROR, messageString);

      if( pMessage->messageLen != pMQRFHeader->StrucLength ){
        sprintf(messageString, "Original Command String:\n");
        cphLogPrintLn(pConfig->pLog, LOG_ERROR, messageString);

        pNameValueString = (PMQCHAR)(pMessage->buffer + pMQRFHeader->StrucLength);
        strncpy(messageString, pNameValueString, pMessage->messageLen - pMQRFHeader->StrucLength);
        messageString[pMessage->messageLen - pMQRFHeader->StrucLength] = '\0';
        cphLogPrintLn(pConfig->pLog, LOG_ERROR, messageString);
      }
    }
  }
  CPHTRACEEXIT(pConfig->pTrc)
}

}
