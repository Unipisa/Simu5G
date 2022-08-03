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

#ifndef __MECORCHESTRATORMANAGER_H_
#define __MECORCHESTRATORMANAGER_H_

//BINDER and UTILITIES
#include "common/LteCommon.h"
#include "nodes/mec/utils/MecCommon.h"
#include "common/binder/Binder.h"           //to handle Car dynamically leaving the Network

//UDP SOCKET for INET COMMUNICATION WITH UE APPs
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEAppPacket
#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"
#include "nodes/mec/MECPlatform/MEAppPacket_m.h"

#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"

using namespace omnetpp;

struct mecAppMapEntry
{
    int contextId;
    std::string appDId;
    std::string mecAppName;
    std::string mecAppIsntanceId;
    int mecUeAppID;         //ID
    cModule* mecHost; // reference to the mecHost where the mec app has been deployed
    cModule* vim;       // for virtualisationInfrastructureManager methods
    cModule* mecpm;     // for mecPlatformManager methods

    std::string ueSymbolicAddres;
    inet::L3Address ueAddress;  //for downstream using UDP Socket
    int uePort;
    inet::L3Address mecAppAddress;  //for downstream using UDP Socket
    int mecAppPort;

    bool isEmulated;

    int lastAckStartSeqNum;
    int lastAckStopSeqNum;

};

class UALCMPMessage;
class MECOrchestratorMessage;
class SelectionPolicyBase;

//
// This module implements the MEC orchestrator of a MEC system.
// It does not follow ETSI compliant APIs, but the it handles the lifecycle operations
// of the standard by using OMNeT++ features.
// Communications with the LCM proxy occur via connections, while the MEC hosts associated with
// the MEC system (and the MEC orchestrator) are managed with the mecHostList parameter.
// This MEC orchestrator provides:
//   - MEC app instantiation
//   - MEC app termination
//   - MEC app run-time onboarding
//

class MecOrchestrator : public cSimpleModule
{
    // Selection Policies modules access grants
    friend class SelectionPolicyBase;
    friend class MecServiceSelectionBased;
    friend class AvailableResourcesSelectionBased;
    friend class MecHostSelectionBased;


    SelectionPolicyBase* mecHostSelectionPolicy_;

    //------------------------------------
    //Binder module
    Binder* binder_;
    //------------------------------------

    //parent modules

    std::vector<cModule*> mecHosts;

    //storing the UEApp and MEApp informations
    //key = contextId - value mecAppMapEntry
    std::map<int, mecAppMapEntry> meAppMap;
    std::map<std::string, ApplicationDescriptor> mecApplicationDescriptors_;

    int contextIdCounter;

    double onboardingTime;
    double instantiationTime;
    double terminationTime;

    public:
        MecOrchestrator();
        const ApplicationDescriptor* getApplicationDescriptorByAppName(std::string& appName) const;
        const std::map<std::string, ApplicationDescriptor>* getApplicationDescriptors() const { return &mecApplicationDescriptors_;}

        /*
         * This method registers the MEC service on all the Service Registry of the MEC host associated
         * with the MEC system
         *
         * @param ServiceDescriptor descriptor of the MEC service to register
         */
        void registerMecService(ServiceDescriptor&) const;

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);



        void handleUALCMPMessage(cMessage* msg);

        // handling CREATE_CONTEXT_APP type
        // it selects the most suitable MEC host and calls the method of its MEC platform manager to require
        // the MEC app instantiation
        void startMECApp(UALCMPMessage*);

        // handling DELETE_CONTEXT_APP type
        // it calls the method of the MEC platform manager of the MEC host where the MEC app has been deployed
        // to delete the MEC app
        void stopMECApp(UALCMPMessage*);


        // sending ACK_CREATE_CONTEXT_APP or ACK_DELETE_CONTEXT_APP
        void sendCreateAppContextAck(bool result, unsigned int requestSno, int contextId = -1);
        void sendDeleteAppContextAck(bool result, unsigned int requestSno, int contextId = -1);

        /*
         * This method selects the most suitable MEC host where to deploy the MEC app.
         * The policies for the choice of the MEC host refer both from computation requirements
         * and required MEC services.
         *
         * The current implementations of the method selects the MEC host based on the availability of the
         * required resources and the MEC host that also runs the required MEC service (if any) has precedence
         * among the others.
         *
         * @param ApplicationDescriptor with the computation and MEC services requirements
         *
         * @return pointer to the MEC host compound module (if any, else nullptr)
         */
        cModule* findBestMecHost(const ApplicationDescriptor&);

        /*
         * MEC hosts associated to the MEC system are configured through the mecHostList NED parameter.
         * This method gets the references to them.
         */
        void getConnectedMecHosts();

        /*
         * The list of the MEC app descriptor to be onboarded at initialization time is
         * configured through the mecApplicationPackageList NED parameter.
         * This method loads the app descriptors in the mecApplicationDescriptors_ map
         *
         */
        void onboardApplicationPackages();

        /*
         * This method loads the app descriptors at runtime.
         *
         * @param ApplicationDescriptor with the computation and MEC services requirements
         *
         * @return ApplicationDescriptor structure of the MEC app descriptor
         */
        const ApplicationDescriptor& onboardApplicationPackage(const char* fileName);
    };

#endif
