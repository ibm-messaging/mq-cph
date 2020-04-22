/*<copyright notice="lm-source" pids="" years="2007,2019">*/
/*******************************************************************************
 * Copyright (c) 2007,2019 IBM Corp.
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

typedef void (MQENTRY *MQCONNPTR) (
  PMQCHAR   pQMgrName,
  PMQHCONN  pHconn,
  PMQLONG   pCompCode,
  PMQLONG   pReason);

typedef void (MQENTRY *MQCONNXPTR) (
  PMQCHAR   pQMgrName,
  PMQCNO    pConnectOpts,
  PMQHCONN  pHconn,
  PMQLONG   pCompCode,
  PMQLONG   pReason);

typedef void (MQENTRY *MQDISCPTR) (
  PMQHCONN  pHconn,
  PMQLONG   pCompCode,
  PMQLONG   pReason);

typedef void (MQENTRY *MQOPENPTR) (
  MQHCONN  Hconn,
  PMQVOID  pObjDesc,
  MQLONG   Options,
  PMQHOBJ  pHobj,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQCLOSEPTR) (
  MQHCONN  Hconn,
  PMQHOBJ  pHobj,
  MQLONG   Options,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQGETPTR) (
  MQHCONN  Hconn,
  MQHOBJ   Hobj,
  PMQVOID  pMsgDesc,
  PMQVOID  pGetMsgOpts,
  MQLONG   BufferLength,
  PMQVOID  pBuffer,
  PMQLONG  pDataLength,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQPUTPTR) (
  MQHCONN  Hconn,
  MQHOBJ   Hobj,
  PMQVOID  pMsgDesc,
  PMQVOID  pPutMsgOpts,
  MQLONG   BufferLength,
  PMQVOID  pBuffer,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQSETPTR) (
  MQHCONN  Hconn,
  MQHOBJ   Hobj,
  MQLONG   SelectorCount,
  PMQLONG  pSelectors,
  MQLONG   IntAttrCount,
  PMQLONG  pIntAttrs,
  MQLONG   CharAttrLength,
  PMQCHAR  pCharAttrs,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQPUT1PTR) (
  MQHCONN  Hconn,
  PMQVOID  pObjDesc,
  PMQVOID  pMsgDesc,
  PMQVOID  pPutMsgOpts,
  MQLONG   BufferLength,
  PMQVOID  pBuffer,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQBACKPTR) (
  MQHCONN  Hconn,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQCMITPTR) (
  MQHCONN  Hconn,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQINQPTR) (
  MQHCONN  Hconn,
  MQHOBJ   Hobj,
  MQLONG   SelectorCount,
  PMQLONG  pSelectors,
  MQLONG   IntAttrCount,
  PMQLONG  pIntAttrs,
  MQLONG   CharAttrLength,

  PMQCHAR  pCharAttrs,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

#ifndef CPH_WMQV6
typedef void (MQENTRY *MQSUBPTR) (
  MQHCONN  Hconn,
  PMQVOID  pSubDesc,
  PMQHOBJ  pHobj,
  PMQHOBJ  pHsub,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQBUFMHPTR) (
  MQHCONN  Hconn,
  MQHMSG   Hmsg,
  PMQVOID  pBufMsgHOpts,
  PMQVOID  pMsgDesc,
  MQLONG   BufferLength,
  PMQVOID  pBuffer,
  PMQLONG  pDataLength,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQCRTMHPTR) (
  MQHCONN  Hconn,
  PMQVOID  pCrtMsgHOpts,
  PMQHMSG  pHmsg,
  PMQLONG  pCompCode,
  PMQLONG  pReason);

typedef void (MQENTRY *MQDLTMHPTR) (
  MQHCONN  Hconn,
  PMQHMSG  pHmsg,
  PMQVOID  pDltMsgHOpts,
  PMQLONG  pCompCode,
  PMQLONG  pReason);
#endif

#if defined(WIN32)

#define ITS_WINDOWS

  #include <windows.h>
  #define EPTYPE HMODULE
  #ifdef MQEA
    #define FUNCPTR FARPROC
  #endif
   /* EstablishMQEpsX will load either the local or remote dll according to its input parameter is Client */
   #define MQLOCAL_NAME  "mqm.dll"
   #define MQREMOTE_NAME "mqic.dll"
   #define MQREMOTE_NAME_IS "mqdc.dll"      /* installation specific client dll */

#elif defined(CPH_UNIX)

#if defined(CPH_AIX)
  #define MQLOCAL_NAME "libmqm_r.a(libmqm_r.o)"  /* installation specific client dll */
  #define MQREMOTE_NAME "libmqic_r.a(mqic_r.o)"
  #define MQREMOTE_NAME_IS "libmqdc_r.a(mqdc_r.o)"
#elif defined(CPH_SOLARIS)
  #define MQLOCAL_NAME "libmqm.so"
  #define MQREMOTE_NAME "libmqic.so"
  #define MQREMOTE_NAME_IS "libmqdc.so"
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

#endif

/*********************************************************************/
/*   Typedefs                                                        */
/*********************************************************************/

typedef struct mq_epList
{
 MQCONNPTR  mqconn;
 MQCONNXPTR mqconnx;
 MQDISCPTR  mqdisc;
 MQOPENPTR  mqopen;
 MQCLOSEPTR mqclose;
 MQGETPTR   mqget;
 MQPUTPTR   mqput;
 MQSETPTR   mqset;
 MQPUT1PTR  mqput1;
 MQBACKPTR  mqback;
 MQCMITPTR  mqcmit;
 MQINQPTR   mqinq;
#ifndef CPH_WMQV6
 MQSUBPTR   mqsub;
 MQBUFMHPTR mqbufmh;
 MQCRTMHPTR mqcrtmh;
 MQDLTMHPTR mqdltmh;
#endif
} mq_epList;

typedef mq_epList * Pmq_epList;

/* Function prototypes */
int cphMQSplitterCheckMQLoaded(CPH_LOG *pLog, int isClient);
long cphMQSplitterLoadMQ(CPH_LOG *pLog, Pmq_epList ep, int isClient);

#if defined(__cplusplus)
  }
#endif
#endif /* of _CPHMQSPLITTER */


