package simu5g.stack.phy.antennamodel;

//
// Model for Very Small Aperture Terminal (VSAT) antennas.
//
simple VSATAntennaModel like IAntennaModel
{
    @display("i=device/antennatower");
    @class(ParabolicAntennaModel);
    double txPower @unit(dBm) = default(33dBm);
    double efficiency = default(0.6);
    double diameter @unit(m) = default(0.6m);
    double txFeederLoss @unit(dB) = default(0.45dB);
    double rxFeederLoss @unit(dB) = default(0.5dB);
    double rxLumpedLoss @unit(dB) = default(0dB);
    double noiseFigure @unit(dB) = default(1.2dB);
    double temperature @unit(K) = default(290K);
    string pointingMode @enum(TRACK_PEER,FIXED) = default("TRACK_PEER");    // TRACK_PEER keeps boresight aligned with the current peer; FIXED uses the configured pointing angles.
    double boresightAzimuth @unit(deg) = default(0deg);                     // Ground-frame azimuth of the antenna boresight, clockwise from north.
    double boresightElevation @unit(deg) = default(90deg);                  // Ground-frame elevation of the antenna boresight above the local horizon.
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR) = default("CIRCULAR");
}
