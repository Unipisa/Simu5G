//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.sdap.common;

import simu5g.stack.sdap.common.IReflectiveQosTable;

//
// ReflectiveQosTable maintains a table of QoS flow mappings for reflective QoS.
//
// This module implements 3GPP TS 37.324 reflective QoS feature, which allows
// UEs to derive uplink QoS flow characteristics from downlink traffic patterns.
//
// The module tracks downlink flows with their associated QFI values and enables
// dynamic uplink QFI derivation based on learned flow patterns. Flows automatically
// expire after a configurable timeout period.
//
simple ReflectiveQosTable like IReflectiveQosTable
{
    parameters:
        double timeout @unit(s) = default(30s);          // Flow expiration timeout
        double cleanupInterval @unit(s) = default(10s);  // Interval for periodic cleanup of expired flows
        bool printFinalStats = default(false);           // Print final statistics at simulation end
        @display("i=block/table");
        @class(simu5g::ReflectiveQosTable);
}
