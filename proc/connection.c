/**
 * @file        connection.c
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        24 August, 2020
 * @brief       Connections implementation
*/

/* Includes ----------------------------------------------- */
#include <connection.h>
#include <vector.h>
#include <stdlib.h>


/* Private types ------------------------------------------ */

#ifdef LISTENERS
typedef struct
{
    int32_t  chid;
    cvector_vector_type(connect_t*) connections;
}listener_t;
#endif

/* Private constants -------------------------------------- */


/* Private macros ----------------------------------------- */


/* Private variables -------------------------------------- */

#ifdef LISTENERS
static cvector_vector_type(listener_t*) listeners = NULL;
#endif

static cvector_vector_type(connect_t*) connections = NULL;
static void (*CloseCallBack)(connect_t*) = NULL;

/* Private function prototypes ---------------------------- */

#ifdef LISTENERS
int32_t ConnectionListenerGet(int32_t chid)
{
    if (listeners)
    {
        size_t i;
		for (i = 0; i < cvector_size(listeners); ++i)
        {
            if(listeners[i]->chid == chid)
            {
                return (int32_t)i;
            }
        }
	}
    return -1;
}
#endif

int32_t ConnectionFind(int32_t scoid)
{
    if (connections)
    {
        size_t i;
		for (i = 0; i < cvector_size(connections); ++i)
        {
            if(connections[i]->scoid == scoid)
            {
                return (int32_t)i;
            }
        }
	}
    return -1;
}

/* Private functions -------------------------------------- */

#ifdef LISTENERS
int32_t ConnectionListenerInstall(int32_t chid)
{
    // Check if there is a listener for this channel
    if (ConnectionListenerGet(chid) != -1)
    {
        return E_BUSY;
    }

    // Create new connection listener for this channel
    listener_t* listener = (listener_t*)malloc(sizeof(listener_t));
    listener->chid = chid;
    listener->connections = NULL;

    // Register listener
    cvector_push_back(listeners, listener);

    return E_OK;
}

int32_t ConnectionListenerUninstall(int32_t chid)
{
    int32_t index = ConnectionListenerGet(chid);

    if(index == -1)
    {
        return E_INVAL;
    }

    listener_t* listener = listeners[index];

    cvector_erase(listeners, index);
    
    // Delete all connectins tracked by this handler
    if (listener->connections)
    {
		connect_t *it;
		for (it = cvector_begin(listener->connections); it != cvector_end(listener->connections); ++it)
        {
			free(it);
		}
        cvector_free(listener->connections) 
	}

    free(listener);

    return E_OK;
}
#endif

int32_t ConnectionSetCloseCallBack(void (*callBack)(connect_t*))
{
    CloseCallBack = callBack;
    
    return E_OK;
}

int32_t ConnectionAttach(notify_t* info)
{
    // Check if there is a connection for this scoid
    int32_t index = ConnectionFind(info->scoid);

    // Nested connection are not supported
    if(index != -1)
    {
        return E_BUSY;
    }

    connect_t* connect = (connect_t*)malloc(sizeof(connect_t));
    connect->scoid = info->scoid;
    connect->state = CONNECTION_CLOSE;
    connect->access = O_RDONLY;
    connect->seek = 0;
    connect->handler = NULL;

    // Register connection
    cvector_push_back(connections, connect);

    return E_OK;
}

int32_t ConnectionDetach(notify_t* info)
{
    // Find connection for this scoid
    int32_t index = ConnectionFind(info->scoid);

    // Connection not found???
    if(index == -1)
    {
        return E_ERROR;
    }

    connect_t* connect = connections[index];

    cvector_erase(connections, index);

    // Do we need to close the connection
    if((connect->state != CONNECTION_CLOSE) && (CloseCallBack != NULL))
    {
        CloseCallBack(connect);
    }

    free(connect);

    return E_OK;
}

connect_t* ConnectionGet(int32_t scoid)
{
    // Find connection for this scoid
    int32_t index = ConnectionFind(scoid);

    // Connection not found
    if(index == -1)
    {
        return NULL;
    }

    return connections[index];
}

void ConnectionSetHandler(connect_t* con, void* handler)
{
    con->handler = handler;
}

int32_t ConnectionSetState(connect_t* con, uint16_t state)
{
    // We cannot pass from closed to mapped
    if((con->state == CONNECTION_CLOSE) && (state == CONNECTION_MAP))
    {
        return E_ERROR;
    }

    con->state = state;

    return E_OK;
}

int32_t ConnectionSetAccess(connect_t* con, uint16_t access)
{
    con->access = access;

    return E_OK;
}
