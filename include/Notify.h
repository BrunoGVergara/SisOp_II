#ifndef NOTIFY_H
#define NOTIFY_H
#include <sys/inotify.h>
#include "Client.h"

class Notify
{
private:
    int startNotify();
    Client *client;
    int startWatch(int fd);
    void handleFileChange(inotify_event *event, int wd);

public:
    Notify(Client *client);
    int init();
};

#endif