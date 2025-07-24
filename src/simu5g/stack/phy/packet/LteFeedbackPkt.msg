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
import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

import simu5g.common.LteCommon;

cplusplus {{
#include "simu5g/stack/phy/feedback/LteFeedback.h"
}}

namespace simu5g;

class LteFeedbackPkt extends inet::FieldsChunk {
    chunkLength = inet::B(1); // TODO: size 0

    // MacNodeId of the source
    MacNodeId sourceNodeId;
}

cplusplus(LteFeedbackPkt) {{
  protected:
    // vector of vectors with RU and TxMode as indexes
    LteFeedbackDoubleVector lteFeedbackDoubleVectorDl_;
    // vector of vectors with RU and TxMode as indexes
    LteFeedbackDoubleVector lteFeedbackDoubleVectorUl_;
    // map of vectors of vectors with peering UEs, RUs, and TxModes as indexes
    std::map<MacNodeId, LteFeedbackDoubleVector> lteFeedbackMapDoubleVectorD2D_;

  public:
    LteFeedbackDoubleVector getLteFeedbackDoubleVectorDl() const;
    void setLteFeedbackDoubleVectorDl(LteFeedbackDoubleVector lteFeedbackDoubleVector_);
    LteFeedbackDoubleVector getLteFeedbackDoubleVectorUl() const;
    void setLteFeedbackDoubleVectorUl(LteFeedbackDoubleVector lteFeedbackDoubleVector_);
    std::map<MacNodeId, LteFeedbackDoubleVector> getLteFeedbackDoubleVectorD2D() const;
    void setLteFeedbackDoubleVectorD2D(MacNodeId peerId, LteFeedbackDoubleVector lteFeedbackDoubleVector_);
}}

cplusplus(LteFeedbackPkt::copy) {{
    lteFeedbackDoubleVectorDl_ = other.lteFeedbackDoubleVectorDl_;
    lteFeedbackDoubleVectorUl_ = other.lteFeedbackDoubleVectorUl_;
    lteFeedbackMapDoubleVectorD2D_ = other.lteFeedbackMapDoubleVectorD2D_;
}}

cplusplus(LteFeedbackPkt::parsimPack) {{
    doParsimPacking(b, lteFeedbackDoubleVectorDl_);
    doParsimPacking(b, lteFeedbackDoubleVectorUl_);
    doParsimPacking(b, lteFeedbackMapDoubleVectorD2D_);
}}

cplusplus(LteFeedbackPkt::parsimUnpack) {{
    doParsimUnpacking(b, lteFeedbackDoubleVectorDl_);
    doParsimUnpacking(b, lteFeedbackDoubleVectorUl_);
    doParsimUnpacking(b, lteFeedbackMapDoubleVectorD2D_);
}}
