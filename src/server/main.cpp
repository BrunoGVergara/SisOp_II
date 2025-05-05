#include "Server.h"
#include <iostream>
#include <string.h>
#include <thread>

using namespace std;

int main(int argc, char *argv[])
{
    char *serverType = argv[1];

    if (strcmp(serverType, "p") == 0)
    {
        if (argc < 3)
        {
            cerr << "Uso: " << argv[0] << "p ip porta" << endl;
            return 1;
        }

        string ip = argv[2];
        int porta = stoi(argv[3]);
        Server server;

        server.start(ip, porta);
    }
    else if (strcmp(serverType, "b") == 0)
    {
        if (argc < 5)
        {
            cerr << "Uso: " << argv[0] << "b ip porta ipServidorPrincipal portaServidorPrincipal" << endl;
            return 1;
        }

        string ip = argv[2];
        int porta = stoi(argv[3]);
        Server server;
        string principalServerIp = argv[4];
        int principalServerPort = stoi(argv[5]);

        int newClientSocket = 0;
        std::thread client_activity([&server, &ip, &porta, &principalServerIp, &principalServerPort, newClientSocket]()
                                    { server.startBackup(ip, porta, principalServerIp, principalServerPort); });
        client_activity.join();
    }
    else
    {
        cerr << "Uso: " << argv[0] << "p porta ou b porta ipServidorPrincipal portaServidorPrincipal" << endl;
        return 1;
    }

    return 0;
}
