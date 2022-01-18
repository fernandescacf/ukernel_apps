#include <fs.h>
#include <string.h>
#include <stdlib.h>

#define FILE_MODE_MASK      (0x0F)

static dir_t root;

dir_t* PathResolve(dir_t* cwd, const char *path, char **remaining)
{
    dir_t *parent = cwd;

    if(cwd == NULL)
    {
        parent = &root;
    }

    if(path == NULL)
    {
        return parent;
    }
    
    dir_t *current;

    char *ptr = (char*)path;

    while(*ptr)
    {
        // Skip '/'
        if(ptr[0] == '/')
        {
            ptr++;
        }
        else
        {
            *remaining = ptr;
            break;
        }

        uint32_t len = 0;
        for( ; ptr[len] && ('/' != ptr[len]); len++) {}
        
        for(current = parent->dirs; current != NULL; current = current->sibling)
        {
            if(current->len == len && !memcmp(ptr, current->name, len))
            {
                break;
            }
        }

        if(current == NULL)
        {
            *remaining = ptr;
            return parent;
        }

        ptr += len;
        parent = current;
    }

    if(ptr[0] == 0)
    {
        *remaining = NULL;
    }

    return parent;
}

void DirectoryAdd(dir_t *parent, dir_t *child)
{
    child->owner = parent;
    child->sibling = parent->dirs;
    parent->dirs = child;
}

dir_t* CreateDirecoty(dir_t *parent, char *path, char **remaining, uint16_t flags)
{
    uint32_t len = 0;
    for( ; path[len] && ('/' != path[len]); len++) {}

    if(path[len] == 0)
    {
        return NULL;
    }

    dir_t *current = (dir_t*)malloc(sizeof(dir_t) + len);

    current->flags = flags;
    current->len = len;
    current->dirs = NULL;
    current->files = NULL;

    memcpy(current->name, path, len);

    DirectoryAdd(parent, current);

    *remaining = (path + len + 1);

    return current;
}

file_t* FileAdd(dir_t* parent, char *name, void* data, size_t size, uint32_t flags)
{
    uint32_t len = strlen(name);
    file_t* file = (file_t*)malloc(sizeof(file_t) + len);

    file->owner = parent;
    file->refs = 0;
    file->size = size;
    file->data = data;
    file->flags = (uint16_t)(flags & 0xFFFF);
    file->len = len;
    memcpy(file->name, name, len);
    file->sibling = parent->files;
    parent->files = file;

    return file;
}

file_t* CreateFile(dir_t* cwd, const char *path, void* data, size_t size, uint32_t flags)
{
    // Get path length
    uint32_t length = strlen(path);

    // Last character has to be different from '/' but first character has to be '/'
    if((path[0] != '/') || (path[length - 1] == '/'))
    {
        return NULL;
    }

    // Get the last namespace in the specified path
    char *remaining;
    dir_t* parent = PathResolve(cwd, path, &remaining);
    
    // If we consumed the whole path there is nothing more to be registered...
    // The path is invalid
    if(*remaining == 0)
    {
        return NULL;
    }
    
    // Create directories if required
    while(1)
    {
        dir_t *current = CreateDirecoty(parent, remaining, &remaining, 0x0);

        // All namesapces where resolved
        if(current == NULL)
        {
            break;
        }

        parent = current;
    }
    
    // At this point we only have the name of the server left
    return FileAdd(parent, remaining, data, size, flags);
}

file_t* GetFile(dir_t* cwd, const char* path)
{
    // Get path length
    uint32_t length = strlen(path);

    // Last character has to be different from '/' but first character has to be '/'
    if(/*(path[0] != '/') ||*/ (path[length - 1] == '/'))
    {
        return NULL;
    }

    // Fisrt we need to find the name space where the server is registered
    char* remaining;
    dir_t* parent = PathResolve(cwd, path, &remaining);

    if(remaining == NULL)
    {
        // Nothing left to search for a server
        return NULL;
    }

    for(length = 0; remaining[length] != '\0'; length++)
    {
        if(remaining[length] == '/')
        {
            // We couldn't resolve the whole path
            return NULL;
        }
    }

    // Search for the server in the name space
    file_t *file;
    for(file = parent->files; file != NULL; file = file->sibling)
    {
        if(file->len == length && !memcmp(remaining, file->name, length))
        {
            return file;
        }
    }

    return NULL;
}

int32_t DeleteFile(file_t* file)
{
    if(file->refs > 0)
    {
        return E_BUSY;
    }

    dir_t* parent = file->owner;
    
    if(parent->files == file)
    {
        parent->files = file->sibling;
    }
    else
    {
        file_t* it;
        for(it = parent->files; it != NULL; it = it->sibling)
        {
            if(it->sibling == file)
            {
                it->sibling = file->sibling;
                break;
            }
        }

        if(it == NULL)
        {
            return E_INVAL;
        }
    }
    
    free(file);

    return E_OK;
}

int32_t FileOpen(file_t* file, int32_t mode)
{
    if(file->flags & (uint16_t)(mode & FILE_MODE_MASK))
    {
        file->refs++;
        return E_OK;
    }

    return E_INVAL;
}

int32_t FileClose(file_t* file)
{
    return ++file->refs;
}