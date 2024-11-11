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

#ifndef NODES_MEC_MECPLATFORM_SERVICEREGISTRY_H_
#define NODES_MEC_MECPLATFORM_SERVICEREGISTRY_H_

#include <map>

#include <omnetpp.h>
#include <inet/networklayer/common/L3Address.h>

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"
#include "nodes/mec/utils/MecCommon.h"
#include "nodes/mec/MECPlatform/ServiceRegistry/resources/ServiceInfo.h"

namespace simu5g {

// This module aims to provide essential functionalities of the Service Registry entity of
// the MEC architecture (ETSI GS MEC 003).
// Service discovery is available via REST API, according to the ETSI GS MEC 011
// Each Service Registry maintains the MEC service endpoint of each MEC service in the
// MEC system.
// MEC services register themselves with:
//  - ServiceName -> IpAddress and port

class ServiceRegistry : public MecServiceBase
{
  private:
    // key serviceName
    std::vector<ServiceInfo> mecServices_;

    /*
     * To be ETSI compliant. Each MEC service has a UUID. This implementation does not
     * take into account this information, i.e. service discovery is only available via
     * service name. It is used only in the responses.
     * uuidBase is fixed at 123e4567-e89b-12d3-a456-4266141, with the last 5 digits
     * used to create a unique id in a quicker way through the servIdCounter
     */
    std::string uuidBase = "123e4567-e89b-12d3-a456-4266141"; // last 5 digits are missing and used to create unique id in a quicker way
    int servIdCounter = 10000; // incremented for every new service and concatenated to the uuidBase

  public:
    ServiceRegistry();

    /*
     * This method is used to register the presence of a MEC service in the MEC system.
     * It is called cascading from the MEC orchestrator upon MEC service initialization
     *
     * @param servDesc MEC service descriptor with ETSI compliant information
     */
    void registerMecService(const ServiceDescriptor& servDesc);

    /*
     * This method is used to retrieve all the MEC services in the MEC platform where the
     * Service Registry runs. It is mainly used by the MEC
     */
    const std::vector<ServiceInfo> *getAvailableMecServices() const;

  protected:

    void initialize(int stage) override;

    void handleStartOperation(inet::LifecycleOperation *operation) override;

    /*
     * MEC apps can require the complete list of MEC services, or specific MEC services by specifying the service name
     */
    void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) override;
    void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)   override;
    void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket)    override;
    void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket *socket) override;

};

} //namespace

#endif /* NODES_MEC_MECPLATFORM_SERVICEREGISTRY_H_ */

