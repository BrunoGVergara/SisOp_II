#include "Packet.h"

Packet::Packet(uint32_t id, uint32_t totalPackets, MessageType type, Status status, uint16_t messageSize, const char *msg)
    : packetId(id), totalPackets(totalPackets), messageType(type), status(status), messageSize(messageSize)
{
    message = new char[messageSize];
    std::memcpy(message, msg, messageSize);
}

Packet::Packet()
    : packetId(0), totalPackets(0), messageType(MessageType::CONNECTION), status(Status::ERROR), messageSize(0), message(nullptr)
{
}

uint32_t Packet::getPacketId() const { return packetId; }
uint32_t Packet::getTotalPackets() const { return totalPackets; }
MessageType Packet::getMessageType() const { return messageType; }
uint16_t Packet::getMessageSize() const { return messageSize; }
const char *Packet::getMessage() const { return message; }

void Packet::setMessage(const std::string &msg)
{
    delete[] message;
    message = new char[msg.size()];
    memcpy(message, msg.c_str(), msg.size());
    messageSize = msg.size();
}

bool Packet::isStatusError()
{
    return status == Status::ERROR;
}

bool Packet::isConnectionPacket()
{
    return messageType == MessageType::CONNECTION;
}

bool Packet::isDisconnectionPacket()
{
    return messageType == MessageType::DISCONNECTION;
}

bool Packet::isDataPacket()
{
    return messageType == MessageType::DATA;
}

bool Packet::isSyncPacket()
{
    return messageType == MessageType::SYNC;
}

bool Packet::isDeletePacket()
{
    return messageType == MessageType::DELETE;
}

bool Packet::isFetchPacket()
{
    return messageType == MessageType::FETCH;
}

bool Packet::isDownloadPacket()
{
    return messageType == MessageType::DOWNLOAD;
}

bool Packet::isInfoPacket()
{
    return messageType == MessageType::INFO;
}

bool Packet::isConnectionServer()
{
    return messageType == MessageType::CONNECTION_SERVER;
}

bool Packet::isDisconnectionServer()
{
    return messageType == MessageType::DISCONNECTION_SERVER;
}
bool Packet::isClientPacket()
{
    return messageType == MessageType::CLIENT;
}
bool Packet::isSocketPacket()
{
    return messageType == MessageType::SOCKET;
}
bool Packet::isIpPacket()
{
    return messageType == MessageType::IP;
}

bool Packet::isHeartbeatPacket()
{
    return messageType == MessageType::HEARTBEAT;
}

bool Packet::isElected()
{
    return messageType == MessageType::ELECTED;
}

bool Packet::isVoteElection()
{
    return messageType == MessageType::VOTE_ELECTION;
}

void Packet::setStatus(Status status)
{
    this->status = status;
}

void Packet::deserialize(const char *buffer)
{
    size_t offset = 0;

    memcpy(&packetId, buffer + offset, sizeof(packetId));
    offset += sizeof(packetId);

    memcpy(&totalPackets, buffer + offset, sizeof(totalPackets));
    offset += sizeof(totalPackets);

    uint8_t msgType;
    memcpy(&msgType, buffer + offset, sizeof(msgType));
    messageType = static_cast<MessageType>(msgType);
    offset += sizeof(msgType);

    int8_t status;
    memcpy(&status, buffer + offset, sizeof(status));
    this->status = static_cast<Status>(status);
    offset += sizeof(status);

    memcpy(&messageSize, buffer + offset, sizeof(messageSize));
    offset += sizeof(messageSize);

    if (message != nullptr)
    {
        delete[] message;
    }
    message = new char[messageSize + 1];
    memcpy(message, buffer + offset, messageSize);
    message[messageSize] = '\0';
}

const char *Packet::serialize() const
{
    char *buffer = new char[sizeof(packetId) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(int8_t) + sizeof(messageSize) + messageSize];
    size_t offset = 0;

    memcpy(buffer + offset, &packetId, sizeof(packetId));
    offset += sizeof(packetId);

    memcpy(buffer + offset, &totalPackets, sizeof(totalPackets));
    offset += sizeof(totalPackets);

    uint8_t msgType = static_cast<uint8_t>(messageType);
    memcpy(buffer + offset, &msgType, sizeof(msgType));
    offset += sizeof(msgType);

    int8_t stat = static_cast<int8_t>(status);
    memcpy(buffer + offset, &stat, sizeof(stat));
    offset += sizeof(stat);

    memcpy(buffer + offset, &messageSize, sizeof(messageSize));
    offset += sizeof(messageSize);

    memcpy(buffer + offset, message, messageSize);

    return buffer;
}

size_t Packet::size() const
{
    return sizeof(packetId) + sizeof(totalPackets) + sizeof(uint8_t) + sizeof(int8_t) +
           sizeof(messageSize) + messageSize;
}

size_t Packet::headerSize() const
{
    return sizeof(packetId) + sizeof(totalPackets) + sizeof(uint8_t) + sizeof(int8_t) +
           sizeof(messageSize);
}