#include "../header.h"


//typedef void(*sighandler_t)(int);
//sighandler_t signal(int signum, sighandler_t handler);
void signal_handler(int signo)
{
    if(signo == SIGINT)
    {
        printf("recv SIGINT\n");
    }
    else if(signo == SIGQUIT)
    {
        printf("recv SIGQUIT\n");
    }
}

int main()
{
    printf("wait for SIGINT or SIGQUIT\n");

    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    while(1);




    return 0;
}

