/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "sys/ctimer.h"
#include "sys/clock.h"
#include "sys/timer.h"
#include "sys/etimer.h"
#include "dev/leds.h"
#include "sys/energest.h"
#include "isr_compat.h"
#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif
#include <stdio.h>
#include <string.h>


#define UDP_CLIENT_PORT 8766
#define UDP_SERVER_PORT 5677

#define UDP_EXAMPLE_ID  190

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#ifndef PERIOD
#define PERIOD 40
#endif
#define MAX_SAMPLE_SZ		38
#define START_INTERVAL		(1 * CLOCK_SECOND)
#define SEND_INTERVAL		(2)
#define SEND_TIME		(random_rand() % (16))
#define MAX_PAYLOAD_LEN		80
#define PING			1
#define PONG			2
#define PING_1                  3
#define PONG_1                  4
/* Callback pointers when interrupt occurs */
void (*adc12_7_cb)(uint8_t reg);


static process_event_t event_data_ready;
static unsigned  char buf[MAX_PAYLOAD_LEN+1];
static unsigned  short buffer_ping[40];
static unsigned  short buffer_ping1[40];
static unsigned  short buffer_pong[40];
static unsigned  short buffer_pong1[40];
static unsigned  short tmp=0;
static unsigned  short tmp1=0;
uint16_t count=0;
uint16_t trig;
uint16_t trig1;
unsigned int delay=200; 
static unsigned  char ping_counter = 0;
static unsigned  char pong_counter = 0;
static unsigned  char ping1_counter = 0;
static unsigned  char pong1_counter = 0;
static unsigned  char counter = 0;
static unsigned  char state=PING; 
unsigned static  char x=0,y=0;
unsigned static char tx_state=PING;
static unsigned  short tx_result=0;
static unsigned short *ptr;
static unsigned short flag_tx=0;
static struct ctimer backoff_timer;
static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;
uint16_t count;
uint16_t t;
static uint8_t adc_on;
static clock_time_t start_time;
static clock_time_t end_time;
int ii;
unsigned int img;
unsigned int img1;
unsigned int myArray[3] = { 0, 0, 0};
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
PROCESS(adc_read_process, "ADC process");
AUTOSTART_PROCESSES(&udp_client_process, & adc_read_process);
/*---------------------------------------------------------------------------*/
//#pragma vector=ADC12_VECTOR
ISR(ADC12, adc_service_routine)
{
printf("check 1\n");
	ADC12CTL0 &= ~ENC;                         // Disable conversions

	
	ADC12CTL0 |= ENC + ADC12SC;            // Trigger next conversion
printf("check 2\n");

}


void configure_adc(void)
{
	
printf("check 3\n");
       

 uint8_t pre = adc_on;

   
    P6DIR = 0xff;
    P6OUT = 0x00;
    P6SEL |= 0x8b; /* bit 7 + 3 + 1 + 0 */

    /* if nothing was started before, start up the ADC system */
    /* Set up the ADC. */
    ADC12CTL0 = REF2_5V + SHT0_6 + SHT1_6 + MSC; /* Setup ADC12, ref., sampling time */
    ADC12CTL1 = SHP + CONSEQ_3 + CSTARTADD_0;	/* Use sampling timer, repeat-sequenc-of-channels */
    /* convert up to MEM4 */

   ADC12MCTL0 = (INCH_0 + SREF_0);
          /* MemReg7 == P6.3/A3 == 5V 2 */
          ADC12MCTL1 = (INCH_3 + SREF_0);
          /* MemReg8 == P6.1/A1 == 3V 1 */
          ADC12MCTL2 = (INCH_1 + SREF_0);
          /* MemReg9 == P6.7/A7 == 3V_2 */
          ADC12MCTL3 = (INCH_7 + SREF_0);
   // ADC12MCTL4 |= EOS;

    ADC12CTL0 |= ADC12ON + REFON;
    ADC12CTL0 |= ENC;		/* enable conversion */
    ADC12CTL0 |= ADC12SC;		/* sample & convert */
    _BIS_SR(LPM0_bits + GIE);               // Enter LPM0,Enable interrupts
	

printf("check 4\n");
}

void configure_dma(void)
{
	printf("DMA Configuration Started \n");

	//DMACTL0 = DMA0TSEL_0;// dma 0
        DMACTL0 = DMA0TSEL_6;

        DMACTL1 = 0x0000;
        
        DMA0SA = &ADC12MEM0;//0x0140;// Address of ADC12MEM0
        
        //DMA0DA = &dma_dst_buf; // For Single Tranfer

        //DMA0SZ = 1; // For Single Transfer
	DMA0SZ = 33; // For Block Transfer
        

        printf("DMACTL0 %x %d \n",DMACTL0,DMACTL0);

        printf("DMACTL1 %x %d \n",DMACTL1,DMACTL1);

        printf("DMA0SA  %02d %x\n",DMA0SA,DMA0SA);
        printf("DMA0SA %x  %d \n",DMA0SA,DMA0SA);
	
	//printf("Buf Value is %d %d\n",buffer[0],buffer[1]);
        //DMA0CTL = DMADT_1 | DMADSTINCR_3 | DMASRCINCR_0; // For Block transfer
	DMA0CTL = DMADT_0 | DMADSTINCR_3 | DMASRCINCR_0; //for single transfer
	//deactivate_adc();
	DMA0CTL |= DMAEN | DMAREQ;

        

}

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
  //static int seq_id;
  static unsigned char seq_id=0,i=0; 
  
  
  
  seq_id++;
 
	for(i=2;i<20;i++)   
printf("\n==============================================\n");
 sprintf(buf, "coffee machine = %u\n",count);

  uip_udp_packet_sendto(client_conn, buf, strlen(buf),&server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
printf("packet sent\n");

}/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
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

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x1F8);
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
  //uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00, 0x00, 1);
#else
/* Mode 3 - derived from server link-local (MAC) address */
  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}
static void pollhandler(void)
{
start_time=clock_seconds();	
trig=0;

///////////////////////////////////////////////////////


//printf("%u \n",ADC12MEM0);
printf("%d\n",img);

if(ADC12MEM0 <= 500)
{
	count=0; //everything off
	leds_off(LEDS_ALL);

      if( clock_time() % 100 == 0)
	{ 
	send_packet(NULL);
	}
}


while(ADC12MEM0 > 350 && ADC12MEM0 <=450)
{

	
  	 myArray[0]=ADC12MEM0;
start_time=clock_time();
	while(clock_time()<=start_time+30)
    {
printf("%d\n",img);
     }
myArray[2]=ADC12MEM0;

		img=(myArray[2]-myArray[0])*(myArray[2]-myArray[0]);
                img1=(myArray[0]-myArray[2])*(myArray[0]-myArray[2]);
        if (img>20 || img1>20)
		{
	for(ii=0;ii<20;ii++)
    {
printf("check\n");
     }			
break;
		}
	printf("%d\n",img);

leds_on(LEDS_GREEN);
leds_off(LEDS_RED);
leds_off(LEDS_BLUE);
count=1;  // grill on



    if( clock_time() % 100 == 0)
{ 
send_packet(NULL);
}

}


while(ADC12MEM0 > 750 && ADC12MEM0 <=850)
{


 	
  	 myArray[0]=ADC12MEM0;
start_time=clock_time();
	while(clock_time()<=start_time+30)
    {
printf("%d\n",img);
     }
myArray[2]=ADC12MEM0;

		img=(myArray[2]-myArray[0])*(myArray[2]-myArray[0]);
img1=(myArray[0]-myArray[2])*(myArray[0]-myArray[2]);
        if (img>20 || img1>20)
		{
	for(ii=0;ii<20;ii++)
    {
printf("check\n");
     }			
break;
		}
	printf("%d\n",img);

leds_on(LEDS_BLUE);
leds_off(LEDS_RED);
leds_off(LEDS_GREEN);
count=2;  // microwave on


    if( clock_time() % 100 == 0)
{ 
send_packet(NULL);
}
}


while(ADC12MEM0 > 1000)
{

	
  	 myArray[0]=ADC12MEM0;
start_time=clock_time();
	while(clock_time()<=start_time+30)
    {
printf("%d\n",img);
     }
myArray[2]=ADC12MEM0;

		img=(myArray[2]-myArray[0])*(myArray[2]-myArray[0]);
img1=(myArray[0]-myArray[2])*(myArray[0]-myArray[2]);
        if (img>20 || img1>20)
		{
	for(ii=0;ii<20;ii++)
    {
printf("check\n");
     }			
break;
		}
	printf("%d\n",img);

leds_on(LEDS_RED);
leds_off(LEDS_BLUE);
leds_off(LEDS_GREEN);
count=3;  // grill + microwave on

    if( clock_time() % 100 == 0)
{ 
send_packet(NULL);
}


}






        process_poll(&adc_read_process);
    //   printf("check 6\n");
       
}
PROCESS_THREAD(adc_read_process, ev, data)
{
	
	

        PROCESS_POLLHANDLER(pollhandler());
        
        PROCESS_BEGIN();
        
        PROCESS_PAUSE();
       
        process_poll(&adc_read_process);

	while(1)
        {
		PROCESS_YIELD();

        }




 
    



	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer;




#if WITH_COMPOWER
  static int print = 0;
#endif

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  set_global_address();
  
  PRINTF("UDP client process started\n");

  print_local_addresses();

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL); 
  if(client_conn == NULL) {
    PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT)); 

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

 // printf("%d \n", CLOCK_SECOND);
  
#if WITH_COMPOWER
  powertrace_sniff(POWERTRACE_ON);
#endif
   configure_adc();
  //etimer_set(&periodic, SEND_INTERVAL);
while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
//      tcpip_handler();
    }
    
 
     if(ev==event_data_ready)
     {

      
#if WITH_COMPOWER
      if (print == 0) {
	powertrace_print("#P");
      }
      if (++print == 3) {
	print = 0;
      }
#endif

    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
