#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct hostent *myent;
    struct in_addr myen;
    long int *add;

    myent = gethostbyname(argv[1]);
    if (myent == NULL)
    {
        perror("ERROR : ");
        exit(0);
    }

    printf("%s\n", myent->h_name);

    while(*myent->h_addr_list != NULL)
    {
        add = (long int *)*myent->h_addr_list;
        myen.s_addr = *add;
        printf("%s\n", inet_ntoa(myen));
        myent->h_addr_list++;
    }
}
