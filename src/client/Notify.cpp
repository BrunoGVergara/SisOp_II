#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include "Notify.h"
#include "Service.h"
#include <unordered_set>
#include <mutex>

#define MAX_EVENTS 1024
#define LEN_NAME 255
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (MAX_EVENTS * (EVENT_SIZE + LEN_NAME))
#define DIR_NAME "sync_dir"

std::unordered_set<std::string> renamedFiles;
std::mutex renamedFilesMutex;

Notify::Notify(Client *client) : client(client)
{
}

int Notify::init()
{
    int length, i = 0, fd, wd;
    char buffer[BUF_LEN];

    fd = startNotify();
    wd = startWatch(fd);

    while (1)
    {
        i = 0;
        length = read(fd, buffer, BUF_LEN);

        while (i < length)
        {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->len)
            {
                handleFileChange(event, wd);
                i += EVENT_SIZE + event->len;
            }
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);

    return fd;
}

bool isSyncFile(string filename)
{
    const std::string suffix = ".sync";
    if (filename.size() < suffix.size())
    {
        return false;
    }
    return filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
}
void Notify::handleFileChange(inotify_event *event, int wd)
{
    string filename(event->name);

    {
        std::lock_guard<std::mutex> lock(renamedFilesMutex);

        if (renamedFiles.find(filename) != renamedFiles.end())
        {
            renamedFiles.erase(filename);
            return;
        }
    }

    if (isSyncFile(filename))
    {
        string originalPath = string(DIR_NAME) + "/" + filename;
        string newFilename = filename.substr(0, filename.size() - 5);
        string newPath = string(DIR_NAME) + "/" + newFilename;

        if (rename(originalPath.c_str(), newPath.c_str()) == 0)
        {
            {
                std::lock_guard<std::mutex> lock(renamedFilesMutex);
                renamedFiles.insert(newFilename);
            }
        }

        return;
    }

    if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM)
    {
        Packet packet(0, 1, MessageType::DELETE, Status::SUCCESS, filename.size(), filename.c_str());
        sendPacket(client->getSocketId(), packet);
    }

    if (event->mask & IN_CLOSE_WRITE || event->mask == IN_MOVED_TO)
    {
        sendFile(client->getSocketId(), DIR_NAME, filename);
    }
}

int Notify::startNotify()
{
    int fd = inotify_init();
    if (fd < 0)
    {
        perror("Couldn't initialize inotify");
        return -1;
    }

    return fd;
}

int Notify::startWatch(int fd)
{
    int wd = inotify_add_watch(fd, DIR_NAME, IN_MOVED_TO | IN_MOVED_FROM | IN_CLOSE_WRITE | IN_DELETE);
    if (wd == -1)
    {
        printf("Couldn't add watch to %s: %s\n", DIR_NAME, strerror(errno));
        return -1;
    }

    return wd;
}