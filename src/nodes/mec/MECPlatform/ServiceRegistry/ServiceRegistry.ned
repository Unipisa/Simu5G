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

package simu5g.nodes.mec.MECPlatform.ServiceRegistry;
import inet.applications.contract.IApp;

//
// This module models the functionalities of the Service Registry entity in
// the MEC architecture (ETSI GS MEC 003). It exposes a REST API
// compliant with ETSI GS MEC 011, allowing MEC applications to discover MEC services
// available in the MEC host. In particular, MEC applications can obtain the IP address
// and port number for each instantiated MEC service, so that they can start sending
// requests to it
//
simple ServiceRegistry like IApp
{
    parameters:
        @display("i=block/source");
        @class("ServiceRegistry");

        string serviceName = default("ServiceRegistry");
        int requestQueueSize = default(0);
        int subscriptionQueueSize = default(0);
        string localAddress = default(""); // local address; may be left empty ("")
        int localPort = default(1000);     // localPort number to listen on

        double requestServiceTime @unit(s) = default(15us);
        double subscriptionServiceTime @unit(s) = default(20us);

        string serverThreadClass = default("simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.SocketManager");

        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value

        string binderModule = default("binder");

        @signal[requestQueueSize];
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);
        @signal[responseTime];
        @statistic[responseTimeStat](title="Response time of foreground requests"; source="responseTime"; record=mean,vector);

    gates:
        output socketOut;
        input socketIn;
}
