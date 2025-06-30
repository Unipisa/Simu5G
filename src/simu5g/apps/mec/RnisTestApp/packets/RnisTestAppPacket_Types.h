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

#ifndef __RNISTESTAPPPACKET_TYPES_H_
#define __RNISTESTAPPPACKET_TYPES_H_

constexpr const char* START_QUERY_RNIS = "StartQueryRnis";
constexpr const char* START_QUERY_RNIS_ACK = "StartQueryRnisAck";
constexpr const char* START_QUERY_RNIS_NACK = "StartQueryRnisNack";

constexpr const char* STOP_QUERY_RNIS = "StopQueryRnis";
constexpr const char* STOP_QUERY_RNIS_ACK = "StopQueryRnisAck";
constexpr const char* STOP_QUERY_RNIS_NACK = "StopQueryRnisNack";

constexpr const char* RNIS_INFO = "RnisInfo";

#endif /* __RNISTESTAPPPACKET_TYPES_H_ */

