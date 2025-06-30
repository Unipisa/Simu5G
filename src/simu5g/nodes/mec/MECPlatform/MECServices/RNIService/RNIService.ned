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

package simu5g.nodes.mec.MECPlatform.MECServices.RNIService;


import simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.IMECService;
import inet.applications.contract.IApp;

//
// This module implements a subset of the Radio Network Information Service API defined by the ETSI
// GS MEC 012 specification. It exposes a REST API that allows MEC applications to retrieve
// layer-2 measurements related to a User Equipment (UE) or base station.
//
simple RNIService like IMECService, IApp
{
    parameters:
        int requestQueueSize = default(0);
        int subscriptionQueueSize = default(0);
        string localAddress = default(""); // local address; may be left empty ("")
        int localPort = default(1000);     // localPort number to listen on

        double requestServiceTime @unit(s) = default(0.5us);
        double subscriptionServiceTime @unit(s) = default(0.5us);

        bool loadGenerator = default(false);
        double betaa = default(0);  // used only if loadGenerator is true
        int numBGApps = default(0); // used only if loadGenerator is true


        string serverThreadClass = default("simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase.SocketManager");

        string serviceName = default("RNIService");
        string serviceVersion = default("v2");
        string serviceSerialize = default("JSON");

        string transportId = default("RNIAPI");
        string transportName = default("RNIAPI");
        string transportType = default("REST_HTTP");
        string transportProtocol = default("HTTP");

        string catId = default("");
        string catName = default("RNI");
        string catUri = default("");
        string catVersion = default("v2");

        string scopeOfLocality = default("MEC_HOST");
        bool consumedLocalOnly = default(true);

        string binderModule = default("binder");

        @display("i=block/app");
        @signal[requestQueueSize];
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);
        @signal[responseTime];
        @statistic[responseTimeStat](title="Response time of foreground requests"; source="responseTime"; record=mean,vector);

    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
}

