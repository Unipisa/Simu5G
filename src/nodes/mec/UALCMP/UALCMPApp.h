//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _UALCMPAPP_H
#define _UALCMPAPP_H

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

namespace simu5g {

using namespace omnetpp;

class MecOrchestrator;
class CreateContextAppMessage;
class UALCMPMessage;

//
// This module implements (part of) the mx2 reference point a device app uses to
// request lifecycle operations of a MEC app (i.e. instantiation, termination, relocation).
// This API follows the ETSI MEC specification of ETSI GS MEC 016 V2.2.1 (2020-04) and in
// particular:
//   - GET /app_list Retrieve available application information.
//   - POST /app_contexts For requesting the creation of a new application context.
//   - DELETE /app_contexts/{contextId} For requesting the deletion of an existing application context
//
// Communications with the MEC orchestrator occur via OMNeT connections and messages

class UALCMPApp : public MecServiceBase
{
  private:

    /*
     * structure used to store request information during MEC orchestrator operation
     */
    struct LcmRequestStatus {
        int connId;    // to retrieve the socket for the response
        unsigned int requestId;
        nlohmann::json appCont; // for POST the app context used in the request is sent back with new fields (according to the result)
    };

    inet::ModuleRefByPar<MecOrchestrator> mecOrchestrator_; // reference to the MecOrchestrator used to get AppList

    unsigned int requestSno = 0;    // counter to keep track of the requests
    std::map<unsigned int, LcmRequestStatus> pendingRequests;

  public:
    UALCMPApp();

  protected:

    void initialize(int stage) override;
    void finish() override;

    void handleMessageWhenUp(cMessage *msg) override;
    void handleStartOperation(inet::LifecycleOperation *operation) override;

    // GET the list of available MEC app descriptors
    void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) override;
    // POST the instantiation of a MEC app
    void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)   override;
    // PUT not implemented, yet
    void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)    override;
    // DELETE a MEC app previously instantiated
    void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) override;

    /*
     * These two methods manage the responses coming from the MEC orchestrator and
     * create the HTTP responses to the Device apps
     */
    void handleCreateContextAppAckMessage(UALCMPMessage *msg);
    void handleDeleteContextAppAckMessage(UALCMPMessage *msg);

    /*
     * Method used to parse the body of POST requests for the instantiation of MEC apps.
     * It checks if the app descriptor is already onboarded in the MEC system or not
     *
     * @param JSON body of the POST request
     *
     * @return nullptr if something is wrong
     * @return CreateContextAppMessage* message to be sent to the MEC orchestrator
     *
     */
    CreateContextAppMessage *parseContextCreateRequest(const nlohmann::json&);


};

} //namespace

#endif //_UALCMPAPP_H

