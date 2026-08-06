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

package simu5g.nodes;

//
// Models a User Equipment (UE) served through a transparent NTN path. It extends
// the ~NrUe module by implementing a ~NtnNrNicUe as a Network Interface Card (NIC)
// module.
//
module NtnNrUe extends NrUe
{
    parameters:
        cellularNic.typename = default("NtnNrNicUe");
}
