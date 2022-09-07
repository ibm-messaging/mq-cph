/*<copyright notice="lm-source" pids="" years="2014,2021">*/
/*******************************************************************************
 * Copyright (c) 2014,2021 IBM Corp.
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
 *    Rowan Lonsdale  - Initial implementation
 *    Various members of the WebSphere MQ Performance Team at IBM Hursley UK
 *******************************************************************************/
/*</copyright>*/
/*******************************************************************************/
/*                                                                             */
/* Performance Harness for IBM MQ C-MQI interface                              */
/*                                                                             */
/*******************************************************************************/

#ifndef CONTROLTHREAD_HPP_
#define CONTROLTHREAD_HPP_

#include "Thread.hpp"
#include "WorkerThread.hpp"

#include "cphDestinationFactory.h"
#include "cphConfig.h"

#include <vector>

namespace cph {
class StatsThread;
class WorkerThread;

class ControlThread {

public:
  CPH_CONFIG * const pConfig;
  char procId[80];
  CPH_DESTINATIONFACTORY * pDestinationFactory;

  ControlThread(CPH_CONFIG * const pConfig);
  virtual ~ControlThread();
  void run();
  void incRunners();
  void decRunners();
  unsigned int getThreadStats(std::vector<unsigned int> &stats) const;
  void getThreadLatencyStats(long latencyStats[], int tid) const;

private:
  uint64_t const threadId;
  std::vector<WorkerThread *> workers;
  StatsThread * pStatsThread;
  bool shutdown;
  unsigned int runningWorkers;
  Lock threadCountLock;

  void startWorkers(unsigned int interval, unsigned int timeout);
  void shutdownWorkers();
  void traceThreadSummary() const;
};

}

#endif /* CONTROLTHREAD_HPP_ */
