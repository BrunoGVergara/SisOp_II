
#include <Service.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void sendPacket(int socket_id, Packet packet)
{
    int totalBytesSent = 0;

    const char *buffer = packet.serialize();
    int packetSize = packet.size();

    while (totalBytesSent < packetSize)
    {
        int bytesSent = send(socket_id, buffer + totalBytesSent, packetSize - totalBytesSent, MSG_NOSIGNAL);
        if (bytesSent == -1)
        {
            throw std::runtime_error("Erro ao enviar pacote pelo socket.");
        }

        totalBytesSent += bytesSent;
    }

    delete[] buffer;
}

Packet receivePacket(int socket_id)
{
    Packet recevedPacket;
    char buffer[2500];
    int totalBytesReceived = 0;
    int headerSize = recevedPacket.headerSize();

    while (totalBytesReceived < headerSize)
    {
        int bytesRead = recv(socket_id, buffer + totalBytesReceived, headerSize - totalBytesReceived, 0);

        if (bytesRead == -1)
        {
            throw std::runtime_error("Erro ao receber pacote do socket. 1");
        }

        totalBytesReceived += bytesRead;
    }

    totalBytesReceived = 0;
    recevedPacket.deserialize(buffer);

    while (totalBytesReceived < recevedPacket.getMessageSize())
    {
        int bytesRead = recv(socket_id, buffer + headerSize + totalBytesReceived, recevedPacket.getMessageSize() - totalBytesReceived, 0);

        if (bytesRead == -1)
        {
            throw std::runtime_error("Erro ao receber pacote do socket.");
        }

        if (bytesRead == 0)
        {
            throw std::runtime_error("A conexão foi fechada pelo cliente.");
        }

        totalBytesReceived += bytesRead;
    }

    recevedPacket.deserialize(buffer);

    if (recevedPacket.isStatusError())
    {
        if (recevedPacket.isDisconnectionPacket())
        {
            throw std::invalid_argument("Limite máximo de conexões por usuário excedido.");
        }
        throw std::invalid_argument("Ocorreu um erro ao processar o pacote recebido.");
    }

    return recevedPacket;
}

void receiveFile(Packet packet, int socket_id, optional<string> username, string dirName)
{
    Packet recevedPacket;
    char buffer[2500];
    int totalBytesReceived = 0;
    list<Packet> filePackets;

    string dir = dirName;

    if (username)
    {
        dir = dir + "/" + *username;
    }

    filePackets.push_front(packet);
    for (int i = 0; i < packet.getTotalPackets() - 1; i++)
    {
        Packet receivedPacket = receivePacket(socket_id);
        filePackets.push_back(receivedPacket);
    }

    fileUnpacking(filePackets, dir);
}

void sendFile(int socket_id, string dir, string filename)
{
    sendFile(socket_id, dir, filename, false, false);
}

void sendFile(int socket_id, string dir, string filename, bool syncFile, bool download)
{
    list<Packet> packets = filePacking(dir, filename, syncFile, download);
    for (const auto &packet : packets)
    {
        sendPacket(socket_id, packet);
    }
}

void syncFiles(int socket_id, string dir, string username)
{
    try
    {
        string dirName = dir + "/" + username;
        for (const auto &entry : std::filesystem::directory_iterator(dirName))
        {
            if (entry.is_regular_file())
            {
                const std::string &filename = entry.path().filename().string();
                sendFile(socket_id, dirName, filename, true, false);
            }
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Erro ao acessar o diretório: " << e.what() << '\n';
    }
}

void deleteFile(string filePath)
{
    remove(filePath.c_str());
}

void getSyncDir(int socket_id, string username)
{
    Packet packet(0, 1, MessageType::FETCH, Status::SUCCESS, username.size(), username.c_str());
    sendPacket(socket_id, packet);
}

std::string formatTimestamp(std::time_t time)
{
    std::tm *tm = std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

string listfFilesInfo(string dir)
{
    std::ostringstream result;

    try
    {
        for (const auto &entry : filesystem::directory_iterator(dir))
        {
            if (filesystem::is_regular_file(entry.status()))
            {
                const auto &path = entry.path();
                std::string fileName = path.filename().string();

                struct stat fileStat;
                if (stat(path.c_str(), &fileStat) == 0)
                {

                    std::string mtime = formatTimestamp(fileStat.st_mtime);
                    std::string atime = formatTimestamp(fileStat.st_atime);
                    std::string ctime = formatTimestamp(fileStat.st_ctime);

                    std::uintmax_t fileSize = filesystem::file_size(path);

                    result << "File: " << fileName << "\n";
                    result << "  Size (bytes): " << fileSize << "\n";
                    result << "  Modification Time (mtime): " << mtime << "\n";
                    result << "  Access Time (atime): " << atime << "\n";
                    result << "  Change Time (ctime): " << ctime << "\n\n";
                }
                else
                {
                    result << "Erro ao obter informações do arquivo: " << fileName << "\n";
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        result << "Erro ao listar arquivos no diretório '" << dir << "': " << e.what() << "\n";
    }

    return result.str();
}

int startSocket(string ip, int port)
{
    struct sockaddr_in serverAddress2;
    serverAddress2.sin_family = AF_INET;
    serverAddress2.sin_port = htons(port);
    serverAddress2.sin_addr.s_addr = inet_addr(ip.c_str());

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress2, sizeof(serverAddress2)) < 0)
    {
        cerr << "Erro ao vincular o socket." << endl;
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 5) < 0)
    {
        cerr << "Erro ao escutar na porta." << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Servidor escutando na porta " << ntohs(serverAddress2.sin_port) << endl;
    return serverSocket;
}

int connectToSocket(string ip, int port)
{

    int newSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in newServerAddress;
    newServerAddress.sin_family = AF_INET;
    newServerAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &newServerAddress.sin_addr) <= 0)
    {
        cerr << "Endereço IP inválido." << endl;
        throw runtime_error("Endereço IP inválido.");
    }
    int a = connect(newSocket, (struct sockaddr *)&newServerAddress, sizeof(newServerAddress));
    if (a < 0)
    {
        cerr << "Erro ao conectar ao servidor." << endl;
        throw invalid_argument("Erro ao conectar ao servidor.");
    }

    return newSocket;
}
