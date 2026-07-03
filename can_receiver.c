/* This program creates the socket binds the program to the network to receive the CAN Messages */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<net/if.h>
#include<sys/ioctl.h>
#include<sys/socket.h>
#include<errno.h>
#include<linux/can.h>
#include<linux/can/raw.h>

int main()
{
	int socket_fd;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;
	int byte_received;
printf("[C Socker] Initializing the system connection....\n");

/* Allocate a Standard POSIX network socket for raw CAN Communication */
/* @param PF_CAN (Protocol Family CAN )domain
 * @param SOCK_RAW (Tells how much processing is required )type
 * @CAN_RAW protocol (This narrows down to the exact suntype of the layout used for classic CAN bus communication
 */
 
socket_fd = socket(PF_CAN,SOCK_RAW,CAN_RAW);
if(socket_fd < 0)
{
	printf("Failed to allocate network socket descriptor\n");
	printf("Error with ID is  : %d %s\n",errno,strerror(errno));
	return 1;
}
/* locate the physical interface index for string "can0"*/
strcpy(ifr.ifr_name,"can0");

/* then get th eunique ident number assigned to the ntwor using ioctl system call*/
/*
 * @param socket_id File descriptor
 * @param SIOCGIFINDEX Global Linux macro constant( Socket Interface Object Command Get IF INDEX)
 * @Param ifr is teh address of the iffr structure struct ifreq ifr
 */
 int ifr_id = 0;
ifr_id = ioctl(socket_fd,SIOCGIFINDEX,&ifr);

if(ifr_id < 0)
{
	printf("Failed to get interface index for cann0");
	close(socket_fd);
	return 1;
}
/* memset stands or memory set when we declare the struct sockaddr_can addr
 * memory has the random data and stack is not cleared automatically in RAM 
 * hence we initiakize the memorx with 0 so that the read data is not being stored wrongly
 * 
 * @param start address of the memory fromwhich initialization starts
 * @param 0 byte value
 * @param size how many bytes wouldbe initialized 
 */
 memset(&addr,0,sizeof(addr));
 addr.can_family = AF_CAN;
 addr.can_ifindex = ifr.ifr_ifindex;
 
 /* Bind our socket descriptor to the configured network interface address */
 if(bind(socket_fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
 {
	 printf("Error while binding the created network\n");
	 printf("Error is : %d %s\n",errno,strerror(errno));
	 return 1;
	 
 }
printf("[C Socket CAN ] sucessfully bound to can0 network.. Awaiting for automotive data frames\n");

/* Run th ewhile lopp to read the can frames continiusly \n*/
while(1)
{
	int bytes_read = read(socket_fd,&frame,sizeof(struct can_frame));
	
	if(bytes_read < 0)
	{
		printf("Error encountered while reading from the can socket\n");
		break;
	}
	printf("Bytes read are %d\n",bytes_read);
	
	/* check if the complete frame is received */
	if(bytes_read == sizeof(struct can_frame))
	{
		printf("------------------------------------------\n");
		printf("Received Frame ID = 0x%x\n",frame.can_id & CAN_EFF_MASK);
		printf("Data Length DLC = %d bytes \n",frame.can_dlc);
		printf("Raw payload Hex  : ");
		
		for(int i = 0;i<frame.can_dlc;i++)
		{
			printf("%02X ",frame.data[i]);
		}
		printf("\n -----------------------\n");
	}
	/*Encoding the incoming data */
	/* check if the received can id is the expected one to fingure out the type of data received */
	
	unsigned int can_id_received = frame.can_id & CAN_EFF_MASK;
	if(can_id_received == 0x100)
	{
		unsigned int  engine_rpm = 0;
		unsigned int vehicle_speed = 0;
		unsigned int coolant_temp = 0;
		
		/* 16 bit Engine RPM Data with Big endian */
		engine_rpm = (frame.data[0] << 8) | frame.data[1];
		
		/* Decode the byte 2 as a vehicle speed */
		vehicle_speed = frame.data[2];
		
		/* Decode Engine Coolant Temperature */
		coolant_temp = frame.data[3];
		
		/* Priting the virtual instrument clustur view */
		printf("\n=======================================\n");
		printf("       VIRTUAL INSTRUMENT CLUSTER        \n");
		printf("\n=======================================\n");
		printf("    Engine Speed : %u RPM\n",engine_rpm);
		printf("   VEHICLE SPEED : %u km/h\n",vehicle_speed);
		printf("   ENGINE TEMO : %u °C\n",coolant_temp);
		printf("=========================================\n"); 
		fflush(stdout);
	}
	else
	{
		printf("Received other can frame with the id : 0x%X\n",frame.can_id & CAN_EFF_MASK);
		fflush(stdout);
	}
	
}

//close the file descriptor channel to safely release system resources
close(socket_fd);
return 0;
}
