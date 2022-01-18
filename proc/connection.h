/**
 * @file        connection.h
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        24 August, 2020
 * @brief       Connections Definition Header File
*/

#ifndef _CONNECTION_H_
#define _CONNECTION_H_

/* Includes ----------------------------------------------- */
#include <types.h>
#include <io_types.h>
#include <fcntl.h>


/* Exported types ----------------------------------------- */
typedef struct
{
    int32_t  scoid;
    uint16_t state;
    uint16_t access;
    off_t    seek;
    void*    handler;
}connect_t;


/* Exported constants ------------------------------------- */
#define CONNECTION_CLOSE    0
#define CONNECTION_OPEN     1
#define CONNECTION_MAP      2


/* Exported macros ---------------------------------------- */


/* Exported functions ------------------------------------- */

#ifdef LISTENERS
int32_t ConnectionListenerInstall(int32_t chid);

int32_t ConnectionListenerUninstall(int32_t chid);
#endif

int32_t ConnectionSetCloseCallBack(void (*callBack)(connect_t*));

int32_t ConnectionAttach(notify_t* info);

int32_t ConnectionDetach(notify_t* info);

connect_t* ConnectionGet(int32_t scoid);

void ConnectionSetHandler(connect_t* con, void* handler);

int32_t ConnectionSetState(connect_t* con, uint16_t state);

int32_t ConnectionSetAccess(connect_t* con, uint16_t access);

#endif