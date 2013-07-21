#define INCL_DOSMEMMGR

#include <os2.h>
#include "heapManagerOB.hpp"

#define BLOCK 4096

/* ***************************** getBaseAddress **************************** */
PVOID heapManagerOB::getBaseAddress ()
{return memObject;
}

/* ******************************** heapManagerOB ******************************* */
heapManagerOB::heapManagerOB (unsigned allocateRequest)
{unsigned		i,	req;
APIRET			rc;

i = allocateRequest/BLOCK;
req = i * BLOCK;
if (req != allocateRequest)
	req += BLOCK;
rc = DosAllocMem (&memObject, req, PAG_READ | PAG_WRITE);
nextCommit = memObject;
nextAllocate = memObject;
totalCommitted = 0;
committedFree = 0;
notCommitted = req;
return;
}

/* *************************** releaseMem ******************************** */
void heapManagerOB::releaseMem (unsigned count)
{
nextAllocate = (PCHAR)nextAllocate - count;
committedFree += count;
return;
}

/* **************************** requestMem ********************************* */
PVOID heapManagerOB::requestMem (unsigned count)
{APIRET		rc;
PVOID		retVal;
ULONG		toCommit;

if (committedFree < count)
	{ULONG		i;
	
	i = count - committedFree;
	toCommit = i/BLOCK;
	toCommit = toCommit*BLOCK + BLOCK;
	rc = DosSetMem (nextCommit, toCommit, PAG_COMMIT | PAG_READ | PAG_WRITE);
	if (rc)
		return NULL;
	nextCommit = (PCHAR) nextCommit + toCommit;
	totalCommitted += toCommit;
	committedFree += toCommit;
	notCommitted -= toCommit;
	}
retVal = nextAllocate;
nextAllocate = (PCHAR)nextAllocate + count;
committedFree -= count;
return retVal;
}

/* *************************** reInitialize *************************** */
void heapManagerOB::reInitialize (void)
{
DosSetMem (memObject, totalCommitted, PAG_DECOMMIT);
nextAllocate = memObject;
nextCommit = memObject;
notCommitted += totalCommitted;
totalCommitted = 0;
committedFree = 0;
}

/* *************************** ~heapManagerOB **************************** */
heapManagerOB::~heapManagerOB ()
{if (memObject)
	DosFreeMem (memObject);
}
