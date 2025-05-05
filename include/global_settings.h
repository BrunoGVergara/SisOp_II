#include <string>
#include <iostream>

#include <concurrent_dictionary.h>

class global_settings
{
public:
    static concurrent_dictionary<string, int> client_name_dictionary;
    static concurrent_dictionary<int, string> socket_id_dictionary;
    static concurrent_dictionary<string, string> client_ip;

    static concurrent_dictionary<string, int> servers;


    static bool connect_client(int socket_id, string client_name, string clientIp);
    static bool disconnect_client(int socket_id, string client_name);
    static bool connect_server(int socket_id, string server_ip);
};