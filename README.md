Simu5G
======

5G NR and LTE/LTE-A user-plane simulation model, compatible with the 
INET Framework.
Website: http://simu5g.org

Disclaimer
----------

  Simu5G is an open source simulator licensed under LGPL, and based on 
OMNeT framework which is available under the Academic Public License and 
a commercial license (see https://omnest.com/licensingfaq.php). You are 
solely responsible for obtaining the appropriate license for your use(s) 
of OMNeT. Intel is not responsible for obtaining any such licenses, nor 
liable for any licensing fees due in connection with your use of OMNeT.
Neither the University of Pisa, nor the authors of this software, are 
responsible for obtaining any such licenses, nor liable for any licensing
fees due in connection with your use of OMNeT.

  Simu5G is based on 3GPP specifications, which may involve patented and 
proprietary technology. See https://www.3gpp.org/contact/3gpp-faqs#L5. 
You are solely responsible for determining if your use of Simu5G requires 
any additional licenses. Intel is not responsible for obtaining any such 
licenses, nor liable for any licensing fees due in connection with your 
use of Simu5G. Neither the University of Pisa, nor the authors of this 
software, are responsible for obtaining any such licenses, nor liable 
for any licensing fees due in connection with your use of OMNeT.

  This software is provided on an "as is" basis, without warranties of
any kind, either express or implied, including, but not limited to, 
warranties of accuracy, adequacy, validity, reliability or compliance 
for any specific purpose. Neither the University of Pisa, nor the 
authors of this software, are liable for any loss, expense or damage 
of any type that may arise in using this software. 

  If you use this software or part of it for your research, please cite 
our work:
  
  G. Nardini, D. Sabella, G. Stea, P. Thakkar, A. Virdis, "Simu5G – An 
    OMNeT++ Library for End-to-End Performance Evaluation of 5G Networks,"
    in IEEE Access, vol. 8, pp. 181176-181191, 2020, 
    doi: 10.1109/ACCESS.2020.3028550.

  If you include this software or part of it within your own software, 
README and LICENSE files cannot be removed from it and must be included 
in the root directory of your software package.

Core contributors
-----------------

- Giovanni Nardini (giovanni.nardini@unipi.it)
- Giovanni Stea (giovanni.stea@unipi.it)
- Antonio Virdis (antonio.virdis@unipi.it)

Dependencies
------------

This version requires:

- OMNeT++ 6.0.3 or later
- INET 4.5 or later

Simu5G Features
---------------

General

- eNodeB, gNodeB and UE models
- Full LTE and NR protocol stack
- Simple PGW/UPF model implementing GTP protocol

PDCP-RRC

- Header compression/decompression
- Logical connection establishment and maintenance
- E-UTRA/NR Dual connectivity
- Split Bearer

RLC

- Multiplexing/Demultiplexing of MAC SDUs
- UM, (AM and TM testing) modes

MAC

- HARQ functionalities
- Allocation management
- AMC
- Scheduling Policies (MAX C/I, Proportional Fair, DRR)
- Carrier Aggregation
- Support to multiple numerologies
- Flexible TDD/FDD

PHY

- Channel Feedback management
- Realistic 3GPP channel model with
  - inter-cell interference
  - path-loss
  - fast fading
  - shadowing
  - (an)isotropic antennas

Advanced features

- X2 communication support
- X2-based handover
- CoMP Coordinated Scheduling support
- Device-to-device communications
- Support for vehicular mobility (integration with Veins 5.2)
- ETSI-compliant model of Multi-access Edge Computing (MEC) systems

Applications

- Voice-over-IP (VoIP)
- Constant Bit Rate (CBR)
- Trace-based Video-on-demand (VoD)

Real-time emulation support
---------------------------

Simu5G supports real-time network emulation capabilities. Navigate to
one of the examples included in the "emulation" folder and take a look
at the README file included therein.

Limitations
-----------

- User Plane only (Control Plane not modeled)
- no EPS bearer support – note: a similar concept, "connections", has 
  been implemented, but they are neither dynamic nor statically 
  configurable via some config file
- radio bearers not implemented, not even statically configured radio 
  bearers (dynamically allocating bearers would need the RRC protocol, 
  which is Control Plane so not implemented)
