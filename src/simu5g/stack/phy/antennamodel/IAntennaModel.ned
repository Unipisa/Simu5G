package simu5g.stack.phy.antennamodel;

//
// This module interface represents the contract that a module must follow
// to implement an antenna model to be used by PHY-layer senders and receivers
//
moduleinterface IAntennaModel
{
    @display("i=device/antennatower");
    double temperature @unit(K);
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR);
}
