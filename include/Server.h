#include <netinet/in.h>
#include <optional>
#include <string>
#include <vector>
#include <thread>
#include <filesystem>
#include "Packet.h"

using namespace std;

class Server
{
private:
    int serverSocket = 0;
    int socketVizinho = 0;
    bool isParticipant;
    bool lostPrincipalServerConnection = false;
    bool hasNewServer = false;
    string ip;
    int porta;
    int totalBackupServers = 3;
    int electionSocket = 0;

    vector<thread> clientThreads;
    chrono::steady_clock::time_point lastHeartbeat;

    static void handle_client_activity(int socket_id);
    static void backup_process(int socket_id, int secundary);
    static void heartbeatRequest();
    void processHeartbeat(Packet receivedPacket, int socket_id);
    void processPacket(Packet receivedPacket, int socketId);
    void backupReceivePacket(int socket_id);
    void backupProcessElectionPacket(int socket_id);
    void checkLastHeartbeat(int socket_id);
    void startElection(string destination);
    void finishBakcupThread(Packet packet, int socket);

public:
    Server();
    ~Server();
    void start(string ip, int port);
    void startBackup(string &serverIp, int serverPort, string &principalServerIp, int principalServerPort);
    void createSyncDir();
    void setIp(string ip);
    void setPorta(int porta);
    string getIp();
    int getPorta();
};