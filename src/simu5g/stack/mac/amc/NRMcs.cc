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

#include "stack/mac/amc/NRMcs.h"

namespace simu5g {

using namespace std;
using namespace omnetpp;

/**
 * <MCS Index> , <Modulation> , <I-TBS> , <threshold>
 * This table contains values taken from (TS 36.213)
 */
NRMcsTable::NRMcsTable(bool extended)
    : extended_(extended) // make it configurable
{
    if (!extended) {
        cqiTable[0] = CQIelem(_QPSK, 0.0);
        cqiTable[1] = CQIelem(_QPSK, 78.0);
        cqiTable[2] = CQIelem(_QPSK, 120.0);
        cqiTable[3] = CQIelem(_QPSK, 193.0);
        cqiTable[4] = CQIelem(_QPSK, 308.0);
        cqiTable[5] = CQIelem(_QPSK, 449.0);
        cqiTable[6] = CQIelem(_QPSK, 602.0);
        cqiTable[7] = CQIelem(_16QAM, 378.0);
        cqiTable[8] = CQIelem(_16QAM, 490.0);
        cqiTable[9] = CQIelem(_16QAM, 616.0);
        cqiTable[10] = CQIelem(_64QAM, 466.0);
        cqiTable[11] = CQIelem(_64QAM, 567.0);
        cqiTable[12] = CQIelem(_64QAM, 666.0);
        cqiTable[13] = CQIelem(_64QAM, 772.0);
        cqiTable[14] = CQIelem(_64QAM, 873.0);
        cqiTable[15] = CQIelem(_64QAM, 948.0);

        table[0] = NRMCSelem(_QPSK, 120.0);
        table[1] = NRMCSelem(_QPSK, 157.0);
        table[2] = NRMCSelem(_QPSK, 193.0);
        table[3] = NRMCSelem(_QPSK, 251.0);
        table[4] = NRMCSelem(_QPSK, 308.0);
        table[5] = NRMCSelem(_QPSK, 379.0);
        table[6] = NRMCSelem(_QPSK, 449.0);
        table[7] = NRMCSelem(_QPSK, 526.0);
        table[8] = NRMCSelem(_QPSK, 602.0);
        table[9] = NRMCSelem(_QPSK, 679.0);
        table[10] = NRMCSelem(_16QAM, 340.0);
        table[11] = NRMCSelem(_16QAM, 378.0);
        table[12] = NRMCSelem(_16QAM, 434.0);
        table[13] = NRMCSelem(_16QAM, 490.0);
        table[14] = NRMCSelem(_16QAM, 553.0);
        table[15] = NRMCSelem(_16QAM, 616.0);
        table[16] = NRMCSelem(_16QAM, 658.0);
        table[17] = NRMCSelem(_64QAM, 438.0);
        table[18] = NRMCSelem(_64QAM, 466.0);
        table[19] = NRMCSelem(_64QAM, 517.0);
        table[20] = NRMCSelem(_64QAM, 567.0);
        table[21] = NRMCSelem(_64QAM, 616.0);
        table[22] = NRMCSelem(_64QAM, 666.0);
        table[23] = NRMCSelem(_64QAM, 719.0);
        table[24] = NRMCSelem(_64QAM, 772.0);
        table[25] = NRMCSelem(_64QAM, 822.0);
        table[26] = NRMCSelem(_64QAM, 873.0);
        table[27] = NRMCSelem(_64QAM, 910.0);
        table[28] = NRMCSelem(_64QAM, 948.0);
    }
    else {
        cqiTable[0] = CQIelem(_QPSK, 0.0);
        cqiTable[1] = CQIelem(_QPSK, 78.0);
        cqiTable[2] = CQIelem(_QPSK, 193.0);
        cqiTable[3] = CQIelem(_QPSK, 449.0);
        cqiTable[4] = CQIelem(_16QAM, 378.0);
        cqiTable[5] = CQIelem(_16QAM, 490.0);
        cqiTable[6] = CQIelem(_16QAM, 616.0);
        cqiTable[7] = CQIelem(_64QAM, 466.0);
        cqiTable[8] = CQIelem(_64QAM, 567.0);
        cqiTable[9] = CQIelem(_64QAM, 666.0);
        cqiTable[10] = CQIelem(_64QAM, 772.0);
        cqiTable[11] = CQIelem(_64QAM, 873.0);
        cqiTable[12] = CQIelem(_256QAM, 711.0);
        cqiTable[13] = CQIelem(_256QAM, 797.0);
        cqiTable[14] = CQIelem(_256QAM, 885.0);
        cqiTable[15] = CQIelem(_256QAM, 948.0);

        table[0] = NRMCSelem(_QPSK, 120.0);
        table[1] = NRMCSelem(_QPSK, 193.0);
        table[2] = NRMCSelem(_QPSK, 308.0);
        table[3] = NRMCSelem(_QPSK, 449.0);
        table[4] = NRMCSelem(_QPSK, 602.0);
        table[5] = NRMCSelem(_16QAM, 378.0);
        table[6] = NRMCSelem(_16QAM, 434.0);
        table[7] = NRMCSelem(_16QAM, 490.0);
        table[8] = NRMCSelem(_16QAM, 553.0);
        table[9] = NRMCSelem(_16QAM, 616.0);
        table[10] = NRMCSelem(_16QAM, 658.0);
        table[11] = NRMCSelem(_64QAM, 466.0);
        table[12] = NRMCSelem(_64QAM, 517.0);
        table[13] = NRMCSelem(_64QAM, 567.0);
        table[14] = NRMCSelem(_64QAM, 616.0);
        table[15] = NRMCSelem(_64QAM, 666.0);
        table[16] = NRMCSelem(_64QAM, 719.0);
        table[17] = NRMCSelem(_64QAM, 772.0);
        table[18] = NRMCSelem(_64QAM, 822.0);
        table[19] = NRMCSelem(_64QAM, 873.0);
        table[20] = NRMCSelem(_256QAM, 682.5);
        table[21] = NRMCSelem(_256QAM, 711.0);
        table[22] = NRMCSelem(_256QAM, 754.0);
        table[23] = NRMCSelem(_256QAM, 797.0);
        table[24] = NRMCSelem(_256QAM, 841.0);
        table[25] = NRMCSelem(_256QAM, 885.0);
        table[26] = NRMCSelem(_256QAM, 916.5);
        table[27] = NRMCSelem(_256QAM, 948.0);
    }
}

unsigned int NRMcsTable::getMinIndex(LteMod mod)
{
    if (!extended_) {
        switch (mod) {
            case _QPSK: return 0;
            case _16QAM: return 10;
            case _64QAM: return 17;
            default: throw cRuntimeError("NRMcsTable::getMinIndex - modulation %d not supported", mod);
        }
    }
    else {
        switch (mod) {
            case _QPSK: return 0;
            case _16QAM: return 5;
            case _64QAM: return 11;
            case _256QAM: return 20;
            default: throw cRuntimeError("NRMcsTable::getMinIndex - modulation %d not supported", mod);
        }
    }
}

unsigned int NRMcsTable::getMaxIndex(LteMod mod)
{
    if (!extended_) {
        switch (mod) {
            case _QPSK: return 9;
            case _16QAM: return 16;
            case _64QAM: return 28;
            default: throw cRuntimeError("NRMcsTable::getMaxIndex - modulation %d not supported", mod);
        }
    }
    else {
        switch (mod) {
            case _QPSK: return 4;
            case _16QAM: return 10;
            case _64QAM: return 19;
            case _256QAM: return 27;
            default: throw cRuntimeError("NRMcsTable::getMaxIndex - modulation %d not supported", mod);
        }
    }
}

const unsigned int nInfoToTbs[TBSTABLESIZE] =
{
    0, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 208, 224, 240, 256, 272, 288, 304, 320,
    336, 352, 368, 384, 408, 432, 456, 480, 504, 528, 552, 576, 608, 640, 672, 704, 736, 768, 808, 848, 888, 928, 984, 1032, 1064, 1128, 1160, 1192, 1224, 1256,
    1288, 1320, 1352, 1416, 1480, 1544, 1608, 1672, 1736, 1800, 1864, 1928, 2024, 2088, 2152, 2216, 2280, 2408, 2472, 2536, 2600, 2664, 2728, 2792, 2856, 2976, 3104, 3240, 3368, 3496,
    3624, 3752, 3824
};

} //namespace

