# NR Standalone with QoS Flows, SDAP and Multiple DRBs

This example demonstrates 5G QoS-flow handling in Simu5G: the SDAP layer,
QFI-to-DRB mapping with multiple Data Radio Bearers per UE, and QoS-aware
MAC scheduling driven by per-DRB QoS parameters.

The network is the standard `SingleCell_Standalone` (one gNB, two UEs, a
remote server behind a UPF); everything specific to this example is plain
ini configuration.

## What is configured

- **SDAP layer** is enabled on the gNB and UE NICs (`hasSdap = true`).
  With SDAP enabled, the static `drbConfig` parameter is the sole authority
  for DRB assignment (Ip2Nic's dynamic per-connection DRB creation is
  bypassed).

- **Two DRBs per UE**, defined in `sdap.drbConfig`:
  QFIs 1,2 (Voice/Video) map to DRB 0, QFIs 3,4 (Gaming/URLLC) map to DRB 1,
  both in RLC UM mode. DRB IDs are per-UE, as in 3GPP: each UE has a DRB 0
  and a DRB 1, distinguished at the gNB by the `ue` (MacNodeId) field
  (2049 = ue[0], 2050 = ue[1]). The first DRB entry of each UE acts as its
  *default DRB*, which carries traffic whose QFI has no explicit mapping.

- **QoS-aware scheduling**: `schedulingDiscipline = "QOS_PF"` with
  `mac.drbQosConfig` supplying per-DRB QoS parameters (GBR flag, packet
  delay budget, packet error rate, priority) to the QoS-aware
  proportional-fair scheduler. DRB 1 is configured as the more demanding
  bearer (50 ms budget, 10^-3 PER, priority 1).

## How packets get their QFI

- **Downlink**: the sender application marks packets with a DSCP value
  (`dscp` parameter of `VoipSender`). At the core-network ingress the
  `TrafficFlowFilter` reads the DSCP as the QFI (simplified PDR matching),
  the QFI travels in the GTP-U header to the gNB, and the gNB's SDAP maps
  it to the UE's DRB. Unmarked traffic (DSCP 0) becomes QFI 0 and falls
  back to the default DRB.

- **Uplink**: the UE application also marks packets with DSCP, and the UE's
  SDAP derives the QFI from it. This is a non-standard, opt-in shortcut
  (`sdap.useDscpAsQfiFallback = true`); the standard mechanisms (QfiReq tag
  from the application, or reflective QoS) take precedence when present.

## Configurations

- `Standalone`: Topology and DRB/QoS setup only, no traffic. Base for the others. |
- `VoIP-DL`: 4 VoIP flows to each UE, downlink. No DSCP marking, so all traffic is QFI 0 and rides the default DRB — demonstrates the default-flow fallback. |
- `VoIP-DL-MultiQfi`: Same as `VoIP-DL`, but apps 0..3 mark DSCP 1..4, so the four flows use QFI 1..4 and spread across both DRBs of each UE. |
- `VoIP-DL-MultiQfi-Heavy`: Same, with heavy traffic (1000 B every 1 ms, no silence periods) to create contention and exercise the QOS_PF scheduler. |
- `VoIP-UL`: 4 VoIP flows from each UE to the server, uplink, with DSCP-derived QFIs 1..4. |

## What to look at

- SDAP behavior in the event log (Qtenv): `SDAP TX: QFI = ... extracted from
  QfiReq`, the selected DRB, and whether an SDAP header is added (it is only
  added when the QFI cannot be recovered from the DRB alone, i.e. on the
  default DRB or on DRBs carrying multiple QFIs).
- Per-application VoIP quality statistics (`voipMos`, `voipFrameDelay`,
  `voipFrameLoss`, `voipReceivedThroughput`), and how they differ per DRB
  under load in `VoIP-DL-MultiQfi-Heavy`.
