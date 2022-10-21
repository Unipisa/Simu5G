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

import inet.common.packet.chunk.Chunk;

class FLaasPacket extends inet::FieldsChunk {
    int type;
    int roundId;
    int learnerId;
}

class FLaasStartRoundPacket extends FLaasPacket {
        double roundDuration;
        bool response;
}

class FLaaSGlobalModelPacket extends FLaasPacket {
    double timeRemaining;
    double modelSize;
}

class FLaaSTrainedModelPacket extends FLaasPacket {
    double modelSize;
}

class LearnerDeploymentPacket extends FLaasPacket {
    int deploymentPosition;
    string ueModuleName;
    string learnerModuleName;
    string ipAddress; 
    int port;
}

class DataForTrainingPacket extends FLaasPacket { 
    int dimension;
}




