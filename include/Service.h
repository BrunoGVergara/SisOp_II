#ifndef SERVICE_H
#define SERVICE_H

#include <string>
#include <sys/socket.h>
#include <Packet.h>
#include <list>
#include <filesystem>
#include <string.h>
#include <sys/stat.h>
#include <chrono>


#include "FileDispacher.h"

#include <Service.h>
#include <optional>

using namespace std;

void sendPacket(int socketdId, Packet packet);
Packet receivePacket(int socket_id);
void receiveFile(Packet packet, int socket_id, optional<string> username, string dirName);
void sendFile(int socket_id, string dir, string filename);
void sendFile(int socket_id, string dir, string filename, bool syncFile, bool download);
void deleteFile(string filePath);
void getSyncDir(int socket_id, string username);
void syncFiles(int socket_id, string dir, string username);
void sendClients(int socket_id, string clientSocket, string username);
string listfFilesInfo(string dir);
int startSocket(string ip, int port);
int connectToSocket(string ip, int port);

#endif
