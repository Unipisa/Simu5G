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


#include "PingPongPacketSerializer.h"

#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include <string>

#include "apps/PingPong/packets/PingPongPacket_m.h"
#include "apps/PingPong/packets/PingPongPacket_Types.h"
#include "apps/PingPong/packets/PingPongPacket_Types.h"

#include <iostream> // used to debug in emulation

namespace inet {

Register_Serializer(PingPongAppPacket, PingPongPacketSerializer);
Register_Serializer(PingPongStartPacket, PingPongPacketSerializer);
Register_Serializer(PingPongPacket, PingPongPacketSerializer);


void PingPongPacketSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    EV << "WarningAlertPacketSerializer::serialize" << endl;

    auto startPosition = stream.getLength();
    auto devAppPacket = staticPtrCast<const PingPongAppPacket>(chunk);
    std::string  ss = devAppPacket->getType();

    if(strcmp(ss.c_str(), START_WARNING) == 0)
    {
        auto startPk = staticPtrCast<const PingPongStartPacket>(chunk);
        stream.writeByte((uint8_t) 0);

        double xCoord = startPk->getCenterPositionX();
        double yCoord = startPk->getCenterPositionY();
        double radius = startPk->getRadius();

        std::stringstream coords;
        coords << xCoord <<"," << yCoord << ","<< radius;
        std::string scoords = coords.str();
        stream.writeByte((uint8_t) scoords.size());
        stream.writeString(scoords);

        int64_t remainders = B(startPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(startPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if(strcmp(ss.c_str(), STOP_WARNING) == 0)
    {
        auto stopPk = staticPtrCast<const PingPongStopPacket>(chunk);
        stream.writeByte((uint8_t) 1); // CODE
        stream.writeByte((uint8_t) 0); // Length

        int64_t remainders = B(stopPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(stopPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if(strcmp(ss.c_str(), WARNING_ALERT) == 0)
    {
        auto alertPk = staticPtrCast<const PingPongPacket>(chunk);
        stream.writeByte((uint8_t) 2);




        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if(strcmp(ss.c_str(),START_PINGPONG ) == 0)
    {
        auto alertPk = staticPtrCast<const PingPongPacket>(chunk);
        stream.writeByte((uint8_t) 2);




        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        //if (remainders < 0)
         //   throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }

    else if(strcmp(ss.c_str(), ANS_PINGPONG) == 0)
    {
        auto alertPk = staticPtrCast<const PingPongPacket>(chunk);
        stream.writeByte((uint8_t) 2);




        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        //if (remainders < 0)
         //   throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }

    else if(strcmp(ss.c_str(),PINGPONG_PACKET ) == 0)
    {
        auto alertPk = staticPtrCast<const PingPongPacket>(chunk);
        stream.writeByte((uint8_t) 2);




        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        //if (remainders < 0)
         //   throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }


    else if(strcmp(ss.c_str(), START_ACK) == 0)
    {
        auto alertPk = staticPtrCast<const PingPongPacket>(chunk);
        stream.writeByte((uint8_t) 3);
        int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
        if (remainders < 0)
            throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
        stream.writeByteRepeatedly('?', remainders);
    }
    else if(strcmp(ss.c_str(), START_NACK) == 0)
        {
            auto alertPk = staticPtrCast<const PingPongPacket>(chunk);
            stream.writeByte((uint8_t) 4);
            int64_t remainders = B(alertPk->getChunkLength() - (stream.getLength() - startPosition)).get();
            if (remainders < 0)
                throw cRuntimeError("Warning application packet length = %d smaller than required %d bytes", (int)B(alertPk->getChunkLength()).get(), (int)B(stream.getLength() - startPosition).get());
            stream.writeByteRepeatedly('?', remainders);
        }
    else
    {
        throw cRuntimeError("WarningAlertPacketSerializer - WarningAlertPacket type %s not recognized", ss.c_str());
    }

}

const Ptr<Chunk> PingPongPacketSerializer::deserialize(MemoryInputStream& stream) const
{
    EV << "WarningAlertPacketSerializer::deserialize" << endl;

    B totalLength = stream.getLength();

    int messageCode = (int)(stream.readByte());

    std::vector<uint8_t> bytes;

    switch (messageCode)
    {
        case 0: // START
        {
            auto startPacket = makeShared<PingPongStartPacket>();

            startPacket->setType(START_WARNING);
            //std::cout << "type " << startPacket->getType()<< endl;
            B messageDataLength = B(stream.readByte());
            std::vector<uint8_t> bytes;
            stream.readBytes(bytes, messageDataLength);
            std::string data(bytes.begin(), bytes.end());

            std::vector<std::string> coords = cStringTokenizer((char*)&bytes[0], ",").asVector();
            if(coords.size() != 3)
                throw cRuntimeError("WarningAlertPacketSerializer::deserialize - start application message must be in form x,y,radius. But is %s", data.c_str());

            double x = std::stod(coords[0]);
            double y = std::stod(coords[1]);
            double c = std::stod(coords[2]);

            startPacket->setCenterPositionX(x);
            startPacket->setCenterPositionY(y);
            startPacket->setRadius(c);

            return startPacket;
            break;
        }
        case 1: // STOP
        {
            auto stopPacket = makeShared<PingPongStopPacket>();
            stopPacket->setType(STOP_WARNING);
            return stopPacket;
            break;
        }
        case 2: // ALERT
        {
            auto alertPacket = makeShared<PingPongPacket>();
            alertPacket->setType(WARNING_ALERT);

            B messageDataLength = B(stream.readByte());
            bool res = (bool)(stream.readByte());
            std::vector<uint8_t> bytes;
            stream.readBytes(bytes, messageDataLength);
            std::string data(bytes.begin(), bytes.end());

            std::vector<std::string> coords = cStringTokenizer((char*)&bytes[0], ",").asVector();

            if(coords.size() != 2)
                throw cRuntimeError("WarningAlertPacketSerializer::deserialize - WARNING application message must be in form x,y. But is %s", data.c_str());


//            throw cRuntimeError("len %d, pos %d", B(stream.getLength()).get(),B(stream.getPosition()).get());
            B remainders = totalLength - (stream.getPosition());
            ASSERT(remainders >= B(0));
            stream.readByteRepeatedly('?', remainders.get());

            return alertPacket;

            break;
        }

        case 3 : // ACK START
        {
            auto ackStartPacket = makeShared<PingPongAppPacket>();
            ackStartPacket->setType(START_ACK);

            B remainders = totalLength - (stream.getPosition());
            ASSERT(remainders >= B(0));
            stream.readByteRepeatedly('?', remainders.get());

            return ackStartPacket;
            break;

        }
        case 4 : // NACK START
        {
            auto nackStartPacket = makeShared<PingPongAppPacket>();
            nackStartPacket->setType(START_NACK);

            B remainders = totalLength - (stream.getPosition());
            ASSERT(remainders >= B(0));
            stream.readByteRepeatedly('?', remainders.get());

            return nackStartPacket;
            break;
        }
        case 69: // ??????????????????????????????????????????
               {
                   auto alertPacket = makeShared<PingPongPacket>();
                   alertPacket->setType(PINGPONG_PACKET);

                   B messageDataLength = B(stream.readByte());
                   bool res = (bool)(stream.readByte());
                   std::vector<uint8_t> bytes;
                   stream.readBytes(bytes, messageDataLength);
                   std::string data(bytes.begin(), bytes.end());

                   std::vector<std::string> coords = cStringTokenizer((char*)&bytes[0], ",").asVector();

                   //if(coords.size() != 2)
                   //    throw cRuntimeError("WarningAlertPacketSerializer::deserialize - WARNING application message must be in form x,y. But is %s", data.c_str());


       //            throw cRuntimeError("len %d, pos %d", B(stream.getLength()).get(),B(stream.getPosition()).get());
                   B remainders = totalLength - (stream.getPosition());
                   ASSERT(remainders >= B(0));
                   stream.readByteRepeatedly('?', remainders.get());

                   return alertPacket;

                   break;
               }



        default:
        {
            throw cRuntimeError("WarningAlertPacketSerializer::deserialize - Code %d not recognized!", messageCode);
        }
    }

}

} // namespace inet

