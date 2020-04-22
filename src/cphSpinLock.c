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

/* The functions in this module provide a machine independent interface
   to machine dependent lock calls. On Windows we use compiler
   intrinsics perform the locking and on UNIX we use Posix mutex calls.
   Of course, mutex calls aren't spinlocks!
*/

#if defined(WIN32) || defined (WIN64)
/* This is the 64 bit specific spinlock code - here we use compiler intrinsics
   instead of embedded assembler */

/* Some of the functionality we use later requires NT 5.0 or greater */
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500

#include <windows.h>

/*********************************************************************/
/* Ignore "C28251: Inconsistent annotation" warnings for Microsoft   */
/* functions.                                                        */
/*********************************************************************/
#pragma warning( push )
#pragma warning( disable : 28251 )
#include <intrin.h>
#pragma warning( pop )

/* This is to specify which intrinsic functions we want to use */
#pragma intrinsic (_interlockedbittestandreset, _bittestandset)

#else
#include <pthread.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "cphUtil.h"
#include "cphSpinLock.h"

/*
** Method: cphSpinLock_Init
**
** Create a lock control structure. Calling this function does perform any locking but
** just prepares the CPH_SPINLOCK structure. The functions cphSpinLock_Lock must be called to
** actually create a lock.
**
** Uutput Parameters: a double pointer to the newly created CPH_SPINLOCK structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphSpinLock_Init(CPH_SPINLOCK **ppSLock) {
#if defined(WIN32) || defined(WIN64)
    *ppSLock = malloc(sizeof(CPH_SPINLOCK));
  memset ((void *) (*ppSLock), 0, sizeof (CPH_SPINLOCK));/* sets all locks to locked */
  cphSpinLock_Unlock(*ppSLock);

#elif defined(CPH_UNIX)
    *ppSLock = malloc(sizeof(CPH_SPINLOCK));
    memset((void *) (*ppSLock), 0, sizeof (CPH_SPINLOCK));/* sets all locks to locked */
    pthread_mutex_init(*ppSLock, NULL);

#endif
   return(CPHTRUE);
}

/*
** Method: cphSpinLock_Lock
**
** Create a lock on a specified CPH_SPINLOCK. On Windows we repeatedly call the
** _interlocked bittestandreset intrinsic until we successfully obtain a lock,
** yielding the processor on each iteration.
**
** Input Parameters: sLock - a pointer to the CPH_SPINLOCK structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphSpinLock_Lock(CPH_SPINLOCK* sLock) {
#if defined(WIN32) || defined (WIN64)

  /* 'sLock' will be 0 when locked (for HP compatability)
   * we assume its 1 and try to set it to 0, until
   * our assumption is proved correct */
spin:

  if (_interlockedbittestandreset( (long *) sLock, 0)) return(CPHTRUE);

  /* Give up the processor before trying again */
  SwitchToThread();
  goto spin;

#elif defined(CPH_UNIX)
        if (0 != pthread_mutex_lock(sLock)) return(CPHFALSE);
#endif
   return(CPHTRUE);
}

/*
** Method: cphSpinLock_Unlock
**
** Unlock a specified CPH_SPINLOCK. On Windows we use the compiler intrinsic
** _bittestandset.
**
** Input Parameters: sLock - a pointer to the CPH_SPINLOCK structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphSpinLock_Unlock(CPH_SPINLOCK *sLock) {
#if defined(WIN32) || defined (WIN64)
  return _bittestandset( (long *) sLock, 0);

#elif defined(CPH_UNIX)
  if (0 != pthread_mutex_unlock(sLock)) return(CPHFALSE);

#endif
   return(CPHTRUE);
}

/*
** Method: cphSpinLock_Destroy
**
** Dispose of a CPH_SPINLOCK control structure.
**
** Input/Output Parameters: Double pointer to the CPH_SPINLOCK structure. If this is
** successfully disposed of, the pointer to the structure is set to NULL.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphSpinLock_Destroy(CPH_SPINLOCK **ppSLock) {
#if defined(WIN32) || defined (WIN64)
    if (NULL != *ppSLock) {
     cphSpinLock_Unlock(*ppSLock);
       free(*ppSLock);
       *ppSLock = NULL;
    }

#elif defined(CPH_UNIX)
    if (NULL != *ppSLock) {
       pthread_mutex_destroy(*ppSLock);
       free((void *) (*ppSLock));
       *ppSLock = NULL;
    }

#endif
   return(CPHTRUE);
}
