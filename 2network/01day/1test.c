#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <event.h>

int main()
{
    struct event_base *base = event_base_new();
    
    const char** p = event_get_supported_methods();

    for(int i=0; ; i++)
    {
        if(p[i] == NULL)    break;
        printf("%s\n", p[i]);
    }
    printf("----------------\n");

    printf("current version: %s\n", event_get_version());

    printf("当前默认后端: %s\n", event_base_get_method(base));


    event_base_free(base);

    return 0;
}

