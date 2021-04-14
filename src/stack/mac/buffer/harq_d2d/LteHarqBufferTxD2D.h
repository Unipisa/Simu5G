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

#ifndef _LTE_LTEHARQBUFFERTXD2D_H_
#define _LTE_LTEHARQBUFFERTXD2D_H_

#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqProcessTxD2D.h"

/*
 * NOTA: e' compito del mac ul usare solo il processo di turno, non c'e' nessun controllo.
 * TODO: aggiungere supporto all'uplink: funzioni in cui si specifica il processo da usare
 * TODO: commenti
 */

class LteHarqBufferTxD2D : public LteHarqBufferTx
{

  public:

    /**
     * Constructor.
     *
     * @param numProc number of H-ARQ processes in the buffer.
     * @param owner simple module instantiating an H-ARQ TX buffer
     * @param nodeId UE nodeId for which this buffer has been created
     */
    LteHarqBufferTxD2D(unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac);

    /**
     * Manages H-ARQ feedback sent to a certain H-ARQ unit and checks if
     * the corresponding process becomes empty.
     *
     * @param fbpkt received feedback packet
     */
    virtual void receiveHarqFeedback(LteHarqFeedback *fbpkt);

    virtual ~LteHarqBufferTxD2D();
};

#endif
