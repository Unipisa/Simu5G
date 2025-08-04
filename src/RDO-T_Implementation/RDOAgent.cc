//
//#include "UeApp.h"
//#include "RDOAgent.h"
//
//// Global map to hold attack nodes
//std::map<int, int> attackNodes;  // Map senderId -> attackType
//std::map<int, double> attackSeverity; // Map senderId -> attack severity
//std::map<int, simtime_t> attackStartTime; // Map senderId -> attack start time
//std::map<int, simtime_t> attackEndTime; // Map senderId -> attack end time
//
//// Global map to hold trusted nodes
//std::set<int> trustedNodes;  // Set of trusted node IDs
//
//Define_Module(UeApp);
//
//void UeApp::initialize()
//{
//    // Initialize vectors for statistics
//    packetSentVector.setName("packetsSent");
//    attackRateVector.setName("attackRate");
//    rdoStateVector.setName("rdoState");
//    rdoGradientVector.setName("rdoGradient");
//
//    // Extract UE ID from module name (e.g., "ue[5]")
//    std::string fullName = getParentModule()->getFullName();
//    size_t start = fullName.find('[') + 1;
//    size_t end = fullName.find(']');
//    std::string idStr = fullName.substr(start, end - start);
//    mySenderId = std::stoi(idStr);
//
//    // Check if RDO is enabled (for Phase 3)
//    rdoEnabled = getSystemModule()->par("useRDO").boolValue();
//
//    // Load attack nodes if attack schedule is enabled
//    if (getSystemModule()->par("useAttackSchedule").boolValue()) {
//        if (attackNodes.empty()) {
//            std::string attackFile = getSystemModule()->par("attackScheduleFile").stringValue();
//            loadAttackNodesFromCSV(attackFile);
//        }
//    }
//
//    // Load trusted nodes if RDO is enabled
//    if (rdoEnabled && trustedNodes.empty()) {
//        std::string rdoConfigFile = getSystemModule()->par("rdoConfigFile").stringValue();
//        loadTrustedNodesFromCSV(rdoConfigFile);
//
//        // Check if this node is trusted
//        isTrusted = (trustedNodes.find(mySenderId) != trustedNodes.end());
//    }
//
//    // Set up normal periodic message sending (all nodes, including attackers)
//    periodicMsgEvent = new cMessage("sendPeriodicMsg");
//    scheduleAt(simTime() + exponential(1.0), periodicMsgEvent);
//
//    // Check if this node is an attacker
//    if (attackNodes.find(mySenderId) != attackNodes.end()) {
//        isAttacker = true;
//        myAttackType = attackNodes[mySenderId];
//        myAttackSeverity = attackSeverity[mySenderId];
//        myAttackStartTime = attackStartTime[mySenderId];
//        myAttackEndTime = attackEndTime[mySenderId];
//
//        EV_INFO << "UE[" << mySenderId << "] is an attacker. Attack Type: " << myAttackType
//                << ", Severity: " << myAttackSeverity
//                << ", Start: " << myAttackStartTime
//                << ", End: " << myAttackEndTime << endl;
//
//        // Schedule attack start
//        attackMsgEvent = new cMessage("attackControl");
//        scheduleAt(myAttackStartTime, attackMsgEvent);
//
//        // For Sybil attack, generate fake IDs
//        if (myAttackType == ATTACK_TYPE_SYBIL) {
//            // Create fake IDs based on severity (higher severity = more fake IDs)
//            int numFakeIds = std::max(1, static_cast<int>(myAttackSeverity * 5));
//            for (int i = 0; i < numFakeIds; i++) {
//                // Create fake IDs that are not real UE IDs (e.g., 1000 + mySenderId + i)
//                fakeIds.push_back(1000 + mySenderId + i);
//            }
//        }
//    }
//    else {
//        EV_INFO << "UE[" << mySenderId << "] is a " << (isTrusted ? "trusted" : "normal") << " node." << endl;
//    }
//
//    // Initialize RDO if enabled
//    if (rdoEnabled) {
//        initializeRDO();
//    }
//}
//
//void UeApp::handleMessage(cMessage *msg)
//{
//    if (msg == periodicMsgEvent) {
//        // Regular message sending for all nodes
//        sendNormalMessage();
//
//        // Schedule next periodic message
//        scheduleAt(simTime() + exponential(1.0), periodicMsgEvent);
//    }
//    else if (msg == attackMsgEvent) {
//        // Start or stop attack based on current time
//        if (simTime() >= myAttackStartTime && simTime() < myAttackEndTime) {
//            startAttack();
//        } else if (simTime() >= myAttackEndTime) {
//            stopAttack();
//        }
//    }
//    else if (msg == rdoUpdateEvent) {
//        // RDO update processing
//        if (rdoEnabled && !isAttacker) {
//            processRDOUpdate();
//
//            // Schedule next RDO update
//            scheduleAt(simTime() + 0.5, rdoUpdateEvent);
//        }
//    }
//    else if (msg->getName() == std::string("attackMsg")) {
//        // Handle ongoing attack messages
//        if (isAttacker && simTime() < myAttackEndTime) {
//            if (myAttackType == ATTACK_TYPE_DOS) {
//                sendDoSAttack();
//            }
//            else if (myAttackType == ATTACK_TYPE_SYBIL) {
//                sendSybilAttack();
//            }
//            else if (myAttackType == ATTACK_TYPE_FALSIFIED) {
//                sendFalsifiedData();
//            }
//        }
//        delete msg;
//    }
//    else {
//        // Handle incoming packets
//        if (dynamic_cast<Packet *>(msg)) {
//            Packet *packet = check_and_cast<Packet *>(msg);
//
//            // Check if it's an RDO state packet
//            if (rdoEnabled && packet->getName() != nullptr &&
//                strstr(packet->getName(), "RDOState") != nullptr) {
//
//                // Process RDO state packet
//                processRDOStatePacket(packet);
//
//                delete packet;
//            }
//            else {
//                // Handle regular packet
//                delete packet;
//            }
//        }
//        else {
//            // Unknown message type
//            delete msg;
//        }
//    }
//}
//
//void UeApp::finish()
//{
//    // Cleanup scheduled messages
//    cancelAndDelete(periodicMsgEvent);
//    if (isAttacker) {
//        cancelAndDelete(attackMsgEvent);
//    }
//    if (rdoEnabled && rdoUpdateEvent) {
//        cancelAndDelete(rdoUpdateEvent);
//    }
//
//    // Cleanup RDO agent
//    if (rdoAgent) {
//        delete rdoAgent;
//        rdoAgent = nullptr;
//    }
//}
//
//// Helper function to load CSV
//void UeApp::loadAttackNodesFromCSV(const std::string& filename)
//{
//    std::ifstream file(filename);
//    if (!file.is_open()) {
//        EV_ERROR << "Failed to open CSV file: " << filename << endl;
//        return;
//    }
//
//    std::string line;
//    getline(file, line); // Skip header
//
//    // Expected format: node_id,attack_type,start_time,end_time,severity
//    while (getline(file, line)) {
//        std::stringstream ss(line);
//        std::string token;
//
//        // Parse node_id
//        getline(ss, token, ',');
//        int senderId = stoi(token);
//
//        // Parse attack_type
//        getline(ss, token, ',');
//        int attackType = stoi(token);
//
//        // Parse start_time
//        getline(ss, token, ',');
//        simtime_t startTime = stod(token);
//
//        // Parse end_time
//        getline(ss, token, ',');
//        simtime_t endTime = stod(token);
//
//        // Parse severity
//        getline(ss, token, ',');
//        double severity = stod(token);
//
//        // Store attack information
//        attackNodes[senderId] = attackType;
//        attackSeverity[senderId] = severity;
//        attackStartTime[senderId] = startTime;
//        attackEndTime[senderId] = endTime;
//    }
//
//    EV_INFO << "Attack nodes loaded successfully from CSV! Total: " << attackNodes.size() << endl;
//}
//
//void UeApp::loadTrustedNodesFromCSV(const std::string& filename)
//{
//    std::ifstream file(filename);
//    if (!file.is_open()) {
//        EV_ERROR << "Failed to open RDO config file: " << filename << endl;
//        return;
//    }
//
//    std::string line;
//
//    // Skip header lines that start with #
//    while (getline(file, line)) {
//        if (line.empty() || line[0] == '#') {
//            continue;
//        }
//
//        std::stringstream ss(line);
//        std::string token;
//
//        // Format: node_id,trust_score,filter_type,consensus_algorithm
//
//        // Parse node_id
//        getline(ss, token, ',');
//        int nodeId = stoi(token);
//
//        // Parse trust_score
//        getline(ss, token, ',');
//        double trustScore = stod(token);
//
//        // If trust score is 1.0, this is a trusted node
//        if (trustScore >= 0.9) {  // Use 0.9 as threshold to account for floating-point errors
//            trustedNodes.insert(nodeId);
//        }
//
//        // Skip remaining fields
//    }
//
//    EV_INFO << "Trusted nodes loaded successfully. Total: " << trustedNodes.size() << endl;
//}
//
//void UeApp::startAttack()
//{
//    EV_INFO << "UE[" << mySenderId << "] starting attack type " << myAttackType << " at " << simTime() << endl;
//
//    // Create a message to trigger attack behavior
//    cMessage* attackMsg = new cMessage("attackMsg");
//
//    // Schedule attack behavior based on attack type
//    if (myAttackType == ATTACK_TYPE_DOS) {
//        // For DoS, schedule frequent messages
//        // Severity affects the rate: higher severity = higher frequency
//        double interval = std::max(0.1, 1.0 - (myAttackSeverity * 0.9));
//        scheduleAt(simTime() + interval, attackMsg);
//    }
//    else if (myAttackType == ATTACK_TYPE_SYBIL) {
//        // For Sybil, schedule less frequent but multiple ID attacks
//        scheduleAt(simTime() + exponential(0.5), attackMsg);
//    }
//    else if (myAttackType == ATTACK_TYPE_FALSIFIED) {
//        // For falsified data, follow normal timing but with bad data
//        scheduleAt(simTime() + exponential(1.0), attackMsg);
//    }
//
//    // Schedule next check at end time
//    scheduleAt(myAttackEndTime, attackMsgEvent);
//}
//
//void UeApp::stopAttack()
//{
//    EV_INFO << "UE[" << mySenderId << "] stopped attack at " << simTime() << endl;
//    // No need to do anything special, just don't reschedule attack messages
//}
//
//void UeApp::sendNormalMessage()
//{
//    // Create a new packet
//    std::string packetName = "UE_" + std::to_string(mySenderId) + "_" + std::to_string(seqNumber++);
//    auto packet = new Packet(packetName.c_str());
//
//    // Add data to payload (simplified - in real implementation, add appropriate chunk)
//    auto payload = makeShared<ByteCountChunk>(B(100)); // 100 bytes payload
//    packet->insertAtBack(payload);
//
//    // Add timestamp
//    packet->addTag<CreationTimeTag>()->setCreationTime(simTime());
//
//    // Add sender ID
//    packet->addTag<L3AddressReq>()->setSrcAddress(L3Address(mySenderId));
//
//    // Send to output gate
//    send(packet, "out");
//
//    // Update statistics
//    packetSentVector.record(1);
//}
//
//void UeApp::sendDoSAttack()
//{
//    // DoS attack sends many messages in short time intervals
//    EV_INFO << "UE[" << mySenderId << "] executing DoS attack at " << simTime() << endl;
//
//    // Create multiple packets with high rate
//    int numPackets = std::max(1, static_cast<int>(myAttackSeverity * 10));
//
//    for (int i = 0; i < numPackets; i++) {
//        // Create a packet similar to normal but with higher frequency
//        std::string packetName = "UE_" + std::to_string(mySenderId) + "_DoS_" + std::to_string(seqNumber++);
//        auto packet = new Packet(packetName.c_str());
//
//        // Add larger payload to consume more bandwidth
//        auto payload = makeShared<ByteCountChunk>(B(500)); // 500 bytes payload
//        packet->insertAtBack(payload);
//
//        // Add timestamp
//        packet->addTag<CreationTimeTag>()->setCreationTime(simTime());
//
//        // Add sender ID (not spoofed in DoS)
//        packet->addTag<L3AddressReq>()->setSrcAddress(L3Address(mySenderId));
//
//        // Send to output gate
//        send(packet, "out");
//    }
//
//    // Update statistics
//    attackRateVector.record(numPackets);
//
//    // Schedule next attack message if still within attack timeframe
//    if (simTime() < myAttackEndTime) {
//        double interval = std::max(0.1, 1.0 - (myAttackSeverity * 0.9));
//        cMessage* nextAttackMsg = new cMessage("attackMsg");
//        scheduleAt(simTime() + interval, nextAttackMsg);
//    }
//}
//
//void UeApp::sendSybilAttack()
//{
//    // Sybil attack sends messages with fake IDs
//    EV_INFO << "UE[" << mySenderId << "] executing Sybil attack at " << simTime() << endl;
//
//    // Send one message for each fake ID
//    for (int fakeId : fakeIds) {
//        std::string packetName = "UE_" + std::to_string(fakeId) + "_Sybil_" + std::to_string(seqNumber++);
//        auto packet = new Packet(packetName.c_str());
//
//        // Normal sized payload
//        auto payload = makeShared<ByteCountChunk>(B(100));
//        packet->insertAtBack(payload);
//
//        // Add timestamp
//        packet->addTag<CreationTimeTag>()->setCreationTime(simTime());
//
//        // Spoof sender ID with fake ID
//        packet->addTag<L3AddressReq>()->setSrcAddress(L3Address(fakeId));
//
//        // Send to output gate
//        send(packet, "out");
//    }
//
//    // Update statistics
//    attackRateVector.record(fakeIds.size());
//
//    // Schedule next attack message if still within attack timeframe
//    if (simTime() < myAttackEndTime) {
//        cMessage* nextAttackMsg = new cMessage("attackMsg");
//        scheduleAt(simTime() + exponential(0.5), nextAttackMsg);
//    }
//}
//
//void UeApp::sendFalsifiedData()
//{
//    // Falsified data attack sends incorrect information
//    EV_INFO << "UE[" << mySenderId << "] executing Falsified Data attack at " << simTime() << endl;
//
//    std::string packetName = "UE_" + std::to_string(mySenderId) + "_False_" + std::to_string(seqNumber++);
//    auto packet = new Packet(packetName.c_str());
//
//    // Normal sized payload but with falsified content
//    auto payload = makeShared<ByteCountChunk>(B(100));
//    packet->insertAtBack(payload);
//
//    // Add timestamp (potentially falsified)
//    // A falsified timestamp could disrupt time-dependent protocols
//    simtime_t falsifiedTime = simTime() - 5.0; // Falsify by 5 seconds
//    packet->addTag<CreationTimeTag>()->setCreationTime(falsifiedTime);
//
//    // Add correct sender ID (the attack is in the data, not the identity)
//    packet->addTag<L3AddressReq>()->setSrcAddress(L3Address(mySenderId));
//
//    // If RDO is enabled, also send falsified state information
//    if (rdoEnabled) {
//        // Intentionally send extreme gradient values to disrupt optimization
//        // Instead of using the correct gradient, provide a manipulated one
//        double falseGradient = 100.0 * sin(0.1 * simTime().dbl()) * myAttackSeverity;
//
//        if (rdoAgent) {
//            rdoAgent->setGradient(falseGradient);
//            rdoGradientVector.record(falseGradient);
//        }
//
//        // Send the falsified RDO state
//        sendRDOState();
//    }
//
//    // Send to output gate
//    send(packet, "out");
//
//    // Update statistics
//    attackRateVector.record(1);
//
//    // Schedule next attack message if still within attack timeframe
//    if (simTime() < myAttackEndTime) {
//        cMessage* nextAttackMsg = new cMessage("attackMsg");
//        scheduleAt(simTime() + exponential(1.0), nextAttackMsg);
//    }
//}
//
//// RDO-related methods
//
//void UeApp::initializeRDO()
//{
//    if (isAttacker) {
//        // Attackers participate in RDO but may behave maliciously
//        EV_INFO << "UE[" << mySenderId << "] initializing RDO (as attacker)" << endl;
//    } else {
//        EV_INFO << "UE[" << mySenderId << "] initializing RDO (as " << (isTrusted ? "trusted" : "normal") << " node)" << endl;
//    }
//
//    // Identify trusted neighbors
//    for (int i = 0; i < getParentModule()->getParentModule()->par("numUe"); i++) {
//        if (trustedNodes.find(i) != trustedNodes.end()) {
//            // This is a trusted node
//            if (i != mySenderId) {  // Don't include self
//                trustedNeighbors.push_back(i);
//            }
//        }
//    }
//
//    // Create RDO agent
//    rdoAgent = new RDOAgent(mySenderId, isTrusted);
//
//    // Initialize with random state
//    double initialState = uniform(-10, 10);
//
//    // Get all neighbors
//    std::vector<int> allNeighbors;
//    for (int i = 0; i < getParentModule()->getParentModule()->par("numUe"); i++) {
//        if (i != mySenderId) {  // Don't include self
//            allNeighbors.push_back(i);
//        }
//    }
//
//    // Initialize RDO agent
//        rdoAgent->initialize(initialState, trustedNeighbors, allNeighbors);
//
//        // Set initial gradient
//        double gradient = generateGradient();
//        rdoAgent->setGradient(gradient);
//
//        // Record initial state and gradient
//        rdoStateVector.record(initialState);
//        rdoGradientVector.record(gradient);
//
//        // Schedule first RDO update
//        rdoUpdateEvent = new cMessage("rdoUpdate");
//        scheduleAt(simTime() + 0.5, rdoUpdateEvent);
//
//        // Send initial RDO state to neighbors
//        sendRDOState();
//
//        EV_INFO << "RDO initialized for UE[" << mySenderId << "] with initial state " << initialState << endl;
//    }
//
//    void UeApp::processRDOUpdate()
//    {
//        if (!rdoAgent) {
//            EV_ERROR << "RDO agent not initialized for UE[" << mySenderId << "]" << endl;
//            return;
//        }
//
//        // Skip update if this is an attacker
//        if (isAttacker) {
//            return;
//        }
//
//        // Calculate new gradient
//        double gradient = generateGradient();
//        rdoAgent->setGradient(gradient);
//
//        // Step size (decreasing as required by the algorithm)
//        double iterationNumber = simTime().dbl() / 0.5;  // Approximate iteration number
//        double stepSize = 1.0 / (iterationNumber + 1.0);  // αk = 1/(k+1)
//
//        // Update state using RDO-T algorithm
//        double newState = rdoAgent->updateState(neighborStates, stepSize);
//        rdoAgent->setState(newState);
//
//        // Record updated state and gradient
//        rdoStateVector.record(newState);
//        rdoGradientVector.record(gradient);
//
//        // Send updated state to neighbors
//        sendRDOState();
//
//        EV_INFO << "UE[" << mySenderId << "] RDO update: state=" << newState
//                << ", gradient=" << gradient
//                << ", step size=" << stepSize << endl;
//    }
//
//    void UeApp::sendRDOState()
//    {
//        if (!rdoAgent) {
//            return;
//        }
//
//        // Create RDO state packet
//        std::string packetName = "RDOState_" + std::to_string(mySenderId) + "_" + std::to_string(seqNumber++);
//        auto packet = new Packet(packetName.c_str());
//
//        // Create RDO state message
//        auto rdoMsg = makeShared<RDOStatePacket>();
//        rdoMsg->setSourceId(mySenderId);
//        rdoMsg->setDestinationId(-1);  // Broadcast to all nodes
//        rdoMsg->setSequenceNumber(seqNumber);
//        rdoMsg->setTimestamp(simTime());
//
//        // Set state value
//        rdoMsg->setStateValue(rdoAgent->getState());
//
//        // Add message to packet
//        packet->insertAtBack(rdoMsg);
//
//        // Add sender ID
//        packet->addTag<L3AddressReq>()->setSrcAddress(L3Address(mySenderId));
//
//        // Send to output gate
//        send(packet, "out");
//    }
//
//    void UeApp::processRDOStatePacket(Packet *packet)
//    {
//        // Extract RDO state message
//        auto rdoMsg = packet->peekAtFront<RDOStatePacket>();
//        if (!rdoMsg) {
//            EV_ERROR << "Failed to extract RDOStatePacket from packet" << endl;
//            return;
//        }
//
//        int sourceId = rdoMsg->getSourceId();
//        double stateValue = rdoMsg->getStateValue();
//
//        // Skip our own packets
//        if (sourceId == mySenderId) {
//            return;
//        }
//
//        // Store neighbor state
//        neighborStates[sourceId] = stateValue;
//
//        EV_INFO << "UE[" << mySenderId << "] received RDO state " << stateValue
//                << " from UE[" << sourceId << "]" << endl;
//    }
//
//    double UeApp::generateGradient()
//    {
//        // In a real implementation, this would be based on a specific optimization problem
//        // For this example, we use simple quadratic functions similar to the paper's example
//
//        // Get current state
//        double state = rdoAgent ? rdoAgent->getState() : 0.0;
//
//        // Simple quadratic cost function: f_i(x) = p_i * (x - q_i)^2
//        // Parameters from the paper's simulation
//        double p = 0.0;
//        double q = 0.0;
//
//        // Different parameters for different nodes (similar to paper's example)
//        if (mySenderId % 3 == 0) {
//            p = 10.0;
//            q = 60.0;
//        } else if (mySenderId % 3 == 1) {
//            p = 20.0;
//            q = 65.0;
//        } else {
//            p = 30.0;
//            q = 70.0;
//        }
//
//        // If this is an attacker with falsified data type during the attack period
//        if (isAttacker && myAttackType == ATTACK_TYPE_FALSIFIED &&
//            simTime() >= myAttackStartTime && simTime() < myAttackEndTime) {
//
//            // Generate falsified gradient to disrupt optimization
//            double falseGradient = 100.0 * sin(0.1 * simTime().dbl()) * myAttackSeverity;
//            return falseGradient;
//        }
//
//        // Gradient of f_i(x) = p_i * (x - q_i)^2 is 2 * p_i * (x - q_i)
//        double gradient = 2.0 * p * (state - q);
//
//        return gradient;
//    }
