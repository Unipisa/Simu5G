# Open defects and cleanups found while accepting the radio medium branch

These are outside the branch's scope, so they were recorded and left alone. Each is
to be fixed on a branch of its own after the merge, with a test that fails before the
fix. Deviations of the model from the specifications are in model-deviations.md, and
the acceptance of the branch is in radio-medium-acceptance.md.

An item comes from reading the code unless it says that it was reproduced. Remove an
item with the commit that fixes it.

## Defects

- **A UE that is between two cells of a handover can abort the simulation.**
  - Reproduced at a374c5146, in nr/test_numerology:
    `simu5g_dbg -u Cmdenv -c MultiCell-CBR-UL --repeat=60 -r 29 --sim-time-limit=5s '--*.carrierAggregation.componentCarrier[1].numerologyIndex=2'`
  - It aborts at t = 4.025 s with a check_and_cast of a nullptr to LteMacBase, in LteMacUe::createTxHarqBuffer(), from macPduMake(), from NrMacUe::handleSelfMessage() (the TTI tick). ue[4]'s nrMac (node 2053) has cellId_ = 0, between detach and attach, and still makes a PDU for destination 0.
  - It depends on the seed: a commit that shifts the random numbers can hide it.
- **The 2049th NR UE gets the first background UE id.**
  - NrUe.ned gives it the id 4097, which is BGUE_MIN_ID, and Binder::registerNode() checks only the range [1025, 32767]. That UE is then a background UE for the channel model (no shadowing) and for LteMaxCi.
  - With background UEs in the same run, the phantom radio's registration on the medium fails at startup instead.
  - Fix shape: registerNode() rejects ids in the background range.
- **Binder::getX2Port() reads an empty list.** It rejects a node without an entry, but calls front() and pop_front() on an empty list when more X2 clients connect to a node than the node has X2 servers.
- **LteX2Manager sends a control message for a non-peer out on gate 0.** It reads x2InterfaceTable_[targetEnb] with operator[], which inserts a zero on a miss, so the message goes to another peer.
- **The multicast destination id counter has no exhaustion check.** After 32768 addresses, Binder::getOrAssignDestIdForMulticastAddress() wraps to 0 (NODEID_NONE) and to base station ids.
- **Several schedulers erase from the set they iterate.** LteMaxCi, LtePf, LteMaxCiMultiband, LteMaxCiComp and QoSAwareScheduler erase the current element inside a range-for. It is believed to be unreachable.
- **A band limit of -2 ("not this carrier") is taken as unlimited in a background UE's retransmission.** scheduleBgRtx() lets a background UE's retransmission on a two-carrier cell take the other carrier's resource blocks. The foreground allocateRtxBytes() has the same loop.
- **Other Binder reads insert on a miss or have no bounds check.** getNumerologyIndexFromCarrierFreq() returns 0 for an unknown carrier, and getUeMaxNumerologyIndex() has no bounds check.
- **The UE and the eNB treat BSR state differently at a D2D mode switch.** The eNB calls clearBsrBuffers() for the UE on every tx-side switch; D2dUeMacHelper::macHandleD2DModeSwitch() calls cancelBsr() only inside its loop over recognized old connections, the blind spot that 486c8dc93 ("radio-cleanup: D2dUeMacHelper: fix: a mode switch left UL H-ARQ processes waiting for feedback forever") removed for H-ARQ. Whether a stale BSR request matters is unverified.
- **LteSchedulerEnbDl::schedulePerAcidRtx() may book the wrong bands.** assignedBlocks counts the non-skipped bands only, and is booked with bandLim->at(i).band_ by loop index, so bands are wrong when some were skipped. To be checked together with the fix of issue #310 (the MAC deleting a PDU when no H-ARQ process is free), which is made on the v1.8.x branch and cherry-picked to dev. Once "LteMacEnb: the downlink band-status peeks deleted" is in, no assertion catches that drop on dev until then.

## Cleanups

- **One factory for the what-if transmissions.** PhyEnb::requestFeedback(), PhyEnbD2D::appendExtraFeedback(), PhyUe::findCandidateEnb() and both background traffic managers build a hypothetical CellularTransmission by hand, next to CellularTransmitter::createTransmission(). One factory would keep them from drifting apart.
- **A module test of which instant the PHY takes for a CQI.** ChannelModelCqiTime pins everything below the PHY; taking the report's start in PhyEnb::requestFeedback() or PhyEnbD2D::appendExtraFeedback() passes every unit test. The factory above would make this easier to test.
- **getWhatIfChannelModel() asks the receiver for one of the medium's own models.** It could use the medium's getChannelModel(carrier, the receiver's RAT), with "the receiver does not operate on the carrier" as a separate check.
- **CellularRadioMedium::addRadio() checks duplicates only for real node ids.** An endpoint registered twice under NODEID_NONE would get every broadcast twice. It should throw.
- **A unit test deletes objects it does not own.** D2dModeSwitchHarqInterrupt printed "Test is deleting an object named LteMacQueue it doesn't own" 7 times when it was added (not re-checked since). It is in the test's cleanup only, and the verdict is unaffected.
