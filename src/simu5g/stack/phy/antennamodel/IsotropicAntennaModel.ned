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
    string pointingMode @enum(TRACK_PEER,FIXED) = default("TRACK_PEER");  // TRACK_PEER keeps boresight aligned with the current peer; FIXED uses the configured pointing angles.
    double boresightAzimuth @unit(deg) = default(0deg);                   // Ground-frame azimuth of the antenna boresight, clockwise from north.
    double boresightElevation @unit(deg) = default(90deg);                // Ground-frame elevation of the antenna boresight above the local horizon.
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR) = default("LINEAR");
}
