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
#include "sys/etimer.h"
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
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
PROCESS(adc_read_process, "ADC process");
AUTOSTART_PROCESSES(&udp_client_process, & adc_read_process);
/*---------------------------------------------------------------------------*/
//#pragma vector=ADC12_VECTOR
ISR(ADC12, adc_service_routine)
{

	ADC12CTL0 &= ~ENC;                         // Disable conversions

	//read ADC12MEM0 and process data
	tmp = ADC12MEM0;
	if((state == PING) && (ping_counter<MAX_SAMPLE_SZ))
        {
		buffer_ping[ping_counter] = tmp;
		ping_counter++;
		if( ping_counter == MAX_SAMPLE_SZ)
			state = PONG;
			
	}
	else if((state == PONG) && (pong_counter<MAX_SAMPLE_SZ))
	{
		buffer_pong[pong_counter] = tmp;
		pong_counter++;
		if( pong_counter >= MAX_SAMPLE_SZ)
			state = PING;
		
	}
	ADC12CTL0 |= ENC + ADC12SC;            // Trigger next conversion

}
/*ISR(ADC12, adc_service_routine)
{
	//dint();        
	ADC12CTL0 &= 0xFFFE;
	tmp = ADC12MEM0;
	switch(state)
	{
		case PING:
			buffer_ping[ping_counter] = tmp;
			ping_counter++;
			if( ping_counter >= MAX_SAMPLE_SZ)
			state = PONG;
		break;
		case PONG:
			buffer_pong[pong_counter] = tmp;
			pong_counter++;
			if( pong_counter >= MAX_SAMPLE_SZ)
			state = PING;

		break;

		/*case PING_1:
			buffer_ping1[ping1_counter] = tmp;
			ping1_counter++;
			if( ping1_counter >= MAX_SAMPLE_SZ)
			state = PONG_1;
		break;
		case PONG_1:
			buffer_pong1[pong1_counter] = tmp;
			pong1_counter++;
			if( pong1_counter >= MAX_SAMPLE_SZ)
			state = PING;
		break;
		default:
		break;
	}
	
	/*if((state == PING) && (ping_counter<MAX_SAMPLE_SZ))
        {
		buffer_ping[ping_counter] = tmp;
		ping_counter++;
		if( ping_counter == MAX_SAMPLE_SZ)
			state = PONG;
			
	}
	else if((state == PONG) && (pong_counter<MAX_SAMPLE_SZ))
	{
		buffer_pong[pong_counter] = tmp;
		pong_counter++;
		if( pong_counter >= MAX_SAMPLE_SZ)
			state = PING_1;
		
	}
	else if((state == PING_1) && (ping1_counter<MAX_SAMPLE_SZ))
	{
		buffer_ping1[ping1_counter] = tmp;
		ping1_counter++;
		if( ping1_counter == MAX_SAMPLE_SZ)
			state = PONG_1;
		
	}
	else if((state == PONG_1) && (pong1_counter<MAX_SAMPLE_SZ))
	{
		buffer_pong1[pong_counter] = tmp;
		pong1_counter++;
		if( pong1_counter == MAX_SAMPLE_SZ)
			state = PING;
		
	}*
	//eint();
	ADC12CTL0 |= ENC;
	
}*/

void configure_adc(void)
{
	
    P6DIR = 0xff;
    P6OUT = 0x00;
    P6SEL |= BIT7;                                               // Enable A/D channel A7
    ADC12CTL0 =  SHT0_8 + MSC; // Ref=2.5V
    ADC12CTL1   =  SHP +  /*ADC12DIV_5*/ ADC12DIV_5+ CONSEQ_2 + CSTARTADD_0;	
    ADC12MCTL0  = 0x17; //set the channel 7	// Verf internally generated which is 2.5V
	
    ADC12CTL0 |= ADC12ON + REFON;	/* ADC12ON ADC REF ON */
    ADC12IE = 0x01;                         // Enable ADC12IFG.0 
    ADC12CTL0 |= ENC;			/* enable conversion */
    ADC12CTL0 |= ADC12SC;		/* sample & convert */
    _BIS_SR(LPM0_bits + GIE);               // Enter LPM0,Enable interrupts
	
/*
    ADC12CTL0 = ADC12ON+SHT0_8+MSC;         // Turn on ADC12, set sampling time
    ADC12CTL1 = SHP+CONSEQ_2;               // Use sampling timer, set mode to repeated single-channel
    ADC12IE = 0x01;                         // Enable ADC12IFG.0 
    ADC12MCTL0  = 0x17; //set the channel 7	// Verf internally generated which is 2.5V	
    ADC12CTL0 |= ENC;                       // Enable conversions
    ADC12CTL0 |= ADC12SC;                   // Start conversion
    _BIS_SR(LPM0_bits + GIE);               // Enter LPM0,Enable interrupts
*/

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
  //PRINTF("DATA send to %d 'Hello %d'\n",
    //    server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1], seq_id);
  //sprintf(buf, "Hello1 %d from the client", seq_id);
  //uip_udp_packet_sendto(client_conn, buf, strlen(buf),
    //                    &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));*/
  buf[0] = seq_id;
  buf[1] = seq_id >> 8;
 
  //clock_delay(1);	
	for(i=2;i<78;i++)   
	   printf("%d",buf[i]);
	   printf("\n==============================================\n");
  uip_udp_packet_sendto(client_conn, buf, 78,&server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));


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
	
	/*if((ADC12IFG & 0x0001) == 0x0001)
        {
		if((state == PING) && (ping_counter<MAX_SAMPLE_SZ) )
                {
			buffer_ping[ping_counter] = ADC12MEM0;
			ping_counter++;
			if( ping_counter >= MAX_SAMPLE_SZ)
			state = PONG;
			
                }
		else if((state == PONG) && (pong_counter<MAX_SAMPLE_SZ) )
                {
			buffer_pong[pong_counter] = ADC12MEM0;
			pong_counter++;
			if (pong_counter >= MAX_SAMPLE_SZ)
			state = PING_1;
                }
		else if((state == PING_1) && (ping1_counter<MAX_SAMPLE_SZ) )
                {
			buffer_ping1[ping1_counter] = ADC12MEM0;
			ping1_counter++;
			if (ping1_counter >= MAX_SAMPLE_SZ)
			state = PONG_1;
                }
                else if((state == PONG_1) && (pong1_counter<MAX_SAMPLE_SZ) )
                {
			buffer_pong1[pong1_counter] = ADC12MEM0;
			pong1_counter++;
			if (pong1_counter >= MAX_SAMPLE_SZ)
			state = PING;
                }
		else
                    printf("Overflow \n");

        }
		printf("flag_tx %d \n",flag_tx);*/
	//printf("%d \n",ping_counter);
	if(ping_counter >= MAX_SAMPLE_SZ && tx_state==PING)
	{
		//memmove(buf+2, (unsigned char )buffer_ping,  66); 
		y = 1;			
		for(x=0;x<MAX_SAMPLE_SZ;x++)		
		{		
		    buf[y+1] = buffer_ping[x];
		    buf[y+2] = buffer_ping[x] >> 8;
		    y=y+2;						
		}
		ping_counter = 0;
		tx_state = PONG;
		
		//printf("Going to Transmit \n");
		//ctimer_set(&backoff_timer, 1, send_packet, NULL);
		send_packet(NULL);
                
	}
	if(pong_counter >= MAX_SAMPLE_SZ && tx_state == PONG)
	{
		//memmove(buf+2, (unsigned char )buffer_ping,  66); 
		y = 1;			
		for(x=0;x<MAX_SAMPLE_SZ;x++)		
		{		
		    buf[y+1] = buffer_pong[x];
		    buf[y+2] = buffer_pong[x] >> 8;			
		    y=y+2;			
		}
		pong_counter = 0;
		send_packet(NULL);
		//ctimer_set(&backoff_timer, 1, send_packet, NULL);
		tx_state = PING;
	}
	//process_poll(&adc_read_process);
        /* if(ping1_counter >= MAX_SAMPLE_SZ)
	{
		//memmove(buf+2, (unsigned char )buffer_ping,  66); 
		y = 1;			
		for(x=0;x<MAX_SAMPLE_SZ;x++)		
		{		
		    buf[y+1] = buffer_ping1[x];
		    buf[y+2] = buffer_ping1[x] >> 8;			
		    y=y+2;			
		}
		ping1_counter = 0;
		//ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);
		send_packet(NULL);
	}
	if(pong1_counter >= MAX_SAMPLE_SZ)
	{
		//memmove(buf+2, (unsigned char )buffer_ping,  66); 
		y = 1;			
		for(x=0;x<MAX_SAMPLE_SZ;x++)		
		{		
		    buf[y+1] = buffer_pong1[x];
		    buf[y+2] = buffer_pong1[x] >> 8;			
		    y=y+2;			
		}
		pong1_counter = 0;
		//send_packet(NULL);		
		ctimer_set(&backoff_timer, 2, send_packet, NULL);
	} */    
	
        process_poll(&adc_read_process);
       
       
}
PROCESS_THREAD(adc_read_process, ev, data)
{
	
	

        PROCESS_POLLHANDLER(pollhandler());
        

        PROCESS_BEGIN();
        
        PROCESS_PAUSE();
       
        //configure_dma();
	
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

  printf("%d \n", CLOCK_SECOND);
  
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
    
    //if(etimer_expired(&periodic)) {
      //PRINTF("Timer Expired ");	
      
      //etimer_reset(&periodic);
    //}

     if(ev==event_data_ready)
     {

	/*if(counter == 33)     
	{
		memmove(buf+2, buffer,  66);  
		counter = 0;    
		ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);
	        //printf("Counter %d \n",counter);
	}*/
      
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
