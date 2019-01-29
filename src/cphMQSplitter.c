/*<copyright notice="lm-source" pids="" years="2007,2019">*/
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

/* The functions in this module provide a thin layer above MQ to load either the
   server or client bindings library and to store pointers to the MQ API entry
   points in the structure ETM_dynamic_MQ_entries. The functions in this module are
   then called which call the real MQ API function via the function pointers.
   The MQ DLL must be explicitly loaded before any MQ calls are made via a call to
   cphMQSplitterCheckMQLoaded(), specifying on the call whether the
   client or server library is required.

   The code in this module is based on an old cat II SupportPac, which provided a
   Lotus Script interface to MQ, written by Stephen Todd.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef CPH_UNIX
   #include <sys/types.h>
   #include <errno.h>
   #if defined(AMQ_AS400)
     #include <qleawi.h>
     #include <qusec.h>
     #include <mih/rslvsp.h>
   #else
     #include <dlfcn.h>      /* dynamic load stuff */
   #endif
#endif
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * Now include MQ header files                                        *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <cmqc.h>

#include "cphMQSplitter.h"


/*********************************************************************/
/*   Global variables                                                */
/*********************************************************************/

mq_epList ETM_dynamic_MQ_entries;      /* the list of real entry points */

unsigned long ETM_DLL_found = 0;    /* 0 means dll not found*/
unsigned long ETM_Tried = 0;        /* 0 means haven't tried establish*/


/*********************************************************************/
/*   The Code                                                        */
/*********************************************************************/



/**********************************************************************************/
/*                                                                                */
/* We have all the standard entry points as copied from cmqc.h - these            */
/* directly pass the incoming call to the dynamic entry address obtained during   */
/* initialization.                                                                */
/*                                                                                */
/**********************************************************************************/



 /*********************************************************************/
 /*  MQBACK Function -- Back Out Changes                              */
 /*********************************************************************/

 void MQENTRY MQBACK (
   MQHCONN  Hconn,      /* Connection handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqback)
  {
    (ETM_dynamic_MQ_entries.mqback)(Hconn,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;
  }
}

 /*********************************************************************/
 /*  MQCLOSE Function -- Close Object                                 */
 /*********************************************************************/

 void MQENTRY MQCLOSE (
   MQHCONN  Hconn,      /* Connection handle */
   PMQHOBJ  pHobj,      /* Object handle */
   MQLONG   Options,    /* Options that control the action of MQCLOSE */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqclose )
  {
    (ETM_dynamic_MQ_entries.mqclose)(Hconn,pHobj,Options,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQCMIT Function -- Commit Changes                                */
 /*********************************************************************/

 void MQENTRY MQCMIT (
   MQHCONN  Hconn,      /* Connection handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqcmit )
  {
    (ETM_dynamic_MQ_entries.mqcmit)(Hconn,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQCONN Function -- Connect Queue Manager                         */
 /*********************************************************************/

 void MQENTRY MQCONN (
   PMQCHAR   pName,      /* Name of queue manager */
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqconn )
  {
     (ETM_dynamic_MQ_entries.mqconn)(pName,pHconn,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;
  }
}

 /*********************************************************************/
 /*  MQCONNX Function -- Connect Queue Manager                         */
 /*  Note: Extra function here compared to rest of calls - this allows*/
 /*        retry aftre 6000 without bring notes down                  */
 /*********************************************************************/

 void MQENTRY MQCONNX (
   PMQCHAR   pName,      /* Name of queue manager */
   PMQCNO    pConnectOpts, /* Connect options */
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqconnx )
  {
     (ETM_dynamic_MQ_entries.mqconnx)(pName,pConnectOpts,pHconn,pCompCode,pReason);

  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;
  }
}

 /*********************************************************************/
 /*  MQDISC Function -- Disconnect Queue Manager                      */
 /*********************************************************************/

 void MQENTRY MQDISC (
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason)    /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqdisc)
  {
    (ETM_dynamic_MQ_entries.mqdisc)(pHconn,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQGET Function -- Get Message                                    */
 /*********************************************************************/

 void MQENTRY MQGET (
   MQHCONN  Hconn,         /* Connection handle */
   MQHOBJ   Hobj,          /* Object handle */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pGetMsgOpts,   /* Options that control the action of
                              MQGET */
   MQLONG   BufferLength,  /* Length in bytes of the Buffer area */
   PMQVOID  pBuffer,       /* Area to contain the message data */
   PMQLONG  pDataLength,   /* Length of the message */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason)       /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqget)
  {
    (ETM_dynamic_MQ_entries.mqget)(Hconn,Hobj,pMsgDesc,pGetMsgOpts,BufferLength,
                                 pBuffer,pDataLength,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQINQ Function -- Inquire Object Attributes                      */
 /*********************************************************************/

 void MQENTRY MQINQ (
   MQHCONN  Hconn,           /* Connection handle */
   MQHOBJ   Hobj,            /* Object handle */
   MQLONG   SelectorCount,   /* Count of selectors */
   PMQLONG  pSelectors,      /* Array of attribute selectors */
   MQLONG   IntAttrCount,    /* Count of integer attributes */
   PMQLONG  pIntAttrs,       /* Array of integer attributes */
   MQLONG   CharAttrLength,  /* Length of character attributes buffer */
   PMQCHAR  pCharAttrs,      /* Character attributes */
   PMQLONG  pCompCode,       /* Completion code */
   PMQLONG  pReason)         /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqinq)
  {
    (ETM_dynamic_MQ_entries.mqinq)(Hconn,Hobj,SelectorCount,pSelectors,IntAttrCount,
                                 pIntAttrs,CharAttrLength,pCharAttrs,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }

}


 /*********************************************************************/
 /*  MQOPEN Function -- Open Object                                   */
 /*********************************************************************/

 void MQENTRY MQOPEN (
   MQHCONN  Hconn,      /* Connection handle */
   PMQVOID  pObjDesc,   /* Object descriptor */
   MQLONG   Options,    /* Options that control the action of MQOPEN */
   PMQHOBJ  pHobj,      /* Object handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason)    /* Reason code qualifying CompCode */
{

  /*  ------- Changes for defect 22444 end here -----  */

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqopen)
  {
    (ETM_dynamic_MQ_entries.mqopen)(Hconn,pObjDesc,Options,pHobj,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }

}


 /*********************************************************************/
 /*  MQPUT Function -- Put Message                                    */
 /*********************************************************************/

 void MQENTRY MQPUT (
   MQHCONN  Hconn,         /* Connection handle */
   MQHOBJ   Hobj,          /* Object handle */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pPutMsgOpts,   /* Options that control the action of
                              MQPUT */
   MQLONG   BufferLength,  /* Length of the message in Buffer */
   PMQVOID  pBuffer,       /* Message data */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason)       /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqput)
  {
    (ETM_dynamic_MQ_entries.mqput)(Hconn,Hobj,pMsgDesc,pPutMsgOpts,BufferLength,
                                 pBuffer,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }

}


 /*********************************************************************/
 /*  MQPUT1 Function -- Put One Message                               */
 /*********************************************************************/

 void MQENTRY MQPUT1 (
   MQHCONN  Hconn,         /* Connection handle */
   PMQVOID  pObjDesc,      /* Object descriptor */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pPutMsgOpts,   /* Options that control the action of
                              MQPUT1 */
   MQLONG   BufferLength,  /* Length of the message in Buffer */
   PMQVOID  pBuffer,       /* Message data */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason)       /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqput1)
  {
    (ETM_dynamic_MQ_entries.mqput1)(Hconn,pObjDesc,pMsgDesc,pPutMsgOpts,BufferLength,
                                 pBuffer,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
//    *pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /*********************************************************************/
 /*  MQSET Function -- Set Object Attributes                          */
 /*********************************************************************/

 void MQENTRY MQSET (
   MQHCONN  Hconn,           /* Connection handle */
   MQHOBJ   Hobj,            /* Object handle */
   MQLONG   SelectorCount,   /* Count of selectors */
   PMQLONG  pSelectors,      /* Array of attribute selectors */
   MQLONG   IntAttrCount,    /* Count of integer attributes */
   PMQLONG  pIntAttrs,       /* Array of integer attributes */
   MQLONG   CharAttrLength,  /* Length of character attributes buffer */
   PMQCHAR  pCharAttrs,      /* Character attributes */
   PMQLONG  pCompCode,       /* Completion code */
   PMQLONG  pReason)         /* Reason code qualifying CompCode */
{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqset)
  {
    (ETM_dynamic_MQ_entries.mqset)(Hconn,Hobj,SelectorCount,pSelectors,IntAttrCount,
                                 pIntAttrs,CharAttrLength,pCharAttrs,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}




 /****************************************************************/
 /*  MQBUFMH Function -- Buffer To Message Handle                */
 /****************************************************************/


 void MQENTRY MQBUFMH (
   MQHCONN   Hconn,         /* I: Connection handle */
   MQHMSG    Hmsg,          /* I: Message handle */
   PMQVOID   pBufMsgHOpts,  /* I: Options that control the action of
                               MQBUFMH */
   PMQVOID   pMsgDesc,      /* IO: Message descriptor */
   MQLONG    BufferLength,  /* IL: Length in bytes of the Buffer area */
   PMQVOID   pBuffer,       /* OB: Area to contain the message buffer */
   PMQLONG   pDataLength,   /* O: Length of the output buffer */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason)       /* OR: Reason code qualifying CompCode */

{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqbufmh)
  {
    (ETM_dynamic_MQ_entries.mqbufmh)(Hconn,Hmsg,pBufMsgHOpts,pMsgDesc,BufferLength,pBuffer,pDataLength,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}

 /****************************************************************/
 /*  MQCRTMH Function -- Create Message Handle                   */
 /****************************************************************/


 void MQENTRY MQCRTMH (
   MQHCONN   Hconn,         /* I: Connection handle */
   PMQVOID   pCrtMsgHOpts,  /* IO: Options that control the action of
                               MQCRTMH */
   PMQHMSG   pHmsg,         /* IO: Message handle */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason)       /* OR: Reason code qualifying CompCode */

{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqcrtmh)
  {
    (ETM_dynamic_MQ_entries.mqcrtmh)(Hconn,pCrtMsgHOpts,pHmsg,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}


 /****************************************************************/
 /*  MQDLTMH Function -- Delete Message Handle                   */
 /****************************************************************/


 void MQENTRY MQDLTMH (
   MQHCONN   Hconn,         /* I: Connection handle */
   PMQHMSG   pHmsg,         /* IO: Message handle */
   PMQVOID   pDltMsgHOpts,  /* I: Options that control the action of
                               MQDLTMH */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason)       /* OR: Reason code qualifying CompCode */

{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqdltmh)
  {
    (ETM_dynamic_MQ_entries.mqdltmh)(Hconn,pHmsg,pDltMsgHOpts,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}



 /*********************************************************************/
 /*  MQSUBSCRIBE Function -- Subscribe to a given destination         */
 /*********************************************************************/

 void MQENTRY MQSUB (

   MQHCONN  Hconn,      /* I: Connection handle */
   PMQVOID  pSubDesc,   /* IO: Subscription descriptor */
   PMQHOBJ  pHobj,      /* IO: Object handle for queue */
   PMQHOBJ  pHsub,      /* O: Subscription object handle */
   PMQLONG  pCompCode,  /* OC: Completion code */
   PMQLONG  pReason)    /* OR: Reason code qualifying CompCode */

{

  if (ETM_DLL_found && ETM_dynamic_MQ_entries.mqsub)
  {
    (ETM_dynamic_MQ_entries.mqsub)(Hconn,pSubDesc,pHobj,pHsub,pCompCode,pReason);
  }
  else
  {
    *pCompCode = MQCC_FAILED;
    //*pReason   = MQRC_LIBRARY_LOAD_ERROR;
    *pReason   = 6000;

  }
}

/*
** Method: cphMQSplitterCheckMQLoaded
**
** This method is called (by cphConnectionParseArgs) to check whether a previous attempt has been made to load the client or server
** MQM dll/shared library. If no attempt has been made, cphMQSplitterLoadMQ is called which will then try to open it and store the
** entry points for the MQ API functions in the ETM_dynamic_MQ_entries structure.
**
** Input Parameters: pLog - pointer to the CPH_LOG structure
**                   isClient - flag to denote whether the client or server library should be loaded. If set to zero, the server
**                   library is loaded. If set to any other value the client library is loaded.
**
** Returns: 0 on successful execution, -1 otherwise
**
*/
int cphMQSplitterCheckMQLoaded(CPH_LOG *pLog, int isClient) {

  /* If this is the first time this method has been called then attempt to load the client/server DLL/shared lib */
  if (ETM_Tried==0) {
   ETM_DLL_found = cphMQSplitterLoadMQ(pLog, &ETM_dynamic_MQ_entries, isClient);
   ETM_Tried = 1;
  }

  if (!ETM_DLL_found)  return(-1);
  return(0);
 }


#if defined(WIN32)

/*
** Method: cphMQSplitterLoadMQ
**
** Attempts to load the client or server MQM DLL/shared library. LoadLibraryA is used on Windows to open the DLL and GetProcAddress
** to retrieve the individual entry points. On Linux, dlopen is used to open the shared library and dlsym to retrieve the entry
** points.
**
** Input Parameters: pLog - pointer to the CPH_LOG structure
**                   isClient - flag to denote whether the client or server library should be loaded. If set to zero, the server
**                   library is loaded. If set to any other value the client library is loaded.
**
** Returns: TRUE if the DLL loaded successfully and FALSE otherwise
**
*/
long cphMQSplitterLoadMQ(CPH_LOG *pLog, Pmq_epList ep, int isClient)
{
  EPTYPE    pLibrary=0L;
  long      rc = FALSE;         /* note that FALSE = FAILURE */
  char      DLL_name[80];
  char      msg[200];
  DWORD     error=0;

  /* first we looksee if user is telling us to load a specific library */
  if (0 == isClient)
  {
     strcpy(DLL_name, MQLOCAL_NAME);
  }
  else
  {
     char *pEnvStr = getenv("CPH_USE_INSTALLATION_SPECIFIC_CLIENT_LIBRARY");  /* get ptr to environment string */

     if (pEnvStr != NULL)
        strcpy(DLL_name, MQREMOTE_NAME_IS);
     else
        strcpy(DLL_name, MQREMOTE_NAME);
  } /* endif */

 #if defined (ITS_WINDOWS)

  pLibrary = LoadLibraryA(DLL_name);

#endif

  if (pLibrary)   /* did we find a DLL? */
  {

    /* tell the world what we found */
    sprintf(msg, "DLL %s loaded ok", DLL_name);
    cphLogPrintLn(pLog, LOG_INFO, msg);

   /**********************************************************/
   /* For the present we will assume that having found a dll */
   /* it will have the things we need !!!!                   */
   /* If it hasn't then code in gmqdyn0a.c will raise error  */
   /**********************************************************/

    ep->mqconn   = (MQCONNPTR)  GetProcAddress(pLibrary,"MQCONN");
    ep->mqconnx  = (MQCONNXPTR) GetProcAddress(pLibrary,"MQCONNX");
    ep->mqput    = (MQPUTPTR)   GetProcAddress(pLibrary,"MQPUT");
    ep->mqget    = (MQGETPTR)   GetProcAddress(pLibrary,"MQGET");
    ep->mqopen   = (MQOPENPTR)  GetProcAddress(pLibrary,"MQOPEN");
    ep->mqclose  = (MQCLOSEPTR) GetProcAddress(pLibrary,"MQCLOSE");
    ep->mqdisc   = (MQDISCPTR)  GetProcAddress(pLibrary,"MQDISC");
    ep->mqset    = (MQSETPTR)   GetProcAddress(pLibrary,"MQSET");
    ep->mqput1   = (MQPUT1PTR)  GetProcAddress(pLibrary,"MQPUT1");
    ep->mqback   = (MQBACKPTR)  GetProcAddress(pLibrary,"MQBACK");
    ep->mqcmit   = (MQCMITPTR)  GetProcAddress(pLibrary,"MQCMIT");
    ep->mqinq    = (MQINQPTR)   GetProcAddress(pLibrary,"MQINQ");
#ifndef CPH_WMQV6
    ep->mqsub    = (MQSUBPTR)   GetProcAddress(pLibrary,"MQSUB");
    ep->mqbufmh  = (MQBUFMHPTR) GetProcAddress(pLibrary,"MQBUFMH");
    ep->mqcrtmh  = (MQCRTMHPTR) GetProcAddress(pLibrary,"MQCRTMH");
    ep->mqdltmh  = (MQDLTMHPTR) GetProcAddress(pLibrary,"MQDLTMH");
#endif

   rc = TRUE;     /* TRUE means it worked - dll was found */
  }
  else
  {
    /* couldn't find a dll !!! */

    #if defined(ITS_WINDOWS)
     #ifdef AMQ_NT    /* Win16 doesn't have GetLastError */
     error = GetLastError();
     #endif
    #endif

    sprintf(msg, "Cannot find lib - %s (error = %u)", DLL_name, error);
    cphLogPrintLn(pLog, LOG_ERROR, msg);

    rc = FALSE;
  }

return rc;
}

#elif defined(AMQ_AS400)

long cphMQSplitterLoadMQ(CPH_LOG *pLog, Pmq_epList ep, int isClient)
{
  long rc = 0;
  _SYSPTR handle = NULL;
  char      DLL_name[80];

  /* first we looksee if user is telling us to load a specific library */
  if (0 == isClient)
  {
     strcpy(DLL_name, MQLOCAL_NAME);
  }
  else
  {
     char *pEnvStr = getenv("CPH_USE_INSTALLATION_SPECIFIC_CLIENT_LIBRARY");  /* get ptr to environment string */

     if (pEnvStr != NULL)
        strcpy(DLL_name, MQREMOTE_NAME_IS);
     else
        strcpy(DLL_name, MQREMOTE_NAME);
  } /* endif */

  handle = rslvsp(WLI_SRVPGM,
                  DLL_name,
                  "QMQM      ",
                  _AUTH_NONE);

  if ( handle == NULL )
  {
    char buff[200];
    /* lets try and tell the world what happened */
    sprintf(buff, "Failed trying to load %s", DLL_name);
    cphLogPrintLn(pLog, LOG_ERROR, buff);
  }
  else
  {
   /**********************************************************/
   /* For the present we will assume that having found a dll */
   /* it will have the things we need !!!!                   */
   /**********************************************************/
   int exptype, explen;
   Qus_EC_t errorinfo;
   unsigned long long  actmark;
   Qle_ABP_Info_Long_t actinfo;
   int actinfolen = sizeof(Qle_ABP_Info_t);

   errorinfo.Bytes_Provided = sizeof(Qus_EC_t);
   errorinfo.Bytes_Available = 0;

   QleActBndPgmLong(&handle, &actmark, &actinfo, &actinfolen, &errorinfo);

   explen = strlen("MQCONN"); QleGetExpLong(&actmark, 0, &explen, "MQCONN", &ep->mqconn, &exptype, &errorinfo);
   explen = strlen("MQCONNX"); QleGetExpLong(&actmark, 0, &explen, "MQCONNX", &ep->mqconnx, &exptype, &errorinfo);
   explen = strlen("MQDISC"); QleGetExpLong(&actmark, 0, &explen, "MQDISC", &ep->mqdisc, &exptype, &errorinfo);
   explen = strlen("MQOPEN"); QleGetExpLong(&actmark, 0, &explen, "MQOPEN", &ep->mqopen, &exptype, &errorinfo);
   explen = strlen("MQCLOSE"); QleGetExpLong(&actmark, 0, &explen, "MQCLOSE", &ep->mqclose, &exptype, &errorinfo);
   explen = strlen("MQGET"); QleGetExpLong(&actmark, 0, &explen, "MQGET", &ep->mqget, &exptype, &errorinfo);
   explen = strlen("MQPUT"); QleGetExpLong(&actmark, 0, &explen, "MQPUT", &ep->mqput, &exptype, &errorinfo);
   explen = strlen("MQSET"); QleGetExpLong(&actmark, 0, &explen, "MQSET", &ep->mqset, &exptype, &errorinfo);
   explen = strlen("MQPUT1"); QleGetExpLong(&actmark, 0, &explen, "MQPUT1", &ep->mqput1, &exptype, &errorinfo);
   explen = strlen("MQBACK"); QleGetExpLong(&actmark, 0, &explen, "MQBACK", &ep->mqback, &exptype, &errorinfo);
   explen = strlen("MQCMIT"); QleGetExpLong(&actmark, 0, &explen, "MQCMIT", &ep->mqcmit, &exptype, &errorinfo);
   explen = strlen("MQINQ"); QleGetExpLong(&actmark, 0, &explen, "MQINQ", &ep->mqinq, &exptype, &errorinfo);
#ifndef CPH_WMQV6
   explen = strlen("MQINQ"); QleGetExpLong(&actmark, 0, &explen, "MQINQ", &ep->mqsub, &exptype, &errorinfo);
   explen = strlen("MQINQ"); QleGetExpLong(&actmark, 0, &explen, "MQINQ", &ep->mqbufmh, &exptype, &errorinfo);
   explen = strlen("MQINQ"); QleGetExpLong(&actmark, 0, &explen, "MQINQ", &ep->mqcrtmh, &exptype, &errorinfo);
   explen = strlen("MQINQ"); QleGetExpLong(&actmark, 0, &explen, "MQINQ", &ep->mqdltmh, &exptype, &errorinfo);
#endif
       rc = 1; /* to show it worked */
  }

  return rc;
}

#elif defined(CPH_UNIX)


long cphMQSplitterLoadMQ(CPH_LOG *pLog, Pmq_epList ep, int isClient)
{
  EPTYPE    pLibrary=0L;
  long      rc = 0;  /* 0 means that we failed */
  char      DLL_name[80];
  char     *ErrorText;     /* Text of error from dlxxx()           */
  char buff[200];

  /* first we looksee if user is telling us to load a specific library */
  if (0 == isClient)
  {
     strcpy(DLL_name, MQLOCAL_NAME);
  }
  else
  {
     char *pEnvStr = getenv("CPH_USE_INSTALLATION_SPECIFIC_CLIENT_LIBRARY");  /* get ptr to environment string */

     if (pEnvStr != NULL)
        strcpy(DLL_name, MQREMOTE_NAME_IS);
     else
        strcpy(DLL_name, MQREMOTE_NAME);
  } /* endif */

  pLibrary = (void *)dlopen(DLL_name,RTLD_NOW
#if defined(_AIX)
                             | RTLD_MEMBER
#endif
                             );
  if (!pLibrary) ErrorText = dlerror();

  if ( pLibrary == NULL )
  {
     /* lets try and tell the world what happened */
      sprintf(buff, "On trying to load %s %s", DLL_name, " ........");
      cphLogPrintLn(pLog, LOG_ERROR, buff);
      cphLogPrintLn(pLog, LOG_ERROR, ErrorText);
  }

  if (pLibrary)   /* did we find a DLL? */
  {

   {  /* tell the world what we found */

    sprintf(buff, "Shared library %s loaded ok", DLL_name);
    cphLogPrintLn(pLog, LOG_INFO, buff);
   }

   /**********************************************************/
   /* For the present we will assume that having found a dll */
   /* it will have the things we need !!!!                   */
   /**********************************************************/

       ep->mqconn   = (MQCONNPTR)   dlsym(pLibrary, "MQCONN");
       ep->mqconnx  = (MQCONNXPTR)  dlsym(pLibrary, "MQCONNX");
       ep->mqdisc   = (MQDISCPTR)   dlsym(pLibrary, "MQDISC");
       ep->mqopen   = (MQOPENPTR)   dlsym(pLibrary, "MQOPEN");
       ep->mqclose  = (MQCLOSEPTR)  dlsym(pLibrary, "MQCLOSE");
       ep->mqget    = (MQGETPTR)    dlsym(pLibrary, "MQGET");
       ep->mqput    = (MQPUTPTR)    dlsym(pLibrary, "MQPUT");
       ep->mqset    = (MQSETPTR)    dlsym(pLibrary, "MQSET");
       ep->mqput1   = (MQPUT1PTR)   dlsym(pLibrary, "MQPUT1");
       ep->mqback   = (MQBACKPTR)   dlsym(pLibrary, "MQBACK");
       ep->mqcmit   = (MQCMITPTR)   dlsym(pLibrary, "MQCMIT");
       ep->mqinq    = (MQINQPTR)    dlsym(pLibrary, "MQINQ");
#ifndef CPH_WMQV6
       ep->mqsub    = (MQSUBPTR)    dlsym(pLibrary,"MQSUB");
       ep->mqbufmh  = (MQBUFMHPTR)  dlsym(pLibrary,"MQBUFMH");
       ep->mqcrtmh  = (MQCRTMHPTR)  dlsym(pLibrary,"MQCRTMH");
       ep->mqdltmh  = (MQDLTMHPTR)  dlsym(pLibrary,"MQDLTMH");
#endif
       rc = 1; /* to show it worked */
  }
  else
  {
    /* couldn't find a dll !!! */
    sprintf(buff, "Cannot find lib - %s!", DLL_name);
    cphLogPrintLn(pLog, LOG_ERROR, buff);
  }

  return rc;
}

#endif
