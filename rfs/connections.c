#include <connections.h>
#include <server.h>
#include <stdlib.h>

static list_t* list = NULL;

static int32_t ListSort(void* it, void* in)
{
    (void)it; (void)in;

    return 0;
}

static int32_t ListCmp(void* it, void* value)
{
    connection_t* current = (connection_t*)it;
    int32_t scoid = (int32_t)value;

    return current->scoid - scoid;
}

void ConnectionsListInit()
{
    list = (list_t*)malloc(sizeof(list_t));
    ListInitialize(list, ListSort, ListCmp);
}

int32_t ConnectionAttachHandler(notify_t *info)
{
    if(list == NULL)
    {
        ConnectionsListInit();
    }

    connection_t* connection = (connection_t*)malloc(sizeof(connection_t));

    connection->scoid = info->scoid;
    connection->file = NULL;
    connection->seek = 0;
    connection->access = CONNECT_NO_ACCESS;
    connection->state = CONNECT_INVALID;

    return ListInsertObject(list, connection);
}

int32_t ConnectionDetachHandler(notify_t *info)
{
    connection_t* connection = ListRemoveObject(list, (void*)info->scoid);

    if(connection->state == CONNECT_SHARED)
    {
        UnshareObject(info->scoid);
    }

    if(connection->file)
        FileClose(connection->file);

    free(connection);

    return E_OK;
}

connection_t* ConnectionGet(int32_t scoid)
{
    return (connection_t*)ListGetObject(list, (void*)scoid);
}