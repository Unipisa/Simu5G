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

import inet.common.TagBase;
import common.LteCommon;

cplusplus {{
    #include "common/LteCommon.h"

    namespace simu5g {
    typedef std::list<X2NodeId> DestinationIdList;
    }
}}

namespace simu5g;

class DestinationIdList { @existingClass; }

//
// X2 Control Info usage:
//
// The X2 Control Info handles communication between the X2 Manager and its users.
// It is attached to the X2InformationElement and contains:
// - The X2 ID of the source eNodeB
// - The X2 IDs of the destination eNodeBs (as a user can send an InformationElement to several peers)
// - a 'init' flag: if set, the X2 Manager uses the IE and the incoming gate to register the user module
//                  into its internal data structures. This allows the X2 Manager to know which user is
//                  connected to which gate, so as to forward messages coming from X2 to the correct
//                  user module.

//
// @class LteX2ControlInfo
//
// This class contains:
// - Source X2 ID
// - List of destination IDs
// - init flag
//
class X2ControlInfoTag extends inet::TagBase {
	MacNodeId sourceId;                       // Source X2NodeId
    DestinationIdList destIdList;             // List of destination X2NodeId

    bool init = false;
}
