class heapManagerOB
	{PVOID		memObject;			//returned by DosAllocMem ()
	PVOID		nextAllocate;
	PVOID		nextCommit;
	unsigned	totalCommitted;
	unsigned	committedFree;
	unsigned	notCommitted;
public:
	void		reInitialize (void);
	void		releaseMem (unsigned count);
	PVOID		requestMem (unsigned count);
				heapManagerOB (unsigned allocateRequest);
	PVOID		getBaseAddress ();
				~heapManagerOB ();
	};
