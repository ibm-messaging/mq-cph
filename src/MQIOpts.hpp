/*<copyright notice="lm-source" pids="" years="2014,2020">*/
/*******************************************************************************
 * Copyright (c) 2014,2020 IBM Corp.
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

#ifndef MQIOPTS_HPP_
#define MQIOPTS_HPP_

extern "C"{
  #include <cmqc.h>
  #include <cmqxc.h>
  #include "cphConfig.h"
}

namespace cph {

/*
 * Enum: ConnectionType
 * --------------------
 *
 * Enumeration of the ways an MQI client can connect to MQ.
 * Incorporates both 'bind-mode' and 'bind-type'.
 */
enum ConnectionType {
  REMOTE,
  STANDARD,
  FASTPATH
};

/*
 * Class: MQOpts
 * -------------
 *
 * Common options for MQIWorkerThread type worker threads.
 */
class MQIOpts {

private:
  MQGMO gmo;
  MQPMO pmo;
  MQCNO cno;
  MQCD cd;
  MQCNO cno2;
  MQCD cd2;
  MQCSP csp;

  MQMD putMD;
  MQMD getMD;

public:
  //Connection options
  char QMName[MQ_Q_MGR_NAME_LENGTH];      //Queue manager to connect to
  ConnectionType connType;          //Connection type (see above for options).
  char channelName[MQ_CHANNEL_NAME_LENGTH]; //SVRCONN channel to connect via
  char hostName[80];              //Name of machine hosting queue manager
  unsigned int portNumber;          //Port to connect to machine on
  char cipherSpec[MQ_SSL_CIPHER_SPEC_LENGTH]; //Name of cipherSpec
  MQCHAR certLabel[MQ_CERT_LABEL_LENGTH + 1]; //Name of cert label
  MQCHAR username[MQ_USER_ID_LENGTH + 1];
  MQCHAR password[MQ_CSP_PASSWORD_LENGTH + 1];
  char autoReconnect[40];
  MQCHAR28 applName;
  bool useChannelTable;
  char ccdtURL[512];

  //Put options
  bool isPersistent;
  bool put1;

  //Get options
  MQLONG timeout;
  bool readAhead;
  MQLONG receiveSize;

  //Put & Get options
  char destinationPrefix[MQ_Q_NAME_LENGTH];
  MQLONG messageSize;
  bool isRandom;
  unsigned int commitFrequency; //How frequently to commit transactions. 0 means non-transactional
  bool commitPGPut;
  bool useRFH2;
  bool useMessageHandle;
  bool txSet;

  MQIOpts(CPH_CONFIG* pConfig, bool putter, bool getter, bool reconnector);
  MQPMO getPMO() const;
  MQGMO getGMO() const;
  MQCNO getCNO() const;
  MQMD getPutMD() const;
  MQMD getGetMD() const;
  void resetConnectionDef(char *hostname, int port);
};

}
#endif /* MQIOPTS_HPP_ */
