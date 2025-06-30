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

package simu5g.nodes.mec.UALCMP;

import inet.applications.contract.IApp;

//
// This module implements (part of) the Mx2 reference point defined by the ETSI GS MEC
// 016 V2.2.1 (2020-04) specification. A ~DeviceApp application module can use this API to
// request MEC app lifecycle operations (i.e., instantiation, termination, relocation).
// In particular, it provides the following methods:
//   - GET /app_list: to retrieve information about available applications
//   - POST /app_contexts: to request the creation of a new application context
//   - DELETE /app_contexts/{contextId}: to request the deletion of an existing application context
// The application communicates directly with the MEC orchestrator without going through the
// protocol stack of the ~UALCMP module.
//
simple UALCMPApp like IApp
{
    parameters:
        @display("i=block/source");
        @class("UALCMPApp");

        string binderModule = default("binder");

        string serviceName = default("UALCMP");
        int requestQueueSize = default(0);
        int subscriptionQueueSize = default(0);
        string localAddress = default(""); // local address; may be left empty ("")
        int localPort = default(1000);     // localPort number to listen on

        // hostname of the reference orchestrator
        string mecOrchestratorHostname;

        double requestServiceTime @unit(s) = default(5us);
        double subscriptionServiceTime @unit(s) = default(10us);

        string serverThreadClass = default("simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.SocketManager");

        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        @signal[requestQueueSize];
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);
        @signal[responseTime];
        @statistic[responseTimeStat](title="Response time of foreground requests"; source="responseTime"; record=mean,vector);

    gates:
        output socketOut;
        input socketIn;

        output toMecOrchestrator;
        input fromMecOrchestrator;
}
