package simu5g.stack.phy.antennamodel;

//
// Model for NTN platform antennas (Satellites and HAPS).
//
moduleinterface NtnPlatformAntennaModel extends IAntennaModel
{
    @display("i=device/antennatower");
    string frequencyBand @enum(S,KA);
    double txPower @unit(dBm);
    double txEfficiency;
    double rxEfficiency;
    double txDiameter;
    double rxDiameter;
    double txFeederLoss;
    double rxFeederLoss;
    double rxLumpedLoss;
    double noiseFigure;
    double temperature @unit(K);
    string pointingMode @enum(TRACK_PEER,FIXED);
    double offNadirAngle @unit(deg);
    double boresightAzimuth @unit(deg);
}

//
// GEO NTN platform antenna, see 3GPP TR 38.821 v16.2.0 (sec. 6.1.1.1, table 6.1.1.1-1).
//
simple GeoNtnPlatformAntennaModel like NtnPlatformAntennaModel
{
    @display("i=device/antennatower");
    @class(NtnPlatformAntennaModel);
    string frequencyBand @enum(S,KA);
    double txPower @unit(dBm) = default(frequencyBand == "S" ? 38dBm : 11.5dBm);
    double txEfficiency = default(frequencyBand == "S" ? 0.592156 : 0.644676);
    double rxEfficiency = default(frequencyBand == "S" ? 0.592156 : 0.645968);
    double txDiameter @unit(m) = default(frequencyBand == "S" ? 22m : 5m);
    double rxDiameter @unit(m) = default(frequencyBand == "S" ? 22m : 3.33m);
    double txFeederLoss @unit(dB) = default(0dB);
    double rxFeederLoss @unit(dB) = default(0dB);
    double rxLumpedLoss @unit(dB) = default(0dB);
    double noiseFigure @unit(dB) = default(frequencyBand == "S" ? 7.376dB : 5.876dB);
    double temperature @unit(K) = default(290K);
    string pointingMode @enum(TRACK_PEER,FIXED) = default("TRACK_PEER");   // TRACK_PEER keeps boresight aligned with the current peer; FIXED uses the configured pointing angles.
    double offNadirAngle @unit(deg) = default(0deg);                       // Satellite-frame boresight tilt away from nadir.
    double boresightAzimuth @unit(deg) = default(0deg);                    // Satellite-frame boresight azimuth around the nadir axis.
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR) = default("CIRCULAR");
}

//
// LEO-1200 NTN platform antenna, see 3GPP TR 38.821 v16.2.0 (sec. 6.1.1.1, table 6.1.1.1-1).
//
simple Leo1200NtnPlatformAntennaModel like NtnPlatformAntennaModel
{
    @display("i=device/antennatower");
    @class(NtnPlatformAntennaModel);
    string frequencyBand @enum(S,KA);
    double txPower @unit(dBm) = default(frequencyBand == "S" ? 40dBm : 1.5dBm);
    double txEfficiency = default(frequencyBand == "S" ? 0.569143 : 0.644676);
    double rxEfficiency = default(frequencyBand == "S" ? 0.569143 : 0.657766);
    double txDiameter @unit(m) = default(frequencyBand == "S" ? 2m : 0.5m);
    double rxDiameter @unit(m) = default(frequencyBand == "S" ? 2m : 0.33m);
    double txFeederLoss @unit(dB) = default(0dB);
    double rxFeederLoss @unit(dB) = default(0dB);
    double rxLumpedLoss @unit(dB) = default(0dB);
    double noiseFigure @unit(dB) = default(frequencyBand == "S" ? 4.276dB : 0.876dB);
    double temperature @unit(K) = default(290K);
    string pointingMode @enum(TRACK_PEER,FIXED) = default("TRACK_PEER");   // TRACK_PEER keeps boresight aligned with the current peer; FIXED uses the configured pointing angles.
    double offNadirAngle @unit(deg) = default(0deg);                       // Satellite-frame boresight tilt away from nadir.
    double boresightAzimuth @unit(deg) = default(0deg);                    // Satellite-frame boresight azimuth around the nadir axis.
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR) = default("CIRCULAR");
}

//
// LEO-600 NTN platform antenna, see 3GPP TR 38.821 v16.2.0 (sec. 6.1.1.1, table 6.1.1.1-1).
//
simple Leo600NtnPlatformAntennaModel like NtnPlatformAntennaModel
{
    @display("i=device/antennatower");
    @class(NtnPlatformAntennaModel);
    string frequencyBand @enum(S,KA);
    double txPower @unit(dBm) = default(frequencyBand == "S" ? 34dBm : -4.5dBm);
    double txEfficiency = default(frequencyBand == "S" ? 0.569143 : 0.644676);
    double rxEfficiency = default(frequencyBand == "S" ? 0.569143 : 0.657766);
    double txDiameter @unit(m) = default(frequencyBand == "S" ? 2m : 0.5m);
    double rxDiameter @unit(m) = default(frequencyBand == "S" ? 2m : 0.33m);
    double txFeederLoss @unit(dB) = default(0dB);
    double rxFeederLoss @unit(dB) = default(0dB);
    double rxLumpedLoss @unit(dB) = default(0dB);
    double noiseFigure @unit(dB) = default(frequencyBand == "S" ? 4.276dB : 0.876dB);
    double temperature @unit(K) = default(290K);
    string pointingMode @enum(TRACK_PEER,FIXED) = default("TRACK_PEER");   // TRACK_PEER keeps boresight aligned with the current peer; FIXED uses the configured pointing angles.
    double offNadirAngle @unit(deg) = default(0deg);                       // Satellite-frame boresight tilt away from nadir.
    double boresightAzimuth @unit(deg) = default(0deg);                    // Satellite-frame boresight azimuth around the nadir axis.
    string antennaPolarization @enum(NONE,LINEAR,CIRCULAR) = default("CIRCULAR");
}
