# Simu5G SDAP Extension [needs update to reflect on many changes for DRB support]

## Overview

This extension adds full SDAP (Service Data Adaptation Protocol) support to Simu5G, enabling more accurate simulation of 5G QoS Flow handling and mapping between the IP layer and the PDCP layer according to 3GPP Release 15+ specifications.

The extension integrates both transmit-side (`NrTxSdapEntity`) and receive-side (`NrRxSdapEntity`) SDAP entities directly into the Simu5G stack for UE nodes. It supports QFI (QoS Flow Identifier) tagging, SDAP header encapsulation, and configurable QFI-to-DRB mapping.

---

## Motivation

In 5G NR, SDAP is responsible for:

- Mapping IP flows to QoS Flows based on QFI.
- Attaching SDAP headers to user-plane packets.
- Supporting multiple QoS flows within a single PDU session.
- Enabling DRB multiplexing and QoS separation at the radio layer.

By adding SDAP functionality to Simu5G, this extension allows researchers to study:

- End-to-end QoS behavior.
- Multi-flow coexistence.
- QoS-aware scheduling and multiplexing.
- Mapping policies for industrial, vehicular, or URLLC use cases.

---

## Components

### 1️⃣ **NED Modules**

- `NRNicUeSdap`  
  Extends the existing `NRNicUe` by inserting SDAP between the IP and PDCP layers.
  
- `NrTxSdapEntity`  
  Transmit-side SDAP functionality: receives IP packets, maps to QFI, attaches SDAP header, forwards to PDCP.

- `NrRxSdapEntity`  
  Receive-side SDAP functionality: receives PDCP PDUs, parses SDAP header, extracts QFI, forwards to IP.

### 2️⃣ **C++ Classes**

- `NrTxSdapEntity` (`NrTxSdapEntity.h/.cc`)
- `NrRxSdapEntity` (`NrRxSdapEntity.h/.cc`)

Each class implements `initialize()` and `handleMessage()` to manage packet processing.

### 3️⃣ **Tag Classes**

- `QosTagBase`, `QosTagReq`, `QosTagInd`  
  Define QFI tagging objects based on INET’s tagging mechanism.


---

## Usage

1. Replace `NRNicUe` with `NRNicUeSdap` in your NED files to enable SDAP.

```ned
*.ue.nic.typename = "NRNicUeSdap"
```

2. Configure optional static mappings via qfiToDrbMapping parameter:
```ini
*.ue.nic.nrTxSdapEntity.qfiToDrbMapping = "1:1, 2:2, 3:3"
*.ue.nic.nrRxSdapEntity.qfiToDrbMapping = "1:1, 2:2, 3:3"
```

## Credits
Author: Mohamed Seliem (University College Cork, CONNECT Centre)

License: As per Simu5G license file.