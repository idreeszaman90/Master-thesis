#ifndef SERIALCOMPONENT_H_
#define SERIALCOMPONENT_H_

void *ThreadFuncRecv(void *arg);


#include <termios.h>
#include <list>


class SerialComponent
{
public:
    SerialComponent();
    virtual ~SerialComponent();
    pthread_t ThreadRecv ;
    int SerialInit();
    struct termios oldtio, newtio;
    int PortID, serstat, sercmd, txDelay;
    void WriteBuff2Port(unsigned char *msg, int len);
    pthread_mutex_t SendQmutex, TxCondMtx, RtsMutex;
    pthread_cond_t txCond;
    struct timeval tp, nt;
    struct timespec ts;



};

extern unsigned int GetBaudrate(int baud);

#endif /*SERIALCOMPONENT_H_*/
