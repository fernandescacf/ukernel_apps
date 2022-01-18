#include <stdio.h>

int main(int argc, const char* argv[])
{
    int i = 1; // Discard process name
    for(; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }

    printf("\n");
    
    return 0;
}