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

#ifndef __SIMU5G_NRRLCAM_H_
#define __SIMU5G_NRRLCAM_H_

#include <omnetpp.h>
#include "LteRlcAm.h"

namespace simu5g {

class NrAmRxQueue;
class NrAmTxQueue;

/**
 * @class NrRlcAm
 * @brief NR RLC Acknowledged Mode (AM) module.
 *
 * Extends LteRlcAm with NR-specific buffer types (NrAmTxQueue, NrAmRxQueue)
 * that implement 3GPP TS 38.322 sliding-window ARQ.
 */
class NrRlcAm : public LteRlcAm
{
  protected:
    typedef std::map<MacCid, NrAmTxQueue *> NrAmTxBuffers;
    typedef std::map<MacCid, NrAmRxQueue *> NrAmRxBuffers;

    NrAmTxBuffers txBuffers_;
    NrAmRxBuffers rxBuffers_;

    NrAmTxQueue *getNrTxBuffer(MacNodeId nodeId, LogicalCid lcid);
    NrAmRxQueue *getNrRxBuffer(MacNodeId nodeId, LogicalCid lcid);

    void handleUpperMessage(cPacket *pkt) override;
    void handleLowerMessage(cPacket *pkt) override;

  public:
    void deleteQueues(MacNodeId nodeId) override;
    void bufferControlPdu(cPacket *pkt) override;
    void routeControlMessage(cPacket *pkt) override;
};

} //namespace

#endif
