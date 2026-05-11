package simu5g.stack.phy.antennamodel;

//
// This module interface represents the contract that a module must follow
// to implement an antenna model to be used by PHY-layer senders and receivers
//
moduleinterface IAntennaModel
{
    @display("i=device/antennatower");
    double temperature @unit(K);
    string pointingMode @enum(TRACK_PEER,FIXED); // TRACK_PEER keeps boresight aligned with the current peer; FIXED uses the configured pointing angles.
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR);
}
