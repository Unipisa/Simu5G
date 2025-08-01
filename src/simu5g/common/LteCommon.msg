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

cplusplus {{
#include "simu5g/common/LteCommon.h"
}}

namespace simu5g;

class MacNodeId {
    @existingClass;
    @opaque;
    @byValue;
    @fromValue(MacNodeId($.intValue()));
    @toValue(num($));
    @fromString(MacNodeId(string2int64($)));
    @toString(int642string(num($)));
}

class LogicalCid {
    @existingClass;
    @opaque;
    @byValue;
    @fromValue(LogicalCid($.intValue()));
    @toValue($);
    @fromString(LogicalCid(string2int64($)));
    @toString(int642string($));
}


struct BandLimit
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct EnbInfo
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct UeInfo
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct UeAllocationInfo
{
    @existingClass;
    @opaque;
    @toString(.str());
}


class LteMacBuffer
{
    @existingClass;
    @opaque;
    @polymorphic(false);
    @toString(.str());
}

class LteMacQueue
{
    @existingClass;
    @opaque;
    @polymorphic(false);
    @toString(.str());
}

class Binder
{
    @existingClass;
    @opaque;
    @polymorphic(false);
    @toString(.str());
}

class LteMacEnb
{
    @existingClass;
    @opaque;
    @polymorphic(false);
    @toString(.str());
}

class LteMacBase
{
    @existingClass;
    @opaque;
    @polymorphic(false);
    @toString(.str());
}

class LtePhyBase
{
    @existingClass;
    @opaque;
    @polymorphic(false);
    @toString(.str());
}

class LteRealisticChannelModel
{
    @existingClass;
    @opaque;
    @polymorphic(false);
    @toString(.str());
}

class ExtCell
{
    @existingClass;
    @opaque;
    @polymorphic(false);
    @toString(.str());
}
