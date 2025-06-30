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

import inet.networklayer.common.L3Address;

namespace simu5g;

message M1Message {
    inet::L3Address clientAddr;   // client address
    int clientPort;           // client UDP port
    long numPkSent;           // number of packets sent
}
