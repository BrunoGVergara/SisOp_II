#ifndef CLIENT_H
#define CLIENT_H
#include <netinet/in.h>
#include <string>
#include <atomic>

using namespace std;

class Client
{
private:
    string username;
    string ip;
    string serverIP;
    int serverPort;
    int clientSocket = 0;
    int deamonSocket = 0;
    std::atomic<bool> stopRequested{false};
    void connectToServer();
    void createSyncDir();
    void createClientDownloadDir();
    void processCommand(const string commandLine);
    void signalHandler(int signal);

public:
    Client();
    Client(string username, string clientIp, string serverIp, int serverPort);
    ~Client();
    void setUsername(const std::string &user);
    string getUsername();
    int getSocketId();
    void sendMessage();
    void run();
    void sync();
    void cli();
    void startDeamon();
};

#endif