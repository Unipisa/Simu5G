package simu5g.stack.phy.antennamodel;

//
// Model for NTN Gateway antennas.
//
simple GatewayAntennaModel like IAntennaModel
{
    @display("i=device/antennatower");
    @class(ParabolicAntennaModel);
    double txPower @unit(dBm) = default(64.022dBm);  // derived considering EIRP = 90dBW at 27GHz with 2.8m, eta=0.7, and 0.45dB TX feeder loss.
    double efficiency = default(0.7);
    double diameter @unit(m) = default(2.8m);
    double txFeederLoss @unit(dB) = default(0.45dB);
    double rxFeederLoss @unit(dB) = default(0.5dB);
    double rxLumpedLoss @unit(dB) = default(0dB);
    double noiseFigure @unit(dB) = default(13.928dB);  // derived considering G/T = 42dB/K at 27GHz with 0.5dB RX feeder loss.
    double temperature @unit(K) = default(290K);
    string pointingMode @enum(TRACK_PEER,FIXED) = default("TRACK_PEER");   // TRACK_PEER keeps boresight aligned with the current peer; FIXED uses the configured pointing angles.
    double boresightAzimuth @unit(deg) = default(0deg);                    // Ground-frame azimuth of the antenna boresight, clockwise from north.
    double boresightElevation @unit(deg) = default(90deg);                 // Ground-frame elevation of the antenna boresight above the local horizon.
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR) = default("CIRCULAR");
}
