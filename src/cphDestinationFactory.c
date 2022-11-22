/*<copyright notice="lm-source" pids="" years="2007,2022">*/
/*******************************************************************************
 * Copyright (c) 2007,2022 IBM Corp.
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
 *    Jerry Stevens - Initial implementation
 *    Various members of the WebSphere MQ Performance Team at IBM Hursley UK
 *******************************************************************************/
/*</copyright>*/
/*******************************************************************************/
/*                                                                             */
/* Performance Harness for IBM MQ C-MQI interface                              */
/*                                                                             */
/*******************************************************************************/

#include <string.h>
#include <assert.h>

#include "cphStringBuffer.h"
#include "cphDestinationFactory.h"

/*
** Method: cphDestinationFactoryIni
**
** Create and initialise the control block for the destination factory
**
** Input Parameters:
**
** Output parameters: A double pointer to the newly created control block
**
*/
void cphDestinationFactoryIni(CPH_DESTINATIONFACTORY **ppDestinationFactory, CPH_CONFIG *pConfig) {
    CPH_DESTINATIONFACTORY *pDestinationFactory = NULL;
    int mode;
    int destBase;
    int destMax = 0;
    int destNumber;
    char destPrefix[50];
  char i_queuePrefix[50];
    char o_queuePrefix[50];
    int nextDest = 0;
    int status = CPHTRUE;
    CPH_SPINLOCK *pDestCreateLock;

    assert(pConfig != NULL);
    assert(pConfig->pTrc != NULL);

    CPHTRACEREF(pTrc, pConfig->pTrc)
    CPHTRACEENTRY(pTrc)

    if (CPHTRUE != cphConfigGetInt(pConfig, &destBase, "db")) {
        cphConfigInvalidParameter(pConfig, "(db) Mult-destination numeric base cannot be retrieved");
        status = CPHFALSE;
    }
    else
        CPHTRACEMSG(pTrc, "destBase: %d.", destBase)

    if (CPHTRUE != cphConfigGetInt(pConfig, &destMax, "dx")) {
        cphConfigInvalidParameter(pConfig,  "(dx) Mult-destination numeric maximum cannot be retrieved");
        status = CPHFALSE;
    }
    else
        CPHTRACEMSG(pTrc, "Multi-destination numeric maximum: %d.", destMax)

    if (CPHTRUE != cphConfigGetInt(pConfig, &destNumber, "dn")) {
        cphConfigInvalidParameter(pConfig, "(dn) Mult-destination numeric range cannot be retrieved");
        status = CPHFALSE;
    }
    else
        CPHTRACEMSG(pTrc, "Multi-destination numeric range: %d.", destNumber)

    if (CPHTRUE != cphConfigGetString(pConfig, destPrefix, sizeof(destPrefix), "d")) {
        cphConfigInvalidParameter(pConfig, "(d) Destination Prefix cannot be retrieved");
        status = CPHFALSE;
    }
    else
        CPHTRACEMSG(pTrc, "Destination prefix: %s.", destPrefix)

  if (CPHTRUE != cphConfigGetString(pConfig, i_queuePrefix, sizeof(i_queuePrefix), "iq")) {
    cphConfigInvalidParameter(pConfig, "(iq) In-Queue cannot be retrieved");
    status = CPHFALSE;
  } else {
    CPHTRACEMSG(pTrc, "Input Queue prefix: %s.", i_queuePrefix)
  }

  if (CPHTRUE != cphConfigGetString(pConfig, o_queuePrefix, sizeof(o_queuePrefix), "oq")) {
    cphConfigInvalidParameter(pConfig, "(oq) Out-Queue cannot be retrieved");
    status = CPHFALSE;
  } else {
    CPHTRACEMSG(pTrc, "Output Queue prefix: %s.", o_queuePrefix)
  }

    if (CPHFALSE == cphSpinLock_Init( &(pDestCreateLock))) {
        cphLogPrintLn(pConfig->pLog, LOG_ERROR, "Cannot create destination create lock" );
        status = CPHFALSE;
    }

    mode = CPH_DESTINATIONFACTORY_MODE_SINGLE;
  if ( destBase>0 && destMax>0 && destNumber>0 ) {

    /* Special case: if *all* elements are set we use -dn as the
       start point in the loop */
    mode = CPH_DESTINATIONFACTORY_MODE_DIST;
    nextDest = destNumber;
    destNumber = destMax - destBase + 1;

    if ( nextDest<destBase || nextDest>destMax ) {
      cphConfigInvalidParameter(pConfig, "-dn must be within the specified bounds of -db and -dx");
            status = CPHFALSE;
    }

  } else if ( destBase>0 || destMax>0 || destNumber>0 ) {
    /* If we have specified any the elements ... */

    mode = CPH_DESTINATIONFACTORY_MODE_DIST;

    if ( destBase==0 ) {
      /* we need to calculate the base */
      if ( destMax>0 && destNumber>0 ) {
        destBase = destMax - destNumber + 1;
      } else {
        destBase = 1;
      }
    }

    /* We know topicBase is set... */
    nextDest = destBase;

    if ( destMax==0 ) {
      /* we need to calculate the max */
      if ( destNumber>0 ) {
        destMax = destNumber + destBase - 1;
      } else {
        /* There is no max !  This implies an ever increasing count */
      }
    }

    if ( destNumber==0 ) {
      /* we need to set topicNumber */
      if ( destMax>0 ) {
        destNumber = destMax - destBase + 1;
      } else {
        /* There is no max! */
      }
    }

  } /* end if any multi-destination settings */

  /* enforce destination constraints */
  //if ( destBase<0 || destMax<destBase ) {
    if ( destBase<0 || ( (destMax > 0) && destMax<destBase ) ) {
    cphConfigInvalidParameter(pConfig, "Destination range is negative." );
        status = CPHFALSE;
  }

    if ( (CPHTRUE == status) &&
         ( NULL != (pDestinationFactory = malloc(sizeof(CPH_DESTINATIONFACTORY)))) ) {
        pDestinationFactory->pConfig = pConfig;

        pDestinationFactory->mode = mode;
        pDestinationFactory->destBase = destBase;
        pDestinationFactory->destMax = destMax;
        pDestinationFactory->destNumber = destNumber;
        pDestinationFactory->nextDest = nextDest;
        strcpy(pDestinationFactory->destPrefix, destPrefix);
        strcpy(pDestinationFactory->i_queuePrefix, i_queuePrefix);
        strcpy(pDestinationFactory->o_queuePrefix, o_queuePrefix);
        pDestinationFactory->pDestCreateLock = pDestCreateLock;
    }

    *ppDestinationFactory = pDestinationFactory;

    CPHTRACEEXIT(pTrc)
}

/*
** Method: cphDestinationFactoryFree
**
** Free a destinfactory control block
**
** Input Parameters: ppDestinationFactory - double pointer to the destination factory to be freed
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphDestinationFactoryFree(CPH_DESTINATIONFACTORY **ppDestinationFactory) {
    int res = CPHFALSE;

    CPHTRACEREF(pTrc, (*ppDestinationFactory)->pConfig->pTrc)
    CPHTRACEENTRY(pTrc)

    if (NULL != *ppDestinationFactory) {
        cphSpinLock_Destroy( &((*ppDestinationFactory)->pDestCreateLock));
        free(*ppDestinationFactory);
    *ppDestinationFactory = NULL;
    res = CPHTRUE;
  }

    CPHTRACEEXIT(pTrc)
    return(res);
}

int cphDestinationFactoryGenerateDestinationIndex(CPH_DESTINATIONFACTORY *pDestinationFactory){
  if(pDestinationFactory->mode == CPH_DESTINATIONFACTORY_MODE_DIST)
    return cphDestinationFactoryGetNextDestination(pDestinationFactory);
  else
    return -1;
}

/*
** Method: cphGenerateDestination
**
** Generates a destination name. If using asingle destination the destination is simply set to the
** destination prefix. If using multiple destinations then the prefix is appended with the next
** numeric suffix as calculated by cphDestinationFactoryGetNextDestination.
**
** Input Parameters: pDestinationFactory - pointer to the destination factory
**
** Output Parameters: destString - the string into which the determined destination string should be copied
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphDestinationFactoryGenerateDestination(CPH_DESTINATIONFACTORY *pDestinationFactory, char *destString, char *inQueue, char *outQueue) {
    CPH_STRINGBUFFER *pSb;
  int nextDest;

    CPHTRACEREF(pTrc,pDestinationFactory->pConfig->pTrc)
    CPHTRACEENTRY(pTrc)

    switch ( pDestinationFactory->mode  ) {

            /* If using single destinations only, then the prefix is the destination */
            case CPH_DESTINATIONFACTORY_MODE_SINGLE:
                strcpy(destString, pDestinationFactory->destPrefix);
                strcpy(inQueue, pDestinationFactory->i_queuePrefix);
                strcpy(outQueue, pDestinationFactory->o_queuePrefix);
                break;

            /* For multiple destinations, determine the next in the sequence determined by the command line options */
            case CPH_DESTINATIONFACTORY_MODE_DIST:
        nextDest = cphDestinationFactoryGetNextDestination(pDestinationFactory);

                cphStringBufferIni(&pSb);
                cphStringBufferAppend(pSb, pDestinationFactory->destPrefix);
                cphStringBufferAppendInt(pSb, nextDest);
                strcpy(destString, cphStringBufferToString(pSb));
                cphStringBufferFree(&pSb);

        cphStringBufferIni(&pSb);
                cphStringBufferAppend(pSb, pDestinationFactory->i_queuePrefix);
                cphStringBufferAppendInt(pSb, nextDest);
                strcpy(inQueue, cphStringBufferToString(pSb));
                cphStringBufferFree(&pSb);

                cphStringBufferIni(&pSb);
                cphStringBufferAppend(pSb, pDestinationFactory->o_queuePrefix);
                cphStringBufferAppendInt(pSb, nextDest);
                strcpy(outQueue, cphStringBufferToString(pSb));
                cphStringBufferFree(&pSb);
                break;

            /* If we don't recognise the mode set the generated destination to NULL */
            default:
              *destString = '\0';
              *inQueue = '\0';
              *outQueue = '\0';

    } /* end switch */

    CPHTRACEMSG(pTrc, "Generated destination: %s.", destString)
    CPHTRACEEXIT(pTrc)

    return CPHTRUE;
}

/*
** Method: cphDestinationFactoryGetNextDestination
**
** Determine the next numeric suffix for a multi-destination topic. This is determined in accord with
** the command line options "db", "dn" and "dx".
**
** Input Parameters: pDestinationFactory - pointer to the destination factory
**
** Returns: the next numeric suffix to be used
**
*/
int cphDestinationFactoryGetNextDestination(CPH_DESTINATIONFACTORY *pDestinationFactory) {
    int num;
    CPHTRACEENTRY(pDestinationFactory->pConfig->pTrc)

    cphSpinLock_Lock(pDestinationFactory->pDestCreateLock);
    num = pDestinationFactory->nextDest++;

    /* Note loose comparison here allows Max=0 to
    leave an infinite sequence. */
    if ( pDestinationFactory->nextDest == (pDestinationFactory->destMax+1) ) {
        pDestinationFactory->nextDest = pDestinationFactory->destBase;
    }

    cphSpinLock_Unlock(pDestinationFactory->pDestCreateLock);

    CPHTRACEEXIT(pDestinationFactory->pConfig->pTrc)
    return num;
}
