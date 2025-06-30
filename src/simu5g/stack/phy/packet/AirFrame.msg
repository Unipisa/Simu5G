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

cplusplus {{
#include "stack/phy/packet/ModulationType.h"
}}

import inet.common.geometry.Geometry;

namespace simu5g;

class ModulationType {@existingClass;};

//
// Format of the messages that are sent to the channel
//
// subclass if you want to create your own ~AirFrame message class
//
packet AirFrame extends cPacket
{
    double pSend; // Power with which this packet is transmitted
    int channelNumber; // Channel on which the packet is sent
    simtime_t duration; // Time it takes to transmit the packet, in seconds
    double bitrate;
    double snr;
    double lossRate;
    double powRec; // Power in the receiver
    inet::Coord senderPos;
    double carrierFrequency;
    double bandwidth;
    ModulationType modulationType;
}
