#ifndef _CONNECTIONS_H_
#define _CONNECTIONS_H_

#include <types.h>
#include <fs.h>
#include <io_types.h>
#include <list.h>

#define CONNECT_NO_ACCESS  -1
#define CONNECT_INVALID    -1
#define CONNECT_CLOSE       0
#define CONNECT_OPEN        1
#define CONNECT_SHARED      2


#define CONNECTION_CLOSE    0
#define CONNECTION_OPEN     1
#define CONNECTION_MAP      2

typedef struct 
{
    int32_t  scoid;
    file_t*  file;
    int32_t  access;
    int32_t  state;
    uint32_t seek;
}connection_t;

int32_t ConnectionAttachHandler(notify_t *info);

int32_t ConnectionDetachHandler(notify_t *info);

connection_t* ConnectionGet(int32_t scoid);

#endif