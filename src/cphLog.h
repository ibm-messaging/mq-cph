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

#ifndef _CPHLOG
#define _CPHLOG
#if defined(__cplusplus)
  extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#include "cphUtil.h"
#include "cphTrace.h"

/* Definitions of the various log levels which cph messages can be assigned to */
#define LOG_NONE    0
#define LOG_ERROR   1
#define LOG_WARNING 2
#define LOG_INFO    3
#define LOG_VERBOSE 4
#define LOG_ALL     4

/* The CPH_LOG control structure */
typedef struct s_cphLog {

    int verbosityToSTDOUT;    /* anything less than this goes to stdout */
    int verbosityToSTDERR;    /* anything less than this goes to stderr */
  int errorsReported;       /* Set to CPH_TRUE if any message is written to stderr */
    CPH_TRACE *pTrc;          /* We keep a pointer to the trace control block so the log facility can trace log lines */

} CPH_LOG;

/* Function prototypes */
int cphLogIni(CPH_LOG **ppLog, CPH_TRACE *pTrc);
void cphLogPrintLn(CPH_LOG *pLog, int logLevel, char const *string);
void cphLogFree(CPH_LOG **ppLog);
void setVerbosity(CPH_LOG *pLog, int i);
void setErrVerbosity(CPH_LOG *pLog, int i);

#if defined(__cplusplus)
  }
#endif
#endif
