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

#ifndef _CPHCONFIG
#define _CPHCONFIG
#if defined(__cplusplus)
  extern "C" {
#endif

#include <stdio.h>
#include "cphLog.h"
#include "cphTrace.h"
#include "cphStringBuffer.h"
#include "cphNameVal.h"

#define CPHCONFIG_UNSPECIFIED "UNSPECIFIED"

/* This is the name of the environment variable used to denote where cph has been installed */
#define CPH_INSTALLDIR "CPH_INSTALLDIR"

/* The length of memory we use for the install directory */
#define CPH_INSTALL_DIRLEN 256

#define CPH_DEFAULTOPTIONS_FILE "defaultOptions.txt"

typedef struct _cphconfig {
    char cphInstallDir[CPH_INSTALL_DIRLEN];
    int moduleType;
  CPH_LOG *pLog;
    CPH_ARRAYLIST *bundles;
  CPH_NAMEVAL *defaultOptions;
  CPH_NAMEVAL *validOptions;
  CPH_NAMEVAL *options;
  int inValid;
    CPH_TRACE *pTrc;  // This is initialised by the cph main program
} CPH_CONFIG;

void cphConfigIni(CPH_CONFIG **ppConfig, CPH_TRACE *pTrc);
void cphConfigFree(CPH_CONFIG **ppConfig);
int cphConfigFreeBundles(CPH_CONFIG *pConfig);
void cphConfigReadCommandLine(CPH_CONFIG *pConfig, int argc, char *argv[]);
void cphConfigInvalidParameter(CPH_CONFIG *pConfig, char* error);
int  cphConfigGetInt(CPH_CONFIG *pConfig, int *res, char const * const in);
int  cphConfigGetFloat(CPH_CONFIG *pConfig, float *res, char const * const in);
int  cphConfigGetString(CPH_CONFIG *pConfig, char *res, char const * const in);
int  cphConfigGetStringPtr(CPH_CONFIG *pConfig, char **res, char const * const in);
int  cphConfigGetBoolean(CPH_CONFIG *pConfig, int *res, char const * const in);
int cphConfigStringToBoolean(int *res, char *aString);

int cphConfigContainsKey(CPH_CONFIG *pConfig, char *name);

int cphConfigIsInvalid(CPH_CONFIG *pConfig);
void cphConfigMarkLoaded(CPH_CONFIG *pConfig);

int cphConfigIsEmpty(CPH_CONFIG *pConfig);

int cphConfigStartTrace(CPH_CONFIG *pConfig);
int cphDisplayHelpIfNeeded(CPH_CONFIG *pConfig);
int cphConfigValidateHelpOptions(CPH_CONFIG *pConfig);

char * cphConfigGetVersion(CPH_CONFIG *pConfigl, char *versionString);

char *cphConfigDescribe(CPH_CONFIG *pConfig, int inFull);
char *cphConfigDescribeModule(CPH_CONFIG *pConfig, void *ptr, int inFull);
int cphConfigDescribeModuleAsText(CPH_CONFIG *pConfig, CPH_STRINGBUFFER *pSb, void *pBundle, int inFull);

int cphConfigRegisterBaseModules(CPH_CONFIG *pConfig);
int cphConfigRegisterWorkerThreadModule(CPH_CONFIG *pConfig);
int cphConfigRegisterModule(CPH_CONFIG *pConfig, char *moduleName);
void *cphConfigGetBundle(CPH_CONFIG *pConfig, char *moduleName);
int cphConfigImportResources(CPH_CONFIG *pConfig, void *pBundle);

int cphConfigConfigureLog(CPH_CONFIG *pConfig);

#if defined(__cplusplus)
  }
#endif
#endif
