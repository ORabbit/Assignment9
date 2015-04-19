/* fileDelete.c - fileDelete */
/* Copyright (C) 2008, Marquette University.  All rights reserved. */
/*                                                                 */
/* Modified by Casey O'Hare and Samuel Ostlund                     */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */

#include <xinu.h>
#include <kernel.h>
#include <memory.h>
#include <file.h>

/*------------------------------------------------------------------------
 * fileDelete - Delete a file.
 *------------------------------------------------------------------------
 */
devcall fileDelete(int fd)
{
    // TODO: Unlink this file from the master directory index,
    //  and return its space to the free disk block list.
    //  Use the superblock's locks to guarantee mutually exclusive
    //  access to the directory index.

	if(supertab == NULL || filetab == NULL)
		return SYSERR;
	
	wait(supertab->sb_dirlock);
	
	if(filetab[fd].fn_state == FILE_FREE) // ERROR checking file is already free
		{signal(supertab->sb_dirlock); return SYSERR; }

	free(filetab[fd].fn_data);	
	filetab[fd].fn_state = FILE_FREE;

	//free(supertab->sb_dirlst->db_fnodes[fd].fn_data);
	//supertab->sb_dirlst->db_fnodes[fd].fn_state = FILE_FREE;
	signal(supertab->sb_dirlock);
	if(SYSERR == sbFreeBlock(supertab, filetab[fd].fn_blocknum))
		return SYSERR;

    return OK;
}
