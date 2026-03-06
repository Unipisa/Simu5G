//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//
import inet.common.INETDefs;
import inet.common.TagBase;
import simu5g.common.LteCommon;

cplusplus {{
   using namespace inet;
}};

namespace simu5g;

//
// QoS Flow Identifier (QFI) Tag Classes
//
// These tags are used to attach QFI information to packets as they traverse
// the SDAP layer. The tags enable QFI-to-DRB mapping and reverse lookup
// during transmission and reception.
//
// QfiReq is typically used when sending packets downwards.
// QfiInd is typically used when receiving packets upwards.
//
class QfiTagBase extends inet::TagBase {
	    Qfi qfi;  // QFI value (range: 0–63)
}

class QfiReq extends QfiTagBase
{
}

class QfiInd extends QfiTagBase
{
}
