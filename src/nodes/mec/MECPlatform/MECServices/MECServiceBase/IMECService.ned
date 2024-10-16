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

package simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase;

//
// This module interface defines the structure that must be implemented by every
// MEC Service (see, e.g., ~LocationService). Common MEC service functionalities are provided
// by the MecServiceBase class, which includes support for both request-response and
// subscribe-notification paradigms over the TCP protocol.
// Furthermore, the MecServiceBase class includes mechanisms to enqueue MEC applications' requests
// to a MEC service in a FIFO queue, allowing the MEC service to serve them one at a time. This
// class also calculates the computation time of each request according to an exponential
// distribution.
//
moduleinterface IMECService
{
    parameters:
        @display("i=block/app");
        int requestQueueSize;
        int subscriptionQueueSize;

        string serviceName;
        string serviceVersion;
        string serviceSerialize;

        string transportId;
        string transportName;
        string transportType;
        string transportProtocol;

        string catId;
        string catName;
        string catUri;
        string catVersion;

        string scopeOfLocality;
        bool consumedLocalOnly;

        bool loadGenerator;
        double betaa;
        int numBGApps;


        string localAddress; // local address; may be left empty ("")
        int localPort;     // localPort number to listen on

        double requestServiceTime;
        double subscriptionServiceTime @unit(s);
        string serverThreadClass;

        @signal[requestQueueSize];
        @statistic[requestQueueSizeStat](title="Request queue size"; source="requestQueueSize"; record=mean,vector);
        @signal[responseTime];
        @statistic[responseTimeStat](title="Response time of foreground requests"; source="responseTime"; record=mean,vector);
}


