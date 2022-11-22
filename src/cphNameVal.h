/*<copyright notice="lm-source" pids="" years="2007,2022">*/
/*******************************************************************************
 * Copyright (c) 2007,2022 IBM Corp.
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

#ifndef _CPHNAMEVAL
#define _CPHNAMEVAL

#include <stdio.h>

/* Linked list type for storing name and value configuration properties */
typedef struct _cphnameval {
  char name[80];
  char value[1024];
  void *next;
} CPH_NAMEVAL;

int cphNameValAdd(CPH_NAMEVAL **ppList, char const * const name, char const * const value);
int cphNameValGet(CPH_NAMEVAL *pList, char const * const name, char *value, size_t buflen);
int cphNameValPtrGet(CPH_NAMEVAL *list, char const * const name, char **valuePtr);
int cphNameValFree(CPH_NAMEVAL **ppList);
int cphNameValNext(CPH_NAMEVAL **ppList);

#ifdef CPH_TESTING
int cphNameValList(CPH_NAMEVAL *pList);
#endif

#endif
