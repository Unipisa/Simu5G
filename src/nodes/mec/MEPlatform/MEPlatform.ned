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

package simu5g.nodes.mec.MEPlatform;

import simu5g.nodes.mec.MEPlatform.IMEService;

import simu5g.nodes.mec.MEPlatform.GeneralServices.RadioNetworkInformation;
// 
//# MEPlatform
//

module MEPlatform
{
    parameters:

		//# Num Services
        int numServices = default(0); // no of UDP ME Services. Specify the service types in INI file with udpManager.udpService[0..1].typename="UDPServ" syntax

        @display("bgb=404.32,126.56");
    gates:

        input meAppIn[];
        output meAppOut[];

    submodules:

        udpService[numServices]: <> like IMEService {
            @display("p=237.25,81.9,row");
        }

        rniService: RadioNetworkInformation {
            @display("p=34,34,row");
        }

    connections allowunconnected:
        
}

