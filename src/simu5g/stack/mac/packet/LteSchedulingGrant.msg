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

import simu5g.common.LteCommonEnum;
import simu5g.common.LteCommon;
import simu5g.stack.mac.amc.UserTxParams;

namespace simu5g;

//
// TODO generated message class
//
class LteSchedulingGrant extends inet::FieldsChunk
{
    chunkLength = inet::B(1); // TODO: size 0
    bool periodic;
    unsigned int period;
    unsigned int expiration;
    unsigned int totalGrantedBlocks;    // blocks granted on all Remotes, all Bands
    unsigned int codewords;    // number of codewords
    unsigned int grantedCwBytes[MAX_CODEWORDS];    // granted bytes per codeword
    const UserTxParams *userTxParams = nullptr @owned @allowReplace;
    Direction direction;    // Traffic Direction
    unsigned int grantId = getChunkId();         // Grant identifier
}

cplusplus(LteSchedulingGrant) {{
  protected:
    RbMap grantedBlocks;

  public:
    const unsigned int getBlocks(Remote antenna, Band b) const { return grantedBlocks.at(antenna).at(b); }
    void setBlocks(Remote antenna, Band b, unsigned int blocks) { grantedBlocks[antenna][b] = blocks; }
    const RbMap& getGrantedBlocks() const { return grantedBlocks; }
    void setGrantedBlocks(const RbMap& rbMap) { handleChange(); grantedBlocks = rbMap; }
}}

cplusplus(LteSchedulingGrant::copy) {{
    grantedBlocks = other.grantedBlocks;
}}

cplusplus(LteSchedulingGrant::parsimPack) {{
    doParsimPacking(b, this->grantedBlocks);
}}

cplusplus(LteSchedulingGrant::parsimUnpack) {{
    doParsimUnpacking(b, this->grantedBlocks);
}}

