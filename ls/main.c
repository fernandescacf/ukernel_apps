#include <types.h>
#include <io_types.h>
#include <server.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int LsNamespace(const char* path)
{
    // Reply buffer
    char reply[256];
    // Message header
    io_hdr_t hdr;
    hdr.type = _IO_INFO;
    hdr.code = INFO_NAMESPACE_LS;
    hdr.sbytes = strlen(path) + 1;
    hdr.rbytes = sizeof(reply) - 1;
    uint32_t replySize;

    // Query System for the received path
    if(MsgSend(SYSTEM_SERVER, &hdr, path, reply, &replySize) == E_OK)
    {
        io_entry_t *entry = (io_entry_t *)reply;
        // Add termination character (not added by default)
        reply[replySize] = 0;

        if(entry->type != INFO_NAMESPACE)
        {
            return E_ERROR;
        }

        nentry_t *ns = &entry->namespace;

        int32_t entries = ns->nentries + ns->sentries;

        for(hdr.code = 1; hdr.code <= entries; hdr.code++)
        {
            uint32_t replySize = 0;

            if(MsgSend(SYSTEM_SERVER, &hdr, path, reply, &replySize) != E_OK)
            {
                return E_ERROR;
            }

            io_entry_t *entry = (io_entry_t *)reply;
            reply[replySize] = 0;

            if(entry->type == INFO_NAMESPACE)
            {
                printf("%s/\n", entry->namespace.name);
            }
            else if (entry->type == INFO_SERVER)
            {
                printf("%s pid: %d chid: %d\n", entry->server.name, entry->server.pid, entry->server.chid);
            }
        }
        return E_OK;
    }

    return E_ERROR;
}

int32_t LsServer(int32_t fd, const char* path)
{
    // Reply buffer
    char reply[256];
    // Message header
    io_hdr_t hdr;
    hdr.type = _IO_INFO;
    hdr.code = INFO_LIST_ALL;
    hdr.sbytes = strlen(path) + 1;
    hdr.rbytes = sizeof(reply) - 1;
    uint32_t replySize;

    if(MsgSend(fd, &hdr, path, reply, &replySize) == E_OK)
    {
        uint32_t offset = 0;
        while(offset < replySize)
        {
            io_entry_t *entry = (io_entry_t *)&reply[offset];

            if(entry->type == INFO_DIR)
            {
                dentry_t *dir = &entry->dir;
                printf("%s/\n", dir->name);
                offset += (sizeof(dentry_t) + dir->len - 3);
            }
            else if(entry->type == INFO_FILE)
            {
                fentry_t *file = &entry->file;
                printf("%s\n", file->name);
                offset += (sizeof(fentry_t) + file->len - 3);
            }
            else
            {
                return E_ERROR;
            }   
        }

        return E_OK;
    }

    return E_ERROR;
}

int32_t Ls(const char* path)
{
    char* remaining = NULL;
    int32_t fd = connect(path, &remaining);

    // LS a server?
    if(fd != -1)
    {
        int32_t ret = LsServer(fd, remaining);
        ConnectDetach(fd);
        return ret;
    }

    // LS System
    return LsNamespace(path);
}

int main(int argc, const char* argv[])
{
    if(argc == 1)
    {
        Ls("/");
    }
    else
    {
        Ls(argv[1]);
    }

    return 0;
}