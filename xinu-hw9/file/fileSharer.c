/* fileSharer.c - fileSharer */
/* Copyright (C) 2007, Marquette University.  All rights reserved. */
/*Modified by Casey O'Hare and Samuel Ostlund */

#include <kernel.h>
#include <xc.h>
#include <file.h>
#include <fileshare.h>
#include <ether.h>
#include <network.h>
#include <nvram.h>

struct fishnode school[SCHOOLMAX];
char   fishlist[DIRENTRIES][FNAMLEN];

static uchar bcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uchar myMAC[ETH_ADDR_LEN];

int fishAnnounce(uchar *packet)
{
	struct ethergram *eg = (struct ethergram *)packet;
	int i = 0;

	for (i = 0; i < SCHOOLMAX; i++)
	{
		/* Check to see if node already known. */
		if (school[i].used && 
			(0 == memcmp(school[i].mac, eg->src, ETH_ADDR_LEN)))
			return OK;
	}
	for (i = 0; i < SCHOOLMAX; i++)
	{
		/* Else find an unused slot and store it. */
		if (!school[i].used)
		{
			school[i].used = 1;
			memcpy(school[i].mac, eg->src, ETH_ADDR_LEN);
			memcpy(school[i].name, eg->data + 1, FISH_MAXNAME);
			//printf("Node %s\n", school[i].name);
			return OK;
		}
	}
	return SYSERR;
}

/*------------------------------------------------------------------------
 * fishPing - Reply to a broadcast FISH request.
 *------------------------------------------------------------------------
 */
void fishPing(uchar *packet)
{
	uchar *ppkt = packet;
	struct ethergram *eg = (struct ethergram *)packet;

	/* Source of request becomes destination of reply. */
	memcpy(eg->dst, eg->src, ETH_ADDR_LEN);
	/* Source of reply becomes me. */
	memcpy(eg->src, myMAC, ETH_ADDR_LEN);
	/* Zero out payload. */
	bzero(eg->data, ETHER_MINPAYLOAD);
	/* FISH type becomes ANNOUNCE. */
	eg->data[0] = FISH_ANNOUNCE;
	strncpy(&eg->data[1], nvramGet("hostname\0"), FISH_MAXNAME-1);
	write(ETH0, packet, ETHER_SIZE + ETHER_MINPAYLOAD);
}

/*------------------------------------------------------------------------
 * fishDirAsk - 
 *------------------------------------------------------------------------
 */
void fishDirAsk(uchar *packet)
{
	int i, n, payloadBytes;
	uchar *ppkt = packet;
	struct ethergram *eg = (struct ethergram *)packet;

	/* Source of request becomes destination of reply. */
	memcpy(eg->dst, eg->src, ETH_ADDR_LEN);
	/* Source of reply becomes me. */
	memcpy(eg->src, myMAC, ETH_ADDR_LEN);
	/* Zero out payload. */
	bzero(eg->data, 1 + (FNAMLEN * DIRENTRIES)); // DOUBLE CHECK
	/* FISH type becomes DIRLIST. */
	eg->data[0] = FISH_DIRLIST;

	n = 1;
	for (i = 0; i < DIRENTRIES; i++)
	{
		if (0 == !(filetab[i].fn_state & (FILE_USED | FILE_OPEN))) // SAM'S VERSION
		{
			strncpy(&eg->data[n], filetab[i].fn_name, FNAMLEN);
			n += FNAMLEN;
		}
	}
	write(ETH0, packet, ETHER_SIZE + 1 + (FNAMLEN * DIRENTRIES)); // 1 for fish type
}

/*------------------------------------------------------------------------
 * fileDirList - 
 *------------------------------------------------------------------------*/
int fishDirList(uchar *packet)
{
	struct ethergram *eg = (struct ethergram *)packet;
	int i, n = 1;

	for(i = 0; i < DIRENTRIES && (eg->data + n) != '\0'; i++)
	{
		memcpy(fishlist[i], eg->data + n, FNAMLEN);
		n += FNAMLEN;
	}
	return OK;
}

/*------------------------------------------------------------------------
 * fishGetFile - 
 *------------------------------------------------------------------------
 */
void fishGetFile(uchar *packet)
{
	int i, j, n;
	char c;
	uchar *ppkt = packet;
	struct ethergram *eg = (struct ethergram *)packet;

	eg->data[0] = FISH_NOFILE;
	for(i = 0; i < DIRENTRIES; i++)
	{
		if(0 == !(filetab[i].fn_state & (FILE_OPEN | FILE_USED)) && 0 == strncmp(filetab[i].fn_name, eg->data + 1, FNAMLEN))
		{
			// Set up data to return here... from file info
			bzero(eg->data, 1 + DISKBLOCKLEN + FNAMLEN);
			eg->data[0] = FISH_HAVEFILE;
			memcpy(eg->data + 1, filetab[i].fn_name, FNAMLEN);
			fileOpen(filetab[i].fn_name); // Might need to check for SYSERR
			strncpy(eg->data + 1 + FNAMLEN, filetab[i].fn_data, filetab[i].fn_length);
			fileClose(i);
			break;
		}
	}

	/* Source of request becomes destination of reply. */
	memcpy(eg->dst, eg->src, ETH_ADDR_LEN);
	/* Source of reply becomes me. */
	memcpy(eg->src, myMAC, ETH_ADDR_LEN);

	if(eg->data[0] == FISH_NOFILE)
		write(ETH0, packet, ETHER_SIZE + ETHER_MINPAYLOAD);
	else
		write(ETH0, packet, ETHER_SIZE + 1 + DISKBLOCKLEN + FNAMLEN); // 1 for the fish type
}

void fishHaveFile(uchar *packet)
{
	int i, n;
	uchar *ppkt = packet;
	struct ethergram *eg = (struct ethergram *)packet;

	char fileName[FNAMLEN];
	strncpy(fileName, eg->data + 1, FNAMLEN);
	i = fileCreate(fileName); // Might need to check for SYSERR
	fileOpen(fileName);
	for(n = 1 + FNAMLEN; eg->data[n] != '\0'; n++)
		filePutChar(i, eg->data[n]);
	fileClose(i);
}

/*------------------------------------------------------------------------
 * fileSharer - Process that shares files over the network.
 *------------------------------------------------------------------------
 */
int fileSharer(int dev)
{
	uchar packet[PKTSZ];
	int size;
	struct ethergram *eg = (struct ethergram *)packet;

	enable();
	/* Zero out the packet buffer. */
	bzero(packet, PKTSZ);
	bzero(school, sizeof(school));
	bzero(fishlist, sizeof(fishlist));

	/* Lookup canonical MAC in NVRAM, and store in ether struct */
 	colon2mac(nvramGet("et0macaddr"), myMAC);

	while (SYSERR != (size = read(dev, packet, PKTSZ)))
	{
		/* Check packet to see if fileshare type with
		   destination broadcast or me. */
		if ((ntohs(eg->type) == ETYPE_FISH)
			&& ((0 == memcmp(eg->dst, bcast, ETH_ADDR_LEN))
				|| (0 == memcmp(eg->dst, myMAC, ETH_ADDR_LEN))))
		{
			switch (eg->data[0])
			{
			case FISH_ANNOUNCE:
				fishAnnounce(packet);
				break;

			case FISH_PING:
				fishPing(packet);
				break;

		// TODO: All of the cases below.

			case FISH_DIRASK:
				fishDirAsk(packet);
				break;
			case FISH_DIRLIST:
				fishDirList(packet);
				break;
			case FISH_GETFILE:
				fishGetFile(packet);
				break;
			case FISH_HAVEFILE:
				fishHaveFile(packet);
				break;
			case FISH_NOFILE:
				printf("ERROR: Remote file %s not found!\n", eg->data + 1);
				break;
			default:
				printf("ERROR: Got unhandled FISH type %d\n", eg->data[0]);
			}
		}
	}

	return OK;
}
