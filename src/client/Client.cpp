#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <filesystem>
#include <csignal>

#include <Client.h>
#include <Packet.h>
#include <Service.h>
#include <thread>
#include "Notify.h"

#define DIR_NAME "sync_dir"

using namespace std;

Client::Client() : clientSocket(-1)
{
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        cerr << "Erro ao criar o socket." << endl;
        exit(EXIT_FAILURE);
    }
}

Client::Client(string username, string clientIp, string serverIp, int serverPort)
{
    this->username = username;
    this->ip = clientIp;
    this->serverIP = serverIp;
    this->serverPort = serverPort;
}

Client::~Client()
{
    close(clientSocket);
}

void Client::connectToServer()
{
    cout << "serverIP: " << serverIP << " server port " << serverPort << endl;

    int newClientSocket = connectToSocket(serverIP, serverPort);

    string message = username + ":" + ip;

    Packet packet(1, 1, MessageType::CONNECTION, Status::SUCCESS, message.size(), message.c_str());
    sendPacket(newClientSocket, packet);
    packet = receivePacket(newClientSocket);

    if (packet.isStatusError())
    {
        throw invalid_argument("Erro ao conectar ao servidor.");
    }

    cout << "Conectado ao servidor!" << newClientSocket << endl;
    clientSocket = newClientSocket;
    if (deamonSocket == 0)
    {
        thread watcherThread4([this]()
                              { startDeamon(); });
        watcherThread4.detach();
    }
}

void Client::createSyncDir()
{
    if (!filesystem::exists(DIR_NAME))
        filesystem::create_directory(DIR_NAME);
}

void Client::createClientDownloadDir()
{
    if (!filesystem::exists("downloads"))
        filesystem::create_directory("downloads");
}

void Client::sendMessage()
{
    while (true)
    {
    }
}

void Client::run()
{
    createSyncDir();
    createClientDownloadDir();
    connectToServer();
    Notify notify(this);

    thread watcherThread1(&Notify::init, &notify);
    thread watcherThread2(&Client::sync, this);
    thread watcherThread3(&Client::cli, this);

    watcherThread1.join();
    watcherThread2.join();
    watcherThread3.join();
}

void Client::startDeamon()
{
    cout << ip << endl;
    deamonSocket = startSocket(ip, 9090);
    while (true)
    {
        int socket_id = accept(deamonSocket, nullptr, nullptr);
        if (socket_id < 0)
        {
            cerr << "Erro ao aceitar conexão." << endl;
            continue;
        }

        Packet receivedPacket = receivePacket(socket_id);
        string fullMessage = receivedPacket.getMessage();
        string newServerIp = fullMessage.substr(0, fullMessage.find(':'));
        string newServerPort = fullMessage.substr(fullMessage.find(':') + 1, fullMessage.size());

        this->serverIP = newServerIp;
        this->serverPort = stoi(newServerPort);

        cout << "Conexão aceita" << endl;

        close(socket_id);

        connectToServer();
    }
}

string Client::getUsername()
{
    return this->username;
}

int Client::getSocketId()
{
    return this->clientSocket;
}

void Client::sync()
{
    Packet packet(0, 1, MessageType::FETCH, Status::SUCCESS, username.size(), username.c_str());
    sendPacket(clientSocket, packet);
    while (true)
    {
        Packet receivedPacket = receivePacket(clientSocket);
        if (receivedPacket.isSyncPacket())
        {
            receiveFile(receivedPacket, clientSocket, nullopt, "sync_dir");
        }
        else if (receivedPacket.isDeletePacket())
        {
            string path = "sync_dir/" + string(receivedPacket.getMessage());
            deleteFile(path);
        }
        else if (receivedPacket.isDownloadPacket())
        {
            string path = "download/" + string(receivedPacket.getMessage());
            receiveFile(receivedPacket, clientSocket, nullopt, "downloads");
        }
        else if (receivedPacket.isInfoPacket())
        {
            cout << receivedPacket.getMessage() << endl;
        }
        else if (receivedPacket.isDisconnectionPacket())
        {
            stopRequested = true;
            exit(0);
        }
    }
}

void Client::processCommand(const string commandLine)
{
    istringstream stream(commandLine);
    string command, argument;
    stream >> command;
    getline(stream, argument);

    if (!argument.empty())
    {
        size_t pos = argument.find_first_not_of(" ");
        if (pos != std::string::npos)
        {
            argument = argument.substr(pos);
        }
        else
        {
            argument.clear();
        }
    }

    if (command == "upload")
    {
        if (!argument.empty())
        {
            sendFile(clientSocket, "..", argument);
        }
        else
        {
            cout << "Erro: O nome do arquivo é obrigatório.\n";
        }
    }
    else if (command == "download")
    {
        if (!argument.empty())
        {
            Packet packet(0, 1, MessageType::DOWNLOAD, Status::SUCCESS, argument.size(), argument.c_str());
            sendPacket(clientSocket, packet);
        }
        else
        {
            cout << "Erro: O nome do arquivo é obrigatório.\n";
        }
    }
    else if (command == "delete")
    {
        if (!argument.empty())
        {
            Packet packet(0, 1, MessageType::DELETE, Status::SUCCESS, argument.size(), argument.c_str());
            sendPacket(clientSocket, packet);
            string path = "sync_dir/" + argument;
            remove(path.c_str());
        }
        else
        {
            cout << "Erro: O nome do arquivo é obrigatório.\n";
        }
    }
    else if (command == "list_server")
    {
        Packet packet(0, 1, MessageType::INFO, Status::SUCCESS, username.size(), username.c_str());
        sendPacket(clientSocket, packet);
    }
    else if (command == "list_client")
    {
        cout << listfFilesInfo("sync_dir") << endl;
    }
    else if (command == "get_sync_dir")
    {
        getSyncDir(clientSocket, username);
    }
    else if (command == "exit")
    {
        Packet packet(0, 1, MessageType::DISCONNECTION, Status::SUCCESS, 0, "");
        sendPacket(clientSocket, packet);
        exit(0);
    }
    else
    {
        cerr << "Error: Unknown command.\n";
    }
}

void Client::cli()
{

    std::string input;

    while (!stopRequested)
    {
        std::cout << "> ";
        std::getline(std::cin, input);

        processCommand(input);
    }
}