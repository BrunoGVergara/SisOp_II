#ifndef FILEDISPATCHER_H
#define FILEDISPATCHER_H

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <string>
#include <list>
#include <Packet.h>


#define MAX_PACKET_SIZE 1024

using namespace std;

list<Packet> filePacking(string dir, string fileName, bool syncFile, bool download);
void fileUnpacking(list<Packet> &packets, string dirName);

#endif