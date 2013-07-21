/* Bugs:
    1. dragging the cursor to select files to copy can be wierd.

*/

/* Revision history:
	5/98 - excludeDialogProc:  made the `Add' push button the default
	9/98 - the zip file to directory would not extract any files.
	9/98 - readZipDirectory () ignored the extended attribute and file
			comment when moving from one directory entry to the next. 
    1/00 - Added the configuration dialog
			
 */


#define KILLME *(char *)0=0;
#define ERROR_FILE_NAME "/fileUp$$.log"

#define INCL_DOSERRORS
#define INCL_DOSMISC
#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#define INCL_WINMESSAGEMGR
#define INCL_WIN
#define INCL_WINDIALOGS
#define INCL_DOSSESMGR
#define INCL_DOSQUEUES

#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <io.h>
#include <process.h>
#include <memory.h>

#include "fileUpdate.hpp"
#include "fileUpGen.hpp"
#include "directoryDlg.hpp"
#include "heapManagerOB.hpp"
#include "splitWindowOB.hpp"
#include "resToWnd.hpp"

enum EN_TIMER_TYPE {EN_TIMER_SetSelect, EN_TIMER_ClearSelect};
enum ENArchiveType {ENATDir, ENATZip};
enum ENUpdateMode {ENUMDirDir, ENUMDirZip, ENUMZipDir};
enum ENShowSubdirs { ENSSNone, ENSSAll, ENSSSelected};

struct copyStatInitST{
	BOOL		enableCancel;
	char		*title1;
	char		*title2;
	};
typedef	void (WORKERPROC (PVOID));
typedef	WORKERPROC *PWORKERPROC;

struct copyThreadArgST {
	PWORKERPROC		worker;
	PVOID			arg;
	};
typedef struct _EXCLUDE_STRING
	{int		length;
	char		*name;
	} EXCLUDE_STRING;
typedef struct _FILE_DESCRIPTION
	{char		*name;
	int			nameStringLength;
	ULONG		attributes;
	FDATE		date;
	FTIME		time;
	ULONG		size;
	}  FILE_DESCRIPTION;
typedef struct _DIRECTORY_CONTROL
	{char		*title;
	char		*directoryName;
	heapManagerOB	*fileControlHeap;
	heapManagerOB	*fileNameText;
	FILE_DESCRIPTION	*firstFile;
	ENArchiveType	archiveType;
	int				fileCount;
	} DIRECTORY_CONTROL;
typedef struct _DISPLAY_LINE {
	FILE_DESCRIPTION	*source,
						*dest;
	unsigned			selected : 1;
	} DISPLAY_LINE;

struct ButtonToMenue
	{int	buttonID,
			menueID;
	};

static HAB			hab;
static HWND			hwndActionBar;
static HWND			hwndClient,
					hwndFrame;
static HMQ			hmq;
static QMSG			qmsg;
static FNWP			clientClassProc;
static int		totalDisplayLines;

static int		currentDrive;
static char		sourceDir[300];
static char		destDir[300];
static char		*sourceTitle = "Source Directory";
static char		*destTitle = "Destination Directory";
heapManagerOB		sourceFileControl (5000000),
				sourceFileNameText(5000000);
heapManagerOB		destFileControl (5000000),
				destFileNameText (5000000),
				excludeListPointers (50000),
				excludeListText (50000),
				lineControlHeap(5000000);
static heapManagerOB	selectedDirPointers(100);
static heapManagerOB	selectedDirNames(5000);
static int				selectedDirCount;
ENShowSubdirs			showSubdirs = ENSSNone;

DIRECTORY_CONTROL	srcDirControl = {sourceTitle, sourceDir, &sourceFileControl,
		&sourceFileNameText, NULL, ENATDir};
DIRECTORY_CONTROL	destDirControl = {destTitle, destDir, &destFileControl, 
		&destFileNameText, NULL, ENATDir};
DISPLAY_LINE		*displayLineControl;

static int		updateModeIDs[]= {IDM_DIR_DIR, IDM_DIR_ZIP, IDM_ZIP_DIR, 0};
static int		showDirectoryIDs[] = {IDM_MI_NO_SUBDIRS, IDM_MI_SELECT_SUBDIRS, 
					IDM_MI_ALL_SUBDIRS, 0};

// Variables which reflect checkable menu options
static int			showOnlyDifferentFiles,
					showOnlyNewerFiles,
					showHiddenFiles,
					showSystemFiles,
					showFileDate,
					showFileSize;
static int			applyExcludeList,
					excludeListEntryCount;
static int			maxDisplayLineLength,
					maxFileNameLength;
static splitWindowOB *pWin;
static ENUpdateMode		updateMode = ENUMDirDir;

static char		*defaultExcludeList[] = {
	".exe",
	".obj",
	".o",
	".a",
	".lib",
	".res",
	".dll",
	NULL
	};

static MRESULT EXPENTRY copyStatusProc (HWND, ULONG, MPARAM, MPARAM);
static void buildDisplayLines (void);
static void compareSelectedFiles (PVOID);
static int conditionalProcessArchive (HWND hwnd, DIRECTORY_CONTROL *control,
	char *title);
static MRESULT EXPENTRY configureDialogProc (HWND, ULONG, MPARAM, MPARAM);
static void copyFiles (PVOID);
#if defined (__IBMCPP__)
static void _Optlink copyThread (void *notUsed);
#else
static void copyThread (void *notUsed);
#endif
static int createFullPath (const char *source, char *dest);
static ULONG dateTimeToLong (FDATE d, FTIME t);
static MRESULT EXPENTRY directorySelectDlgProc (HWND, ULONG msg, MPARAM, MPARAM);
static MRESULT EXPENTRY errorLogDisplayDlgProc ( HWND hwnd, ULONG msg,
	MPARAM mp1, MPARAM mp2);
static MRESULT EXPENTRY excludeDialogProc (HWND, ULONG, MPARAM, MPARAM);
static void extractFiles (PVOID);
static PANEL_FORMAT_FUNC	formatPanel;
static int		getDirectory (HWND hwnd, DIRECTORY_CONTROL *control);
static int	isLineDisplayed (DISPLAY_LINE *pDL);
static int isLineExcluded (DISPLAY_LINE *pDL);
static void menueItemForceState (int ID, int *state, int value);
static void		mustComplete (HWND hwnd);
static void		readDirectory (PVOID control);
static int processCentralDirectory (DIRECTORY_CONTROL *control,
		 FILE *zipfd, char *buff, int readSize);
static int	readOneDirectory (DIRECTORY_CONTROL *control, char *caption, int enableCancel);
static void readTwoDirectories (void);
static int readZipDirectory (DIRECTORY_CONTROL *control);
static void setHScrollBar (void);
static void setModeMenuItem (int id, int *pi);
static int setUpdateMode (ENArchiveType srcType, ENArchiveType destType);
static void				setUpFiles (HWND hwnd);
static void toggleMenuItem (int ID, int * state);
static void updateZipFile (PVOID);
int verifyZipArchive (DIRECTORY_CONTROL *control, FILE **zipfd, char **bf, 
		int readSize);

/* variables for communication with the copy thread  */
static int		killFileCopy;
static HWND		hwndCopyStatus;
static HFILE	hReadPipe, hWritePipe;
static copyStatInitST	initDirDir = {TRUE, "Copying files", "Current file"};
static copyStatInitST	initDirZip = {FALSE, "Updating zip archive", " "};
static copyStatInitST	initZipDir = {FALSE, "Extracting From Archive", " "};
static copyThreadArgST extractFilesArgs = {extractFiles, NULL};
static copyThreadArgST compareFilesArgs = {compareSelectedFiles, NULL};
static copyThreadArgST copyFilesArgs = {copyFiles, NULL};
static copyThreadArgST updateZipArgs = {updateZipFile, NULL};

#define WM_USERINIT WM_USER
#define UM_CURRENT_FILE_COPY WM_USER+1
#define UM_COPY_COMPLETE WM_USER+2
#define UM_COPY_ERROR WM_USER+3
#define UM_QUERY_OVERWRITE WM_USER+4
#define UM_NO_ZIP_CENTRAL_DIRECTORY WM_USER+5
#define UM_COMPARE_STATUS WM_USER+6

#define ZIPFILE_CENTRAL_DIRECTORY_SIZE 350  /* allow enough room for a 
											   really long file name */
#define ZIPFILE_READ_SIZE 512 * 16

main (int argc, char **argv)
{char *clientClass = "clientClass";
ULONG		clientStyles = CS_SIZEREDRAW;
ULONG		frameStyles = FS_TASKLIST | FS_SIZEBORDER;
ULONG		creationFlags = FCF_TITLEBAR | FCF_SYSMENU | 
				FCF_VERTSCROLL | FCF_HORZSCROLL | FCF_SHELLPOSITION |
				FCF_MENU;
APIRET		rc;
char		** pEL;
char		*argSourceDir = NULL,
			*argDestDir = NULL;
char		*parg,
			**pparg;
int			i,
			limit;

rc = DosCreatePipe (&hReadPipe, &hWritePipe, 0);
if (rc)
	exit (1);
for (pparg=argv+1, i=1;	i<argc;	++i, ++pparg)
	{parg = *pparg;
	if (*parg == '-')
		{if ( !strcmp ("-dd", parg))
			updateMode = ENUMDirDir;
		else if ( !strcmp ("-dz", parg))
			updateMode = ENUMDirZip;
		else if ( !strcmp ("-zd", parg))
			updateMode = ENUMZipDir;
		}
	else if (argSourceDir == NULL)
		argSourceDir = parg;
	else
		argDestDir = parg;
	}
if ( argSourceDir)
	strcpy (sourceDir, argSourceDir);
if ( argDestDir)
	strcpy (destDir, argDestDir);
switch (updateMode)
	{
case ENUMDirDir:
	srcDirControl.archiveType = ENATDir;
	destDirControl.archiveType = ENATDir;
	break;
case ENUMDirZip:
	srcDirControl.archiveType = ENATDir;
	destDirControl.archiveType = ENATZip;
	break;
case ENUMZipDir:
	srcDirControl.archiveType = ENATZip;
	destDirControl.archiveType = ENATDir;
	break;
	}

for (pEL=defaultExcludeList;	*pEL;	++pEL, ++excludeListEntryCount)
	{int			i;
	EXCLUDE_STRING	*pES;
	
	i = strlen (*pEL)+1;
	pES = (EXCLUDE_STRING *)excludeListPointers.requestMem(sizeof ( EXCLUDE_STRING));
	pES->name = (char *) excludeListText.requestMem(i);
	memcpy (pES->name, *pEL, i);
	pES->length = strlen (pES->name);
	}
hab = WinInitialize (0);
hmq = WinCreateMsgQueue (hab, 0L);
rc = WinRegisterClass (hab, clientClass, clientClassProc, 
		clientStyles, 0);
hwndFrame = WinCreateStdWindow (HWND_DESKTOP, frameStyles, &creationFlags, 
		clientClass, NULL, clientStyles, NULL, IDD_FRAME1, &hwndClient);
WinPostMsg (hwndClient, WM_USERINIT, NULL, NULL);
WinShowWindow (hwndFrame, TRUE);

if (hwndFrame)
	{while (WinGetMsg (hab, &qmsg, NULLHANDLE, 0l, 0L) )
		WinDispatchMsg (hab, &qmsg);
	}

WinDestroyWindow (hwndFrame);
WinDestroyMsgQueue (hmq);
WinTerminate (hab);
}

/************************ clientClassProc ****************************** */
MRESULT EXPENTRY clientClassProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
int				i;
static ULONG	currentTimerID;
char			*fontID="8.Courier";

switch (msg)
	{
case WM_CREATE:
	break;
case WM_USERINIT:

	pWin = new splitWindowOB (hwndFrame, hwnd, fontID, 2, 1, formatPanel);	
	setUpFiles (hwnd);
	WinInvalidateRect (hwnd, NULL, FALSE);
	WinPostMsg (hwnd, WM_SIZE, NULL, NULL);
	hwndActionBar = WinWindowFromID (hwndFrame, FID_MENU);
	setModeMenuItem (IDM_DIR_DIR, updateModeIDs);
	setModeMenuItem (IDM_MI_NO_SUBDIRS, showDirectoryIDs);
	break;
case UM_COMPARE_STATUS:
		{int hError;
		int	*pherror;
		
		hError = (int) mp1;
		if ( !hError)
			WinMessageBox (HWND_DESKTOP, hwnd, "No errors during compare", "",
				0, MB_OK);
		else
			{pherror = &hError;
			WinDlgBox (HWND_DESKTOP, hwnd, errorLogDisplayDlgProc,
				NULL, IDD_ERROR_LOG, (PVOID) pherror);
			}
		break;
		}
case WM_SIZE:
	if (pWin)
		pWin->resize ();
	break;
case WM_PAINT:
	if (pWin)
		pWin->refreshClient ();
	break;
case WM_BUTTON1DBLCLK:
	
	break;
case WM_BUTTON1CLICK:
	pWin->buttonClick (mp1, mp2);
	break;
case WM_BEGINSELECT:
	currentTimerID = WinStartTimer (hab, hwnd, EN_TIMER_SetSelect, 50);
	pWin->selectionTimerProc (1, 1);
	break;
case WM_ENDSELECT:
	WinStopTimer (hab, hwnd, currentTimerID);
	break;
case WM_TIMER:
	pWin->selectionTimerProc (0, 1);
	break;
case WM_VSCROLL:
	pWin->VScrollMsg (mp1, mp2);
	break;
case WM_HSCROLL:
	pWin->HScrollMsg (mp1, mp2);
	break;
case WM_COMMAND:
	if (SHORT1FROMMP (mp2) == CMDSRC_MENU)
		{int		modeChangeStarted;
		
		switch (SHORT1FROMMP (mp1) )
			{ULONG 		result;
		case IDM_COMPARE_FILES:
				{APIRET		rc;
				FILESTATUS3	infoBuff;
				ULONG		messageResponse;
				
				rc = DosQueryPathInfo (ERROR_FILE_NAME, FIL_STANDARD, &infoBuff, 
				  sizeof (infoBuff) );
				if (!rc)
					{messageResponse = WinMessageBox (HWND_DESKTOP, hwnd, 
						"fileUp$$.log exists - Overwrite?", "", 0, MB_OKCANCEL);
					if (messageResponse != MBID_OK)
						break;
					}
				_beginthread (copyThread, NULL, 4 * 4096, (PVOID) 
					&compareFilesArgs);
				WinDlgBox (HWND_DESKTOP, hwnd, copyStatusProc, 
						NULL, IDD_FILE_COPY, (PVOID) &initDirDir);
				break;
				}
		case IDM_MI_SEL_ALL:
			pWin->assignSelectedField (0, totalDisplayLines, 1);
			break;
		case IDM_MI_SEL_CURRENT_FIRST:
			i = pWin->queryLatestSelection ();
			pWin->assignSelectedField (0, i, 1);
			break;
		case IDM_MI_SEL_CURRENT_LAST:
			i = pWin->queryLatestSelection ();
			pWin->assignSelectedField (i, totalDisplayLines, 1);
			break;
		case IDM_MI_CLEAR_ALL:
			pWin->assignSelectedField (0, totalDisplayLines, 0);
			break;
		case IDM_MI_CLEAR_CURRENT_FIRST:
			i = pWin->queryLatestSelection ();
			pWin->assignSelectedField (0, i, 0);
			break;
		case IDM_MI_CLEAR_CURRENT_LAST:
			i = pWin->queryLatestSelection ();
			pWin->assignSelectedField (i, totalDisplayLines, 0);
			break;
		case IDM_MI_SOURCE_NEWER:
			if (showOnlyDifferentFiles)
				toggleMenuItem (IDM_MI_DIFFERENTFILES, &showOnlyDifferentFiles);
			toggleMenuItem (IDM_MI_SOURCE_NEWER, &showOnlyNewerFiles);
			buildDisplayLines ();
			break;
		case IDM_MI_DIFFERENTFILES:
			if (showOnlyNewerFiles)
				toggleMenuItem (IDM_MI_SOURCE_NEWER, &showOnlyNewerFiles);
			toggleMenuItem (IDM_MI_DIFFERENTFILES, &showOnlyDifferentFiles);
			buildDisplayLines ();
			break;
		case IDM_MI_EXCLUDE:
			result = WinDlgBox (HWND_DESKTOP, hwnd, excludeDialogProc, 
					NULL, IDD_EXCLUDEDIALOG, NULL);
			if ( !applyExcludeList)
				break;
			buildDisplayLines ();
			break;
		case IDM_CONFIGURE:
			result = WinDlgBox (HWND_DESKTOP, hwnd, configureDialogProc,
				NULL, IDD_DLG_CONFIGURE, NULL);
			break;
		case IDM_APPLY_EXCLUSION:
			toggleMenuItem (IDM_APPLY_EXCLUSION, &applyExcludeList);
			buildDisplayLines ();
			break;
		case IDM_MI_HIDDEN:
			toggleMenuItem (IDM_MI_HIDDEN, &showHiddenFiles);
			buildDisplayLines ();
			break;
		case IDM_MI_SYSTEM:
			toggleMenuItem (IDM_MI_SYSTEM, &showSystemFiles);
			buildDisplayLines ();
			break;
		case IDM_MI_NO_SUBDIRS:
			killFileCopy = 0;
			showSubdirs = ENSSNone;
			setModeMenuItem (IDM_MI_NO_SUBDIRS, showDirectoryIDs);
			readTwoDirectories ();
			break;
		case IDM_MI_ALL_SUBDIRS:
			killFileCopy = 0;
			showSubdirs = ENSSAll;
			setModeMenuItem (IDM_MI_ALL_SUBDIRS, showDirectoryIDs);
			readTwoDirectories ();
			break;
		case IDM_MI_SELECT_SUBDIRS:
			showSubdirs = ENSSSelected;
			setModeMenuItem (IDM_MI_SELECT_SUBDIRS, showDirectoryIDs);
			result = WinDlgBox (HWND_DESKTOP, hwnd, directorySelectDlgProc, 
					NULL, IDD_SELECT_DIRS_DLG, NULL);
			if (result)
				break;
			readTwoDirectories ();
			break;
		case IDM_MI_SIZE:
			toggleMenuItem (IDM_MI_SIZE, &showFileSize);
			setHScrollBar ();
			WinInvalidateRect (hwnd, NULL, FALSE);
			break;
		case IDM_MI_DATE:
			toggleMenuItem (IDM_MI_DATE, &showFileDate);
			setHScrollBar ();
			WinInvalidateRect (hwnd, NULL, FALSE);
			break;
		case IDM_SOURCEDIR:
			if ( !getDirectory (hwnd, &srcDirControl))
				buildDisplayLines ();
			break;
		case IDM_DESTINATIONDIR:
			if ( !getDirectory (hwnd, &destDirControl) )
				buildDisplayLines ();
			break;
		case IDM_DIR_DIR:
			if (updateMode == ENUMDirDir)
				break;
			if (setUpdateMode (ENATDir, ENATDir))
				break;
			setModeMenuItem (IDM_DIR_DIR, updateModeIDs);
			updateMode = ENUMDirDir;
			buildDisplayLines ();
			break;
		case IDM_DIR_ZIP:
			if (updateMode == ENUMDirZip)
				break;
			if (setUpdateMode (ENATDir, ENATZip) )
				break;
			setModeMenuItem (IDM_DIR_ZIP, updateModeIDs);
			updateMode = ENUMDirZip;
			buildDisplayLines ();
			break;
		case IDM_ZIP_DIR:
			if (updateMode == ENUMZipDir)
				break;
			if (setUpdateMode (ENATZip, ENATDir) )
				break;
			setModeMenuItem (IDM_ZIP_DIR, updateModeIDs);
			updateMode = ENUMZipDir;
			buildDisplayLines ();
			break;
		case IDM_UPDATE_FILES:
			switch (updateMode)
				{
			case ENUMZipDir:
				_beginthread (copyThread, NULL, 4*4096, &extractFilesArgs);
				WinDlgBox (HWND_DESKTOP, hwnd, copyStatusProc, 
						NULL, IDD_FILE_COPY, (PVOID) &initZipDir);
				readOneDirectory (&destDirControl, "Reading dest.", 0);
				buildDisplayLines ();
				break;
			case ENUMDirZip:
				_beginthread (copyThread, NULL, 4*4096, (PVOID) &updateZipArgs );
				WinDlgBox (HWND_DESKTOP, hwnd, copyStatusProc, 
						NULL, IDD_FILE_COPY, (PVOID) &initDirZip);
				readOneDirectory (&destDirControl, "Reading dest.", 0);
				buildDisplayLines ();
				break;
			case ENUMDirDir:
				killFileCopy = 0;
				_beginthread (copyThread, NULL, 4 * 4096, (PVOID) &copyFilesArgs);
				WinDlgBox (HWND_DESKTOP, hwnd, copyStatusProc, 
						NULL, IDD_FILE_COPY, (PVOID) &initDirDir);
				readOneDirectory (&destDirControl, "Reading dest.", 0);
				buildDisplayLines ();
				break;
				}
			break;
			}
		
		}
	break;
default:
	break;
	}
return WinDefWindowProc (hwnd, msg, mp1, mp2);
}		// clientClassProc

static radioButtonWindow	modeRB (IDD_RB_DIR_DIR),
							displayConditionRB ( IDD_RB_ALL),
							subdirRB ( IDD_RB_NO_SUBDIR);
static checkBoxWindow		applyExcludesCB (IDD_CB_APPLY_EXCLUDES),
							showSizeCB ( IDD_CB_SHOW_SIZE),
							showDateCB (IDD_CB_SHOW_DATE),
							showHiddenFilesCB (IDD_CB_HIDDEN_FILES),
							showSystemFilesCB (IDD_CB_SYSTEM_FILES),
							updateExcludesCB (IDD_CB_UPDATE_EXCLUDES),
							updateSrcDirCB (IDD_CB_UPDATE_SRC_DIR),
							updateDestDirCB (IDD_CB_UPDATE_DEST_DIR);

/* ************************ configureDialogProc ************************ */
MRESULT EXPENTRY configureDialogProc (HWND hwnd, ULONG msg, 
	MPARAM mp1, MPARAM mp2)
{

switch (msg)
	{int		i,
				j;
case WM_INITDLG:
	modeRB.setInterface (hwnd,  IDD_RB_DIR_DIR);
	modeRB.createIdentArray (hwnd);
	if (updateMode == ENUMDirZip)
		i = IDD_RB_DIR_ZIP;
	else if (updateMode == ENUMZipDir)
		i = IDD_RB_ZIP_DIR;
	else i = IDD_RB_DIR_DIR;
	modeRB.write (EN_OMSend, i);
	displayConditionRB.setInterface (hwnd, IDD_RB_ALL);
	displayConditionRB.createIdentArray (hwnd);
	i = IDD_RB_ALL;
	if (showOnlyNewerFiles)
		i = IDD_RB_SRC_NEWER;
	else if (showOnlyDifferentFiles)
		i = IDD_RB_DIFFERENT;
	displayConditionRB.write (EN_OMSend, i);
	subdirRB.setInterface (hwnd, IDD_RB_NO_SUBDIR);
	subdirRB.createIdentArray (hwnd);
	i = IDD_RB_NO_SUBDIR;
	j = 0;
	if (showSubdirs == ENSSAll)
		i = IDD_RB_ALL_SUBDIR;
	else if (showSubdirs == ENSSSelected)
		i = IDD_RB_SELECT_SUBDIR;
	subdirRB.write (EN_OMSend, i);
	applyExcludesCB.setInterface (hwnd, IDD_CB_APPLY_EXCLUDES);
	applyExcludesCB.write (EN_OMSend, applyExcludeList);
	showSizeCB.setInterface (hwnd, IDD_CB_SHOW_SIZE);
	showSizeCB.write (EN_OMSend, showFileSize);
	showDateCB.setInterface (hwnd, IDD_CB_SHOW_DATE);
	showDateCB.write (EN_OMSend, showFileDate);
	showHiddenFilesCB.setInterface (hwnd, IDD_CB_HIDDEN_FILES);
	showHiddenFilesCB.write (EN_OMSend, showHiddenFiles);
	showSystemFilesCB.setInterface (hwnd, IDD_CB_SYSTEM_FILES);
	showSystemFilesCB.write (EN_OMSend, showSystemFiles);
	updateExcludesCB.setInterface (hwnd, IDD_CB_UPDATE_EXCLUDES);
	updateSrcDirCB.setInterface (hwnd, IDD_CB_UPDATE_SRC_DIR);
	updateDestDirCB.setInterface (hwnd, IDD_CB_UPDATE_DEST_DIR);
	break;
case WM_COMMAND:
	if (SHORT1FROMMP (mp2) == CMDSRC_PUSHBUTTON)
		{switch (SHORT1FROMMP (mp1))
			{
		case IDD_PB_OK:
			modeRB.read();
			switch (modeRB.value)
				{
			case IDD_RB_DIR_DIR:
				i = ENUMDirDir;
				j = IDM_DIR_DIR;
				if (i != updateMode)
					{setUpdateMode (ENATDir, ENATDir);
					}
				break;
			case IDD_RB_DIR_ZIP:
				i = ENUMDirZip;
				j = IDM_DIR_ZIP;
				if (i != updateMode)
					{setUpdateMode (ENATDir, ENATZip);
					}
				break;
			case IDD_RB_ZIP_DIR:
				i = ENUMZipDir;
				j = IDM_ZIP_DIR;
				if (i != updateMode)
					setUpdateMode (ENATZip, ENATDir);
				break;
				}
			if (updateMode != i)
				{setModeMenuItem (j, updateModeIDs);
				updateMode = (ENUpdateMode) i;
				}
			displayConditionRB.read();
			menueItemForceState (IDM_MI_SOURCE_NEWER, &showOnlyNewerFiles, 0);
			menueItemForceState (IDM_MI_DIFFERENTFILES, &showOnlyDifferentFiles, 0);
			if (displayConditionRB.value == IDD_RB_SRC_NEWER)
				toggleMenuItem (IDM_MI_SOURCE_NEWER, &showOnlyNewerFiles);
			else if (displayConditionRB.value == IDD_RB_DIFFERENT)
				toggleMenuItem (IDM_MI_DIFFERENTFILES, &showOnlyDifferentFiles);
			subdirRB.read();
			switch (subdirRB.value)
				{
			case IDD_RB_NO_SUBDIR:
				killFileCopy = 0;
				if (showSubdirs != ENSSNone)
					{showSubdirs = ENSSNone;
					setModeMenuItem (IDM_MI_NO_SUBDIRS, showDirectoryIDs);
					readTwoDirectories ();
					}
				break;
			case IDD_RB_ALL_SUBDIR:
				killFileCopy = 0;
				if (showSubdirs != ENSSAll)
					{showSubdirs = ENSSAll;
					setModeMenuItem (IDM_MI_ALL_SUBDIRS, showDirectoryIDs);
					readTwoDirectories ();
					}
				break;
			case IDD_RB_SELECT_SUBDIR:
				if (showSubdirs != ENSSSelected)
					{showSubdirs = ENSSSelected;
					setModeMenuItem (IDM_MI_SELECT_SUBDIRS, showDirectoryIDs);
					readTwoDirectories ();
					}
				break;
				}
			applyExcludesCB.read();
			menueItemForceState (IDM_APPLY_EXCLUSION, &applyExcludeList,
			  applyExcludesCB.value);
			showSizeCB.read();
			menueItemForceState (IDM_MI_SIZE, &showFileSize, showSizeCB.value);
			showDateCB.read();
			menueItemForceState (IDM_MI_DATE, &showFileDate, showDateCB.value);
			showHiddenFilesCB.read();
			menueItemForceState (IDM_MI_HIDDEN, &showHiddenFiles,
			  showHiddenFilesCB.value);
			showSystemFilesCB.read();
			menueItemForceState (IDM_MI_SYSTEM, &showSystemFiles, 
			  showSystemFilesCB.value);
			updateExcludesCB.read ();
			if (updateExcludesCB.value)
				WinDlgBox (HWND_DESKTOP, hwnd, excludeDialogProc, 
					NULL, IDD_EXCLUDEDIALOG, NULL);
			updateSrcDirCB.read ();
			if (updateSrcDirCB.value)
				getDirectory (hwnd, &srcDirControl);
			updateDestDirCB.read ();
			if (updateDestDirCB.value)
				getDirectory (hwnd, &destDirControl);
			buildDisplayLines ();
			break;
			}
		
		}
	break;
	}
return WinDefDlgProc (hwnd, msg, mp1, mp2);
}		// configurationDialogProc

/* *************************** copyStatusProc *************************  */
static MRESULT EXPENTRY copyStatusProc (HWND hwnd, ULONG msg, 
		MPARAM mp1, MPARAM mp2)
{static copyStatInitST		*copyStatInit;
BOOL rc;
ERRORID		err;
BOOL		enable;
QMSG		qmsg;

switch (msg)
	{
case WM_CREATE:
	break;
case WM_INITDLG:
	copyStatInit = (copyStatInitST *) mp2;
	hwndCopyStatus = hwnd;
	rc = WinSetWindowText (hwnd, copyStatInit->title1);
	if ( !rc)
		err = WinGetLastError (hab);
	rc = WinSetDlgItemText (hwnd, IDD_STATIC1, copyStatInit->title2);
	enable = copyStatInit->enableCancel?	TRUE:	FALSE;
	rc = WinEnableWindow (WinWindowFromID (hwnd, IDD_PB_CANCEL), 
			enable);
	WinSendDlgItemMsg (hwnd, IDD_MLE1, MLM_SETREADONLY, (MPARAM) TRUE, NULL);
	break;
case UM_CURRENT_FILE_COPY:
	while (rc=WinPeekMsg( hab, &qmsg, hwnd, UM_CURRENT_FILE_COPY, 
		UM_CURRENT_FILE_COPY, PM_REMOVE) )
	if (rc)
		{mp1 = qmsg.mp1;
		mp2 = qmsg.mp2;
		}
	WinSendDlgItemMsg (hwnd, IDD_MLE1, MLM_DISABLEREFRESH, NULL, NULL);
	WinSendDlgItemMsg (hwnd, IDD_MLE1, MLM_DELETE, (MPARAM) 0, (MPARAM) 0xffff);
	WinSendDlgItemMsg (hwnd, IDD_MLE1, MLM_INSERT, mp1, NULL);
	WinSendDlgItemMsg (hwnd, IDD_MLE1, MLM_ENABLEREFRESH, NULL, NULL);
	break;
case UM_COPY_ERROR:
	WinMessageBox (HWND_DESKTOP, hwnd, (char *) mp1, (char *) mp2, 0, MB_OK);
	break;
case UM_QUERY_OVERWRITE:
		{ULONG rsp;
		rsp = WinMessageBox (HWND_DESKTOP, hwnd, 
				"destinition is newer - overwrite?", (char *) mp1, 0, MB_YESNOCANCEL);
		write (hWritePipe, (void *)&rsp, 4);
		}
	break;
case UM_NO_ZIP_CENTRAL_DIRECTORY:
	
	break;
case UM_COPY_COMPLETE:
	WinDismissDlg (hwnd, 0);
	return 0;
case WM_COMMAND:
	if (SHORT1FROMMP (mp2) != CMDSRC_PUSHBUTTON)
		break;
// Don't destroy the window.  Stop the copy process and let the
// copy process send a UM_COPY_COMPLETE message
	killFileCopy = 1;
	return (MPARAM) 1;
	}
return WinDefDlgProc (hwnd, msg, mp1, mp2);
}

/* ************************** directorySelectDlgProc ********************* */
static MRESULT EXPENTRY directorySelectDlgProc (HWND hwnd, ULONG msg,
		MPARAM mp1, MPARAM mp2)
{
switch (msg)
	{FILEFINDBUF3	findBuff;
	APIRET			rc;
	ULONG			fileCount;
	HDIR			hdir;
	int				selectIndex;
	char			nameTemplate[300],
					**ppch;
	int				directoryCount,
					i;
	
case WM_INITDLG:
	hdir = HDIR_CREATE;
	fileCount = 1;
	if (ENUMZipDir == updateMode)
		strcpy (nameTemplate, destDirControl.directoryName);
	else
		strcpy (nameTemplate, srcDirControl.directoryName);
	strcat (nameTemplate, "*");
	rc = DosFindFirst (nameTemplate, &hdir, MUST_HAVE_DIRECTORY, (PVOID) &findBuff, 
			sizeof (findBuff), &fileCount, FIL_STANDARD);
		WinSendDlgItemMsg (hwnd, IDD_SUBDIR_NAMES, LM_DELETEALL, NULL, NULL);
	for (directoryCount=0; !rc;	)
		{
		if ( !strcmp ("..", findBuff.achName) )
			goto l4;
		if ( !strcmp (".", findBuff.achName) )
			goto l4;
		WinSendDlgItemMsg (hwnd, IDD_SUBDIR_NAMES, LM_INSERTITEM, 
				(MPARAM) LIT_END, findBuff.achName);
		++directoryCount;
	// Select any directory which was selected in 
	// a previos execution of this dialog
		for (i=0, ppch=(char **)selectedDirPointers.getBaseAddress();
			  i<selectedDirCount;	++i, ++ppch)
			{
			if ( !strcmp (findBuff.achName, *ppch) )
				{
				WinSendDlgItemMsg (hwnd, IDD_SUBDIR_NAMES, LM_SELECTITEM,
					(MPARAM) (directoryCount-1), (MPARAM) TRUE);
				break;
				}
			}
		
l4:
		
		rc = DosFindNext (hdir, (PVOID) &findBuff, sizeof (findBuff),
				&fileCount);
		}
	break;
case WM_COMMAND:
	if (SHORT1FROMMP (mp2) != CMDSRC_PUSHBUTTON)
		break;
	if (SHORT1FROMMP (mp1) != IDD_PB_OK)
		{WinDismissDlg (hwnd, 1);
		return 0;
		}
	selectIndex = LIT_FIRST;
	selectIndex = (int) WinSendDlgItemMsg (hwnd, IDD_SUBDIR_NAMES, 
			LM_QUERYSELECTION, (PVOID) selectIndex, NULL);
	if (selectIndex == LIT_NONE)
		{WinDismissDlg (hwnd, 1);
		return 0;
		}
	selectedDirCount = 0;
	selectedDirNames.reInitialize ();
	selectedDirPointers.reInitialize ();
	while (selectIndex != LIT_NONE)
		{char		**ppch;
		int			textLength;
		
		++selectedDirCount;
		textLength = (int) WinSendDlgItemMsg (hwnd, IDD_SUBDIR_NAMES,
				LM_QUERYITEMTEXTLENGTH, (MPARAM) selectIndex, NULL) + 1;
		ppch = (char **) selectedDirPointers.requestMem (sizeof (char *));
		*ppch = (char *) selectedDirNames.requestMem (textLength);
		WinSendDlgItemMsg (hwnd, IDD_SUBDIR_NAMES, LM_QUERYITEMTEXT, 
			MPFROM2SHORT (selectIndex, textLength),
			(MPARAM) *ppch);
		selectIndex = (int) WinSendDlgItemMsg (hwnd, IDD_SUBDIR_NAMES, 
			LM_QUERYSELECTION, (PVOID) selectIndex, NULL);
		}
	WinDismissDlg (hwnd, 0);
	return 0;
	}
return WinDefDlgProc (hwnd, msg, mp1, mp2);
}

/* ******************** errorLogDisplayDlgProc ************************* */
static MRESULT EXPENTRY errorLogDisplayDlgProc ( HWND hwnd, ULONG msg,
	MPARAM mp1, MPARAM mp2)
{int hError;
int		i,
		j;
char	errorMessage[500];
char	*pch;

switch (msg)
	{
case WM_INITDLG:
	hError = *((int *) mp2);
	lseek (hError, 0, SEEK_SET);
	while (1)
		{for (i=0, pch=errorMessage;	;	++i, ++pch)
			{
			j = read (hError, pch, 1);
			if ( j != 1)
				{close (hError);
				goto l4;
				}
			if (*pch == -1)
				goto l4;;
			if (*pch == '\n')
				break;
			}
		*pch = 0;
		WinSendDlgItemMsg (hwnd, IDD_LB_ERROR_LOG, LM_INSERTITEM, (MPARAM) LIT_END,
			(PSZ) errorMessage);
		}
	break;
	}

l4:

return WinDefDlgProc (hwnd, msg, mp1, mp2);
}

/* ************************ buildDisplayLines **************************** */
static void buildDisplayLines (void)
{
FILE_DESCRIPTION		*ps,
						*pd;
int						sourceCount,
						destCount;

sourceCount = 0;
destCount = 0;
ps = srcDirControl.firstFile;
pd = destDirControl.firstFile;
lineControlHeap.reInitialize ();
totalDisplayLines = 0;
maxFileNameLength = 0;
while (1)
	{int			compareResult;
	DISPLAY_LINE	*currentLine;
	
	currentLine = (DISPLAY_LINE *) lineControlHeap.requestMem (sizeof (DISPLAY_LINE));
	
l2:
	
	if ( (sourceCount<srcDirControl.fileCount) && (destCount<destDirControl.fileCount) )
		compareResult = stricmp(ps->name, pd->name);
	else if (sourceCount < srcDirControl.fileCount)
		compareResult = -1;
	else if (destCount < destDirControl.fileCount)
		compareResult = 1;
	else
		break;
	if (compareResult < 0)
		{currentLine->source = ps;
		currentLine->dest = NULL;
		++ps;
		++sourceCount;
		}
	else if (compareResult == 0)
		{currentLine->source = ps;
		currentLine->dest = pd;
		++ps;
		++sourceCount;
		++pd;
		++destCount;
		}
	else
		{currentLine->source = NULL;
		currentLine->dest = pd;
		++pd;
		++destCount;
		}
	if ( isLineExcluded (currentLine))
		goto l2;
	if ( !isLineDisplayed (currentLine))
		goto l2;
	if (sourceCount < srcDirControl.fileCount)
		{if (maxFileNameLength < ps->nameStringLength)
			maxFileNameLength = ps->nameStringLength;
		}
	if (destCount < destDirControl.fileCount)
		{if (maxFileNameLength < pd->nameStringLength)
			maxFileNameLength = pd->nameStringLength;
		}
	currentLine->selected = 0;
	++totalDisplayLines;
	
	}
displayLineControl = (DISPLAY_LINE *) lineControlHeap.getBaseAddress ();
pWin->newDisplayList (totalDisplayLines, maxDisplayLineLength);
setHScrollBar ();
WinInvalidateRect (hwndClient, NULL, FALSE);
return;
}

/* ************************ compareSelectedFiles *************************** */
static void compareSelectedFiles (PVOID notUsed)
{
#define COMPARE_BUFFER_SIZE 32768
char		*srcBuff,
			*destBuff,
			*pSrc,
			*pDest;
int			hSrc,
			hDest,
			hError;
int			badCompares = 0,
			i,
			j,
			sourceDirLength,
			destDirLength;
int			sourceBytesRead,
			destBytesRead;
DISPLAY_LINE	*pDL;
FSALLOCATE		allocBuff;
FILE_DESCRIPTION	*ps,
					*pd;
char			errorBuff[300],
				sourceFileName[300],
				destFileName[300],
				workFileName[300];
ULONG			availableBytes;
APIRET			rc;
BOOL			ret;
FILESTATUS3		infoBuffer;
ULONG			originalFileAttributes;


srcBuff = (char *)malloc (COMPARE_BUFFER_SIZE);
destBuff = (char *) malloc (COMPARE_BUFFER_SIZE);
strcpy (sourceFileName, sourceDir);
strcpy (destFileName, destDir);
strcpy (workFileName, destDir);
sourceDirLength = strlen (sourceDir);
destDirLength = strlen (destDir);
hError = open (ERROR_FILE_NAME, O_CREAT|O_TRUNC|O_RDWR, S_IWRITE|S_IREAD);
if ( !stricmp (sourceDir, destDir))
	{WinPostMsg (hwndCopyStatus, UM_COPY_ERROR, 
		(MPARAM)"Error - source and dest directories are identical", NULL);
	return;
	}
for (i=0, pDL=displayLineControl;	i<totalDisplayLines;	++i, ++pDL)
	{
	if ( !pWin->querySelect (i))
		continue;
	if ( !pDL->source)
		continue;
	if (killFileCopy)
		return;
	ps = pDL->source;
	if ( !ps)
		continue;
	strcpy (sourceFileName + sourceDirLength, ps->name);
	pd = pDL->dest;
	if ( !pd)
		{sprintf (errorBuff, "%s: No destination file\n", sourceFileName);
		++badCompares;
		write (hError, errorBuff, strlen(errorBuff) );
		continue;
		}
	strcpy (destFileName + destDirLength, pd->name);
	if (pDL->source->size != pDL->dest->size)
		{sprintf (errorBuff, "%s: source and dest are different sizes\n", sourceFileName);
		write (hError, errorBuff, strlen (errorBuff) );
		++badCompares;
		continue;
		}
	hSrc = open (sourceFileName, O_RDONLY, 0);
	if (hSrc == -1)
		{sprintf (errorBuff, "Open error on source: %s\n", sourceFileName);
		write (hError, errorBuff, strlen (errorBuff) );
		++badCompares;
		continue;
		}
	hDest = open (destFileName, O_RDONLY, 0);
	if (hDest == -1)
		{sprintf (errorBuff, "Open error on destination: %s\n", destFileName);
		write (hError, errorBuff, strlen (errorBuff) );
		++badCompares;
		close (hSrc);
		continue;
		}
	ret = WinPostMsg (hwndCopyStatus, UM_CURRENT_FILE_COPY, (MPARAM) sourceFileName, 
			NULL);
	for (	;	;	)
		{sourceBytesRead = read (hSrc, srcBuff, COMPARE_BUFFER_SIZE);
		destBytesRead = read (hDest, destBuff, COMPARE_BUFFER_SIZE);
		if (memcmp ( (void *) destBuff, (void *)srcBuff, sourceBytesRead) )
			{sprintf (errorBuff, "%s: Data compare error\n", destFileName);
			++badCompares;
			break;
			}
		if (sourceBytesRead!=COMPARE_BUFFER_SIZE)
			break;
		}

L5:
		
	close (hSrc);
	close (hDest);
	}


l10:


free (srcBuff);
free (destBuff);
if ( !badCompares)
	{WinPostMsg (hwndClient, UM_COMPARE_STATUS, (MPARAM) 0, (MPARAM) 0);
 	close (hError);
	unlink (ERROR_FILE_NAME);
	}
else
	{WinPostMsg (hwndClient, UM_COMPARE_STATUS, (MPARAM) hError, (MPARAM) 0);
 	close (hError);
	
	}
return;
}

/* *********************** conditionalProcessArchive ************************ */
static int conditionalProcessArchive (HWND hwnd, DIRECTORY_CONTROL *control,
	char *title)
{
if (control->archiveType == ENATZip)
	{
	readOneDirectory (control, title, 0);
	}
else
	{conditionalDirectoryDlg (hwnd, IDD_DIRECTORYDIALOG, control->directoryName, 
		control->title);
	readOneDirectory (control, title, 0);
	}
return 0;
}

static char		*tempName = ERROR_FILE_NAME;

/* ******************************* copyFiles **************************** */
static void copyFiles (void *notUsed)
{
int				destDisk,
				i,
				j,
				sourceDirLength,
				destDirLength;
DISPLAY_LINE	*pDL;
FSALLOCATE		allocBuff;
FILE_DESCRIPTION	*ps,
					*pd;
static char			sourceFileName[300],
				destFileName[300],
				workFileName[300];
ULONG			availableBytes;
APIRET			rc;
BOOL			ret;
FILESTATUS3		infoBuffer;
ULONG			originalFileAttributes;

strcpy (sourceFileName, sourceDir);
strcpy (destFileName, destDir);
strcpy (workFileName, destDir);
strcat (workFileName, tempName);
sourceDirLength = strlen (sourceDir);
destDirLength = strlen (destDir);
if ( !stricmp (sourceDir, destDir))
	{WinPostMsg (hwndCopyStatus, UM_COPY_ERROR, 
		(MPARAM)"Error - source and dest directories are identical", NULL);
	return;
	}
destDisk = toupper (*destDir) - 'A' + 1;
for (i=0, pDL=displayLineControl;	i<totalDisplayLines;	++i, ++pDL)
	{
	if ( !pWin->querySelect (i))
		continue;
	if ( !pDL->source)
		continue;
	ps = pDL->source;
	pd = pDL->dest;
	if (pd)
		{ULONG		sdate, ddate;
		
		sdate = dateTimeToLong (ps->date, ps->time);
		ddate = dateTimeToLong (pd->date, pd->time);
		if (sdate < ddate)
			{ULONG rsp;
			
			WinPostMsg (hwndCopyStatus, UM_QUERY_OVERWRITE, (MPARAM) ps->name, 0);
			
			read (hReadPipe, (void*) &rsp, sizeof (rsp));
			if (rsp == MBID_NO)
				continue;
			if (rsp == MBID_CANCEL)
				return;
			}
		}
	DosQueryFSInfo (destDisk, FSIL_ALLOC, (PVOID) &allocBuff, sizeof (allocBuff) );
	availableBytes = allocBuff.cSectorUnit * allocBuff.cUnitAvail * 
			allocBuff.cbSector;
	if (availableBytes <= (ps->size + 512) )
		{WinPostMsg (hwndCopyStatus, UM_COPY_ERROR, 
			(MPARAM) "Insufficient space for copy", NULL);
		return;
		}
	strcpy (sourceFileName + sourceDirLength, ps->name);
	if (killFileCopy)
		return;
	ret = WinPostMsg (hwndCopyStatus, UM_CURRENT_FILE_COPY, (MPARAM) sourceFileName, 
			NULL);
/* If the destination is defined it has the same name as the source file.
   Use the source name in case there is no destination
*/
	strcpy (destFileName + destDirLength, ps->name);
	DosQueryPathInfo (destFileName, FIL_STANDARD, (PVOID) &infoBuffer,
				sizeof(infoBuffer));
	originalFileAttributes = infoBuffer.attrFile;
	infoBuffer.attrFile &= ~(FILE_SYSTEM | FILE_HIDDEN | FILE_READONLY);
	DosSetPathInfo (destFileName, FIL_STANDARD, (PVOID) &infoBuffer,
				sizeof(infoBuffer), 0);
	DosMove (destFileName, workFileName);
	rc = DosCopy (sourceFileName, destFileName, DCPY_EXISTING);
	if (rc)
		{j = createFullPath (sourceFileName, destFileName);
		if (j)
			{
			WinPostMsg (hwndCopyStatus, UM_COPY_ERROR, 
				(MPARAM) "Error copying file - aborting copy", NULL);
			DosMove (workFileName, destFileName);
			infoBuffer.attrFile = originalFileAttributes;
			DosSetPathInfo (destFileName, FIL_STANDARD, (PVOID) &infoBuffer,
					sizeof(infoBuffer), 0);
			return;
			}
		}
	DosDelete (workFileName);
	}
return;
}

/* ************************** copyThread ***************************** */
#if defined (__IBMCPP__)
static void _Optlink copyThread (void *workerArgs)
#else
static void  copyThread (void *workerArgs)
#endif
{copyThreadArgST	*args;

args = (copyThreadArgST *) workerArgs;
DosSetPriority (PRTYS_THREAD, 0, -1, 0);
args->worker (args->arg);
WinPostMsg (hwndCopyStatus, UM_COPY_COMPLETE, NULL, NULL);
return;
}

/* ************************** createFullPath **************************** */
/* Input is two strings defining fully-qualified path names.  The routine
   assumes that a failing attempt has already been made to copy the 
   source to the destination.  Examine each subdirectory name component
   of the destination path.  Create the subdirectory if it does not exist. 
   after the subdirectory processing is complete attempt to copy the
   source to the destination.  Since the caller has already made a 
   failed attempt to perform this copy, it is an error if no subdirectory
   was created.  Return non-zero if the copy failed, else 0;
*/

static int createFullPath (const char *source, char *dest)
{
APIRET		rc;
char		save,
			*tok;
int			retVal = 1;
int			madeDir = 0;
FILESTATUS3	statBuff;
char		*dirSep = "/\\";

tok = strpbrk (dest, dirSep);
while (tok)
	{
	++tok;
	tok = strpbrk (tok, dirSep);
	if ( !tok)
		break;
	save = *tok;
	*tok = 0;
	rc = DosQueryPathInfo (dest, FIL_STANDARD, &statBuff, sizeof (statBuff));
//	if (rc && (rc!= ERROR_PATH_NOT_FOUND))
//		goto l10;
	if (rc)
		{rc = DosCreateDir (dest, NULL);
		madeDir = 1;
		if (rc)
			goto l10;
		}
 	else
		{
	// if the path exists it must be a directory
		if ( !statBuff.attrFile & FILE_DIRECTORY)
			goto l10;
		}
	*tok = save;
	}

if ( !madeDir)
	goto l10;
retVal = (int) DosCopy (source, dest, DCPY_EXISTING);
return retVal;

l10:

if (tok)
	*tok = save;
return retVal;
}

/* ************************** dateTimeToLong ****************************** */
static ULONG dateTimeToLong (FDATE d, FTIME t)
{
ULONG rVal;

rVal = (d.year<<9) + (d.month<<5) + d.day;
rVal = (rVal<<16) + (t.hours<<11) + (t.minutes<<5) + t.twosecs;
return rVal;
}

/* ************************** excludeDialogProc ************************** */
static MRESULT EXPENTRY excludeDialogProc (HWND hwnd, ULONG msg, 
		MPARAM mp1, MPARAM mp2)
{
static HWND		hwndComboBox;

switch (msg)
	{char			buff[50];
	EXCLUDE_STRING	*pES;
	int				i, j, k;
	HWND			hwndAdd;
	
case WM_INITDLG:
	hwndComboBox = WinWindowFromID (hwnd, IDD_CB_EXCLUDE);
	pES = (EXCLUDE_STRING *)excludeListPointers.getBaseAddress ();
	for (i=0;	i<excludeListEntryCount;	++i, ++pES)
		{
		WinSendDlgItemMsg(hwnd, IDD_CB_EXCLUDE, LM_INSERTITEM, (MPARAM)LIT_END, 
			(MPARAM) pES->name);
		}
	WinShowWindow (hwnd, TRUE);
	hwndAdd = WinWindowFromID (hwnd, IDD_PB_ADD);
	WinSendMsg (hwndAdd, BM_SETDEFAULT, (MPARAM)TRUE, (MPARAM)0);
	break;
case WM_COMMAND:
	if (SHORT1FROMMP (mp2) != CMDSRC_PUSHBUTTON)
		break;
	switch ( SHORT1FROMMP (mp1))
		{WNDPARAMS		wndParams;
	case IDD_PB_OK:
		excludeListPointers.reInitialize ();
		excludeListText.reInitialize ();
		j = (int) WinSendMsg (hwndComboBox, LM_QUERYITEMCOUNT, NULL, NULL);
		for (i=0;	i<j;	++i)
			{
			
			k = (int)WinSendMsg (hwndComboBox, LM_QUERYITEMTEXTLENGTH, 
				(MPARAM) i, NULL) + 1;
			pES = (EXCLUDE_STRING *)excludeListPointers.requestMem (sizeof (EXCLUDE_STRING));
			pES->name = (char *) excludeListText.requestMem (k);
			WinSendMsg (hwndComboBox, LM_QUERYITEMTEXT, 
				MPFROM2SHORT (i, k), (MPARAM) pES->name);
			pES->length = strlen (pES->name);
			}
		excludeListEntryCount = j;
		WinDismissDlg (hwnd, 0);
		return 0;
	case IDD_PB_CANCEL:
		WinDismissDlg (hwnd, 1);
		return 0;
	case IDD_PB_ADD:
		WinQueryWindowText (hwndComboBox, 49, buff);
		WinSendMsg (hwndComboBox, LM_INSERTITEM, (MPARAM) (LIT_END),
				(MPARAM) buff);
		WinSetFocus (HWND_DESKTOP, hwndComboBox);
		return (MRESULT) 1;
	case IDD_PB_DELETE:
		i = (int) WinSendMsg (hwndComboBox, LM_QUERYSELECTION,
			(MPARAM) 0, NULL);
		if (i == LIT_NONE)
			return (MRESULT) 1;
		WinSendMsg (hwndComboBox, LM_DELETEITEM, (MPARAM) i, NULL);
		return (MRESULT) 1;
	default:
		break;
		}
	break;
	}
return WinDefDlgProc (hwnd, msg, mp1, mp2);
}

/* ************************* addStringToHeap ***************************** */
static char * addStringToHeap (char *src, heapManagerOB *heap)
{char		*pch;
int			i;

i = strlen (src);
pch = (char *) heap->requestMem(i+1);
strcpy (pch, src);
*(pch+i) = ' ';
return pch+i;
}

/* ************************** extractFiles ************************** */
static void extractFiles (PVOID)
{
STARTDATA	sd;
ULONG		idSession;
PID			pid;
char *		waitQueue = "\\QUEUES\\WaitQueue";
char		objBuff[260];
heapManagerOB	optionBuff(20000);
char 		*optionPtr;
char		*baseOption = "-o -C";
HQUEUE		hqueue;
APIRET		rc;
REQUESTDATA reqData;
ULONG		removeCount;
PVOID		removeData;
BYTE		priority;
char		*pch1;
int			i, j;
DISPLAY_LINE	*pDL;
int				destDisk,
				fileCount;
FILE_DESCRIPTION	*ps,
					*pd;
static char			sourceFileName[300];
BOOL			ret;
FILESTATUS3		infoBuffer;


pch1 = destDirControl.directoryName;
strcpy (objBuff, pch1);
if ( (i=strlen (pch1)) >3)
	*(objBuff+i-1) = 0;
DosSetDefaultDisk ( toupper (*pch1)-'A'+1);
i = chdir (objBuff);
rc = DosCreateQueue (&hqueue, QUE_FIFO, waitQueue);
if (rc)
	{return;
	}
for (i=0, pDL=displayLineControl;	i<totalDisplayLines;	)
	{
	optionBuff.reInitialize();
	addStringToHeap (baseOption, &optionBuff);
	addStringToHeap (srcDirControl.directoryName, &optionBuff);
	for (fileCount = 0;	(fileCount<5) && (i<totalDisplayLines);	++i, ++pDL)
		{char		*pName;
	
		if ( !pDL->source)
			continue;
		if (!pWin->querySelect (i))
			continue;
		pName = pDL->source->name;
		if ( !pName)
			continue;
		ps = pDL->source;
		pd = pDL->dest;
		if (pd)				// Destination exist ?
			{ULONG		sdate, ddate;
		
			sdate = dateTimeToLong (ps->date, ps->time);
			ddate = dateTimeToLong (pd->date, pd->time);
			if (sdate < ddate)	// Destination newer?
				{ULONG rsp;
				
				WinPostMsg (hwndCopyStatus, UM_QUERY_OVERWRITE, (MPARAM) ps->name, 0);
				
				read (hReadPipe, (void*) &rsp, sizeof (rsp));
				if (rsp == MBID_NO)
					continue;
				if (rsp == MBID_CANCEL)
					return;
				}				//Destination newer
			}				// Destination exists
		optionPtr = addStringToHeap (pName, &optionBuff);
		}
	if ( !fileCount)
		continue;
	*optionPtr = 0;
	++fileCount;
	memset ( (void *) &sd, 0, sizeof (sd) );
	sd.Length = sizeof (sd);
	sd.Related = SSF_RELATED_CHILD;
	sd.FgBg = SSF_FGBG_FORE;
	sd.PgmName = "unzip.exe";
	sd.PgmInputs = (char *)optionBuff.getBaseAddress ();
	sd.TermQ = waitQueue;
	sd.InheritOpt = SSF_INHERTOPT_PARENT;
	sd.SessionType = SSF_TYPE_WINDOWABLEVIO;
	sd.PgmControl = SSF_CONTROL_MAXIMIZE;
	sd.ObjectBuffer = objBuff;
	sd.ObjectBuffLen = 260;
	rc = DosStartSession (&sd, &idSession, &pid);
	// DosStartSession always seems to return an error, so ignore it
	// if ( 1)
		{DosReadQueue (hqueue, &reqData, &removeCount, &removeData, 0, 
				DCWW_WAIT, &priority, 0);
		DosFreeMem ( (VOID *) &reqData);
		}
	}
rc = DosCloseQueue (hqueue);
return;
}

/* **************************** fileCompare ****************************** */
#if defined (__IBMCPP__)
static int _Optlink fileCompare (const void * key, const void * element)
#else
static int fileCompare (const void * key, const void * element)
#endif
{
return stricmp ( (((FILE_DESCRIPTION *) key))->name, 
		(((FILE_DESCRIPTION *) element))->name);
}

/* **************************** formatFileInfo ***************************** */
static void		formatFileInfo (char *work, FILE_DESCRIPTION *pf)
{
char			*ch;
int				year;

ch = work;
*ch++ = ' ';
if (showFileSize)
	ch+= sprintf (ch, "%7d ", pf->size);
if (showFileDate)
	{year = pf->date.year+80;
	if (year >=100)
		year -= 100;
	ch += sprintf (ch, "%02d-%02d-%02d %02d:%02d ", pf->date.month,
		pf->date.day, year, pf->time.hours, pf->time.minutes);
	}
strcpy (ch, pf->name);
return;
}

/* **************************** format Panel ************************** */
char * formatPanel (int panel, int line)
{static char work[300];
DIRECTORY_CONTROL	*pd;
FILE_DESCRIPTION	*pf;
DISPLAY_LINE		*pl;

if (line > totalDisplayLines )
	{work[0] = 0;
	return work;
	}
if ( !panel)
	pd = &srcDirControl;
else
	pd = &destDirControl;
if ( !line)
	return pd->directoryName;
pl = displayLineControl + line - 1;
if ( !panel)
	pf = pl->source;
else
	pf = pl->dest;
*work = 0;
if (pf)
	formatFileInfo (work, pf);
return work;
}

/* ***************************** getDirectory ****************************** */
// Returns 1 if user aborts the dialog else returns 0
static int		getDirectory (HWND hwnd, DIRECTORY_CONTROL *control)
{int			i;

if (control->archiveType == ENATDir)
	{i = directoryDlg (hwnd, IDD_DIRECTORYDIALOG, control->directoryName,
		control->title);
	if (i)
		return 1;
	readOneDirectory (control, "Reading directory", 0);
	return 0;
	}
else
	{FILEDLG		fdlg;
	
	memset (&fdlg, 0, sizeof (fdlg));
	fdlg.cbSize = sizeof (fdlg);
	fdlg.fl = FDS_CENTER | FDS_OPEN_DIALOG;
	fdlg.pszTitle = control->title;
	strcpy (fdlg.szFullFile, "*.zip");
	WinFileDlg (HWND_DESKTOP, hwnd, &fdlg);
	if (fdlg.lReturn != DID_OK)
		return 1;
	strcpy (control->directoryName, fdlg.szFullFile);
	readOneDirectory (control, "Reading zip archive.", 0);
	return 0;
	}
return 0;
}

/* ***************************** isLineDisplayed ************************* */
static int	isLineDisplayed (DISPLAY_LINE *pDL)
{FILE_DESCRIPTION		*ps, *pd;
ULONG		sourceTime,
			destTime;

ps = pDL->source;
pd = pDL->dest;
if (ps)
	{if (ps->attributes & FILE_SYSTEM)
		{if (!showSystemFiles)
			return 0;
		}
	if (ps->attributes & FILE_HIDDEN)
		{if (!showHiddenFiles)
			return 0;
		}
	}
if ( (!pDL->source)  || ( !pDL->dest) )
	return 1;
if (showOnlyNewerFiles)
	{if (ps->date.year > pd->date.year)
		return 1;
	if (ps->date.year < pd->date.year)
		return 0;
	if (ps->date.month > pd->date.month)
		return 1;
	if (ps->date.month < pd->date.month)
		return 0;
	if (ps->date.day > pd->date.day)
		return 1;
	if (ps->date.day < pd->date.day)
		return 0;
	sourceTime = 3600*ps->time.hours + 60*ps->time.minutes + 2*ps->time.twosecs;
	destTime = 3600*pd->time.hours + 60*pd->time.minutes + 2*pd->time.twosecs;
	if (sourceTime > destTime)
		return 1;
	return 0;
	}
if ( !showOnlyDifferentFiles)
	return 1;
if ( (ps->date.day != pd->date.day) || (ps->date.month != pd->date.month ) ||
	( ps->date.year != pd->date.year) || (ps->time.twosecs != pd->time.twosecs) 
	|| (ps->time.minutes != pd->time.minutes) || (ps->time.hours != pd->time.hours) )
	return 1;
return 0;
}

/* *************************** isLineExcluded **************************** */
static int isLineExcluded (DISPLAY_LINE *pDL)
{EXCLUDE_STRING		*pES;
int					i;
FILE_DESCRIPTION	*pFD;

if ( !applyExcludeList)
	return 0;
if (pDL->source)
	pFD = pDL->source;
else
	pFD = pDL->dest;
pES = (EXCLUDE_STRING *) excludeListPointers.getBaseAddress ();
for (i=0;	i<excludeListEntryCount;	++i, ++pES)
	{
	if ( !stricmp (pFD->name+pFD->nameStringLength-pES->length, pES->name) )
		return 1;
	}
return 0;
}

struct dirM{
	int			allocatedForName,
				entriesRead;
	char *		name;
	};

/* ****************************** mustComplete ************************** */
static void mustComplete (HWND hwnd)
{
WinMessageBox (HWND_DESKTOP, hwnd, "Mode change has started.\nYou must complete it",
			NULL, 0, MB_OK);
return;
}

/* ***************************** readDirectory ***************************** */
static void		readDirectory (PVOID ctrl)
{int			baseDirNameLen,
				currentDirNameLen,
				dirLevel = 0,
				i,
				j;
ULONG			fileCount;
FILEFINDBUF3	findBuff;
/* Totally pathological behaviour:  When this address is passed to the
   copy file dialog it is sometimes invalid.  I could understand if it were
   last last message sent, but it happens on the first message
*/
static char			nameTemplate[300];
HDIR			hdir;
APIRET			rc;
heapManagerOB	dirManage(50000),
				dirNameText(50000);
dirM 			*currentDir,
				*workDir;
DIRECTORY_CONTROL	*control;

control = (DIRECTORY_CONTROL *) ctrl;
totalDisplayLines = 1;
if (control->archiveType == ENATZip)
	{readZipDirectory (control);
     return;
	}
control->fileCount = 0;
control->fileNameText->reInitialize ();
control->fileControlHeap->reInitialize ();
currentDir = (dirM *) dirManage.requestMem (sizeof (dirM));
currentDir->name = control->directoryName;
baseDirNameLen = strlen (control->directoryName);
currentDir->entriesRead = 0;

l2:

*nameTemplate = 0;
currentDirNameLen = 0;
/* Concantnate the names of all subdirectories currently being
   processed in order to create a fully-qualified name for the
   current directory
*/
for (i=0, workDir=(dirM *)dirManage.getBaseAddress();	i<=dirLevel;	
	++i, ++workDir)
	{j = strlen (workDir->name);
	memcpy (nameTemplate+currentDirNameLen, workDir->name, j);
	currentDirNameLen += j;
	}
*(nameTemplate+currentDirNameLen) = 0;
WinPostMsg (hwndCopyStatus, UM_CURRENT_FILE_COPY, (MPARAM) nameTemplate, NULL);
currentDirNameLen -= baseDirNameLen;
strcat (nameTemplate, "*");
hdir = HDIR_CREATE;
fileCount = 1;
rc = DosFindFirst (nameTemplate, &hdir, 
		FILE_ARCHIVED | FILE_DIRECTORY | FILE_SYSTEM | FILE_HIDDEN | FILE_READONLY,
		(PVOID) &findBuff, sizeof (findBuff), &fileCount, FIL_STANDARD);
/* We may have suspended processing of the current directory in
   order to process a sub directory.  Skip all the files which have
   already been processed.
*/
if (rc)
    {return;
    }
for (i=0;	i<currentDir->entriesRead;	++i)
	rc = DosFindNext (hdir, &findBuff, sizeof (findBuff), &fileCount);
while ( !rc)
	{
	char		**ppch;
	FILE_DESCRIPTION		*pdesc;
	
	++currentDir->entriesRead;
	if (findBuff.attrFile & FILE_DIRECTORY)
		{if ( !showSubdirs)
			goto l4;
		if (dirLevel)
			goto l3;
		if (showSubdirs != ENSSSelected)
			goto l3;
		for (j=0, ppch=(char **)selectedDirPointers.getBaseAddress();
				j<selectedDirCount;	++j, ++ppch)
			{if ( !stricmp (findBuff.achName, *ppch))
				goto l3;
			}
		goto l4;
		
l3:
		
		if ( !strcmp (".", findBuff.achName) | !strcmp ("..", findBuff.achName) )
			goto l4;
		++dirLevel;
		DosFindClose (hdir);
		currentDir = (dirM *)dirManage.requestMem (sizeof (dirM));
		currentDir->entriesRead = 0;
		currentDir->allocatedForName = findBuff.cchName + 2;
		currentDir->name = (char *)dirNameText.requestMem (currentDir->allocatedForName);
		strcpy (currentDir->name, findBuff.achName);
		strcat (currentDir->name, "/");
		i = strlen (nameTemplate) - 1;		// Must ignore the trailing `*'
		currentDirNameLen = findBuff.cchName + i + 1 - baseDirNameLen;
		strcpy (nameTemplate+i, currentDir->name);
		WinPostMsg (hwndCopyStatus, UM_CURRENT_FILE_COPY, (MPARAM) nameTemplate,
			NULL);
		strcat (nameTemplate, "*");
		hdir = HDIR_CREATE;
		rc = DosFindFirst (nameTemplate, &hdir, 
			FILE_ARCHIVED | FILE_DIRECTORY | FILE_SYSTEM | FILE_HIDDEN | FILE_READONLY,
			(PVOID) &findBuff, sizeof (findBuff), &fileCount, FIL_STANDARD);
		continue;
		}
	pdesc = (FILE_DESCRIPTION *)control->fileControlHeap->requestMem (sizeof (FILE_DESCRIPTION) );
	pdesc->name = (char *)control->fileNameText->requestMem (
				currentDirNameLen + findBuff.cchName+1);
	strncpy (pdesc->name, nameTemplate+baseDirNameLen, currentDirNameLen);
	strcat (pdesc->name, findBuff.achName);
	pdesc->nameStringLength = findBuff.cchName + currentDirNameLen;
	pdesc->attributes = findBuff.attrFile;
	pdesc->date = findBuff.fdateLastWrite;
	pdesc->time = findBuff.ftimeLastWrite;
	pdesc->size = findBuff.cbFile;
	++control->fileCount;
	
l4:

	if (killFileCopy)
		break;	
	rc = DosFindNext (hdir, &findBuff, sizeof (findBuff), &fileCount);
	}
DosFindClose (hdir);
--dirLevel;
dirNameText.releaseMem (currentDir->allocatedForName );
dirManage.releaseMem (sizeof (dirM));
--currentDir;

if (dirLevel >= 0)
	goto l2;
if (control->fileCount)
	{FILE_DESCRIPTION	*plist;
	
	plist = (FILE_DESCRIPTION *) control->fileControlHeap->getBaseAddress ();
	control->firstFile = plist;
	qsort ( (control->firstFile), control->fileCount, 
			sizeof (FILE_DESCRIPTION ), fileCompare);
	}
return;
}

/* ************************ readOneDirectory **************************** */
static int	readOneDirectory (DIRECTORY_CONTROL *control, 
		char *caption, int enableCancel)
{
copyThreadArgST args;
copyStatInitST initDlg;

killFileCopy = 0;
args.worker = readDirectory;
args.arg = (PVOID) control;
_beginthread (copyThread, NULL, 10*4096, (PVOID) &args);
initDlg.enableCancel = enableCancel;
initDlg.title1 = caption;
initDlg.title2 = "Current dir:";
WinDlgBox (HWND_DESKTOP, hwndClient, copyStatusProc, NULL,
		IDD_FILE_COPY, (PVOID) &initDlg);
return killFileCopy;
}

/* ********************** readTwoDirectories *************************** */
static void readTwoDirectories (void)
{
int		enableCancel = 1;

l2:

if (readOneDirectory (&srcDirControl, "Reading source", enableCancel))

l4:

	{BOOL rc;
	
	rc = WinPostMsg (hwndClient, WM_COMMAND, (MPARAM) IDM_MI_NO_SUBDIRS, 
			(MPARAM) CMDSRC_MENU);
	return;
	}
if (readOneDirectory (&destDirControl, "Reading dest.", enableCancel))
	goto l4;
buildDisplayLines ();
return;
}

/* *********************** menueItemForceState *********************** */
static void menueItemForceState (int ID, int *state, int value)
{
*state = value;
WinSendMsg (hwndActionBar, MM_SETITEMATTR, MPFROM2SHORT( ID, TRUE),
				MPFROM2SHORT (MIA_CHECKED, value? MIA_CHECKED: 0) );

}

/* ************************* fillBuffer ******************************* */
int		fillBuffer (FILE *fd, char *buff, char **currentPosition,
			int bufferSize, int charsInBuff)
{
int			bytesRead,
			i,
			unusedChars;
char		*pch1, *pch2;

/* Move unprocessed bytes to the start of the buffer */
unusedChars = charsInBuff - (*currentPosition - buff);
/* We have a trap here.  The last file could have a large set of extended
   attributes or a long file comment which was not read into the buffer.
   This will show up as a negative value for unusedChars.  In this case,
   just seek a negative unusedChars.  (This is valid since neither extended 
   attributes nor file comments are processed by this program).  Then read 
   the file into the start of the buffer. 
*/
if (unusedChars >=0)
    {for (i=0, pch1=buff, pch2=*currentPosition;	i<unusedChars;	++i, ++pch1, ++pch2)
     *pch1 = *pch2;
    }
else
    {fseek (fd, -unusedChars, SEEK_CUR);
    unusedChars = 0;
    pch1 = buff;
    }
/* read an integral number of sectors */
bytesRead = (bufferSize-unusedChars)/512;
bytesRead *= 512;
bytesRead = fread ( (void *) pch1, 1, bytesRead, fd);
*currentPosition = buff;
return bytesRead + unusedChars;
}

static char *signature = "PK\005\006";


/* Determine whether a file has a valid zip central directory.
   If so, reads the central directory into the first byte of the
   input buffer and returns 0.
   Else, frees the input buffer and returns 0;
   zipfd will remain open on success and will be closed on failure.
   The DIRECTORY_CONTROL structure is always initialized.
*/
/* *********************** verifyZipArchive ***************************** */
int verifyZipArchive (DIRECTORY_CONTROL *control, FILE **zipfd, char **bf, 
		int readSize)
{
int			bytesInBuffer,
			centralDirectoryOffset,
			fileByteOffset,
			fileSize,
			i,
			residualBytes,
			zipHandle;
char		*buff,
			*pch1,
			*pch2;
struct stat	statBuff;

buff = (char *)malloc (readSize + ZIPFILE_CENTRAL_DIRECTORY_SIZE);
*bf = buff;
*zipfd = fopen (control->directoryName, "rb");
control->fileCount = 0;
control->fileNameText->reInitialize ();
control->fileControlHeap->reInitialize ();
if ( ! *zipfd)
	return 1;
zipHandle = fileno (*zipfd);
fstat (zipHandle, &statBuff);
fileSize = statBuff.st_size;
readSize = ZIPFILE_READ_SIZE;
residualBytes = 0;
fileByteOffset = fileSize - readSize;

l2:

if (fileByteOffset < 0)
	{readSize += fileByteOffset;
	fileByteOffset = 0;
	}

/* Move any upprocessed bytes from the start of the buffer to the end */
for (i=0, pch1=buff+2, pch2=buff+readSize;	i<residualBytes;	++i, --pch1, --pch2)
	*pch2 = *pch1;
bytesInBuffer = readSize + residualBytes;
fseek (*zipfd, fileByteOffset, SEEK_SET);
fread ( (void *) buff, 1, readSize, *zipfd);
for (i=bytesInBuffer-3, pch1=buff+i-1;	i;	--i, --pch1)
	{if ( !strncmp (pch1, signature, 4))
		goto l4;
	}
residualBytes = 3;
if ( !fileByteOffset)
	{WinMessageBox (HWND_DESKTOP, hwndClient, "No central directory found",
			" ", 0, MB_OK);
	free ( (void *)buff);
	fclose (*zipfd);
	return 1;
	}
fileByteOffset -= readSize;
goto l2;

l4:

fileByteOffset += (pch1-buff);
fseek (*zipfd, fileByteOffset, SEEK_SET);
fread ( (void *) buff, 1, readSize, *zipfd);
return 0;
}

/* ************************ processCentralDirectory *************************** */
static int processCentralDirectory (DIRECTORY_CONTROL *control,
		 FILE *zipfd, char *buff, int readSize)
{
char		*pch1,
			*pch2;
FILE_DESCRIPTION		*pdesc;
// These variables are read from the central directory
short		fileCount;
ULONG		firstFileDirectoryDisplacement;
int			nextDirDisplacement;
int			cDirRecordSize;
int			bytesInBuffer,
			i;
int			extendedAttributesLength,
			fileCommentLength;

pch1 = buff;

l4:

control->fileCount = 0;
control->fileNameText->reInitialize ();
control->fileControlHeap->reInitialize ();
fileCount = *((short *) (pch1+10));
firstFileDirectoryDisplacement = *((ULONG *) (pch1+16));
fseek (zipfd, firstFileDirectoryDisplacement, SEEK_SET);
bytesInBuffer = fread ( (void *) buff, 1, readSize, zipfd);
pch1 = buff;

/* Note 1:
	The following loop will not work if the central directory
	will not fit into a single read block
   Note 2:
	the numeric constants in the following loop were determined
	by inspecting the infozip code.
*/
for (i=fileCount;	fileCount;	--fileCount)
	{
	if (pch1+ZIPFILE_CENTRAL_DIRECTORY_SIZE-buff > bytesInBuffer)
		bytesInBuffer = fillBuffer (zipfd, buff, &pch1, readSize, bytesInBuffer);
	pdesc = (FILE_DESCRIPTION *)control->fileControlHeap->requestMem (sizeof (FILE_DESCRIPTION) );
	pdesc->nameStringLength = *((USHORT *)(pch1+28));
	extendedAttributesLength = *(USHORT *)(pch1+30);
	fileCommentLength = *(USHORT *) (pch1 + 32);
	nextDirDisplacement = 46 + pdesc->nameStringLength + 
		extendedAttributesLength + fileCommentLength;
	if (pch1+nextDirDisplacement-buff > bytesInBuffer)
		bytesInBuffer = fillBuffer (zipfd, buff, &pch1, readSize, bytesInBuffer);
	pdesc->name = (char *)control->fileNameText->requestMem (
				pdesc->nameStringLength+1);
	strncpy (pdesc->name, pch1+46, pdesc->nameStringLength);
	*(pdesc->name+pdesc->nameStringLength) = 0;
	pdesc->attributes = *((USHORT *)(pch1+36));
	pdesc->date = *((FDATE *) (pch1+14));
	pdesc->time = *((FTIME *) (pch1+12));
	pdesc->size = *((ULONG *) (pch1+24));
	++(control->fileCount);
	pch1 += nextDirDisplacement;
	}

if (control->fileCount)
	{FILE_DESCRIPTION	*plist;
	
	plist = (FILE_DESCRIPTION *) control->fileControlHeap->getBaseAddress ();
	control->firstFile = plist;
	qsort ( (control->firstFile), control->fileCount, 
			sizeof (FILE_DESCRIPTION ), fileCompare);
	}
fclose (zipfd);
free ( (void *)buff);
return 0;
}

/* ************************ readZipDirectory *************************** */
static int readZipDirectory (DIRECTORY_CONTROL *control)
{FILE		*zipfd;
char		*buff;
int			readSize;

readSize = ZIPFILE_READ_SIZE;
if ( verifyZipArchive (control, &zipfd, &buff, readSize) )
	{killFileCopy = 1;
	return 1;
	}
processCentralDirectory (control, zipfd, buff, readSize);
return 0;
}

/* ***************************** setHScrollBar ************************ */
static void setHScrollBar (void)
{
maxDisplayLineLength =maxFileNameLength + 1;
if (showFileDate)
	maxDisplayLineLength += 15;
if (showFileSize )
	maxDisplayLineLength += 8;
pWin->newDisplayWidth (maxDisplayLineLength);
pWin->setHScrollBar ();
return;
}


/* ************************* setModeMenuItem ************************** */
static void setModeMenuItem (int id, int *pi)
{
short		set;


for (	;	*pi;	++pi)
	{if (id == *pi)
		set = MIA_CHECKED;
	else
		set = 0;
	WinSendMsg (hwndActionBar, MM_SETITEMATTR, MPFROM2SHORT( *pi, TRUE),
			MPFROM2SHORT (MIA_CHECKED, set));
	}
return;
}

/* ***************************** setUpdateMode ************************ */
static int setUpdateMode (ENArchiveType srcType, ENArchiveType destType)
{
int				modeChangeStarted;
ENArchiveType	oldSrc,
				oldDest;

if (srcDirControl.archiveType != srcType)
	{oldSrc = srcDirControl.archiveType;
	srcDirControl.archiveType = srcType;
	if (getDirectory (hwndClient, &srcDirControl))
		{srcDirControl.archiveType = oldSrc;
		return 1;
		}
	totalDisplayLines = 1;
	modeChangeStarted = 1;
//	readOneDirectory (&srcDirControl, "Src dir", 0);
	}			//Source type has changed;
if (destDirControl.archiveType != destType)
	{oldDest = destDirControl.archiveType;
	destDirControl.archiveType = destType;
	
	l2:
	
	if (getDirectory (hwndClient, &destDirControl))
		{if (modeChangeStarted)
			{mustComplete(hwndClient);
			goto l2;
			}
		else
			return 1;
		}
//	readOneDirectory (&destDirControl, "Dest dir", 0);
	}
return 0;
}

/* **************************** setUpFiles **************************** */
static void		setUpFiles (HWND hwnd)
{

conditionalProcessArchive (hwnd, &srcDirControl, "Source dir");
conditionalProcessArchive (hwnd, &destDirControl, "Destination dir");
buildDisplayLines ();
return;
}

/* ************************** toggleMenuItem ************************ */
static void toggleMenuItem (int ID, int * state)
{
*state = *state ^ 1;
WinSendMsg (hwndActionBar, MM_SETITEMATTR, MPFROM2SHORT( ID, TRUE),
				MPFROM2SHORT (MIA_CHECKED, *state? MIA_CHECKED: 0) );

}

/* **************************** updateZipFile *************************** */
static void updateZipFile (PVOID)
{
STARTDATA	sd;
ULONG		idSession;
PID			pid;
char *		waitQueue = "\\QUEUES\\WaitQueue";
char		objBuff[260];
char 		optionBuff[300];
HQUEUE		hqueue;
APIRET		rc;
REQUESTDATA reqData;
ULONG		removeCount;
PVOID		removeData;
BYTE		priority;
char		fileNames[L_tmpnam];
char		*pch1;
HFILE		fh1, fh2;
int			i, j;
DISPLAY_LINE	*pDL;
ULONG		action,
			written;

pch1 = srcDirControl.directoryName;
strcpy (objBuff, pch1);
if ( (i=strlen (pch1)) >3)
	*(objBuff+i-1) = 0;
DosSetDefaultDisk ( toupper (*pch1)-'A'+1);
i = chdir (objBuff);
if ( !tmpnam (fileNames))
	return;

DosOpen (fileNames, &fh1, &action, 0, 0, OPEN_ACTION_CREATE_IF_NEW,
		OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE, NULL);
fh2 = 0;
DosDupHandle (fh1, &fh2);
for (i=0, pDL=displayLineControl;	i<totalDisplayLines;	++i, ++pDL)
	{char		*pName;
	
	if ( !pDL->source)
		continue;
	if (!pWin->querySelect (i))
		continue;
	pName = pDL->source->name;
	if ( !pName)
		continue;
	DosWrite (fh2, pName, strlen (pName), &written);
	DosWrite (fh2, "\n", 1, &written);
	}
lseek (fh2, 0, SEEK_SET);
rc = DosCreateQueue (&hqueue, QUE_FIFO, waitQueue);
if (rc)
	{return;
	}
memset ( (void *) &sd, 0, sizeof (sd) );
sd.Length = sizeof (sd);
sd.Related = SSF_RELATED_CHILD;
sd.FgBg = SSF_FGBG_FORE;
sd.PgmName = "zip.exe";
strcpy (optionBuff, " -@ -o ");
strcat (optionBuff, destDirControl.directoryName);
sd.PgmInputs = optionBuff;
sd.TermQ = waitQueue;
sd.InheritOpt = SSF_INHERTOPT_PARENT;
sd.SessionType = SSF_TYPE_WINDOWABLEVIO;
sd.PgmControl = SSF_CONTROL_MAXIMIZE;
sd.ObjectBuffer = objBuff;
sd.ObjectBuffLen = 260;
rc = DosStartSession (&sd, &idSession, &pid);
// DosStartSession always seems to return an error, so ignore it
// if ( 1)
	{DosReadQueue (hqueue, &reqData, &removeCount, &removeData, 0, 
			DCWW_WAIT, &priority, 0);
	DosFreeMem ( (VOID *) &reqData);
	}
rc = DosCloseQueue (hqueue);
DosClose (fh1);
DosClose (fh2);
DosDelete (fileNames);
return;
}
