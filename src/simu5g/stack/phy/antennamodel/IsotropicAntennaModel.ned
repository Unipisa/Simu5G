package simu5g.stack.phy.antennamodel;

//
// Model for isotropic antennas. When using this module,
// transmitting and receiving gain is 1 in every direction
//
simple IsotropicAntennaModel like IAntennaModel
{
    @display("i=device/antennatower");
    double txPower @unit(dBm) = default(23dBm);
    double txFeederLoss @unit(dB) = default(0dB);
    double rxFeederLoss @unit(dB) = default(0dB);
    double rxLumpedLoss @unit(dB) = default(0dB);
    double noiseFigure @unit(dB) = default(9dB);
    double temperature @unit(K) = default(290K);
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR) = default("LINEAR");
}
