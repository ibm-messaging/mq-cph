/*<copyright notice="lm-source" pids="" years="2014,2017">*/
/*******************************************************************************
 * Copyright (c) 2014,2017 IBM Corp.
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
 *    Rowan Lonsdale - Initial implementation
 *    Various members of the WebSphere MQ Performance Team at IBM Hursley UK
 *******************************************************************************/
/*</copyright>*/
/*******************************************************************************/
/*                                                                             */
/* Performance Harness for IBM MQ C-MQI interface                              */
/*                                                                             */
/*******************************************************************************/

#ifndef DUMMY_HPP_
#define DUMMY_HPP_

#include "WorkerThread.hpp"

#include <vector>

namespace cph {

class Dummy: public WorkerThread {
private:
  std::vector<unsigned long> primes;
  unsigned int largestGap;
  unsigned int largestGapIndex;

public:
  Dummy(ControlThread * pControlThread);
  virtual ~Dummy();

  virtual void openSession();
  virtual void closeSession();
  virtual void oneIteration();
};
REGISTER_WORKER_IMPL(Dummy)

}

#endif /* DUMMY_HPP_ */