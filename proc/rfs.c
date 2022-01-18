/**
 * @file        rfs.c
 * @author      Carlos Fernandes
 * @version     1.0
 * @date        28 May, 2020
 * @brief       Raw File System implementation
*/

/* Includes ----------------------------------------------- */
#include <rfs.h>
#include <mman.h>


/* Private types ------------------------------------------ */
typedef struct
{
    uint32_t type;
    uint32_t version;
    uint32_t arch;
    uint32_t machine;
    size_t   fs_size;
    uint32_t script_off;
    uint32_t script_cmds;
    uint32_t ram_off;
    uint32_t irq_off;
    uint32_t devices_off;
    uint32_t devices_count;
    uint32_t names_off;
    uint32_t names_size;
    uint32_t files_off;
    uint32_t files_count;
}header_t;

typedef struct
{
    uint32_t type;
    uint16_t prio;
    uint16_t priv;
    uint32_t file_off;
    uint32_t cmd_off;
}cmd_t;

typedef struct
{
    uint32_t addr;
    size_t   size;
}ram_t;

typedef struct
{
    uint32_t priv;
    size_t   shared;
}intr_t;

typedef struct
{
    uint32_t addr;
    size_t   size;
    uint32_t access;
    uint32_t name_off;
}device_t;

typedef struct
{
    uint32_t type;
    size_t   size;
    uint32_t data_off;
    uint32_t name_off;
}file_t;


/* Private constants -------------------------------------- */
#define RFS_ID      ((uint32_t)(-1))

#define RFS_TYPE	0xCACFCACF

#define EXEC_TYPE	0x1
#define LIB_TYPE	0x2
#define OBJ_TYPE	0x3



/* Private macros ----------------------------------------- */



/* Private variables -------------------------------------- */

static struct
{
	header_t	*hdr;
	cmd_t		*cmds;
	ram_t		*ram;
    intr_t      *intr;
	device_t	*devices;
	file_t		*files;
	char        *strings;
}Rfs;


/* Private function prototypes ---------------------------- */

char *RfsGetString(uint32_t offset)
{
	if(offset >= Rfs.hdr->names_size)
	{
		return NULL;
	}

	return &Rfs.strings[offset];
}

file_t *RfsGetFile(uint32_t offset)
{

	return (file_t *)((uint32_t)Rfs.hdr + offset);
}


/* Private functions -------------------------------------- */

int32_t RfsInit()
{
    if(Rfs.hdr != NULL)
    {
        return E_INVAL;
    }

    Rfs.hdr = (header_t*)mmap(NULL, 0, (PROT_READ |PROT_WRITE), (MAP_PHYS | MAP_ANON | MAP_PRIVATE), NOFD, RFS_ID);

    if(Rfs.hdr == NULL)
    {
        return E_ERROR;
    }

    return E_OK;
}

int32_t RfsDelete()
{
    if(Rfs.hdr == NULL)
    {
        return E_INVAL;
    }

    munmap((void*)Rfs.hdr, Rfs.hdr->fs_size);

    Rfs.hdr = NULL;

    return E_OK;
}

#if 0
int32_t RfsDumpHeader()
{
    printf("Raw Filesystem Header\n");
    printf(" - Type:    0x%x\n", Rfs.hdr->type);
    printf(" - Version: %s\n", RfsGetVersion());
    printf(" - Arch:    %s\n", RfsGetArch());
    printf(" - Mach:    %s\n", RfsGetMach());
    printf(" - Size:    0x%x\n", Rfs.hdr->fs_size);

    return E_OK;
}
#endif

int32_t RfsParse()
{
    // Validate Raw File System object
	if(Rfs.hdr->type != RFS_TYPE)
	{
		return E_INVAL;
	}

	// Get RFS script commands
	Rfs.cmds = (cmd_t*)((uint32_t)Rfs.hdr + Rfs.hdr->script_off);

	// Get RAM information
	Rfs.ram = (ram_t*)((uint32_t)Rfs.hdr + Rfs.hdr->ram_off);

    // Get Interrupt information
	Rfs.intr = (intr_t*)((uint32_t)Rfs.hdr + Rfs.hdr->irq_off);

	// Get supported Devices
	Rfs.devices = (device_t*)((uint32_t)Rfs.hdr + Rfs.hdr->devices_off);

	// Get Raw File System owned Files
	Rfs.files = (file_t*)((uint32_t)Rfs.hdr + Rfs.hdr->files_off);

	// Get Strings Table
	Rfs.strings = (char*)((uint32_t)Rfs.hdr + Rfs.hdr->names_off);

	return E_OK;
}

int32_t RfsGetRamInfo(void **addr, size_t *size)
{
	*addr = (void*)Rfs.ram->addr;
	*size = Rfs.ram->size;

	return E_OK;
}

uint32_t RfsDevicesCount()
{
    return Rfs.hdr->devices_count;
}

int32_t RfsDeviceParse(uint32_t dev, char** name, void** addr, size_t* size)
{
    if(dev >= Rfs.hdr->devices_count)
    {
        return E_INVAL;
    }

    *name = RfsGetString(Rfs.devices[dev].name_off);
    *addr = (void*)Rfs.devices[dev].addr;
    *size = Rfs.devices[dev].size;

    return E_OK;
}

uint32_t RfsFilesCount()
{
    return Rfs.hdr->files_count;
}

int32_t RfsFileParse(uint32_t file, char** name, int32_t* type, void** data, size_t *size)
{
    if(file >= Rfs.hdr->files_count)
    {
        return E_INVAL;
    }

    *name = RfsGetString(Rfs.files[file].name_off);
    *type = Rfs.files[file].type;
    *size = Rfs.files[file].size;
    *data = (void *)((uint32_t)Rfs.hdr + Rfs.files[file].data_off);

    return E_OK;
}

char* RfsGetVersion()
{
	return RfsGetString(Rfs.hdr->version);
}

char* RfsGetArch()
{
	return RfsGetString(Rfs.hdr->arch);
}

char* RfsGetMach()
{
	return RfsGetString(Rfs.hdr->machine);
}