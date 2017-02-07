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

#include "cphUtil.h"
#include "cphNameVal.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
** Method: cphNameValAdd
**
** Adds a name and value pair of configuration properties to a CPH_NAMEVAL linked
** list
**
** Input Parameters: list - double pointer to a CPH_NAMEVAL linked list
**                   name - character string representation of the name to be stored in the list
**                   value - character string representation of the value to be stored in the list
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphNameValAdd(CPH_NAMEVAL **list, char const * const name, char const * const value) {

    CPH_NAMEVAL *nv1;
    CPH_NAMEVAL* nv;

    nv = (CPH_NAMEVAL *) malloc(sizeof(CPH_NAMEVAL));

    if (NULL != nv) {
        strcpy(nv->name, name);
        strcpy(nv->value, value);
        nv->next = NULL;
    }

    if (*list == NULL)
        *list = nv;
    else
    {
        nv1 = *list;
        while (nv1->next) nv1 = nv1->next;
        nv1->next = nv;
    }

    return CPHTRUE;
}

/*
** Method: cphNameValGet
**
** Given a name, this function returns a value associated with that name from a
** CPH_NAMEVAL linked list of configuration options
**
** Input Parameters: list - pointer to the CPH_NAMEVAL linked list
**                   name - the name of the configuration option to search for (character string)
**
** Output Parameters: value - the matching value from the list (character string)

** Returns: CPHTRUE on successful exection, CPHFALSE if the option does not exist in the list or
**          on other error
**
*/
int cphNameValGet(CPH_NAMEVAL *list, char const * const name, char *value) {
    CPH_NAMEVAL *nv;

    if (list != NULL) {
        nv = list;
        while (1)
        {
            if (0 == strcmp(nv->name, name))
            {
                strcpy(value, nv->value);
                return CPHTRUE;
            }
            if (NULL == nv->next) break;
            nv = nv->next;
        }
    }

    return CPHFALSE;
}

/*
** Method: cphNameValFree
**
** Free all the entries in a CPH_NAMEVAL linked list
**
** Input/output Parameters: ppList - double pointer to the CPH_NAMEVAL linked list
**                                   This is set to NULL once the whole list is freed
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphNameValFree(CPH_NAMEVAL **ppList) {
    CPH_NAMEVAL *nv, *nv1;

    if (NULL != *ppList) {
        nv = *ppList;
        while (nv) {
            nv1 = nv->next;
            free(nv);
            nv = nv1;
        }
        *ppList = NULL;
    }

    return CPHTRUE;
}

/*
** Method: cphNameValNext
**
** Given a pointer to a CPH_NAMEVAL entry, set that pointer to the next entry in the list
**
** Input/output Parameters: ppList - double pointer to a CPH_NAMEVAL entry. This is set to the
**                                   the next value in the list if there is one
**
** Returns: CPHTRUE on successful exection, CPHFALSE if there is no next value or on other error
**
*/
int cphNameValNext(CPH_NAMEVAL **ppList) {

    if (NULL == *ppList) return CPHFALSE;
    if (NULL == (*ppList)->next) return CPHFALSE;

    *ppList = (*ppList)->next;

    return CPHTRUE;
}

/*
** Method: cphNameValList
**
** List out the name/value pairs in a CPH_NAMEVAL linked list of configuration options
** This function is for testing purposes only
**
** Input Parameters: list - pointer to the CPH_NAMEVAL linked list
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
#ifdef CPH_TESTING
int cphNameValList(CPH_NAMEVAL *list) {

    CPH_NAMEVAL *nv;

    if (list != NULL) {
        nv = list;
        while (1)
        {
            printf("NAME: %s.\n", nv->name);
            printf("VALUE: %s.\n", nv->value);

            if (NULL == nv->next) break;
            nv = nv->next;
        }
    }
    return CPHTRUE;
}
#endif
