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

#ifndef _LTE_VODUDPSTRUCT_H_
#define _LTE_VODUDPSTRUCT_H_

#include <inet/networklayer/common/L3Address.h>

namespace simu5g {

/**
 * Stores information on a video stream
 */
struct Media1
{
    inet::L3Address clientAddr;   // client address
    int clientPort;               // client UDP port
    long numPkSent;               // number of packets sent
};

} //namespace

#endif

