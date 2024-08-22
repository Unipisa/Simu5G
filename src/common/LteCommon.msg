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
#include "common/LteCommon.h"
}}

namespace simu5g;

class MacNodeId {
    @existingClass;
    @opaque;
}

// class X2NodeId {
//    @existingClass;
// }

struct Lambda
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct RemoteTable
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct LtePhyFrameTable
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct LteNodeTable
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct BandLimit
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct RemoteUnitPhyData
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

struct QCIParameters
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct ApplicationTable
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct SchedDisciplineTable
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct TxTable
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct TxDirectionTable
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct FeedbackRequest
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct FeedbackGeneratorTypeTable
{
    @existingClass;
    @opaque;
    @toString(.str());
}

struct DeploymentScenarioMapping
{
    @existingClass;
    @opaque;
    @toString(.str());
}

class X2InformationElement
{
    @customize;
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

class LteCellInfo
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
