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
import inet.common.packet.Packet;

cplusplus {{
namespace simu5g {
using namespace omnetpp;
using namespace inet;
} // namespace
}};

namespace simu5g;

//
// This is the MAC message flowing through LTE stack.
//
class LteMacPdu extends inet::FieldsChunk
{
    /// ID of the MAC PDU: incrementally set according to the static variable numMacPdus
    int64_t macPduId_ = getChunkId();

    // Length of the PDU
    int64_t macPduLength_ = 0;

    // List Of MAC SDUs
    inet::Packet* sdu[] @owned @getter(getSduPtr);

    // List of MAC CEs
    MacControlElement* ce[] @owned;

    unsigned int headerLength = 0;
    // need separate macPduId (e.g. for feedback packets) since OMNET PDU id is automatically updated
    // whenever a new packet is created, e.g. when this PDU is duplicated
    long macPduId = 0;
}

cplusplus(LteMacPdu) {{
  public:
    /**
     * pushSdu() gets ownership of the packet
     * and stores it inside the MAC SDU list
     * in back position
     *
     * @param pkt packet to store
     */
    virtual void pushSdu(Packet *pkt);

    /**
     * popSdu() pops a packet from the front of
     * the SDU list and drops ownership before
     * returning it
     *
     * @return popped packet
     */
    virtual Packet* popSdu();

    /**
     * hasSdu() verifies if there are other
     * SDUs inside the SDU list
     *
     * @return true if the list is not empty, false otherwise
     */
    virtual bool hasSdu() { return sdu_arraysize != 0; }

    const inet::Packet& getSdu(size_t k) const { return *getSduPtr(k); }

    int64_t getByteLength() const { return macPduLength_ + getHeaderLength(); }

    int64_t getBitLength() const { return getByteLength() * 8; }

    /**
     * pushCe() stores a CE inside the
     * MAC CE list in back position
     *
     * @param pkt CE to store
     */
    virtual void pushCe(MacControlElement *ce) { appendCe(ce); }

    /**
     * popCe() pops a CE from the front of
     * the CE list and returns it
     *
     * @return popped CE
     */
    virtual MacControlElement *popCe() {
        MacControlElement *ce = removeCe(0);
        eraseCe(0);
        return ce;
    }

    /**
     * hasCe() verifies if there are other
     * CEs inside the CE list
     *
     * @return true if the list is not empty, false otherwise
     */
    virtual bool hasCe() const { return ce_arraysize != 0; }

    virtual std::string str() const override;
    virtual void forEachChild(cVisitor *v) override;

    long getId() const { return macPduId_; }
}}

cplusplus(LteMacPdu::setHeaderLength) {{
    this->setChunkLength(B(macPduLength_ + headerLength)); // use the headerLength parameter
}}

cplusplus(LteMacPdu::copy) {{
    macPduLength_ = other.macPduLength_;
    macPduId_ = other.macPduId_;

    // duplication of the SDU queue duplicates all packets but not
    // the ControlInfo - iterate over all packets and restore ControlInfo if necessary
    for (size_t idx = 0; idx < sdu_arraysize; idx++) {
        Packet *p1 = sdu[idx];
        Packet *p2 = other.sdu[idx];
        if (p1->getControlInfo() == nullptr && p2->getControlInfo() != nullptr) {
            p1->setControlInfo(p2->getControlInfo()->dup());
        }
    }
}}

//
// Mac Control Element
//
class MacControlElement extends cObject
{
    double timestamp;
}

//
// Mac Buffer Status Report
//
class MacBsr extends MacControlElement
{
    int size;
}
