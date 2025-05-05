#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <string>
#include <list>
#include <Packet.h>
#include <vector>
#include <cmath>
#include <iostream>

#include "FileDispacher.h"

#define DIR_NAME "sync_dir"
#define MAX_PACKET_SIZE 1024

using namespace std;

list<Packet> filePacking(string dir, const string fileName, bool syncFile, bool download)
{
    string fileName1 = fileName;
    if (fileName.find("/") != std::string::npos)
    {
        fileName1 = fileName.substr(fileName.find("/") + 1, fileName.size());
    }

    ifstream file(string(dir) + "/" + fileName1, std::ios::binary);

    if (!file.is_open())
    {
        cerr << "ERROR: couldn't open file." << endl;
    }

    MessageType type;

    if (syncFile)
    {
        type = MessageType::SYNC;
    }
    else if (download)
    {
        type = MessageType::DOWNLOAD;
    }
    else
    {
        type = MessageType::DATA;
    }

    list<Packet> filePackets;
    uint32_t id = 1;
    size_t bytesToRead = 0;
    std::streampos remaining_bytes = 0;

    remaining_bytes = file.tellg();
    file.seekg(0, std::ios::end);
    remaining_bytes = file.tellg() - remaining_bytes;
    file.seekg(0, std::ios::beg);

    while (remaining_bytes > 0)
    {
        if (remaining_bytes >= MAX_PACKET_SIZE)
        {
            bytesToRead = MAX_PACKET_SIZE;
            remaining_bytes -= MAX_PACKET_SIZE;
        }
        else
        {
            bytesToRead = remaining_bytes;
            remaining_bytes = 0;
        }
        vector<char> buffer(bytesToRead);

        file.read(buffer.data(), bytesToRead);
        std::streamsize readBytes = file.gcount();

        if (readBytes > 0)
        {
            Packet packet(id, 0, type, Status::SUCCESS, bytesToRead, buffer.data());
            filePackets.push_back(packet);
            id++;
        }
    }

    file.close();
    string finalFilename = syncFile ? fileName + ".sync" : fileName;
    Packet fileNamePacket(0, filePackets.size() + 1, type, Status::SUCCESS, finalFilename.size(), finalFilename.c_str());
    fileNamePacket.setMessage(finalFilename);
    filePackets.push_front(fileNamePacket);

    return filePackets;
}

void fileUnpacking(list<Packet> &packets, string dirName)
{

    string fileName = packets.front().getMessage();
    packets.pop_front();

    std::string fullPath = dirName + "/" + fileName;

    std::ofstream file(fullPath, std::ios::binary | std::ios::trunc);

    if (!file)
    {
        cerr << "ERROR: couldn't create or locate file." << endl;
        return;
    }

    for (const auto &packet : packets)
    {
        const char *message = packet.getMessage();
        uint16_t messageSize = packet.getMessageSize();

        file.write(message, messageSize);
    }

    file.close();
}