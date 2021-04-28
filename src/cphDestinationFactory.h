/*<copyright notice="lm-source" pids="" years="2007,2017">*/
/*******************************************************************************
 * Copyright (c) 2007,2017 IBM Corp.
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

#ifndef _CPHDESTINATIONFACTORY
#define _CPHDESTINATIONFACTORY
#if defined(__cplusplus)
  extern "C" {
#endif

#include "cphConfig.h"
#include "cphSpinLock.h"

#define CPH_DESTINATIONFACTORY_MODE_SINGLE 0   /* If "mode" is set to this, it signifies we are not using multiple destinations */
#define CPH_DESTINATIONFACTORY_MODE_DIST 1     /* If "mode" is set to this, then we are using multiple destinations */

/* The destination factory control structure */
typedef struct _cphdestinationfactory {

   CPH_CONFIG *pConfig;   /* Pointer to the cph configuration control structure */

   int mode;              /* Whether we are running in single or multi-destination mode */

   int destBase;          /* The numeric base for multi-destination mode */
   int destMax;           /* The numeric maximum for multi-destination mode */
   int destNumber;        /* The numeric range for multi-destination mode */
   char destPrefix[50];   /* The prefix to the destination - for single destination mode the destination is set to
                             just the prefix. For multi-destination mode the prefix is appended with the next numeric
                             destination number */
   char i_queuePrefix[50];   /* The prefix of the input queue for RR clients - for single destination mode the destination is set to
                             just the prefix. For multi-destination mode the prefix is appended with the next numeric
                             destination number */
   char o_queuePrefix[50];   /* The prefix of the output queue for RR clients - for single destination mode the destination is set to
                             just the prefix. For multi-destination mode the prefix is appended with the next numeric
                             destination number */

   int nextDest;          /* The next destination to be used */
   CPH_SPINLOCK *pDestCreateLock;  /* Lock to be used when generating destinations to protect the next destination number */

} CPH_DESTINATIONFACTORY;

/* Function prototypes */
void cphDestinationFactoryIni(CPH_DESTINATIONFACTORY **ppDestinationFactory, CPH_CONFIG *pConfig);
int cphDestinationFactoryFree(CPH_DESTINATIONFACTORY **ppDestinationFactory);
int cphDestinationFactoryGenerateDestinationIndex(CPH_DESTINATIONFACTORY *pDestinationFactory);
int cphDestinationFactoryGenerateDestination(CPH_DESTINATIONFACTORY *pDestinationFactory, char *destString, char *inQueue, char *outQueue);
int cphDestinationFactoryGetNextDestination(CPH_DESTINATIONFACTORY *pDestinationFactory);

/*
 * Macro: CPH_DESTINATIONFACTORY_CALL_PRINTF
 * -----------------------------------------
 *
 * Build a correctly-indexed destination from a given prefix string
 * and suffix index, by appropriately calling a function that accepts
 * a printf-style set of arguments.
 */
#define CPH_DESTINATIONFACTORY_CALL_PRINTF(FUNC, PREFIX, SUFFIX)\
  if(SUFFIX==-1)\
    FUNC(PREFIX);\
  else\
    FUNC("%s%d", PREFIX, SUFFIX);

#if defined(__cplusplus)
  }
#endif
#endif
