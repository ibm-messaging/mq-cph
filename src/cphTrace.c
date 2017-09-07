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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef _MSC_VER
#include "msio.h"
#endif
#include "cphTrace.h"
#include "cphListIterator.h"

/*
** Method: cphTraceIni
**
** This function creates and initialises the cph trace control block. This is one such trace control block in a running
** cph process. The control block contains a list of CPH_TRACETHREAD * pointers which are used to maintain the thread
** specific trace data. The trace control block contains a CPH_SPINLOCK * pointer which is used when adding new threads
** to the list of thread specific data.
**
** Input Parameters: ppTrace - double pointer to the newly created control block
**
*/
void cphTraceIni(CPH_TRACE** ppTrace) {
    CPH_TRACE *pTrace = NULL;
    CPH_SPINLOCK *pTraceAddLock;
    int status = CPHTRUE;

    /* Give a warning that we are running in trace mode */
    printf("\n%s%s%s%s%s",
           "***************************************************************************\n",
           "* WARNING - this is a trace build of cph. There is a performance penalty  *\n",
           "*           running with built in trace. Rebuild cph without the          *\n",
           "*           CPH_DOTRACE preprocessor definition to remove built in trace. *\n",
           "***************************************************************************\n\n");


    if (CPHFALSE == cphSpinLock_Init( &(pTraceAddLock))) {
        //strcpy(errorString, "Cannot create trace add lock");
        //cphConfigInvalidParameter(pConfig, "Cannot create trace add lock");
        printf("ERROR - cannot create trace add lock");
        status = CPHFALSE;
    }

    if ((CPHTRUE == status) && (NULL != (pTrace = malloc(sizeof(CPH_TRACE))))) {
        pTrace->traceOn = CPHFALSE;
        cphArrayListIni(&(pTrace->threadlist));

        pTrace->pTraceAddLock = pTraceAddLock;

        pTrace->traceFileName[0] = '\0';
        pTrace->tFp = NULL;
    }
    *ppTrace = pTrace;
}

/*
** Method: cphTraceOpenTraceFile
**
** This function opens the trace file - we do not do this at the time of initialising the trace
** structure as at that time we have not read the configuration parameters to know whether
** trace has been requested.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceOpenTraceFile(CPH_TRACE *pTrace) {
    int status = CPHTRUE;

    sprintf(pTrace->traceFileName, "cph_%d.txt", cphUtilGetProcessId());
    if (NULL == (pTrace->tFp = fopen(pTrace->traceFileName, "wt"))) {
        printf("ERROR - cannot open trace file.\n");
        status = CPHFALSE;
    }

    return(status);
}

/*
** Method: cphTraceFree
**
** This method frees up the trace control block
**
** Input Parameters: ppTrace - double pointer to the trace control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphTraceFree(CPH_TRACE** ppTrace) {
    CPH_LISTITERATOR *pIterator;

    if (NULL != *ppTrace) {
        if (NULL != (*ppTrace)->tFp) {
            fclose((*ppTrace)->tFp);
        }
        cphSpinLock_Destroy( &((*ppTrace)->pTraceAddLock));

        pIterator = cphArrayListListIterator((*ppTrace)->threadlist);
        while(cphListIteratorHasNext(pIterator)){
          CPH_ARRAYLISTITEM *pItem = cphListIteratorNext(pIterator);
          CPH_TRACETHREAD *pTraceThread = pItem->item;
          free(pTraceThread->buffer);
        }
        cphListIteratorFree(&pIterator);

        cphArrayListFree(&(*ppTrace)->threadlist);
        free(*ppTrace);
        *ppTrace = NULL;

        return(CPHTRUE);
    }

    return(CPHFALSE);
}

/*
** Method: cphTraceOn
**
** This function activates tracing by setting the traceOn member of the trace control block.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceOn(CPH_TRACE *pTrace) {
    if (NULL != pTrace) {
        pTrace->traceOn = CPHTRUE;
        return(CPHTRUE);
    }
    return (CPHFALSE);
}

/*
** Method: cphTraceOff
**
** This method deactivates tracing by unsetting the traceOn member of the trace control block.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceOff(CPH_TRACE *pTrace) {
    if (NULL != pTrace) {
        pTrace->traceOn = CPHTRUE;
        return(CPHTRUE);
    }
    return(CPHFALSE);
}

/*
** Method: cphTraceIsOn
**
** Function to inquire whether tracing has been activated or not.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: CPHTRUE if trace is activated and CPHFALSE otherwise
**
*/
int cphTraceIsOn(CPH_TRACE *pTrace) {
    int res = CPHFALSE;

    if (NULL != pTrace) {
        res = pTrace->traceOn;
    }
    return(res);
}

/*
** Method: cphTraceLine
**
** This function writes a line of data to the trace file. The line is preprended with correct date and time to
** millisecond granularity and the correct indentation level for this thread.
**
** Input Parameters: pTrace - pointer to the trace control block
**                   pTraceThread - pointer to the CPH_TRACETHREAD structure for the thread under
**                                  which this activity is to be traced
**                   aLine - character string containing the line to be sent to the trace.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceLine(CPH_TRACE *pTrace, CPH_TRACETHREAD *pTraceThread) {
    char chTime[80];

    cphUtilGetTraceTime(chTime);
#if defined(CPH_UNIX)
    fprintf(pTrace->tFp, "%s\t%lu%*s%s\n", chTime, pTraceThread->threadId, pTraceThread->indent+1, " ", pTraceThread->buffer);
#else
    fprintf(pTrace->tFp, "%s\t%d%*s%s\n", chTime, pTraceThread->threadId, pTraceThread->indent+1, " ", pTraceThread->buffer);
#endif
    fflush(pTrace->tFp);
    return(CPHTRUE);
}

/*
** Method: cphTraceEntry
**
** This is the function that gets called by a function that wants to trace its entry.
** The function prepends a "{ " prefix to the function name before calling cphTraceLine
** which adds the time, thread number and correct indentation.
**
** Input Parameters: pTrace - pointer to the trace control block
**                   aFunction - character string which names the function
**
**
*/
void cphTraceEntry(CPH_TRACE *pTrace, char const * const aFunction) {

    if (CPHTRUE == pTrace->traceOn) {
        /* Find or allocate the thread slot for this thread */
        CPH_TRACETHREAD *pTraceThread = cphTraceGetTraceThread(pTrace, strlen(aFunction)+3);

        pTraceThread->indent++;
        snprintf(pTraceThread->buffer, pTraceThread->bufferLen, "{ %s", aFunction);

        cphTraceLine(pTrace, pTraceThread);
    }
}

/*
** Method: cphTraceExit
**
** This is the function that gets called by a function that wants to trace its exit.
** The function prepends a "} " prefix to the function name before calling cphTraceLine
** which adds the time, thread number and correct indentation.
**
** Input Parameters: pTrace - pointer to the trace control block
**                   aFunction - character string which names the function
**
**
*/
void cphTraceExit(CPH_TRACE *pTrace, char const * const aFunction) {

    if (CPHTRUE == pTrace->traceOn) {
        /* Find or allocate the thread slot for this thread */
        CPH_TRACETHREAD *pTraceThread = cphTraceGetTraceThread(pTrace, strlen(aFunction)+3);

        snprintf(pTraceThread->buffer, pTraceThread->bufferLen, "} %s", aFunction);

        cphTraceLine(pTrace, pTraceThread);
        pTraceThread->indent--;

    }
}

/*
** Method: cphTraceMsg
**
** This function allows a function to send an informative comment to the trace
** file. It supports a variable number of arguments with the syntax of a printf
** format string. Like cphTraceEntry and cphTraceExit, the function calls
** cphTraceLine to finish formatting the trace data with time, thread and indentation
** and write it to the trace file.
**
** Input Parameters: pTrace - pointer to the trace control block
**                   aFunction - character string naming the function being traced
**                   aFmt - printf style format string
**                   ...  - variable number of arguments according to the format string
**
**
*/
void cphTraceMsg(CPH_TRACE *pTrace, char const * const aFunction, char const * const aFmt, ...) {

    if (CPHTRUE == pTrace->traceOn) {
        va_list marker;
        /* Find or allocate the thread slot for this thread */
        CPH_TRACETHREAD *pTraceThread = cphTraceGetTraceThread(pTrace, 1024);

        va_start( marker, aFmt );     /* Initialize variable arguments. */
        vsnprintf(pTraceThread->buffer, pTraceThread->bufferLen, aFmt, marker);
        va_end( marker );              /* Reset variable arguments.      */

        cphTraceLine(pTrace, pTraceThread);
    }
}

/*
** Method: cphTraceGetTraceThread
**
** This function determines whether this thread is known to the exiting list of thread specific
** data in the trace control block. If this thread is not known to the list, a new entry is added.
** If the thread is known of already, then the pointer to the relevant CPH_TRACETHREAD variable is
** determined. The address of the new or existing CPH_TRACETHREAD data is then returned to the
** caller.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: a CPH_TRACETHREAD pointer to the new or existing CPH_TRACETHREAD data
**
*/
CPH_TRACETHREAD *cphTraceGetTraceThread(CPH_TRACE *pTrace, size_t const bufferLen) {
    cph_pthread_t thisThreadId;
    CPH_TRACETHREAD *pTraceThread = NULL;
    int found = CPHFALSE;
    char * newBuf;

    /* Get the thread id of this process */
    thisThreadId = cphUtilGetThreadId();

    /* If the list is empty then just set found to false so we add it to the list */
    if (CPHTRUE == cphArrayListIsEmpty(pTrace->threadlist))
        found = CPHFALSE;
    else {
        if (NULL != (pTraceThread = cphTraceLookupTraceThread(pTrace, thisThreadId))) {
            found = CPHTRUE;
        }
    }

    /* We don't know about this thread so it is a new thread we need to add to the list */
    if (CPHFALSE == found) {
        if (NULL != (pTraceThread = malloc(sizeof(CPH_TRACETHREAD)))) {
            pTraceThread->threadId = thisThreadId;
            pTraceThread->indent = CPHTRACE_STARTINDENT;
            if(NULL == (pTraceThread->buffer = malloc(bufferLen*sizeof(char)))){
              free(pTraceThread);
              pTraceThread = NULL;
            } else {
              pTraceThread->bufferLen = bufferLen;
              cphSpinLock_Lock(pTrace->pTraceAddLock);
              cphArrayListAdd(pTrace->threadlist, pTraceThread);
              cphSpinLock_Unlock(pTrace->pTraceAddLock);
            }
        }
    } else if(bufferLen>pTraceThread->bufferLen && NULL!=(newBuf = malloc((bufferLen)*sizeof(char)))){
      if (pTraceThread->buffer) {
        free(pTraceThread->buffer);
      }
      pTraceThread->buffer = newBuf;
      pTraceThread->bufferLen = bufferLen;
    }

    if (NULL == pTraceThread) {
        printf("ERROR - internal trace ERROR!!\n");
        exit(1);
    }
    return(pTraceThread);
}

/*
** Method: cphTraceListThreads
**
** This function displays a summary of the threads that have been active in the program. This is called
** at the end of cph as a summary to help identify the various thread numbers.
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphTraceListThreads(CPH_TRACE *pTrace) {

    /* If this thread is the same one as the last time we used trace then we still have the offset into the list */

    CPH_LISTITERATOR *pIterator = cphArrayListListIterator( pTrace->threadlist);

    /* If the list is empty then just set found to false so we add it to the list */
    if (CPHTRUE == cphListIteratorHasNext(pIterator)) {

        /* Look up this thread id in our list */
        do {
            CPH_ARRAYLISTITEM *pItem = cphListIteratorNext(pIterator);
            CPH_TRACETHREAD *pTraceThread = pItem->item;

            printf("   pItem: %p, pTraceThread: %p, pTraceThread->threadId: %d.\n", pItem, pTraceThread, pTraceThread->threadId);

        } while (cphListIteratorHasNext(pIterator));
        cphListIteratorFree(&pIterator);
    }

   return(CPHTRUE);
}

/*
** Method: cphTraceLookupTraceThread
**
**
** Input Parameters: pTrace - pointer to the trace control block
**
** Output parameters: Lookup a thread id in the trace's list of registered trace numbers
**
** Returns: a CPH_TRACETHREAD pointer to the existing CPH_TRACETHREAD data
**
*/
CPH_TRACETHREAD *cphTraceLookupTraceThread(CPH_TRACE *pTrace, cph_pthread_t threadId) {
    int found = CPHFALSE;
    CPH_TRACETHREAD *pTraceThread = NULL;
    CPH_LISTITERATOR *pIterator = cphArrayListListIterator( pTrace->threadlist);


    /* Look up this thread id in our list */
    do {
        CPH_ARRAYLISTITEM *pItem = cphListIteratorNext(pIterator);
        pTraceThread = pItem->item;

        if (threadId == pTraceThread->threadId) {
            found = CPHTRUE;
            break;
        }
    } while (cphListIteratorHasNext(pIterator));
    cphListIteratorFree(&pIterator);

    if (CPHFALSE == found) return(NULL);
    return(pTraceThread);
}
