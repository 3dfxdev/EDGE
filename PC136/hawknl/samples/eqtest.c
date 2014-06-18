/* the EverQuest protocol is documented at
   http://everquest.station.sony.com/support/general/server_status_protocal.jsp*/

#include <stdio.h>
#include "nl.h"

int main(int argc, char **argv)
{
    NLsocket    sock;
    NLaddress   addr;
    NLbyte      server[] = "status.everquest.com";
    NLushort    port = 24252;
    NLbyte      command[] = {0xFF, 0xFF, 0x09, 0x00};
    NLenum      type = NL_UNRELIABLE; /* UDP */
    NLbyte      buffer[1024];
    NLint       count;

    if(!nlInit())
       return 1;
    if(!nlSelectNetwork(NL_IP))
    {
        nlShutdown();
        return 1;
    }
    nlEnable(NL_BLOCKING_IO);
    /* create server the address */
    nlGetAddrFromName(server, &addr);
    nlSetAddrPort(&addr, port);
    /* create the socket */
    sock = nlOpen(0, type);
    if(sock == NL_INVALID)
    {
        nlShutdown();
        return 1;
    }
    /* set the destination address */
    nlSetRemoteAddr(sock, &addr);
    /* send the message */
    nlWrite(sock, (NLvoid *)command, (NLint)sizeof(NLulong));
    /* read the reply */
    count = nlRead(sock, (NLvoid *)buffer, (NLint)sizeof(buffer));
    if(count > 0)
    {
        printf("Banner is: %s\n", &buffer[4]);
    }
    nlShutdown();
    return 0;
}

