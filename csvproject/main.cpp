


//============================================================================
// Name        : Serial communication.cpp
// Author      : wajid
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>


#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <sys/ioctl.h>
/////
#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <strstream>
#include <string>
#include <fstream>
#include <iostream>
#include "serialcomponent.h"

using namespace std;



int main (void)
{
    SerialComponent obj;
    obj.SerialInit();
    char ch = 0;
    while (ch != 'x')
    {
            cin >> ch;
    }
    cout << "Program Ends:" << endl;

    return 0;
}
