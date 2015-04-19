/* sbFreeBlock.c - sbFreeBlock */
/* Copyright (C) 2008, Marquette University.  All rights reserved. */
/*                                                                 */
/* Modified by Casey O'Hare and Samuel Ostlund                     */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */

#include <xinu.h>
#include <kernel.h>
#include <device.h>
#include <memory.h>
#include <disk.h>
#include <file.h>

/*------------------------------------------------------------------------
 * sbFreeBlock - Add a block back into the free list of disk blocks.
 *------------------------------------------------------------------------
 */
devcall sbFreeBlock(struct superblock *psuper, int block)
{
    // TODO: Add the block back into the filesystem's list of
    //  free blocks.  Use the superblock's locks to guarantee
    //  mutually exclusive access to the free list, and write
    //  the changed free list segment(s) back to disk.

	struct freeblock *freeblk, *newfreeblk;
	struct dentry *phw;
	int diskfd;
	struct dirblock *dirblk;

	// ERROR null = psuper
	phw = psuper->sb_disk;
	if(NULL == phw)
		return SYSERR;

	diskfd = phw - devtab; // What is this?
	freeblk = psuper->sb_freelst;
	dirblk = psuper->sb_dirlst;

	wait(psuper->sb_freelock);

	if(NULL == freeblk) // Case 1: No collector nodes at all, must create a new one (with this block to be freed)
	{
		if((newfreeblk = malloc(sizeof(struct freeblock))) == NULL)
			{signal(psuper->sb_freelock); return SYSERR;}
		newfreeblk->fr_blocknum = block;
		newfreeblk->fr_count = 0;
		newfreeblk->fr_next = NULL;

		psuper->sb_freelst = (struct freeblock *) block;

		seek(diskfd, psuper->sb_blocknum);
		if(SYSERR == write(diskfd, psuper, sizeof(struct superblock)))
			{signal(psuper->sb_freelock); return SYSERR;}
		psuper->sb_freelst = newfreeblk;

		seek(diskfd, newfreeblk->fr_blocknum);
		if(SYSERR == write(diskfd, newfreeblk, sizeof(struct freeblock)))
			{signal(psuper->sb_freelock); return SYSERR;}
		
		signal(psuper->sb_freelock);
		return OK;	
	}

	while(freeblk->fr_next != NULL) // Get to last freeblk collector node
		freeblk = freeblk->fr_next;

	// Case 2: Last collector node is full, must create a new one (with this block to be freed)
	if(freeblk->fr_count == FREEBLOCKMAX)
	{
		if((newfreeblk = malloc(sizeof(struct freeblock))) == NULL)
			{signal(psuper->sb_freelock); return SYSERR;}
		newfreeblk->fr_blocknum = block;
		newfreeblk->fr_count = 0;
		newfreeblk->fr_next = NULL;
		
		freeblk->fr_next = (struct freeblock *) block;
		
		seek(diskfd, freeblk->fr_blocknum);
		if(SYSERR == write(diskfd, freeblk, sizeof(struct freeblock)))
			{signal(psuper->sb_freelock); return SYSERR;}
		freeblk->fr_next = newfreeblk;

		seek(diskfd, newfreeblk->fr_blocknum);
		if(SYSERR == write(diskfd, newfreeblk, sizeof(struct freeblock)))
			{signal(psuper->sb_freelock); return SYSERR;}
	}else // Case 3: Last collector node has room, increment count and add this block to the end
	{
		freeblk->fr_count++;
		freeblk->fr_free[freeblk->fr_count - 1] = block;

		seek(diskfd, freeblk->fr_blocknum);
		if(SYSERR == write(diskfd, freeblk, sizeof(struct freeblock)))
			{signal(psuper->sb_freelock); return SYSERR;}
		
		
	}

	seek(diskfd, dirblk->db_blocknum);
	if(SYSERR == write(diskfd, dirblk, sizeof(struct dirblock)))
		{signal(psuper->sb_freelock); return SYSERR;}
	signal(psuper->sb_freelock);
	return OK;
}
