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

#include "MQIOpts.hpp"
#include <string>
#include <cstring>
#include <stdexcept>

using namespace std;

static inline void configError(CPH_CONFIG* pConfig, string msg){
  cphConfigInvalidParameter(pConfig, (char*) msg.data());
  throw runtime_error(msg);
}

namespace cph {

MQIOpts::MQIOpts(CPH_CONFIG* pConfig, bool putter, bool getter) {
  CPHTRACEREF(pTrc, pConfig->pTrc)
  CPHTRACEENTRY(pTrc)

  char temp[80];
  int tempInt;
  bool txSet=false;

  /*
   * ------------------
   * Connection Options
   * ------------------
   */

  //QM Name
  if (CPHTRUE != cphConfigGetString(pConfig, QMName, "jb"))
    configError(pConfig, "(jb) Cannot retrieve Queue Manager name.");
  CPHTRACEMSG(pTrc, "QMName: %s", QMName)

  //Connection Type
  if (CPHTRUE != cphConfigGetString(pConfig, temp, "jt"))
    configError(pConfig, "(jt) Connection type (client/bindings) cannot be retrieved.");
  CPHTRACEMSG(pTrc, "Connection type: %s", temp)
  if(string(temp)=="mqb"){
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &tempInt, "jf"))
      configError(pConfig, "(jf) Cannot retrive fastpath option.");
    connType = tempInt==CPHTRUE ? FASTPATH : STANDARD;
    CPHTRACEMSG(pTrc, "Bind type: %s", connType==FASTPATH ? "fastpath" : "standard")
  } else if (string(temp)=="mqc"){
    connType = REMOTE;
  } else
    configError(pConfig, string("(jt) Unrecognised connection type: ") + temp);

  MQCNO protoCNO = {MQCNO_DEFAULT};
  MQCD protoCD = {MQCD_CLIENT_CONN_DEFAULT};
  MQCSP protoCSP = {MQCSP_DEFAULT};

  //CNO Will be updated to version 4 or 5 if SSL or User/Password authentication is in use
  protoCNO.Version = MQCNO_VERSION_2;

  /* This needs to be set to at least version 9 for shared conv to work */
  protoCD.Version = 9;


  if(connType==REMOTE){
    //Channel name
    if (CPHTRUE != cphConfigGetString(pConfig, channelName, "jc"))
      configError(pConfig, "(jc) Default channel name cannot be retrieved.");
    CPHTRACEMSG(pTrc, "Default channel name: %s", channelName)

    //Host name
    if (CPHTRUE != cphConfigGetString(pConfig, hostName, "jh"))
      configError(pConfig, "(jh) Default host name cannot be retrieved.");
    CPHTRACEMSG(pTrc, "Default host name: %s", hostName)

    //Port number
    if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &portNumber, "jp"))
      configError(pConfig, "(jp) Default port number cannot be retrieved.");
    CPHTRACEMSG(pTrc, "Default port number: %d", portNumber)

    //SSL Cipher
    if (CPHTRUE != cphConfigGetString(pConfig, cipherSpec, "jl"))
      configError(pConfig, "(jl) Default CipherSpec cannot be retrieved.");
    CPHTRACEMSG(pTrc, "Cipher Spec: %s", cipherSpec)

    //Username
    if (CPHTRUE != cphConfigGetString(pConfig, username, "us"))
      configError(pConfig, "(us) Default username cannot be retrieved.");
    CPHTRACEMSG(pTrc, "Username: %s", username)

    //Password
    if (CPHTRUE != cphConfigGetString(pConfig, password, "pw"))
      configError(pConfig, "(pw) Default password cannot be retrieved.");
    CPHTRACEMSG(pTrc, "Password: %s", password)


    CPHTRACEMSG(pTrc, "Setting up MQCD for remote connection.")

    char tempName[MQ_CONN_NAME_LENGTH];
    sprintf(tempName, "%s(%u)", hostName, portNumber);
    strncpy(protoCD.ConnectionName, tempName, MQ_CONN_NAME_LENGTH);
    CPHTRACEMSG(pTrc, "Connection name: %s", protoCD.ConnectionName)

    strncpy(protoCD.ChannelName, channelName, MQ_CHANNEL_NAME_LENGTH);
    CPHTRACEMSG(pTrc, "Channel name: %s", protoCD.ChannelName)

    /* Setting a very large value means that the client will obey the setting on the server */
    protoCD.SharingConversations = 99999999;
    CPHTRACEMSG(pTrc, "Setting SHARECNV value to %d.", protoCD.SharingConversations)

    /*Set the max messagelength on the clientconn channel to the maximum permissable */
	protoCD.MaxMsgLength = 104857600;
    CPHTRACEMSG(pTrc, "Setting MaxMsgLength value to %d.", protoCD.MaxMsgLength)
	

    //Setup SSL structures only if Cipher has been set and not left as the default UNSPECIFIED == ""
    if (strcmp(cipherSpec,"") != 0) {
      //CSP is only used for password authentication
      //MQCSP protoCSP = {MQCSP_DEFAULT};
      //SCO structure used in conjunction with SSL fields in MQCD
      //MQSCO protoSCO = {MQSCO_DEFAULT};
      //Override key repository
      //strncpy(protoSCO.KeyRepository, "c:\\progra~2\\IBM\\WebSph~1\\IBM\\ssl\\key", MQ_SSL_KEY_REPOSITORY_LENGTH);

      //SSL requires at least Version 4 of MQCNO (Connection Options),
      //            Version 7 of MQCD(Channel Definition),
      //            Version 1 of MQCSP(Security Parameters) and
      //            Version 4 of MQSCO (SSL Configuration Options)

      protoCNO.Version = MQCNO_VERSION_4;
      //MQCD/protoCD set to version 9 above
      //protoSCO.Version = MQSCO_VERSION_4;   //Not used

      //Set CipherSpec
      strncpy(protoCD.SSLCipherSpec, cipherSpec, MQ_SSL_CIPHER_SPEC_LENGTH);

      //Were only currently setting values in the MQCD, so might not need to bother setting the MQCSP and MQSCO into the MQCNO
      //protoCNO.SSLConfigPtr = &protoSCO;
    }

    // Setup userid and password if they have been defined
    if (strcmp(username,"") != 0) {
        protoCSP.CSPUserIdPtr = &username;
        protoCSP.CSPUserIdLength = (MQLONG) strlen(username);

        if (strcmp(password,"") != 0) {
        	protoCSP.CSPPasswordPtr = &password;
        	protoCSP.CSPPasswordLength = (MQLONG) strlen(password);
        }

        protoCSP.AuthenticationType = MQCSP_AUTH_USER_ID_AND_PWD;

        //Username/password authentication requires MQCNO_VERSION 5
        protoCNO.Version = MQCNO_VERSION_5;

        csp = protoCSP;
        protoCNO.SecurityParmsPtr = &csp;
    }

    cd = protoCD;
    protoCNO.ClientConnPtr = &cd;
  } else if(connType==FASTPATH) {
    CPHTRACEMSG(pTrc, "Setting fastpath option")
    protoCNO.Options |= MQCNO_FASTPATH_BINDING;
  }

  cno = protoCNO;

  /*
   * -----------------
   * Put & Get Options
   * -----------------
   */

  MQMD protoMD = {MQMD_DEFAULT};

  if(putter||getter){
    //Destination prefix
    if (CPHTRUE != cphConfigGetString(pConfig, destinationPrefix, "d"))
      configError(pConfig, "(d) Cannot retrieve destination prefix.");
    CPHTRACEMSG(pTrc, "Destination prefix: %s", destinationPrefix)

    //Transactionality
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &tempInt, "tx"))
      configError(pConfig, "(tx) Cannot retrieve transactionality option.");
    CPHTRACEMSG(pTrc, "Transactional: %s", tempInt>0 ? "yes" : "no")
    if(tempInt==CPHTRUE){
      if (CPHTRUE != cphConfigGetInt(pConfig, (int*) &commitFrequency, "cc"))
        configError(pConfig, "(cc) Cannot retrieve commit count value.");
	  txSet=true;
      CPHTRACEMSG(pTrc, "Commit count: %d", commitFrequency)
    } else
      commitFrequency = 0;
	
    //Message Size
    if (CPHTRUE != cphConfigGetInt(pConfig, (int *) &messageSize, "ms"))
      configError(pConfig, "(ms) Cannot retrieve message size.");
    if(messageSize==0) messageSize = 1000;
    CPHTRACEMSG(pTrc, "Message size: %d", messageSize)

    //Use message handle?
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &tempInt, "mh"))
      configError(pConfig, "(mh) Cannot retrieve message handle option.");
    useMessageHandle = tempInt==CPHTRUE;
    CPHTRACEMSG(pTrc, "Use message handle: %s", useMessageHandle ? "yes" : "no")

    //Use RFH2?
    if(useMessageHandle){
      if (CPHTRUE != cphConfigGetBoolean(pConfig, &tempInt, "rf"))
        configError(pConfig, "(rf) Cannot retrieve RFH2 option.");
      useRFH2 = tempInt==CPHTRUE;
    } else
      useRFH2 = false;
    CPHTRACEMSG(pTrc, "Use RFH2 header: %s", useRFH2 ? "yes" : "no")

    memcpy(protoMD.Format, (useRFH2||useMessageHandle) ? MQFMT_RF_HEADER_2 : MQFMT_STRING, (size_t)MQ_FORMAT_LENGTH);
  }
  
  /*
   * --------------------------------
   * Put/Get pair transaction options
   * --------------------------------
   */
  
  //For modules that loop round Put/Get, (PutGet & Requester)
  //we can specify more granular transaction scopes with the
  //txp & txg parms (these are mutually exclusive to the tx parm)
  if(putter && getter){  
	 char module[80] = {'\0'};
	 cphConfigGetString(pConfig, module, "tc");
	 if(strcmp(module,"Requester") == 0 || strcmp(module, "PutGet") == 0){
    
        commitPGPut = false;
	 
	    if(CPHTRUE == cphConfigGetBoolean(pConfig, &tempInt, "txp")){
		    CPHTRACEMSG(pConfig->pTrc, "txp flag set: %s", tempInt ? "yes" : "no")
		    if(tempInt == CPHTRUE){
		 	   if(txSet) configError(pConfig, "tx and txp flags cannot both be true");
			   commitPGPut = true;
		    } else {
		 	   if(txSet) commitPGPut = true;
		    }
	    }
	 
	    if(CPHTRUE == cphConfigGetBoolean(pConfig, &tempInt, "txg")){
		    CPHTRACEMSG(pConfig->pTrc, "txg flag set: %s", tempInt ? "yes" : "no")
		    if(tempInt == CPHTRUE){
		 	   if(txSet) configError(pConfig, "tx and txg flags cannot both be true");
			   commitFrequency = 1;
		    }
	    }
	 } 
  }
  
  /*
   * -----------
   * Put Options
   * -----------
   */

  if(putter){
    //Persistence
    if (CPHTRUE != cphConfigGetBoolean(pConfig, &tempInt, "pp"))
      configError(pConfig, "(pp) Cannot retrieve persistence option.");
    isPersistent = tempInt==CPHTRUE;
    CPHTRACEMSG(pTrc, "Persistent: %s", isPersistent ? "yes" : "no")

    // Use PUT1?
    if(CPHTRUE != cphConfigGetBoolean(pConfig, &tempInt, "p1"))
      configError(pConfig, "(p1) Cannot determine whether to use PUT1.");
    put1 = tempInt==CPHTRUE;
    CPHTRACEMSG(pConfig->pTrc, "Use PUT1: %s", put1 ? "yes" : "no")

    MQPMO protoPMO = {MQPMO_DEFAULT};
    protoPMO.Version = MQPMO_VERSION_3;
    protoPMO.Options |= MQPMO_NEW_MSG_ID | MQPMO_NEW_CORREL_ID;
    if (txSet  || commitPGPut) protoPMO.Options |= MQPMO_SYNCPOINT;
    pmo = protoPMO;

    putMD = protoMD;
    if(isPersistent) putMD.Persistence = MQPER_PERSISTENT;
  }

  /*
   * -----------
   * Get Options
   * -----------
   */

  if(getter){
    //Timeout
    if (CPHTRUE != cphConfigGetInt(pConfig, (int *) &timeout, "to"))
      configError(pConfig, "(to) Timeout value cannot be retrieved.");
    CPHTRACEMSG(pTrc, "MQGET Timeout: %d", timeout)

    //Receive Buffer Size
    if (CPHTRUE != cphConfigGetInt(pConfig, (int *) &receiveSize, "rs"))
      configError(pConfig, "(rs) Cannot retrieve receive message buffer size.");
    if (receiveSize == 0) {
      receiveSize = messageSize;
    }
    CPHTRACEMSG(pTrc, "Receive Message buffer size: %d", receiveSize)

    //Read-ahead
    if(connType==REMOTE){
      if (CPHTRUE != cphConfigGetBoolean(pConfig, &tempInt, "jy"))
        configError(pConfig, "(jy) Cannot retrieve Read Ahead option.");
      readAhead = tempInt==CPHTRUE;
      CPHTRACEMSG(pTrc, "Read-ahead: %s", readAhead ? "yes" : "no")
    } else readAhead = false;

    MQGMO protoGMO = {MQGMO_DEFAULT};
    protoGMO.Version = MQGMO_VERSION_3;
    protoGMO.Options = MQGMO_WAIT | MQGMO_CONVERT;
    if(commitFrequency>0) protoGMO.Options |= MQGMO_SYNCPOINT;
    if(useRFH2) protoGMO.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
    protoGMO.WaitInterval = timeout==-1 ? MQWI_UNLIMITED : timeout * 1000;
    gmo = protoGMO;

    getMD = protoMD;
  }

  CPHTRACEEXIT(pTrc)
}

MQGMO MQIOpts::getGMO() const {
  return gmo;
}

MQPMO MQIOpts::getPMO() const {
  return pmo;
}

MQCNO MQIOpts::getCNO() const {
  return cno;
}

MQMD MQIOpts::getPutMD() const {
  return putMD;
}

MQMD MQIOpts::getGetMD() const {
  return getMD;
}

}
