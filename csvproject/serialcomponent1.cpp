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
#include<iostream>
#include <sys/mman.h>
#include <string>
#include <vector>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <sstream>
#include <iostream>
#include <cctype>
#include <ctime>
#include "CkCsv.h"

using namespace std;

#include "serialcomponent.h"

char RecvBuffer[1024];
bool bAlive;
FILE *fLog = NULL;

SerialComponent::SerialComponent()
{
    bAlive = false;

}

SerialComponent::~SerialComponent()
{
    pthread_exit(NULL);
}

unsigned int GetBaudrate(int baud)
{
    unsigned int BR=0;
    switch(baud)
    {
        case 50:
            BR=B50;
            break;
        case 150:
            BR=B150;
            break;
        case 300:
            BR=B300;
            break;
        case 600:
            BR=B600;
            break;
        case 1200:
            BR=B1200;
            break;
        case 2400:
            BR=B2400;
            break;
        case 4800:
            BR=B4800;
            break;
        case 9600:
            BR=B9600;
            break;
        case 19200:
            BR=B19200;
            break;
        case 115200:
            BR=B115200;
            break;
    }

    return (BR);
}


int SerialComponent::SerialInit()
{
    string devPort = "/dev/ttyUSB0";
    printf("opening %s: ", devPort.c_str());
    unsigned int baud = 115200;
    printf(" baud rate : %d \n", baud);

    PortID = open(devPort.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);// | O_NDELAY);
    printf("\n PortID on Serial Init is %d\n", PortID);

    if(PortID <= 0)
    {
        printf("\nSerial Port Cannot be Opened !");
        return PortID;
    } else {
        printf("Serial Port Opened Sucessfully !\n");
    }

    tcgetattr(PortID, &oldtio);
    memcpy(&newtio, &oldtio, sizeof(struct termios));



    newtio.c_cflag = 0 |GetBaudrate(baud)| CS8 | CLOCAL| CREAD ;
    //newtio.c_iflag = IGNPAR|ICRNL;
    newtio.c_iflag = IGNPAR; // Ignore Parity
    newtio.c_oflag = 0;
    newtio.c_lflag = ~(ICANON|ECHO|ECHOE|ISIG); // for sending Raw packet
    newtio.c_oflag = ~OPOST;
    /*
        If MIN > 0 and TIME = 0, MIN sets the number of characters to receive before the read is satisfied. As TIME
                                is zero, the timer is not used.
        If MIN = 0 and TIME > 0, TIME serves as a timeout value. The read will be satisfied if a single character is
                                read, or TIME is exceeded (t = TIME *0.1 s). If TIME is exceeded, no character will
                                be returned.
        If MIN > 0 and TIME > 0, TIME serves as an inter−character timer. The read will be satisfied if MIN
                                characters are received, or the time between two characters exceeds
                                TIME. The timer is restarted every time a character is received and only becomes
                                active after the first character has been received.
        If MIN = 0 and TIME = 0, read will be satisfied immediately. The number of characters currently available,
                                or the number of characters requested will be returned. According to
                                Antonino (see contributions), you could issue a fcntl(fd, F_SETFL, FNDELAY); before
                                reading to get the same result.
     */

    newtio.c_cc[VTIME] =1;   /* inter-character timer 10th of sec*/
    newtio.c_cc[VMIN]  = 1; // pgMib->macMib->maxFragLen;   /* blocking read until chars received */

    //printf("CTS current value is %X\n",oldtio.c_cflag&CRTSCTS);
    //tcflush(PortID, TCIFLUSH);

    if (tcsetattr(PortID, TCSANOW, &newtio) != 0)
    {
        printf("\nSerial Communication not Enabled");
    }
    else
    {
        // Serial port opened need a receiving thread
        int rt = pthread_create(&ThreadRecv, NULL, ThreadFuncRecv, (void*)this);
        if(rt<0)
        {
            printf("Error Creating ThreadFuncRecv\n");
        }


        printf("ThreadFuncRecv Created:\n");
    }
    return 0;
}


void SerialComponent::WriteBuff2Port(unsigned char *msg, int len)
{
    /*
    printf("\n");
    for(int a=0; a< len ;a++)
    {
        printf("%02X ",*(msg +a));
    }
    printf("\n");
    */

    int SendLen;

    SendLen=write(PortID, msg, len);
    if(SendLen <= 0)
    {
        printf("Message Not Sent \n");
        fflush(stdout);
    }
    printf("[%d] byte Message Sent",len);
}

int dayofweek(int d, int m, int y)
{
    char mo[15]="Monday";
    char tu[15]="Tuesday";
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

    y -= m < 3;
    
return ( y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;



}


void *ThreadFuncRecv(void*arg)
{
    //std::vector<int> kitchen;
    std::vector<int> motion;
    std::vector<int> kitchen_door;
    std::vector<int> window_status;
    std::vector<int> persons;
    std::vector<int> chair_status;
    CkCsv csv;

    fLog = fopen("Node_data.txt", "wb");
    SerialComponent *pSer = (SerialComponent*)arg;
    printf("ThreadFuncRecv Started:\n");
    int ReceiveLen;
    int max_fd;
    fd_set input;
    struct timeval timeout;
    
    char recev_string_true[] = "at_kitchen_on";
    char recev_string_false[] = "at_kitchen_off";    
    unsigned int index=0;
    time_t start = time(0);

    FD_ZERO(&input);
    FD_SET(pSer->PortID, &input);
    max_fd = 1;

    bAlive = true;
    while(bAlive)
    {
	
        timeout.tv_sec = 20;    
        timeout.tv_usec = 0;

        FD_SET(pSer->PortID, &input);
        max_fd = pSer->PortID+1;
        int retVal = select(max_fd, &input, NULL, NULL, &timeout);
        if(retVal == -1)
        {
            perror("Error in select\n");
            continue;
        }
        else if(FD_ISSET(pSer->PortID, &input))
        {
            printf("Select is working OK \n");
        }
        else
        {
            printf("time-out\n");
            continue;
        }
		
        int readyBytes = 0;
         
       ioctl(pSer->PortID, FIONREAD, &readyBytes);
        
       
        if (readyBytes==0)
        {
        //printf("Still no data received \n");
            continue;
        }
  	printf("Received some data \n");
       
	    time_t timer1;   //initializing variable for printing time
        char  buffer_time[26];
        struct tm* tm_info;

        time(&timer1);
        tm_info =localtime(&timer1);
        
        strftime(buffer_time,26,"%d:%m:%Y %H:%M:%S \n", tm_info);
 
        	   
        int n=0;
        ReceiveLen = 1;
        while(n<readyBytes && ReceiveLen>0)
        {
            ReceiveLen = read(pSer->PortID, &RecvBuffer[n],readyBytes);
            if(ReceiveLen == -1) break;
            n+=ReceiveLen;
        }

        if (n ==0)
        {
            printf("Cannot receive: Error %d\n", errno);
            fflush(stdout);
            continue;
        }
        else
        {
            printf("Received data is : \n");
            
            if (fLog == NULL)
                return NULL;

          printf("length of buffer =%d \n",n);

  printf(" ******************************check4\n");                           

     char s1 [1024];
     char s2 [3];
    std::string temp;
    std::string temp1;
    
int result = n * 60 / 100;

strncpy(s1, RecvBuffer, n);
/*   
   for(int a=result; a< n ;a++)
	{

		s1[a-result]=RecvBuffer[a];

	}

*/
printf(" ******************************check5\n");
 printf (" String 1: %s\n",s1);
/////////////////////////////////////////////////////////////

    csv.put_HasColumnNames(true);

    csv.SetColumnName(0,"Week_day");
    csv.SetColumnName(1,"Time_of_day");
    csv.SetColumnName(2,"Motion0");
    csv.SetColumnName(3,"Kitchen_door");
    csv.SetColumnName(4,"Window_status");    
    csv.SetColumnName(5,"no of persons");
    csv.SetColumnName(6,"Chairs_occupied");
    csv.SetColumnName(7,"Cup_board");   
    csv.SetColumnName(8,"Motion1");
    csv.SetColumnName(9,"Appliance used");

    
    




////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////

time_t now = time(0);

    tm *ltm = localtime(&now);

   // print various components of tm structure.
int year;
int month;
int date;
int day;
int hour;

year=1900 + ltm->tm_year;
month=1 + ltm->tm_mon;
date=ltm->tm_mday;

    day = dayofweek(date, month, year);
switch (day)
{
  case 1:
      csv.SetCell(index,0,"1");
     break;
   case 2:
csv.SetCell(index,0,"2");
     break;
 case 3:
   csv.SetCell(index,0,"3");
     break;
 case 4:
csv.SetCell(index,0,"4");
     break;
 case 5:
    csv.SetCell(index,0,"5");
     break;
 case 6:
    csv.SetCell(index,0,"6");
     break;
 case 7:
     csv.SetCell(index,0,"7");
     break;
 }

 
                
    





/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////


hour=1 + ltm->tm_hour;

//string my_str(buffer_time);
//const char* conv_my_str = my_str.c_str();

if(hour>=6 && hour<=11 )
{
csv.SetCell(index,1,"1");
}

if(hour>11 && hour<=14 )
{
csv.SetCell(index,1,"2");
}

if(hour>14 && hour<=21 )
{
csv.SetCell(index,1,"3");
}

if(hour>21 && hour<=24 )
{
csv.SetCell(index,1,"4");
}
if(hour>=0 && hour<6 )
{
csv.SetCell(index,1,"4");
}


/////////////////////////////////////////////////////////

/////////////// Movement detection /////////////////// 
           
                if(strstr(s1,"Motion = 1") != NULL)
                 {
                motion.push_back(1)
                 //   printf("Movement detected\n");
                     csv.SetCell(index,2,"1");
                
                 }

               if(strstr(s1,"Motion = 0") != NULL)
                  {
motion.push_back(0)
                    csv.SetCell(index,2,"0");
                 //   printf("No Movement detected\n");   

                  }
////////////////////////////////////////////////////


//////////// Kitchen door opened or closed ////////
               
                if(strstr(s1,"Kitchen door = 1") != NULL)
                 {
                
                 //   printf("Kitchen door opened\n");
                   csv.SetCell(index,3,"1");
                
                 }
               if(strstr(s1,"Kitchen door = 0") != NULL)
                 {
                
                 //   printf("Kitchen door closed\n");
                   csv.SetCell(index,3,"0");
                
                 }

///////////////////////////////////////////////////

///////////////// Window status ///////////////////

               if(strstr(s1,"Window status = 0") != NULL)
                 {
                
                  //  printf("Window closed\n");
                   csv.SetCell(index,4,"0");
                
                 }

               if(strstr(s1,"Window status = 1") != NULL)
                 {
                
                  //  printf("Window partially opened\n");
                    csv.SetCell(index,4,"1");
                
                 }

               if(strstr(s1,"Window status = 2") != NULL)
                 {
                
                 //   printf("Window completely opened\n");
                  csv.SetCell(index,4,"2");
                
                 }
///////////////////////////////////////////////////

/////////////////Persons in kitchen////////////////
                 
               if(strstr(s1,"persons in room ") != NULL)
                 {
    
		temp1=s1;

       		int number=0;
    
                for (unsigned int i=0; i < temp1.size(); i++)
    {
   
        if (isdigit(temp1[i]))
        {
            for (unsigned int a=i; a<temp1.size(); a++)
            {
                temp += temp1[a];               
            }
               break;
        }    
    }
    
    std::istringstream stream(temp);
    stream >> number;

   stringstream temp_str;
   temp_str<<(number);
   std::string str = temp_str.str();
   const char* cstr2 = str.c_str();   


  //  const char * c = temp.c_str();




               
  //  for (unsigned int tt=0;tt<20;tt++)
//{
 //                printf("%d",number);
//}

               if (number>0)
{
                    csv.SetCell(index,5,cstr2);
 }               
                 }

////////////////////////////////////////////////////////////


////////////////// Chair status ///////////////////////////

 if(strstr(s1,"Chair occupied 0") != NULL)
                 {
                
                 //   printf("chair empty\n");
                    csv.SetCell(index,6,"0");
                
                 }

if(strstr(s1,"Chair occupied 1") != NULL)
                 {
                
                 //   printf("chair occupied\n");
                    csv.SetCell(index,6,"1");
                
                 }


////////////////////////////////////////////////////////


////////////////// Cup board status ///////////////////////////

 if(strstr(s1,"Cup board = 0") != NULL)
                 {
                
                  //  printf("cup board closed\n");
                    csv.SetCell(index,7,"0");
                
                 }

//if(strstr(s1,"Cup board = 1") != NULL)
                 {
                
                 //   printf("cup board opened\n");
                    csv.SetCell(index,7,"1");
                
                 }


////////////////////////////////////////////////////////


///////////////////////////Motion1/////////////////////


 if(strstr(s1,"motion1= 0") != NULL)
                 {
                
                  //  printf("cup board closed\n");
                    csv.SetCell(index,8,"0");
                
                 }

if(strstr(s1,"motion1= 1") != NULL)
                 {
                
                 //   printf("cup board opened\n");
                    csv.SetCell(index,8,"1");
                
                 }





















/////////////////////////////////////////////////////////


////////////////// Appliance used ///////////////////////////
if(strstr(s1,"coffee machine = 0") != NULL)
                 {
                
                    csv.SetCell(index,9,"0");
                
                 }


if(strstr(s1,"coffee machine = 1") != NULL)
                 {
                
                    csv.SetCell(index,9,"1");
                
                 }

if(strstr(s1,"coffee machine = 2") != NULL)
                 {
                
                    csv.SetCell(index,9,"2");
                
                 }

if(strstr(s1,"coffee machine = 3") != NULL)
                 {
                
                    csv.SetCell(index,9,"3");
                
                 }

if(strstr(s1,"microwave = 1") != NULL)
                 {
                
                    csv.SetCell(index,9,"1");
                
                 }
if(strstr(s1,"fridge = 1") != NULL)
                 {
                
                    csv.SetCell(index,9,"1");
                
                 }

//if(strstr(s1,"hello 123123") == NULL)
                // {
                
                //    csv.SetCell(index,8,"No appliance used");
                
             //    }


///////////////////////////////////////////////

int seconds_since_start = difftime( time(0), start);
printf("%d && %d\n",seconds_since_start,index);
if(seconds_since_start>=3)
{
motion.clear();
index=index++;
printf("index value %d",index);
start = time(0);
}


////////////////////////////////////////////////////////

/*
for(int a=49; a< n ;a++)
{
                     
                fprintf(fLog,"%c",RecvBuffer[a]);
 }          


           printf("\n");
*/

printf(" ******************************check1\n");

/////////////////////////Making CSV file/////////////////////////////

   const char * csvDoc;
    csvDoc = csv.saveToString();
   // printf("%s\n",csvDoc);

printf(" ******************************check2\n");

    bool success;

    //  Save the CSV to a file:
    success = csv.SaveFile("out.csv");

printf(" ******************************check3\n");
    if (success != true) {
        printf("%s\n",csv.lastErrorText());
    }


/////////////////////////////////////////////////////////////////////

/*for(unsigned int i=0; i< motion.size() ;i++)
{
          //  printf("Motion : %d\n",motion[i]);
              fputs(buffer_time,fLog);
              fprintf(fLog," Motion : %d\n",motion[i]);
}


for(unsigned int i=0; i< kitchen_door.size() ;i++)
{
          //  printf("Kitchen door : %d\n",kitchen_door[i]);
          
fputs(buffer_time,fLog);
fprintf(fLog," Kitchen door : %d\n",kitchen_door[i]);
}
 
for(unsigned int i=0; i< window_status.size() ;i++)
{
          //  printf("Window status : %d\n",window_status[i]);
            fputs(buffer_time,fLog);
            fprintf(fLog," Window status : %d\n",window_status[i]);
}         

for(unsigned int i=0; i< persons.size() ;i++)
{
fputs(buffer_time,fLog);
          //  printf("Persons in kitchen : %d\n",persons[i]);
            fprintf(fLog,"%c Persons in kitchen : %d\n",buffer_time,persons[i]);
}         


for(unsigned int i=0; i< chair_status.size() ;i++)
{
fputs(buffer_time,fLog);
           // printf("Chair status : %d\n",chair_status[i]);
            fprintf(fLog," Chair status : %d\n",chair_status[i]);
} 
*/

 // puts(buffer_time);
           // printf("\n");

          //  fputs(buffer_time,fLog);
               
          //  printf("\n");
 	  
//
          //  fflush(fLog);



        }

    } // Main while loop
    return NULL;
}

