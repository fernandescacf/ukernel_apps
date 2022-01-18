/**
 * @file        main.c
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        June 28, 2020
 * @brief       GPIO Drivir Main File
*/

/* Includes ----------------------------------------------- */
#include <types.h>
#include <ipc.h>
#include <dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <gpio.h>

/* Constants ---------------------------------------------- */
#define GPIO_PATH           "/dev/gpio"

#define GPIO_PIN_INVALID    (-1)
#define GPIO_ACCESS_INVALID (-1)
#define GPIO_PINS           (12 * 32)


/* Types -------------------------------------------------- */
typedef struct Client
{
    struct Client *next, *prev;
    int32_t scoid;
    int32_t pin;
    int32_t access;
}client_t;


/* Macros ------------------------------------------------- */


/* Variables ---------------------------------------------- */
static struct
{
    client_t *first;
    client_t *last;
}clients;


/* Functions prototypes------------------------------------ */

client_t* Client(int32_t scoid);

void ClientInsert(client_t* client);

void ClientRemove(client_t* client);

client_t* ClientFind(int32_t scoid);

int32_t GpioConnect(notify_t* info);

int32_t GpioDisconnect(notify_t* info);

int32_t GpioOpen(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset);

int32_t GpioClose(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset);

int32_t GpioRead(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset);

int32_t GpioWrite(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset);


/* Functions ---------------------------------------------- */

client_t* Client(int32_t scoid)
{
    client_t *client = (client_t*)malloc(sizeof(client_t));
    client->scoid = scoid;
    client->pin = GPIO_PIN_INVALID;
    client->access = GPIO_ACCESS_INVALID;
    return client;
}

void ClientInsert(client_t* client)
{
    client->next = clients.first;
    clients.first = client;
    client->prev = NULL;
    if(clients.last == NULL) clients.last = client;
}

void ClientRemove(client_t* client)
{
    if(client == NULL) return;

    if(client == clients.first) clients.first = client->next;
    else client->prev->next = client->next;
    
    if(client == clients.last) clients.last = client->prev;
    else client->next->prev = client->prev;

    free(client);
}

client_t* ClientFind(int32_t scoid)
{
    client_t* client = clients.first;
    for( ; client != NULL; client = client->next)
    {
        if((client->scoid == scoid))
        {
            return client;
        }
    }
    return NULL;
}

int32_t GpioConnect(notify_t* info)
{
    ClientInsert(Client(info->scoid));
    return E_OK;
}

int32_t GpioDisconnect(notify_t* info)
{
    ClientRemove(ClientFind(info->scoid));

    return E_OK;
}

int32_t GpioOpen(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset)
{
    client_t* client = ClientFind(scoid);

    // This use case should never happen
    if(client == NULL)
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    // Check if this client hasn't already open the connection
    if(client->pin != GPIO_PIN_INVALID)
    {
        return MsgRespond(rcvid, E_BUSY, NULL, 0);
    }

    // We are expecting to receive at least 1 bytes that identifies the gpio pin
    if(offset < sizeof(char))
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
    }

    // In case we didn't reveived the null character
    buffer[offset] = '\0';

    // Get gpio pin
    int32_t pin = strtoul(buffer, NULL, 10);

    // Check we the gpio pin is valid
    if(pin > GPIO_PINS)
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
    }

    // Check Open Mode
    if(hdr->code == O_RDONLY)
    {
        client->access = GPIO_INPUT;
    }
    else if((hdr->code == O_WRONLY) || (hdr->code == O_RDWR))
    {
        client->access = GPIO_OUTPUT;
    }
    else
    {
        return MsgRespond(rcvid, E_INVAL, NULL, 0);
    }

    // Set pin for current client
    client->pin = pin;

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t GpioClose(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset)
{
    client_t* client = ClientFind(scoid);

    // This use case should never happen
    if(client == NULL)
    {
        return MsgRespond(rcvid, E_ERROR, NULL, 0);
    }

    // Set gpio pin informations to invalid
    client->pin = GPIO_PIN_INVALID;
    client->access = GPIO_ACCESS_INVALID;

    return MsgRespond(rcvid, E_OK, NULL, 0);
}

int32_t GpioRead(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset)
{
    client_t* client = ClientFind(scoid);

    // This use case should never happen
    if(client == NULL)
    {
        return MsgRespond(rcvid, -1, NULL, 0);
    }

    // Check if client as permitions to read and has a valid pin set
    if((client->access == GPIO_ACCESS_INVALID) || (client->pin == GPIO_PIN_INVALID))
    {
        return MsgRespond(rcvid, -1, NULL, 0);
    }

    // Read gpio pin
    GpioSetCfgpin(client->pin , GPIO_INPUT);
    int32_t value = GpioInput(client->pin);
    memcpy(buffer, &value, sizeof(int32_t));

    return MsgRespond(rcvid, sizeof(int32_t), buffer, sizeof(int32_t));
}

int32_t GpioWrite(int32_t rcvid, int32_t scoid, io_hdr_t* hdr, char* buffer, uint32_t offset)
{
    client_t* client = ClientFind(scoid);

    // This use case should never happen
    if(client == NULL)
    {
        return MsgRespond(rcvid, -1, NULL, 0);
    }

    // Check if client as permitions to write, has a valid pin set and we received sizeof(int32_t) bytes for for the pin balue
    if((client->access != GPIO_OUTPUT) || (client->pin == GPIO_PIN_INVALID) || (offset != sizeof(int32_t)))
    {
        return MsgRespond(rcvid, -1, NULL, 0);
    }

    // Ensure pin is configured as output
    GpioSetCfgpin(client->pin , GPIO_OUTPUT);

    // Get pin value (0 is LOW, anything different is HIGH)
    if(*((int32_t*)buffer) == 0)
    {
        GpioOutput(client->pin, GPIO_VALUE_LOW);
    }
    else
    {
       GpioOutput(client->pin, GPIO_VALUE_HIGH);
    }
    
    return MsgRespond(rcvid, sizeof(int32_t), NULL, 0);
}

int main(int argc, const char* argv[])
{
    StdOpen();

    if(GpioDriverInit() != E_OK)
    {
        printf("\nERROR: Failed to initialize GPIO Driver\n");
        return -1;
    }

    StdClose();

    // Install Server Dispatcher to handle client messages
    dispatch_attr_t attr = {0x0, 5, 4, 1};
    ctrl_funcs_t gpio_ctrl_funcs = { GpioConnect, GpioDisconnect, NULL };
    io_funcs_t gpio_io_funcs = { NULL, GpioRead, GpioWrite, GpioOpen, GpioClose,
                                 NULL, NULL,     NULL,      NULL,     NULL};
    dispatch_t* gpio_disp = DispatcherAttach(GPIO_PATH, &attr, &gpio_io_funcs, &gpio_ctrl_funcs);

    // Sart the dispatcher, will not return
    DispatcherStart(gpio_disp,  TRUE);

    GpioDriverClose();

    return 0;
}