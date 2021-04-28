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

#ifndef _CPHLISTITERATOR
#define _CPHLISTITERATOR

#include "cphArrayList.h"

/* The CPH_LISTITERATOR structure */
typedef struct _cphlistiterator {
  CPH_ARRAYLISTITEM *listStart;    /* The address of the first item in the list */
  CPH_ARRAYLISTITEM *currentItem;  /* The address of the current item in the list */
} CPH_LISTITERATOR;

/* Function prototypes */
void cphListIteratorIni(CPH_LISTITERATOR **ppListIterator, CPH_ARRAYLIST *pArrayList);
int cphListIteratorFree(CPH_LISTITERATOR **ppListIterator);
int cphListIteratorHasNext(CPH_LISTITERATOR *pListIterator);
CPH_ARRAYLISTITEM* cphListIteratorNext(CPH_LISTITERATOR *pListIterator);
CPH_LISTITERATOR* cphArrayListListIterator(CPH_ARRAYLIST *pArrayList);

#endif
