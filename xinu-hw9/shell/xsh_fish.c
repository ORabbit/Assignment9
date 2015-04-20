/**
 * @file     xsh_fish.c
 * @provides xsh_fish
 *
 */
/* Embedded XINU, Copyright (C) 2013.  All rights reserved. */

#include <xinu.h>

/**
 * Local function for sending simply FiSh packets with empty payloads.
 * @param dst MAC address of destination
 * @param type FiSh protocal type (see fileshare.h)
 * @return OK for success, SYSERR otherwise.
 */
static int fishSend(uchar *dst, char fishtype)
{
	uchar packet[PKTSZ];
	uchar *ppkt = packet;
	int i = 0;

	// Zero out the packet buffer.
	bzero(packet, PKTSZ);

	for (i = 0; i < ETH_ADDR_LEN; i++)
	{
		*ppkt++ = dst[i];
	}

	// Source: Get my MAC address from the Ethernet device
	control(ETH0, ETH_CTRL_GET_MAC, (long)ppkt, 0);
	ppkt += ETH_ADDR_LEN;
		
	// Type: Special "3250" packet protocol.
	*ppkt++ = ETYPE_FISH >> 8;
	*ppkt++ = ETYPE_FISH & 0xFF;
		
	*ppkt++ = fishtype;
		
	for (i = 1; i < ETHER_MINPAYLOAD; i++)
	{
		*ppkt++ = i;
	}
//printf("ppkt: %X\n", ppkt);
	write(ETH0, packet, ppkt - packet);
	
	return OK;
}

/**
 * Local function for sending simply FiSh packets with payloads.
 * @param dst MAC address of destination
 * @param type FiSh protocal type (see fileshare.h)
 * @param payload
 * @param payload length
 * @return OK for success, SYSERR otherwise.
 */
static int fishSendPayload(uchar *dst, char fishtype, char payload[], int length)
{
	uchar packet[PKTSZ];
	uchar *ppkt = packet;
	int i = 0;

	// Zero out the packet buffer.
	bzero(packet, PKTSZ);

	for (i = 0; i < ETH_ADDR_LEN; i++)
	{
		*ppkt++ = dst[i];
	}

	// Source: Get my MAC address from the Ethernet device
	control(ETH0, ETH_CTRL_GET_MAC, (long)ppkt, 0);
	ppkt += ETH_ADDR_LEN;
		
	// Type: Special "3250" packet protocol.
	*ppkt++ = ETYPE_FISH >> 8;
	*ppkt++ = ETYPE_FISH & 0xFF;
		
	*ppkt++ = fishtype;	
//printf("%s\n", payload);	
	for(i = 0; i < length; i++)
		*ppkt++ = (uchar) payload[i];
//	for (i = length; i < ETHER_MINPAYLOAD - 1; i++)
	for (i = 1; i < ETHER_MINPAYLOAD - length; i++)
	{
		*ppkt++ = '\0';
	}
		
//printf("ppkt: %X\n", ppkt);
	write(ETH0, packet, ppkt - packet);
	
	return OK;
}

// Finds node in school[] and returns its index. Returns -1 if not found.
int fishLocateNode(char toFind[])
{
	int i;
	for (i = 0; i < SCHOOLMAX; i++)
		if (school[i].used && strncmp(school[i].name, toFind, FISH_MAXNAME) == 0)
			return i;
	return -1;
}
		
/**
 * Shell command (fish) is file sharing client.
 * @param args array of arguments
 * @return OK for success, SYSERR for syntax error
 */
command xsh_fish(ushort nargs, char *args[])
{
	uchar bcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	int i = 0;

	if (nargs == 2 && strncmp(args[1], "ping", 4) == 0)
    {
		fishSend(bcast, FISH_PING);
		
		sleep(1000);

		printf("Known FISH nodes in school:\n");
		for (i = 0; i < SCHOOLMAX; i++)
		{
			if (school[i].used)
			{
				printf("\t%02X:%02X:%02X:%02X:%02X:%02X",
					   school[i].mac[0],
					   school[i].mac[1],
					   school[i].mac[2],
					   school[i].mac[3],
					   school[i].mac[4],
					   school[i].mac[5]);
				printf("\t%s\n", school[i].name);
			}
		}
		printf("\n");
		return OK;
	}
	else if (nargs == 3 && strncmp(args[1], "list", 4) == 0)
	{
		if((i = fishLocateNode(args[2])) == -1)
			{printf("No FiSh \"%s\" found in school.\n", args[2]); return SYSERR;}

		fishSend(school[i].mac, FISH_DIRASK);

		sleep(1000);

		printf("Files at node \"%s\":\n", args[2]);
		for(i = 0; i < DIRENTRIES; i++)
			if(fishlist[i][0] != '\0')
				printf("%s\n", fishlist[i]);

		// TODO: Locate named node in school,
		//   and send a FISH_DIRASK packet to it.
		//   Wait one second for reply to come in, and
		//   then print contents of fishlist table.
		
		return OK;
	}
	else if (nargs == 4 && strncmp(args[1], "get", 4) == 0)
	{
		// TODO: Locate named node in school,
		//   and send a FISH_GETFILE packet to it.
		//   FileSharer puts file in system when it arrives.
		if((i = fishLocateNode(args[2])) == -1)
			{printf("No FiSh \"%s\" found in school.\n", args[2]); return SYSERR;}
		
		fishSendPayload(school[i].mac, FISH_GETFILE, args[3], FNAMLEN);

		//sleep(1000); // Maybe? Doesn't say we have to
//printf("FISH_GETFILE: %d\n", FISH_GETFILE);
//		printf("NOTHING HERE YET\n");
		return OK;
	}
	else
    {
        fprintf(stdout, "Usage: fish [ping | list <node> | get <node> <filename>]\n");
        fprintf(stdout, "Simple file sharing protocol.\n");
        fprintf(stdout, "ping - lists available FISH nodes in school.\n");
        fprintf(stdout, "list <node> - lists directory contents at node.\n");
        fprintf(stdout, "get <node> <file> - requests a remote file.\n");
        fprintf(stdout, "\t--help\t display this help and exit\n");

        return 0;
    }



    return OK;
}
