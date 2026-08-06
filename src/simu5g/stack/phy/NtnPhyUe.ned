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

package simu5g.stack.phy;

//
// NTN-specific NR PHY for User Equipments. It sends air frames to the serving
// gNodeB through the transparent satellite path, and evaluates the reception of
// air frames arriving from that path with a dedicated NTN channel model.
//
simple NtnPhyUe extends NrPhyUe
{
    parameters:
        @class("NtnPhyUe");
        string ntnChannelModelModule = default("^.ntnChannelModel[0]");
        string ntnAntennaModelModule = default("^.ntnAntennaModel");
}
