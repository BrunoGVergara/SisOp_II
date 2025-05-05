#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <cstring>
#include <iostream>

enum class MessageType
{
    CONNECTION = 1,
    DATA = 2,
    DISCONNECTION = 3,
    SYNC = 4,
    DELETE = 5,
    FETCH = 6,
    DOWNLOAD = 7,
    INFO = 8,
    CONNECTION_SERVER = 9,
    DISCONNECTION_SERVER = 10,
    CLIENT = 11,
    SOCKET = 12,
    IP = 13,
    HEARTBEAT = 14,
    ELECTED = 15,
    VOTE_ELECTION = 16
};

enum class Status
{
    SUCCESS = 1,
    ERROR = -1
};

class Packet
{
private:
    uint32_t packetId;
    uint32_t totalPackets;
    MessageType messageType;
    Status status;
    uint16_t messageSize;
    char *message;

public:
    Packet();
    Packet(uint32_t id, uint32_t totalPackets, MessageType type, Status status, uint16_t messageSize, const char *msg);

    uint32_t getPacketId() const;
    uint32_t getTotalPackets() const;
    MessageType getMessageType() const;
    uint16_t getMessageSize() const;
    const char *getMessage() const;

    void setStatus(Status status);
    void setMessage(const std::string& msg);

    const char *serialize() const;
    void deserialize(const char *buffer);

    bool isStatusError();
    bool isDataPacket();
    bool isConnectionPacket();
    bool isDisconnectionPacket();
    bool isSyncPacket();
    bool isDeletePacket();
    bool isFetchPacket();
    bool isDownloadPacket();
    bool isInfoPacket();
    bool isConnectionServer();
    bool isDisconnectionServer();
    bool isClientPacket();
    bool isSocketPacket();
    bool isIpPacket();
    bool isHeartbeatPacket();
    bool isVoteElection();
    bool isElected();


    size_t size() const;
    size_t headerSize() const;
};

#endif
