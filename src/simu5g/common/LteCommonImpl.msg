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

import inet.common.geometry.Geometry;

import simu5g.common.LteCommonEnum;

cplusplus {{
// #include "simu5g/common/LteCommon.h"
}}

namespace simu5g;

struct RemoteUnitPhyData
{
    int txPower;
    inet::Coord m;
}

struct FeedbackRequest
{
    bool request;
    FeedbackGeneratorType genType;
    FeedbackType type;
    //used if genType==real
    TxMode txMode;
    bool dasAware;
    RbAllocationType rbAllocationType;
};

