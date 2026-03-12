//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef STACK_RLC_AM_PACKET_NRRLCAMDATAPDU_H_
#define STACK_RLC_AM_PACKET_NRRLCAMDATAPDU_H_
#include "simu5g/stack/rlc/packet/LteRlcDataPdu.h"

namespace simu5g {


class NrRlcAmDataPdu: public LteRlcDataPdu {
private:
    void copy(const NrRlcAmDataPdu& other) {
        pollStatus_ = other.pollStatus_;
        snoMainPacket=other.snoMainPacket;
        startOffset=other.startOffset;
        endOffset=other.endOffset;
        lengthMainPacket = other.lengthMainPacket;
    }
protected:

    bool pollStatus_;
    unsigned int snoMainPacket;
    unsigned int startOffset;
    unsigned int endOffset;
    unsigned int lengthMainPacket;

public:
    NrRlcAmDataPdu();
    NrRlcAmDataPdu(const NrRlcAmDataPdu& other) : LteRlcDataPdu(other)
    {
        copy(other);
    }

    NrRlcAmDataPdu& operator=(const NrRlcAmDataPdu& other)
    {
        if (&other == this)
            return *this;
        LteRlcDataPdu::operator=(other);
        copy(other);

        return *this;
    }

    NrRlcAmDataPdu *dup() const override
    {
        return new NrRlcAmDataPdu(*this);
    }

    virtual ~NrRlcAmDataPdu();
    void setPollStatus(bool p) { pollStatus_ = p; }
    bool getPollStatus() const { return pollStatus_; }

    unsigned int  getSnoMainPacket() const {return snoMainPacket;}
    void setSnoMainPacket(unsigned int n) { snoMainPacket=n;}

    unsigned int getEndOffset() const {
        return endOffset;
    }

    void setEndOffset(unsigned int endOffset) {
        this->endOffset = endOffset;
    }

    unsigned int getLengthMainPacket() const {
        return lengthMainPacket;
    }

    void setLengthMainPacket(unsigned int lengthMainPacket) {
        this->lengthMainPacket = lengthMainPacket;
    }

    unsigned int getStartOffset() const {
        return startOffset;
    }

    void setStartOffset(unsigned int startOffset) {
        this->startOffset = startOffset;
    }
};
Register_Class(NrRlcAmDataPdu);
} /* namespace simu5g */

#endif /* STACK_RLC_AM_PACKET_NRRLCAMDATAPDU_H_ */
