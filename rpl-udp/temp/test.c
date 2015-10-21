#include "contiki.h"
#include "lib/random.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/z1-phidgets.h"
#include "dev/i2cmaster.h"
#include "dev/light-ziglet.h"
#include "sys/timer.h"

#include "dev/temperature-sensor.h"
#include "dev/battery-sensor.h"
#include "node-id.h"

#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "sys/ctimer.h"
#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif
#include <stdio.h>
#include <string.h>

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define UDP_EXAMPLE_ID  190

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#ifndef PERIOD
#define PERIOD 10
#endif

#define START_INTERVAL		(3 * CLOCK_SECOND)
#define SEND_INTERVAL		(PERIOD * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		30


static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

 /* Variables: the application specific event value */
 static process_event_t event_data_ready;

 /*---------------------------------------------------------------------------*/
 /* We declare the two processes */
 PROCESS(temp_process, "Temperature process");
 PROCESS(print_process, "Print process");

 /* We require the processes to be started automatically */
 AUTOSTART_PROCESSES(&temp_process, &print_process);
 /*---------------------------------------------------------------------------*/


static void
tcpip_handler(void)
{
  char *str;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("DATA recv '%s'\n", str);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  static struct timer t;
  static int seq_id;

   uint16_t count1;
 
   seq_id=node_id;

   char buf[MAX_PAYLOAD_LEN];

   
  count1=0;

  seq_id++;
  //PRINTF("Temp = %dC, Bat = %dV \n",mytemp,mybatt);
  sprintf(buf, "persons in room %u \n",count1);
  uip_udp_packet_sendto(client_conn, buf, strlen(buf),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
  printf("Packet sent");
}

/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  printf("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

/* The choice of server address determines its 6LoPAN header compression.
 * (Our address will be compressed Mode 3 since it is derived from our link-local address)
 * Obviously the choice made here must also be selected in udp-server.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the 6LowPAN protocol preferences,
 * e.g. set Context 0 to aaaa::.  At present Wireshark copies Context/128 and then overwrites it.
 * (Setting Context 0 to aaaa::1111:2222:3333:4444 will report a 16 bit compressed address of aaaa::1111:22ff:fe33:xxxx)
 *
 * Note the IPCMV6 checksum verification depends on the correct uncompressed addresses.
 */
 
#if 0
/* Mode 1 - 64 bits inline */
   uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
#elif 1
/* Mode 2 - 16 bits inline */
  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
#else
/* Mode 3 - derived from server link-local (MAC) address */
  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}





 /* Implementation of the first process */
 PROCESS_THREAD(temp_process, ev, data)
 {
     // variables are declared static to ensure their values are kept
     // between kernel calls.
     static struct stimer t;
     static struct etimer timer;
     static int count = 0;
     int count1 =0;
     int delay,i;
   
     delay=200;
     // any process mustt start with this.
     PROCESS_BEGIN();

SENSORS_ACTIVATE(phidgets);

     /* allocate the required event */
     event_data_ready = process_alloc_event();

     /* Initialize the temperature sensor */
    
     // set the etimer module to generate an event in one second.
     etimer_set(&timer, CLOCK_CONF_SECOND/4);

     while (1)
     {
         // wait here for the timer to expire
         PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

         leds_toggle(LEDS_BLUE);


         
       i=0;
     
     if (phidgets.value(PHIDGET5V_2) >= 500)
     {
        //printf("check1\n");
     
         while(i!=500)
        {
            if (phidgets.value(PHIDGET5V_1) >= 500 && i<500)
            {
             printf("check2\n");
             count=count++;
             leds_toggle(LEDS_BLUE);
             printf("Persons in room %u\n",count);
             i=500;
             //timer_restart(&t);
           // clock_wait(delay);
           //  printf("hello");
             }
i=i++;
        }

      }

      
    i=0;
      if (phidgets.value(PHIDGET5V_1) >= 500)
    {
        printf("check2\n");
       // printf("sensor2 %d\n",phidgets.value(PHIDGET5V_2));
              //   printf("sensor1 %d\n",count);
       while(i!=500)
      {
         if (phidgets.value(PHIDGET5V_2) >= 500 && i<500)
         {
           printf("check1\n");
           count=count--;
           leds_toggle(LEDS_BLUE);
           printf("Persons in room %u\n",count);
          // clock_wait(delay);
          // printf("hello");
                 i=500;
         }
i=i++;
       }

    }


       count1 ++;

       /*  if (count1 == 100)
         {
             
             // post an event to the print process
             // and pass a pointer to the last measure as data
             process_post(&print_process, event_data_ready, &count);
         }*/

         // reset the timer so it will generate another event
         etimer_reset(&timer);
     }
     // any process must end with this, even if it is never reached.
     PROCESS_END();
 }
 /*---------------------------------------------------------------------------*/
 /* Implementation of the second process */
 PROCESS_THREAD(print_process, ev, data)
 {
     PROCESS_BEGIN();

     while (1)
     {
         // wait until we get a data_ready event
         PROCESS_WAIT_EVENT_UNTIL(ev == event_data_ready);

         // display it
         printf("temperature = %u\n", data);

     }
     PROCESS_END();
 }
 /*---------------------------------------------------------------------------*/
