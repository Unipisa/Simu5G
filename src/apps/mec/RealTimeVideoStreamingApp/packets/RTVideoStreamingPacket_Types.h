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

#ifndef APPS_MEC_RTVIDEOSTREAMING_PACKET_TYPES_H_
#define APPS_MEC_RTVIDEOSTREAMING_PACKET_TYPES_H_

enum RTVideoStreamingPacketType {
    START_RTVIDEOSTREAMING,
    STOP_RTVIDEOSTREAMING,

    START_RTVIDEOSTREAMING_SESSION,
    STOP_RTVIDEOSTREAMING_SESSION,

    RTVIDEOSTREAMING_FRAME,
    RTVIDEOSTREAMING_SEGMENT,

    RTVIDEOSTREAMING_COMMAND,

    START_RTVIDEOSTREAMING_ACK,
    START_RTVIDEOSTREAMING_NACK,
    STOP_RTVIDEOSTREAMING_ACK,
    STOP_RTVIDEOSTREAMING_NACK,

    START_RTVIDEOSTREAMING_SESSION_ACK,
    START_RTVIDEOSTREAMING_SESSION_NACK,
    STOP_RTVIDEOSTREAMING_SESSION_ACK,
    STOP_RTVIDEOSTREAMING_SESSION_NACK,
};

#endif /* APPS_MEC_RTVIDEOSTREAMING_PACKET_TYPES_H_ */

