/**
 * @file     xsh_test.c
 * @provides xsh_test
 *
 */
/* Embedded XINU, Copyright (C) 2009.  All rights reserved. */

#include <xinu.h>



void printtemplst(struct superblock *psuper)
{

	int i;
	struct freeblock *fb = psuper->sb_freelst;
	printf("Freelst collector nodes:\n");
	while(fb != NULL)
	{
		printf("Blk %3d, cnt %3d = ", fb->fr_blocknum, fb->fr_count);
		for(i = 0; (i < fb->fr_count) && (i < 10); i++)
			printf("[%03d]", fb->fr_free[i]);
		if(fb->fr_count >= 10)
			printf("...[%03d]", fb->fr_free[fb->fr_count - 1]);
		printf("\n");
		fb = fb->fr_next;
	}
}

/**
 * Shell command (test) is testing hook.
 * @param args array of arguments
 * @return OK for success, SYSERR for syntax error
 */
command xsh_test(int nargs, char *args[])
{
    //TODO: Test your O/S.
    printf("This is where you should put some testing code.\n");

/*
	int i, blknum, num;
	for(i = 5; i < FREEBLOCKMAX; i++){
	blknum = fileCreate("test");
	num = (int)blknum;
	}
*/

	// Empty List
	int i;
	for(i = 2; i < 253; i++)
	{
		sbGetBlock(supertab);
	}
	

/*
	// Empty List
	struct superblock *psuper = malloc(sizeof(struct superblock));
	psuper->sb_blocknum = 0;
	psuper->sb_freelst = NULL;
	psuper->sb_disk = supertab->sb_disk;

	//printtemplst(psuper);
	sbFreeBlock(psuper, 2);
	printtemplst(psuper);
	sbFreeBlock(psuper, 3);
	printtemplst(psuper);
*/
    return OK;
}
