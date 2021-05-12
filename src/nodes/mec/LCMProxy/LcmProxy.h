/*
 * LcmProxy.h
 *
 *  Created on: May 6, 2021
 *      Author: linofex
 */


#ifndef _LCMPROXY_H
#define _LCMPROXY_H

#include "nodes/mec/MEPlatform/MeServices/MeServiceBase/MeServiceBase.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"

class MecOrchestrator;
class CreateContextAppMessage;
class LcmProxyMessage;




class LcmProxy: public MeServiceBase
{
  private:

    typedef struct
    {
        int connId;
        unsigned int requestId;
        nlohmann::json appCont;
    } LcmRequestStatus;

    bool scheduledSubscription;
    MecOrchestrator *mecOrchestrator_; //reference to the MecOrchestrator used to get AppList
    unsigned int requestSno;

    std::map<unsigned int, LcmRequestStatus> pendingRequests;

  public:
    LcmProxy();
  protected:

    virtual void initialize(int stage) override;
    virtual void finish() override;


    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    void handleStartOperation(inet::LifecycleOperation *operation) override;

    virtual void handleGETRequest(const std::string& uri, inet::TcpSocket* socket) override;
    virtual void handlePOSTRequest(const std::string& uri, const std::string& body, inet::TcpSocket* socket) override;
    virtual void handlePUTRequest(const std::string& uri, const std::string& body, inet::TcpSocket* socket) override;
    virtual void handleDELETERequest(const std::string& uri, inet::TcpSocket* socket) override;

    void handleCreateContextAppAckMessage(LcmProxyMessage *msg);
    void handleDeleteContextAppAckMessage(LcmProxyMessage *msg);


    /*
     */
    CreateContextAppMessage* parseContextCreateRequest(const nlohmann::json&);

    virtual ~LcmProxy();


};

#endif //_LCMPROXY_H

