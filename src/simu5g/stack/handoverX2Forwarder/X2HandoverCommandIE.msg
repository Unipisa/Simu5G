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


import simu5g.x2.packet.X2InformationElement;
import simu5g.common.LteCommon;

namespace simu5g;

//
// X2HandoverCommandIE
//
class X2HandoverCommandIE extends X2InformationElement
{
    type = X2_HANDOVER_CMD_IE;
    length = sizeof(MacNodeId) + sizeof(uint8_t);
    bool startHandover @getter(isStartHandover) = false;
    MacNodeId ueId = NODEID_NONE;      // ID of the user performing the handover
};

cplusplus(X2HandoverCommandIE) {{
  public:
    void setStartHandover() { startHandover = true; }
}}
