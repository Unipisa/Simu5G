//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _RNISERVICE_H
#define _RNISERVICE_H

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/L2Meas.h"


/**
 * Radio Network Information Service (RNIS)
 * This class inherits the MECServiceBase module interface for the implementation 
 * of the RNI Service defined in ETSI GS MEC 012 RNI API.
 * The current available functionalities are related to the L2 measures information resource
 */


class RNIService: public MecServiceBase
{
  private:

    L2Meas L2MeasResource_;
    
  public:
    RNIService();
  protected:

    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;
    virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)   override;
    virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)    override;
    virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;

    virtual ~RNIService();


};


#endif // ifndef _RNISERVICE_H

