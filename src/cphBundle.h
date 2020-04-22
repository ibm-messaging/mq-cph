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

#ifndef _CPHBUNDLE
#define _CPHBUNDLE

#include "cphConfig.h"

/* Directory separators for the different platforms */
#if defined(WIN32)
#define CPH_DIRSEP '\\'
#elif defined(CPH_UNIX)
#define CPH_DIRSEP '/'
#endif

/* Definition of the CPH_BUNDLE structure */
typedef struct _cphbundle {
    CPH_CONFIG *pConfig;        /* Pointer to the configuration control block */
    char clazzName[80];         /* The name of the module to which this CPH_BUNDLE applies */
    CPH_NAMEVAL *pNameValList;  /* A CPH_NAMEVAL linked list representing the entries in the property file */

} CPH_BUNDLE;

/* Function prototypes */
void *cphBundleLoad(CPH_CONFIG *pConfig, char *moduleName);
void cphBundleIni(CPH_BUNDLE **ppBundle, CPH_CONFIG *pConfig, char *moduleName);
int  cphBundleFree(CPH_BUNDLE **ppBundle);
int cphBundleGetBundle(CPH_BUNDLE **pBundle, CPH_CONFIG *pConfig, char *module);

#endif
