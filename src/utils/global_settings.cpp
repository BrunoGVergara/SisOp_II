#include <string>
#include <iostream>
#include <sstream>

#include <global_settings.h>

using namespace std;

concurrent_dictionary<string, int> global_settings::client_name_dictionary;
concurrent_dictionary<string, string> global_settings::client_ip;
concurrent_dictionary<int, string> global_settings::socket_id_dictionary;
concurrent_dictionary<string, int> global_settings::servers;

bool global_settings::connect_client(int socket_id, string client_name, string clientIp)
{
    int socket_ids = 1;
    bool success = false;
    ostringstream terminal_output;

    bool client_name_exists = client_name_dictionary.contains(client_name);

    if (client_name_exists)
    {
        socket_ids = client_name_dictionary.get(client_name);

        if (socket_ids != 2)
        {
            socket_ids++;

            terminal_output << "Success: connected client; Client: " << client_name << " socketId: " << socket_ids;

            success = true;
        }
        else
        {
            terminal_output << "Success: connected client; Client: " << client_name << " socketId: " << socket_ids;
        }
    }
    else
    {
        terminal_output << "Success: connected client; Client: " << client_name << " socketId: " << socket_ids;
        success = true;
    }

    if (success == true)
    {
        client_name_dictionary.insert_or_update(client_name, socket_ids);
        socket_id_dictionary.insert_or_update(socket_id, client_name);
        client_ip.insert_or_update(clientIp, clientIp);
    }

    cout << terminal_output.str() << endl;

    return success;
}

bool global_settings::connect_server(int socket_id, string server_ip)
{

    bool success = false;

    bool server_exists = servers.contains(server_ip);

    if (!server_exists)
    {
        servers.insert_or_update(server_ip, socket_id);
        success = true;
    }

    return success;
}

bool global_settings::disconnect_client(int socket_id, string client_name)
{
    bool success = false;
    ostringstream terminal_output;

    bool client_name_exists = client_name_dictionary.contains(client_name);
    bool socket_id_exists = socket_id_dictionary.contains(socket_id);

    if (client_name_exists && socket_id_exists)
    {
        int socket_ids = client_name_dictionary.get(client_name);
        socket_ids--;
        if (socket_ids == 0)
        {
            client_name_dictionary.remove(client_name);
        }
        else
        {
            client_name_dictionary.insert_or_update(client_name, socket_ids);
        }

        socket_id_dictionary.remove(socket_id);

        success = true;
        terminal_output << "Disconnect: connected client; Client: " << client_name << " socketId: " << socket_ids;
    }
    else
    {
        terminal_output << "Disconnect: connected client; Client: " + client_name;
    }

    cout << terminal_output.str() << endl;

    return success;
}