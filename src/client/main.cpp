#include "Notify.h"
#include "Client.h"
#include <iostream>
#include <string>
#include <thread>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        cerr << "Uso: " << argv[0] << " usuario ip ipServidor portaServidor" << endl;
        return 1;
    }

    string username = argv[1];
    string clientIp = argv[2];
    string serverIP = argv[3];
    int serverPort = stoi(argv[4]);

    try
    {
        Client client(username, clientIp, serverIP, serverPort);
        client.run();
    
    }
    catch (runtime_error &e)
    {
        cout << e.what();
    }

    return 0;
}
