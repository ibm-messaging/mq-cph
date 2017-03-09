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

#include "ControlThread.hpp"

#include "cphConfig.h"
#include "cphLog.h"
#include "cphUtil.h"

#ifdef AMQ_AS400
  #include <qusec.h>
  #include <qsygetph.h>
  #include <qwtsetp.h>
#endif

/*
** Method: main
**
** main program which:
**
** - sets up the configuration control block
** - loads the property files
** - parses the command line options
** - initialises and starts the control thread
** - waits for the worker threads to finish
** - disposes of the various control blocks
**
** Input Parameters: argc - the number of arguments supplied on the command line
**                   argv - command line arguments as an array of pointers to character strings
**
** Returns: 0 on successful completion, non zero otherwise
**
*/
int main(int argc, char * argv[]) {
  CPH_CONFIG *myConfig;
  CPH_TRACE *pTrc=NULL;

#ifdef AMQ_AS400
  char prfh[12];
  Qus_EC_t ec = {0};
  ec.Bytes_Provided = sizeof(ec);
  QSYGETPH("QMQM      ", "*NOPWD    ", prfh, &ec);
  if (ec.Bytes_Available)
  {
    printf("QSYGETPH failed %.7s\n", ec.Exception_Id);
    return (-1);
  }
  else
  {
    QWTSETP(prfh, &ec);

    if (ec.Bytes_Available)
    {
      printf("QWTSETP failed %.7s\n", ec.Exception_Id);
      return(-1);
    }
  }
#endif

  /* Set up a trace structure if trace is compiled in */
  CPHTRACEINI(&pTrc)

  /* Set up the configuration control block */
  cphConfigIni(&myConfig, pTrc);

#if defined(WIN32)
  //Enable dump generation on windows, cygwin hides dump creating and message boxes
  UINT mode = GetErrorMode();
  CPHTRACEMSG(pTrc, "Windows Error Mode: %u", mode)
  SetErrorMode(0);
  mode = GetErrorMode();
  CPHTRACEMSG(pTrc, "Windows Error Mode Now: %u", mode)
#endif

  /* Read the property files for the "fixed" modules */
  if (CPHTRUE == cphConfigRegisterBaseModules(myConfig)) {
    /* Read the command line arguments */
    cphConfigReadCommandLine(myConfig, argc, argv);
	cphConfigRegisterWorkerThreadModule(myConfig);
    cphConfigMarkLoaded(myConfig);

    /* Start trace if it has been requested */
    cphConfigStartTrace(myConfig);

    if(CPHTRUE == cphConfigConfigureLog(myConfig)){

      if (CPHTRUE == cphConfigValidateHelpOptions(myConfig)) {

        /* Initiliase the control thread structure */
        cph::ControlThread * pControlThread = new cph::ControlThread(myConfig);

        /* Run the control thread - this actually runs in the current thread, not another */
        pControlThread->run();

        /* Release the control thread structure */
        delete pControlThread;
      }
    }
  }

  /* Release the configuration control block */
  cphConfigFree(&myConfig);

  /* Dispose of the trace structure if trace was compiled in */
  CPHTRACEFREE(&pTrc)

  return 0;
}

