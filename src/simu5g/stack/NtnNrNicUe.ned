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

package simu5g.stack;

//
// NR NIC for User Equipments served through a transparent NTN path. It replaces
// the PHY layers with NTN-specific modules, which reach the serving gNodeB via
// the satellite service link.
//
module NtnNrNicUe extends NrNicUe
{
    parameters:
        // both PHY layers are NTN-capable, as they share the NTN channel and antenna models
        phy.typename = default("NtnPhyUe");
        nrPhy.typename = default("NtnPhyUe");
}
