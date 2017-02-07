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

#ifndef _CPHSPINLOCK
#define _CPHSPINLOCK

#if defined(WIN32) || defined(WIN64)

#define CACHE_LINE_BYTES 64

/* The CPH_SPINLOCK structure definition */
typedef struct _cphspinlock {
  union {
    char padding [CACHE_LINE_BYTES];
    char lock;
  } lock;
} CPH_SPINLOCK;


#elif defined(CPH_UNIX)

#include <pthread.h>
#define CPH_SPINLOCK  pthread_mutex_t

#endif

/* Function prototypes */
int cphSpinLock_Init(CPH_SPINLOCK ** ppSLock);
int cphSpinLock_Lock(CPH_SPINLOCK *pSLock);
int cphSpinLock_Unlock(CPH_SPINLOCK *pSLock);
int cphSpinLock_Destroy(CPH_SPINLOCK **ppSLock);

#endif
