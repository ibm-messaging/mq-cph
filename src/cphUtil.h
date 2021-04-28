/*<copyright notice="lm-source" pids="" years="2007,2021">*/
/*******************************************************************************
 * Copyright (c) 2007,2021 IBM Corp.
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

#ifndef _CPHUTIL
#define _CPHUTIL
#if defined(__cplusplus)
   extern "C" {
#endif

/* CPHTRUE and CPHFALSE are used everywhere as an explicit TRUE/FALSE check */
#define CPHTRUE  1
#define CPHFALSE 0

/* This definition is used by the subscriber to indicate no message was received by cphSubsriberOneIteration in
   the wait interval */
#define CPH_RXTIMEDOUT -1

/* Define the CPH_TIME structure - a machine independent wrapper for machine dependent time values */
#if defined(WIN32)  || defined(WIN64)
#include <Windows.h>
#define CPH_TIME LARGE_INTEGER
#elif defined(AMQ_AS400) || defined(AMQ_MACOS)
#include <sys/time.h>
#define CPH_TIME struct timeval
#else
#include <time.h>
#define CPH_TIME struct timespec
#endif

#include <signal.h>
#include <stdlib.h>
#include <cmqc.h>

#if defined(WIN32)
  #define cph_pthread_t int
#elif defined(AMQ_AS400)
  #define cph_pthread_t int
#elif defined(AMQ_MACOS)
  #define cph_pthread_t uint64_t
#elif defined(CPH_UNIX)
  #define cph_pthread_t pthread_t
#else
  #error "Don't know how to define pthread_t on this platform"
#endif

/* Function prototypes */
void cphUtilSleep( int mSecs );
CPH_TIME cphUtilGetNow(void);
int cphUtilTimeIni(CPH_TIME *pTime);
long cphUtilGetTimeDifference(CPH_TIME time1, CPH_TIME time2);
long cphUtilGetUsTimeDifference(CPH_TIME time1, CPH_TIME time2);
int cphUtilTimeCompare(CPH_TIME time1, CPH_TIME time2);
void cphCopyTime(CPH_TIME *pTimeDst, CPH_TIME *pTimeSrc);
int cphUtilTimeIsZero(CPH_TIME pTime);
cph_pthread_t cphUtilGetThreadId(void);
int cphUtilGetProcessId(void);
void cphUtilSigInt(int dummysignum);
int cphUtilGetTraceTime(char *chTime);
char *cphUtilRTrim(char *aLine);
char *cphUtilLTrim(char *aLine);
char *cphUtilTrim(char *aLine);
int cphUtilStringEndsWith(char *aString, char *ending);
char *cphUtilstrcrlf(char *aString);
char *cphUtilstrcrlfTotabcrlf(char *aString);
char *cphUtilMakeBigString(int size, int randomise);
char *cphUtilMakeBigStringWithRFH2(int size, size_t *pRfh2Len);
char *cphBuildRFH2(MQLONG *pSize);
int cphGetEnv(char *varName, char *varValue, size_t buffSize);
double cphUtilGetDoubleDuration(CPH_TIME start, CPH_TIME end);

#define CPH_SLEEP_GRANULARITY 1000

#if defined(__cplusplus)
   }
#endif
#endif
