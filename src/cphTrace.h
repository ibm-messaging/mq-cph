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

#ifndef _CHPTRACE
#define _CHPTRACE

#include "cphdefs.h"

#define CPHTRACE_STARTINDENT 1

#include "cphArrayList.h"
#include "cphSpinLock.h"

#include <stdio.h>
#include "cphUtil.h"

/* These defines are used to enable or disable trace accordng to whether the preprocessor directive CPH_DOTRACE
   has been set */
#ifdef CPH_DOTRACE

#if defined(AMQ_NT)
  #define FUNCTION_NAME __FUNCTION__
#elif defined(__GNUC__)
  #define FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(SUNPRO)
  #define FUNCTION_NAME "UNKNOWN"
#else
  #define FUNCTION_NAME __func__
#endif


#define CPHTRACEINI(T)           cphTraceIni(T);
#define CPHTRACEFREE(T)          cphTraceFree(T);
#define CPHTRACEOPENTRACEFILE(T) cphTraceOpenTraceFile(T);
#define CPHTRACEON(T)            cphTraceOn(T);
#define CPHTRACEENTRY(T)         cphTraceEntry(T,FUNCTION_NAME);
#define CPHTRACEEXIT(T)          cphTraceExit(T,FUNCTION_NAME);
#define CPHTRACEMSG(T,...)       cphTraceMsg(T,FUNCTION_NAME,__VA_ARGS__);
#define CPHTRACELINE(T,Tr,L)     cphTraceLine(T,Tr,L);
#define CPHTRACEREF(V,T)         CPH_TRACE *V = T;
#else // #ifdef CPH_DOTRACE
#define CPHTRACEINI(T)
#define CPHTRACEFREE(T)
#define CPHTRACEOPENTRACEFILE(T)
#define CPHTRACEON(T)
#define CPHTRACEENTRY(T)
#define CPHTRACEEXIT(T)
#define CPHTRACEMSG(T,...)
#define CPHTRACELINE(T,Tr,L)
#define CPHTRACEREF(V,T)
#endif // #ifdef CPH_DOTRACE ... #else

/* The CPH_TRACETHREAD structure - we maintain a linked list of these, there is one CPH_TRACETHREAD slot for every thread registered
   to cph */
typedef struct _cphtracethread {
    cph_pthread_t threadId;
  int indent;
  char * buffer;
  size_t bufferLen;
} CPH_TRACETHREAD;

/* The CPH_TRACE structure - there is a single one of these for a cph process */
typedef struct _cphtrace {
    int traceOn;                 /* Flag used to stop and start tracing */
    char traceFileName[256];     /* The filename of the trace output file */
    FILE *tFp;                   /* File pointer for the trace file */
    CPH_ARRAYLIST *threadlist;   /* Pointer to the list of CPH_TRACETHREAD entries - there is one per cph thread */
    CPH_SPINLOCK *pTraceAddLock; /* Lock to be used when adding entries to the CPH_TRACETHREAD list */
} CPH_TRACE;

/* Function prototypes */
void cphTraceIni(CPH_TRACE** ppTrace);
int cphTraceWarning(int dummy);
int  cphTraceFree(CPH_TRACE** ppTrace);
int cphTraceOpenTraceFile(CPH_TRACE *pTrace);
int cphTraceOn(CPH_TRACE *pTrace);
int cphTraceOff(CPH_TRACE *pTrace);
int cphTraceIsOn(CPH_TRACE *pTrace);
void cphTraceEntry(CPH_TRACE *pTrace, char const * const aFunction);
void cphTraceExit(CPH_TRACE *pTrace, char const * const aFunction);
void cphTraceMsg(CPH_TRACE *pTrace, char const * const aFunction, char const * const aFmt, ...);
int cphTraceLine(CPH_TRACE *pTrace, CPH_TRACETHREAD *pTraceThread);
int cphTraceListThreads(CPH_TRACE *pTrace);
CPH_TRACETHREAD *cphTraceGetTraceThread(CPH_TRACE *pTrace, size_t const bufferLen);
CPH_TRACETHREAD *cphTraceLookupTraceThread(CPH_TRACE *pTrace, cph_pthread_t threadId);

#endif
