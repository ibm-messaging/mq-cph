/*<copyright notice="lm-source" pids="" years="2007,2018">*/
/*******************************************************************************
 * Copyright (c) 2007,2018 IBM Corp.
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

/* The functions in this module provide a logging mechanism for cph. This permits messages to
** be written to stdout or stderr according to the specified verbosity levels and the level
** at which an individual message is written */

#include "cphLog.h"

/*
** Method: cphLogIni
**
** Create and initialise a CPH_LOG control block.
**
** Output Parameters: ppLog - double pointer in which the address for the newly created
** control block is set
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphLogIni(CPH_LOG **ppLog, CPH_TRACE *pTrc) {

  CPH_LOG *pLog = NULL;

  pLog = malloc(sizeof(CPH_LOG));
  if (pLog != NULL) {
     pLog->verbosityToSTDOUT = LOG_VERBOSE;
     pLog->verbosityToSTDERR = LOG_NONE;
     pLog->errorsReported = CPHFALSE;
       pLog->pTrc = pTrc;
  }
  *ppLog = pLog;
  return CPHTRUE;
}

/*
** Method: cphLogFree
**
** This method frees the log control block. The pointer to the control
** block is set to NULL on successful execution.
**
** Input/Output Parameters: ppLog - double pointer to the CPH_LOG control block
**
*/
void cphLogFree(CPH_LOG **ppLog) {
  if (NULL != *ppLog) {
    free(*ppLog);
    *ppLog = NULL;
  }
}

/*
** Method: cphLogPrintLn
**
** Write a specified message to the logging mechanism according to the
** specified verbosity levels
**
** Input Parameters: pLog - a pointer to the CPH_LOG control structure
**                   lovLevel - the verbosity level pertaining to this message
**                   aString - character string containing the contents of the log message
**
*/
void cphLogPrintLn(CPH_LOG *pLog, int logLevel, char const *string)
{
    CPHTRACEENTRY(pLog->pTrc)

    if ( logLevel==LOG_ERROR ) {
        pLog->errorsReported = CPHTRUE;
    }
    if ( logLevel <= pLog->verbosityToSTDERR ) {
        fprintf(stderr, "%s\n", string);
        fflush(stderr);
    } else if (logLevel<=pLog->verbosityToSTDOUT ) {
        printf("%s\n", string);
        fflush(stdout);
    }

    /* Write the log message to the trace file if we are tracing */
    CPHTRACEMSG(pLog->pTrc, "%s", string)

    CPHTRACEEXIT(pLog->pTrc)

}

/*
** Method: setVerbosity
**
** This function is called to set the stdout verbosity level. Log messages with a
** verbosity level less than or equal to this (but greater than the stderr verbosity
** level), will be written to stdout.
**
** Input Parameters: pLog - pointer to the CPH_LOG control block
**                   i - the new level for the stdout verbosity
**
*/
void setVerbosity(CPH_LOG *pLog, int i) {
   pLog->verbosityToSTDOUT = i;
}

/*
** Method: setErrVerbosity
**
** This function is called to set the stderr verbosity level. Log messages with a
** verbosity level less than or equal to this will be written to stderr and not to
** stdout.
**
** Input Parameters: pLog - pointer to the CPH_LOG control block
**                   i - the new level for the stderr verbosity
**
*/
void setErrVerbosity(CPH_LOG *pLog, int i) {
   pLog->verbosityToSTDERR = i;
}
