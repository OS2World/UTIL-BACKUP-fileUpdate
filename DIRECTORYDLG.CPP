#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_GPILCIDS
#define INCL_GPIPRIMITIVES
#define INCL_WIN

#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "directoryDlg.hpp"

typedef struct _DRIVE_DEF {
	char		label[12];
	char		driveLetter;
	}  DRIVE_DEF;
struct CREATION_PARAMS{
	char		*initialDirectory;
	char		*title;
	};
static DRIVE_DEF		*driveList;
static int				driveCount;
static int				expandLevel;

static HWND		hwndDlg;
static HWND		hwndDisk;
static HWND		hwndDirectory;
static unsigned	dialogID,
			OKID = 2000,
			cancelID = 2001,
			driveBoxID = 2002,
			directoryBoxID = 2003;
static char	*dir;


static MRESULT EXPENTRY		newDlgProc (HWND hwnd, ULONG msg,
			MPARAM mp1, MPARAM mp2);
static int	createDlg (HWND owner, unsigned dialogID, char *directory,
				char *dialogTitle);
static void fillDirectoryList (char *dirName);
static void fillDriveList (void);
static int isPathDirectory (char *initDir, char *dirName);
static void selectDrive (char drive);

#define NUM_CONTROLS 1
static DLGTITEM dialogItem = {1, NUM_CONTROLS-1, 0, 1, 0, 0, 
			FS_DLGBORDER | WS_CLIPSIBLINGS, 
			173,74,250, 165, 0, 0, 0};


/* ******************** conditionalDirectoryDlg ************************** */

int	conditionalDirectoryDlg (
					HWND		owner,
					unsigned 	dialogID,
					char		*directory,
					char		*dialogTitle
					)
{
int			i;
char		FQDir[300];

if ( !driveList)
	fillDriveList( );
i = isPathDirectory (directory, FQDir);
strcpy (directory, FQDir);
if (i)
	return 1;
/* the following values were obtainedfrom using the debugger on a 
   program which called DosResource for a rc generated resource and
   examining the fields of the returned DLGTEMPLATE *
*/
return createDlg (owner, dialogID, directory, dialogTitle);
}

/* ************************** directoryDlg **************************** */
int directoryDlg (	HWND		owner,
					unsigned	dialog,
					char		*directory,
					char		*dialogTitle)

{
int			i, j;
APIRET		rc;
char		FQDir[300];

dialogID = dialog;
dir = directory;
if ( !driveList)
	fillDriveList( );
isPathDirectory (directory, FQDir);
strcpy (directory, FQDir);
return createDlg (owner, dialog, directory, dialogTitle);
}

/* ************************ buildDirectoryName *************************** */
static void buildDirectoryName (HWND hwnd, char *name)
{int		select, i, j, last, offset;
char		*pch;
char		buff[300];

select = (int) WinSendMsg (hwnd, LM_QUERYSELECTION, (MPARAM) 0, NULL);
if (select >= expandLevel)
	last = expandLevel;
else
	last = select;
for (i=0, pch=name, offset=0;	i<last;	++i, offset+=2)
	{
	j = (int)WinSendMsg (hwnd, LM_QUERYITEMTEXT, MPFROM2SHORT (i, 260),
			(MPARAM) (buff));
	memcpy (pch, buff+offset, j-offset);
	pch += (j-offset);
	*pch++ = '/';
	}
j = (int)WinSendMsg (hwnd, LM_QUERYITEMTEXT, MPFROM2SHORT (select, 260),
		(MPARAM) buff);
memcpy (pch, buff+offset, j-offset);
*(pch+j-offset) = '/';
*(pch+j+1-offset) = 0;
return;
}

/* *************************** createDlg ************************** */
static int	createDlg (HWND owner, unsigned dialogID, char *directory,
				char *dialogTitle)
{
DLGTEMPLATE	*pDT;
HWND		hwndDlg;
CREATION_PARAMS	parameters;
DLGTITEM	*pdlg;
int			templateSize,
			i;

templateSize = sizeof (DLGTEMPLATE) + NUM_CONTROLS * sizeof (DLGTITEM);
pDT = (DLGTEMPLATE *) malloc (templateSize);
pDT->cbTemplate = sizeof (DLGTEMPLATE) + sizeof (DLGTITEM);
pDT->type = 0;
pDT->codepage = 437;
pDT->offadlgti = 14;
pDT->fsTemplateStatus = 1;
pDT->iItemFocus = 0;		//I made this up
pDT->coffPresParams = 0;

parameters.initialDirectory = directory;
parameters.title = dialogTitle;
pdlg = pDT->adlgti;
*pdlg = dialogItem;
pdlg->id = dialogID;
hwndDlg = WinCreateDlg (HWND_DESKTOP, owner, newDlgProc, pDT,
	(PVOID) &parameters);
i = WinProcessDlg (hwndDlg);

free (pDT);
return i;
}

/* ************************* newDlgProc ****************************** */
static MRESULT EXPENTRY		newDlgProc (HWND hwnd, ULONG msg,
			MPARAM mp1, MPARAM mp2)
{
static char *initialDirectory;

switch (msg)
	{

case WM_INITDLG:
		{char		buff[50];
		int			i, j1, j2, k;
		DRIVE_DEF	*pdrive;
		CREATION_PARAMS	*pCreate;
		FRAMECDATA	cData;
		HWND		hwork;
		FONTMETRICS	fontMetrics;
		int			CharHeight,	CharWidth, hUnit, vUnit;
		HPS			hps;
		POINTL		aptl[TXTBOX_COUNT+1];
		char		*directoryText = "Directory";
		char		*driveText = "Drive";
		int			directoryTextSize, driveTextSize;
		RECTL		rectl;
		
		hps = WinBeginPaint (hwnd, 0L, NULL);
		GpiQueryFontMetrics (hps, sizeof(fontMetrics), &fontMetrics);
		CharHeight = fontMetrics.lMaxBaselineExt;
		CharWidth = fontMetrics.lMaxCharInc;
		GpiQueryTextBox (hps, strlen (directoryText), directoryText, 
			TXTBOX_COUNT, aptl);
		directoryTextSize = aptl[TXTBOX_BOTTOMRIGHT].x - aptl[TXTBOX_BOTTOMLEFT].x;
		i = GpiQueryTextBox (hps, strlen (driveText), driveText, 
			TXTBOX_COUNT, aptl);
		CharHeight = aptl[TXTBOX_TOPLEFT].y - aptl[TXTBOX_BOTTOMLEFT].y;
		driveTextSize = aptl[TXTBOX_BOTTOMRIGHT].x - aptl[TXTBOX_BOTTOMLEFT].x;
		WinEndPaint (hps);
		CharWidth = 9;		// The font is not propagating properly
		cData.cb = sizeof (cData);
		cData.flCreateFlags = FCF_TITLEBAR | FCF_SYSMENU;
		cData.hmodResources = 0;
		cData.idResources = 0;
		pCreate = (CREATION_PARAMS *) mp2;
		initialDirectory = pCreate->initialDirectory;
		i = (int)WinCreateFrameControls (hwnd, &cData, pCreate->title);
		WinShowWindow (hwnd, TRUE);
		WinSendMsg (hwnd, WM_UPDATEFRAME, (MPARAM) cData.flCreateFlags, NULL);
	/* right-align the two text boxes */
		i = directoryTextSize + 8;
		WinQueryWindowRect (hwnd, &rectl);
		j1 = rectl.yTop - 2*CharHeight - CharHeight/2;
		j2 = rectl.yTop - 4*CharHeight;
		hwork = WinCreateWindow (hwnd, WC_STATIC, driveText,
			SS_TEXT | DT_RIGHT | DT_VCENTER | WS_VISIBLE, 
			i-driveTextSize-4, j1-CharHeight, driveTextSize+4, CharHeight, 
			 hwnd, HWND_TOP, 4000, NULL, NULL);
		hwork = WinCreateWindow (hwnd, WC_STATIC, directoryText,
			SS_TEXT | DT_RIGHT | DT_VCENTER | WS_VISIBLE, 
			i-directoryTextSize-4, j2-CharHeight, directoryTextSize+4, CharHeight, 
			hwnd, HWND_TOP, 4001, NULL, NULL);
		i += 5;
		k = rectl.xRight;
		hwndDisk = WinCreateWindow (hwnd, WC_COMBOBOX, NULL, 
			CBS_DROPDOWNLIST | WS_VISIBLE,
			i, 7*CharHeight, k-i-directoryTextSize, j1-7*CharHeight,
			 hwnd, HWND_TOP, driveBoxID, NULL, NULL);
		hwndDirectory = WinCreateWindow (hwnd, WC_COMBOBOX, NULL, 
			CBS_SIMPLE | WS_VISIBLE,
			i, 4*CharHeight, k-i-directoryTextSize/2, j2-4*CharHeight,
			 hwnd, HWND_TOP, directoryBoxID, NULL, NULL);
		WinCreateWindow (hwnd, WC_BUTTON, "OK", 
			BS_PUSHBUTTON | WS_VISIBLE, 
			(40*CharWidth)/4, (6*CharHeight)/8, (30*CharWidth)/4, (14*CharHeight)/8, 
			hwnd, HWND_TOP, OKID, NULL, NULL);
		WinCreateWindow (hwnd, WC_BUTTON, "Cancel", 
			BS_PUSHBUTTON | WS_VISIBLE, 
			(80*CharWidth)/4, (6*CharHeight)/8, (30*CharWidth)/4, (14*CharHeight)/8, 
			hwnd, HWND_TOP, cancelID, NULL, NULL);
		hwndDlg = hwnd;
		fillDirectoryList (initialDirectory);
		for (i=0, pdrive=driveList;	i<driveCount;	++i, ++pdrive)
			{
			*buff = pdrive->driveLetter;
			*(buff+1) = ':';
			memset (buff+2, ' ', 3);
			strcpy (buff+5, pdrive->label);
			WinSendMsg (hwndDisk, LM_INSERTITEM, (MPARAM) LIT_END,
					(MPARAM) buff);
			}
		selectDrive (*initialDirectory);
		WinShowWindow (hwnd, TRUE);
		break;
		}
case WM_COMMAND:
	if (SHORT1FROMMP (mp2) != CMDSRC_PUSHBUTTON)
		break;
	if ( (ULONG) mp1 == OKID)
		{
		HWND		hwndLB;
		
		hwndLB = WinWindowFromID (hwnd, directoryBoxID);
		buildDirectoryName (hwndLB, initialDirectory);
		WinDismissDlg (hwnd, 0);
		return 0;
		}
	else if ( (ULONG) mp1 == cancelID)
		{WinDismissDlg (hwnd, 1);
		return 0;
		}
	break;
case WM_CONTROL:
	if (SHORT1FROMMP (mp1) == driveBoxID)
		{if ( (SHORT2FROMMP (mp1)==CBN_ENTER) )
			{
			char		buff[5],
						buff2[300];
			int			i;
			
			i = (int)WinSendMsg (hwndDisk, LM_QUERYSELECTION, 
					(MPARAM) 0, NULL);
			WinSendMsg (hwndDisk, LM_QUERYITEMTEXT, MPFROM2SHORT (i, 5),
					(MPARAM) buff);
			buff[2] = 0;
			isPathDirectory (buff, buff2);
			fillDirectoryList (buff2);
			}
		
		}
	else if (SHORT1FROMMP (mp1) == directoryBoxID)
		{if (SHORT2FROMMP (mp1) == CBN_ENTER)
			{int		first, i, j;
			char		buff[300],
						*pch;
			HDIR		hdir;
			FILEFINDBUF3	findBuff;
			ULONG		fileCount;
			APIRET		rc;
			
			i = (int) WinSendMsg (hwndDirectory, LM_QUERYSELECTION, 
					(MPARAM) 0, NULL);
			j = (int) WinSendMsg (hwndDirectory, LM_QUERYITEMCOUNT,
					NULL, NULL);
			first = i+1;
			while (first < j)
				j = (int) WinSendMsg (hwndDirectory, LM_DELETEITEM,
						(MPARAM) first, NULL);
			if (i >= expandLevel)
				{first = expandLevel;
				for (j=expandLevel;	j<i;	++j)
					WinSendMsg (hwndDirectory, LM_DELETEITEM,
						(MPARAM) expandLevel, NULL);
				++expandLevel;
				}
			else
				expandLevel = first;
			buildDirectoryName (hwndDirectory, buff);
			strcat (buff, "*");
			hdir = HDIR_CREATE;
			fileCount = 1;
			rc = DosFindFirst (buff, &hdir, MUST_HAVE_DIRECTORY|FILE_DIRECTORY,
				(PVOID)&findBuff, sizeof (findBuff), &fileCount, 1);
			memset (buff, ' ', 2*expandLevel);
			pch = buff + 2*expandLevel;
			while ( !rc)
				{
				if ( !strcmp (".", findBuff.achName))
					goto l4;
				if ( !strcmp ("..", findBuff.achName) )
					goto l4;
				memcpy (pch, findBuff.achName ,findBuff.cchName+1);
				WinSendMsg (hwndDirectory, LM_INSERTITEM, (MPARAM) LIT_END,
						(MPARAM) buff);
l4:
				rc = DosFindNext (hdir, (PVOID) &findBuff, sizeof (findBuff),
					&fileCount);
				}
			DosFindClose (hdir);
			}
		
		}
	else
		break;
	break;
	
	}
return WinDefDlgProc (hwnd, msg, mp1, mp2);
}

/* *************************** fillDirectoryList ************************** */
static void fillDirectoryList (char *dirName)
{
char		*endOfName,
			*pch1,
			*pch2;
char		buff[300];
int			i;
unsigned	ul;


WinSendDlgItemMsg (hwndDlg, directoryBoxID, LM_DELETEALL, NULL, NULL);
endOfName = dirName + strlen (dirName);
for (pch1=dirName, pch2=pch1,  i=0;	;	)
	{if ( (*pch2=='\\') || (*pch2=='/') || (*pch2==0) )
		{int		endLoop;
		
		endLoop = 0;
		if ( ! *pch2)
			endLoop = 1;
		memset (buff, ' ', 2*i);
		*pch2 = 0;
		memcpy (buff + 2*i, pch1, pch2-pch1 + 1);
		WinSendDlgItemMsg (hwndDlg, directoryBoxID, LM_INSERTITEM, 
				(MPARAM) LIT_END, (MPARAM) buff);
		*pch2 = '/';
		if (endLoop)
			break;
		if ( !*(pch2+1) )
			break;
		pch1 = pch2 + 1;
		++i;
		}
	++pch2;
	}
expandLevel = i;
WinSendMsg (hwndDirectory, LM_SELECTITEM, (MPARAM) i, (MPARAM) TRUE);
return;
}

/* ************************ fillDriveList ****************************** */
static void fillDriveList (void)
{
int			i, j;
ULONG		currentDisk,
			diskMask;
DRIVE_DEF	*pdisk;
FSALLOCATE	level1Info;
FSINFO		fsinfo;
APIRET		rc;

driveList = (DRIVE_DEF *) malloc (26*sizeof(DRIVE_DEF));
DosQueryCurrentDisk (&currentDisk, &diskMask);
DosError (FERR_DISABLEHARDERR);
for (i=j=0, pdisk=driveList;	i<26;	++i)
	{if (diskMask & 1)
		{++j;
		pdisk->driveLetter = 'A'+i;
		if (i > 1)
			{
			rc = DosQueryFSInfo (i+1, FSIL_VOLSER, &fsinfo, sizeof (fsinfo) );
			if (!rc && fsinfo.vol.cch )
				memcpy (pdisk->label, (void *)fsinfo.vol.szVolLabel, 
						fsinfo.vol.cch+1);
			else
				pdisk->label[0] = 0;
			}
		else
			pdisk->label[0] = 0;
		++pdisk;
		}
	diskMask = diskMask >> 1;
	}
DosError (FERR_ENABLEHARDERR);
driveList = (DRIVE_DEF *)realloc (driveList, j*sizeof (DRIVE_DEF));
driveCount = j;
}

/* ************************* isPathDirectory *************************** */
/* Accept an input path.  Convert to a fully qualified path in the
   required format (i.e. use `/' instead of `\' as the directory
   delimiter).  If the converted path is a directory return 1.  Else
   convert to a default path and return 0.
*/

static int isPathDirectory (char *initDir, char *dirName)

{
char		*workSource,
			*workDest;
int			i,
			j;
ULONG		ui;
ULONG		diskNum;
ULONG		map;
char		buff[300];
APIRET		rc;

for (workSource=initDir;    *workSource;    ++workSource)
    {if (*workSource == '\\')
        *workSource = '/';
    }
workSource = initDir;
workDest = dirName;
if (*initDir)
	{FILESTATUS3	fileStat;
	
 	if ( !strcmp (initDir, "."))
		{/* Caller has specified the current directory.  
            change the input string to the empty string.  
            Since the input is no longer a valid directory
            we will use the default which is the current
            directory. */
		*workSource = 0;
		*(workSource+1) = 0;
		}
	if (*(initDir+1) == ':')
		{memcpy (workDest, workSource, 2);
		workSource += 2;
		diskNum = toupper (*initDir) - 'A' + 1;
		}
	else	{
		
		rc = DosQueryCurrentDisk (&diskNum, &map);
		if (rc)
			diskNum = 3;		/* force drive to C: if there is an error */
		*workDest = diskNum + 'A' - 1;
		*(workDest + 1) = ':';
		}
	workDest += 2;
	DosError (FERR_DISABLEHARDERR);
	if ( *workSource != '/'  )
	/*  directory not fully qualified - path is relative to the current
        directory for this drive. */
		{
		
		ui = 299;
		rc = DosQueryCurrentDir (diskNum, buff, &ui);
		*workDest++ = '/';
		if ( !rc)
			{i = strlen (buff);
			memcpy (workDest, buff, i);
			workDest += i;
			if (i)
				*workDest++ = '/';
			}
		}   /* directory not fully qualifies */
	strcpy (workDest, workSource);
	i = strlen (dirName) - 1;
/* DosQueryPathInfo will not recognize `C:' as a directory.  It will
   recognize `C:/'.  OTH it will not recognize `C:/foo/'.  It will
   recognize `C:/foo'.  The following code deals with this situation
*/
	if ( (i>2) && (*(dirName+i) == '/') )
		{*(dirName+i) = 0;
		--i;
		}
	rc = DosQueryPathInfo (dirName, FIL_STANDARD, (PVOID) &fileStat,
			sizeof (fileStat) );
	DosError (FERR_ENABLEHARDERR);
	if (!rc)
		{if (fileStat.attrFile & FILE_DIRECTORY)
			{if (i >2)
				{*(dirName+i+1) = '/';
				*(dirName+i+2) = 0;
				}
			return 1;
			}
		}
	}			/* input directory defined  */
rc = DosQueryCurrentDisk (&diskNum, &map);
ui = 299;
dirName[0] = 'A' + diskNum-1;
dirName[1] = ':';
dirName[2] = '/';
rc = DosQueryCurrentDir (diskNum, dirName+3, &ui);
return 0;
}

/* ************************** selectDrive ********************************* */
static void selectDrive (char drive)
{
DRIVE_DEF	*pdr;
int			i;

for (i=0, pdr=driveList;	i<driveCount;	++i, ++pdr)
	{if (drive == pdr->driveLetter)
		{WinSendMsg (hwndDisk, LM_SELECTITEM, (MPARAM) i ,
				(MPARAM) TRUE);
		break;
		}
	}
return;
}
