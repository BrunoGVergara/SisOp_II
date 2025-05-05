#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <filesystem>

#include <Server.h>
#include <Service.h>
#include <Util.h>
#include <Packet.h>
#include <global_settings.h>
#include <arpa/inet.h>

#define DIR_NAME "dir"
#define PORT 5000

using namespace std;

std::mutex serverSocketMutex;

void createDir(const char *dirName)
{
    if (!std::filesystem::exists(dirName))
        std::filesystem::create_directory(dirName);
}

Server::Server()
{
    lerArquivo();
    createDir(DIR_NAME);
    lastHeartbeat = chrono::steady_clock::now();
}

Server::~Server()
{
    close(serverSocket);

    for (auto &th : clientThreads)
    {
        if (th.joinable())
        {
            th.join();
        }
    }
}

void Server::start(string ip, int port)
{
    setIp(ip);
    setPorta(port);
    serverSocketMutex.lock();
    serverSocket = startSocket(ip, port);

    std::thread heartbeatThread(heartbeatRequest);
    heartbeatThread.detach();
    serverSocketMutex.unlock();

    while (true)
    {
        int socket_id = accept(serverSocket, nullptr, nullptr);
        if (socket_id < 0)
        {
            cerr << "Erro ao aceitar conexão.1" << endl;
            continue;
        }
        clientThreads.emplace_back([this, socket_id]()
                                   { this->handle_client_activity(socket_id); });
    }
}

void Server::backupReceivePacket(int socket_id)
{
    while (!lostPrincipalServerConnection)
    {
        Packet receivedPacket = receivePacket(socket_id);
        processHeartbeat(receivedPacket, socket_id);
        processPacket(receivedPacket, socket_id);
    }
}

void Server::finishBakcupThread(Packet packet, int socket)
{
    hasNewServer = true;
    lostPrincipalServerConnection = false;
    sendPacket(socket, packet);
}

void Server::backupProcessElectionPacket(int socket_id)
{
    while (lostPrincipalServerConnection)
    {
        Packet receivedPacket = receivePacket(socket_id);
        string ip = getIp();
        string porta = to_string(getPorta());
        string origem = ip + ":" + porta;
        string destination = buscarDestino(origem);
        int serverId = stoi(destination.substr(destination.find('-') + 1, destination.size()));

        if (socketVizinho == 0)
        {
            string ipVizinho = destination.substr(0, destination.find(':'));
            string portaVizinho = destination.substr(destination.find(':') + 1, destination.find('-'));

            socketVizinho = connectToSocket(ipVizinho, stoi(portaVizinho));
        }

        if (receivedPacket.isElected())
        {

            totalBackupServers -= 1;
            string ip = getIp();
            lostPrincipalServerConnection = false;
            string message = receivedPacket.getMessage();
            string destinationIp = message.substr(0, message.find(':'));
            string destinationPort = message.substr(message.find(':') + 1, message.size());
            hasNewServer = true;
            cout << "Servidor primário reconectado" << endl;
            startBackup(ip, getPorta(), destinationIp, 8085); // Mudar para 8080 no lab
        }
        else if (receivedPacket.isVoteElection())
        {
            int receivedServerId = stoi(receivedPacket.getMessage());

            if (serverId > receivedServerId)
            {
                if (!isParticipant)
                {
                    receivedPacket = Packet(1, 1, MessageType::VOTE_ELECTION, Status::SUCCESS, to_string(serverId).size(), to_string(serverId).c_str());
                    sendPacket(socketVizinho, receivedPacket);
                    isParticipant = true;
                }
            }

            else if (serverId == receivedServerId)
            {

                vector<string> usernames = global_settings::client_name_dictionary.keys();
                for (string username : usernames)
                {
                    global_settings::client_name_dictionary.remove(username);
                }
                string message = ip + ":" + porta;
                serverSocketMutex.lock();
                receivedPacket = Packet(1, 1, MessageType::ELECTED, Status::SUCCESS, message.size(), message.c_str());
                std::thread startThread([this, ip]()
                                        { 
                                            close(serverSocket);
                                            this->start(ip, 8085); });

                std::thread finishfinish([this, receivedPacket]()
                                         { this->finishBakcupThread(receivedPacket, socketVizinho); });

                serverSocketMutex.unlock();
                cout << "Fui eleito" << endl;

                vector<string> ips = global_settings::client_ip.keys();
                for (string ip : ips)
                {
                    int socket = connectToSocket(ip, 9090);
                    cout << "socketCliente " << socket << endl;
                    string message = this->ip + ":8085";
                    Packet packet(1, 1, MessageType::CONNECTION, Status::SUCCESS, message.size(), message.c_str());
                    sleep(1);
                    sendPacket(socket, packet);
                }
                finishfinish.join();
                startThread.join();
            }
            else if (serverId < receivedServerId)
            {
                sendPacket(socketVizinho, receivedPacket);
                isParticipant = false;
            }
        }
    }
}

void Server::checkLastHeartbeat(int socket_id)
{
    lastHeartbeat = chrono::steady_clock::now();
    while (!lostPrincipalServerConnection)
    {
        if (lastHeartbeat + chrono::seconds(15) < chrono::steady_clock::now())
        {
            string ip = getIp();
            string porta = to_string(getPorta());
            string origem = ip + ":" + porta;
            cout << "Servidor primário desconectado" << endl;
            if (totalBackupServers == 2)
            {
                cout << "Único servidor secundário, fui eleito" << endl;
                std::thread startThread([this, ip]()
                                        { this->start(ip, 8085); });
                startThread.detach();
                vector<string> ips = global_settings::client_ip.keys();
                sleep(1);
                for (string ip : ips)
                {
                    int socket = connectToSocket(ip, 9090);
                    cout << "socketCliente " << socket << endl;
                    string message = this->ip + ":8085";
                    Packet packet(1, 1, MessageType::CONNECTION, Status::SUCCESS, message.size(), message.c_str());
                    sleep(1);
                    sendPacket(socket, packet);
                }
            }
            else
            {
                cout << "Iniciando eleição" << endl;
                string destino = buscarDestino(origem);
                lostPrincipalServerConnection = true;
                startElection(destino);
            }
            break;
        }
    }
}

void Server::startBackup(string &serverIp, int serverPort, string &principalServerIp, int principalServerPort)
{
    setIp(serverIp);
    setPorta(serverPort);

    printf("%s, %d\n", principalServerIp.c_str(), principalServerPort);

    int newClientSocket = connectToSocket(principalServerIp, principalServerPort);

    Packet packet(1, 1, MessageType::CONNECTION_SERVER, Status::SUCCESS, serverIp.size(), serverIp.c_str());
    sendPacket(newClientSocket, packet);

    Packet receivedPacket = receivePacket(newClientSocket);

    if (!receivedPacket.isStatusError())
    {
        createDir(DIR_NAME);

        if (hasNewServer)
        {
            isParticipant = false;
            lastHeartbeat = chrono::steady_clock::now();
            lostPrincipalServerConnection = false;
            hasNewServer = false;
        }

        std::thread client_activity([this, newClientSocket]()
                                    { this->backupReceivePacket(newClientSocket); });
        std::thread checkHeartbeat([this, newClientSocket]()
                                   { this->checkLastHeartbeat(newClientSocket); });
        client_activity.detach();
        checkHeartbeat.detach();

        string ip = getIp();

        if (serverSocket == 0)
        {
            string ip = getIp();
            serverSocket = startSocket(ip, 8888);
            while (!hasNewServer)
            {
                int socket_id = accept(serverSocket, nullptr, nullptr);
                if (socket_id < 0)
                {
                    cerr << "Erro ao aceitar conexão.2" << endl;
                    continue;
                }
                lostPrincipalServerConnection = true;
                cout << "Conexão aceita" << endl;
                backupProcessElectionPacket(socket_id);
            }
        }
    }
    else
    {
        cout << "Erro ao conectar ao servidor principal" << endl;
    }
}

void Server::startElection(string destination)
{

    string ipVizinho = destination.substr(0, destination.find(':'));
    string portaVizinho = destination.substr(destination.find(':') + 1, destination.find('-'));
    string serverId = destination.substr(destination.find('-') + 1, destination.size());

    socketVizinho = connectToSocket(ipVizinho, stoi(portaVizinho));
    Packet packet(1, 1, MessageType::VOTE_ELECTION, Status::SUCCESS, serverId.size(), serverId.c_str());
    sendPacket(socketVizinho, packet);
    cout << "socketVizinho " << socketVizinho << endl;
}

void sendClientInfo(int socketId, string info1, string info2, MessageType messageType)
{
    string message = info1 + ":" + info2;
    Packet packet(1, 1, messageType, Status::SUCCESS, message.size(), message.c_str());
    sendPacket(socketId, packet);
}

void Server::heartbeatRequest()
{
    while (true)
    {
        vector<string> secundaryIps = global_settings::servers.keys();
        for (string secundaryIp : secundaryIps)
        {
            int secondarySocketId = global_settings::servers.get(secundaryIp);
            // cout << "Heartbeat enviado " << to_string(secondarySocketId) << endl;
            Packet packet(1, 1, MessageType::HEARTBEAT, Status::SUCCESS, 0, "");
            try
            {
                sendPacket(secondarySocketId, packet);
            }
            catch (const std::exception &e)
            {
                global_settings::servers.remove(secundaryIp);
                // cout << "Servidor secundário desconectado" << endl;
            }
        }
        this_thread::sleep_for(chrono::seconds(5));
    }
}

void Server::processHeartbeat(Packet receivedPacket, int socket_id)
{
    if (receivedPacket.isHeartbeatPacket())
    {
        lastHeartbeat = chrono::steady_clock::now();
        cout << "Heartbeat recebido " << to_string(socket_id) << endl;
    }
}

void Server::processPacket(Packet receivedPacket, int socket_id)
{
    if (receivedPacket.isDataPacket())
    {
        string username = global_settings::socket_id_dictionary.get(socket_id);
        receiveFile(receivedPacket, socket_id, username, DIR_NAME);
    }
    else if (receivedPacket.isIpPacket())
    {
        string fullMesage = receivedPacket.getMessage();
        string socketId = fullMesage.substr(0, fullMesage.find(':'));
        string ip = fullMesage.substr(fullMesage.find(':') + 1, fullMesage.size());
        cout << "ip " << ip << endl;
        global_settings::client_ip.insert_or_update(ip, ip);
    }
    else if (receivedPacket.isClientPacket())
    {
        string fullMessage = receivedPacket.getMessage();
        string username = fullMessage.substr(0, fullMessage.find(':'));
        string qtd_str = fullMessage.substr(fullMessage.find(':') + 1, fullMessage.size());
        cout << "Recebido username: " << fullMessage << endl;
        int qtd = stoi(qtd_str);
        global_settings::client_name_dictionary.insert_or_update(username, qtd);
        string dirName = "dir/" + username;
        createDir(dirName.c_str());
    }
    else if (receivedPacket.isDeletePacket())
    {

        remove(receivedPacket.getMessage());
    }
}

void Server::handle_client_activity(int socket_id)
{
    while (true)
    {

        Packet receivedPacket = receivePacket(socket_id);

        if (receivedPacket.isConnectionPacket())
        {
            string fullMessage = receivedPacket.getMessage();
            string username = fullMessage.substr(0, fullMessage.find(':'));
            string clientIp = fullMessage.substr(fullMessage.find(':') + 1, fullMessage.size());

            bool success = global_settings::connect_client(socket_id, username, clientIp);
            std::string message = success ? "Conexão bem-sucedida." : "Erro ao conectar.";

            if (success)
            {
                Packet replyPacket(1, 1, MessageType::CONNECTION, Status::SUCCESS, message.size(), message.c_str());
                string userDirFolderName = string(DIR_NAME) + "/" + username;
                createDir(userDirFolderName.c_str());
                sendPacket(socket_id, replyPacket);

                vector<string> secundaryIps = global_settings::servers.keys();
                for (string secundaryIp : secundaryIps)
                {
                    int secondarySocketId = global_settings::servers.get(secundaryIp);
                    sendClientInfo(secondarySocketId, username, to_string(socket_id), MessageType::CLIENT);
                    sendClientInfo(secondarySocketId, clientIp, clientIp, MessageType::IP);
                }
            }
            else
            {
                cout << "Erro ao conectar" << endl;
                Packet replyPacket(1, 1, MessageType::DISCONNECTION, Status::ERROR, 0, "");
                sendPacket(socket_id, replyPacket);
                break;
            }
        }
        else if (receivedPacket.isDisconnectionPacket())
        {
            global_settings::disconnect_client(socket_id, global_settings::socket_id_dictionary.get(socket_id));
            vector<string> secundaryIps = global_settings::servers.keys();
            for (string secundaryIp : secundaryIps)
            {
                int secondarySocketId = global_settings::servers.get(secundaryIp);
                sendPacket(secondarySocketId, receivedPacket);
            }
            break;
        }
        else if (receivedPacket.isDataPacket())
        {
            string username = global_settings::socket_id_dictionary.get(socket_id);
            receiveFile(receivedPacket, socket_id, username, "dir");
            auto syncDeviceSocket = global_settings::socket_id_dictionary.findFirstDifferentValue(username, socket_id);
            if (syncDeviceSocket)
            {
                string filename = receivedPacket.getMessage();
                string dirName = "dir/" + username;
                sendFile(*syncDeviceSocket, dirName, filename, true, false);
            }

            // Tornar isso uma função, por hora replicar nos demais ifs
            // pensando bem tem muita coisa pra refatorar nesse método.

            vector<string> secundaryIps = global_settings::servers.keys();
            for (string secundaryIp : secundaryIps)
            {
                int secundarySocketId = global_settings::servers.get(secundaryIp);
                string filename = receivedPacket.getMessage();
                string dirName = "dir/" + username;
                string message = username + "/" + filename;
                sendFile(secundarySocketId, dirName, message, false, false);
            }
        }
        else if (receivedPacket.isDeletePacket())
        {

            string username = global_settings::socket_id_dictionary.get(socket_id);
            auto syncDeviceSocket = global_settings::socket_id_dictionary.findFirstDifferentValue(username, socket_id);
            string path = "dir/" + username + "/" + receivedPacket.getMessage();
            remove(path.c_str());
            if (syncDeviceSocket)
            {
                sendPacket(*syncDeviceSocket, receivedPacket);
            }

            vector<string> secundaryIps = global_settings::servers.keys();
            for (string secundaryIp : secundaryIps)
            {
                int secundarySocketId = global_settings::servers.get(secundaryIp);
                string message = "dir/" + username + "/" + receivedPacket.getMessage();
                Packet packet(1, 1, MessageType::DELETE, Status::SUCCESS, message.size(), message.c_str());
                sendPacket(secundarySocketId, packet);
            }
        }
        else if (receivedPacket.isFetchPacket())
        {
            string username = global_settings::socket_id_dictionary.get(socket_id);
            syncFiles(socket_id, "dir", username);
        }
        else if (receivedPacket.isDownloadPacket())
        {
            string username = global_settings::socket_id_dictionary.get(socket_id);
            string filename = receivedPacket.getMessage();
            string dirName = "dir/" + username;
            sendFile(socket_id, dirName, filename, false, true);
        }
        else if (receivedPacket.isInfoPacket())
        {
            string username = receivedPacket.getMessage();
            string dirName = "dir/" + username;
            string result = listfFilesInfo(dirName);
            Packet replyPacket(1, 1, MessageType::INFO, Status::SUCCESS, result.size(), result.c_str());
            sendPacket(socket_id, replyPacket);
        }
        else if (receivedPacket.isConnectionServer())
        {
            string serverIp = receivedPacket.getMessage();
            cout << "ipa " << serverIp << endl;
            bool success = global_settings::connect_server(socket_id, serverIp);
            std::string message = success ? "Conexão bem-sucedida do servidor secundário" : "Erro ao conectar.";

            if (success)
            {
                Packet replyPacket(1, 1, MessageType::CONNECTION_SERVER, Status::SUCCESS, message.size(), message.c_str());
                sendPacket(socket_id, replyPacket);
            }
            else
            {
                Packet replyPacket(1, 1, MessageType::CONNECTION_SERVER, Status::ERROR, 0, "");
                sendPacket(socket_id, replyPacket);
                break;
            }

            vector<string> ips = global_settings::client_ip.keys();
            for (string ip : ips)
            {
                sendClientInfo(socket_id, ip, ip, MessageType::IP);
            }
        }
    }
    close(socket_id);
}

void Server::setIp(string ip)
{
    this->ip = ip;
}

void Server::setPorta(int porta)
{
    this->porta = porta;
}

string Server::getIp()
{
    return this->ip;
}

int Server::getPorta()
{
    return this->porta;
}