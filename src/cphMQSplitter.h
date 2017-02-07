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

#ifndef _CPHMQSPLITTER
#define _CPHMQSPLITTER

#include "cphLog.h"

#if defined(__cplusplus)
  extern "C" {
#endif

#if defined(WIN32)

#define ITS_WINDOWS

  #include <windows.h>
  #define EPTYPE HMODULE
  #ifndef MQEA
    typedef int (MQENTRY *MQFUNCPTR)();
  #else
    #define FUNCPTR FARPROC
    typedef int (MQENTRY *MQFUNCPTR)();
  #endif
   /* EstablishMQEpsX will load either the local or remote dll according to its input parameter is Client */
   #define MQLOCAL_NAME  "mqm.dll"
   #define MQREMOTE_NAME "mqic.dll"
   #define MQREMOTE_NAME_IS "mqdc.dll"      /* installation specific client dll */

/*********************************************************************/
/*   Typedefs                                                        */
/*********************************************************************/

typedef struct mq_epList
{
 MQFUNCPTR mqconn;
 MQFUNCPTR mqconnx;
 MQFUNCPTR mqdisc;
 MQFUNCPTR mqopen;
 MQFUNCPTR mqclose;
 MQFUNCPTR mqget;
 MQFUNCPTR mqput;
 MQFUNCPTR mqset;
 MQFUNCPTR mqput1;
 MQFUNCPTR mqback;
 MQFUNCPTR mqcmit;
 MQFUNCPTR mqinq;
#ifndef CPH_WMQV6
 MQFUNCPTR mqsub;
 MQFUNCPTR mqbufmh;
 MQFUNCPTR mqcrtmh;
 MQFUNCPTR mqdltmh;
#endif
} mq_epList;

typedef mq_epList * Pmq_epList;

#elif defined(CPH_UNIX)

#if defined(CPH_AIX)
  #define MQLOCAL_NAME "libmqm_r.a(libmqm_r.o)"  /* installation specific client dll */
  #define MQREMOTE_NAME "libmqic_r.a(mqic_r.o)"
  #define MQREMOTE_NAME_IS "libmqdc_r.a(mqdc_r.o)"
#elif defined(CPH_SOLARIS)
  #define MQLOCAL_NAME "libmqm.so"
  #define MQREMOTE_NAME "libmqic.so"
  #define MQREMOTE_NAME_IS "libmqdc.so"
#elif defined(CPH_HPUX)
  #define MQLOCAL_NAME "libmqm_r.so"
  #define MQREMOTE_NAME "libmqic_r.so"
  #define MQREMOTE_NAME_IS "libmqdc_r.so"
#elif defined(CPH_IBMI)
  #define MQLOCAL_NAME "LIBMQM_R  "
  #define MQREMOTE_NAME "LIBMQIC_R "
  #define MQREMOTE_NAME_IS "LIBMQDC_R "
#elif defined(CPH_OSX)
  #define MQLOCAL_NAME "libmqm_r.dylib"
  #define MQREMOTE_NAME "libmqic_r.dylib"
  #define MQREMOTE_NAME_IS "libmqdc_r.dylib"
#else
  #define MQLOCAL_NAME "libmqm_r.so"
  #define MQREMOTE_NAME "libmqic_r.so"
  #define MQREMOTE_NAME_IS "libmqdc_r.so"
#endif

#define EPTYPE void *
typedef int (MQENTRY *MQFUNCPTR)();

typedef struct mq_epList
{
 MQFUNCPTR mqconn;
 MQFUNCPTR mqconnx;
 MQFUNCPTR mqdisc;
 MQFUNCPTR mqopen;
 MQFUNCPTR mqclose;
 MQFUNCPTR mqget;
 MQFUNCPTR mqput;
 MQFUNCPTR mqset;
 MQFUNCPTR mqput1;
 MQFUNCPTR mqback;
 MQFUNCPTR mqcmit;
 MQFUNCPTR mqinq;
#ifndef CPH_WMQV6
 MQFUNCPTR mqsub;
 MQFUNCPTR mqbufmh;
 MQFUNCPTR mqcrtmh;
 MQFUNCPTR mqdltmh;
#endif
} mq_epList;

typedef mq_epList * Pmq_epList;


#endif

/* Function prototypes */
int cphMQSplitterCheckMQLoaded(CPH_LOG *pLog, int isClient);
long cphMQSplitterLoadMQ(CPH_LOG *pLog, Pmq_epList ep, int isClient);

#if defined(__cplusplus)
  }
#endif
#endif /* of _CPHMQSPLITTER */


