/*  Copyright (C) 1995  Robert A. O'Brien, Jr

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#define INCL_PM
#define INCL_DOSRESOURCES
#define INCL_WINMESSAGEMGR
#define INCL_WINDIALOGS
#define INCL_WINWINDOWMGR
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "resToWnd.hpp"

char * limitErrorString = "Limit Error";
static char *rangeErrorMessage = "Range error";
static char *illegalCharMessage = "Illegal character in field";
static char	*noButtonSelectedMessage = "No radio button is selected";
static char *noItemSelected = "No item is selecter";

/* ***************************** baseEntry ***************************** */
void baseEntry::toScreen (HWND handle, char * pch, WINDOW_OUTPUT_METHOD method)

{POINTL		point;
HPS			hps;
WNDPARAMS	params;

switch (method)
	{
case EN_OMSend:
	WinSetWindowText (handle, (PSZ)pch);
	return;
	break;
case EN_OMPost:
	params.fsStatus = WPM_TEXT;
	params.cchText = strlen (pch)+1;
	params.pszText = (PSZ)pch;
	params.cbPresParams = 0;
	params.cbCtlData = 0;
	WinPostMsg (handle, WM_SETWINDOWPARAMS, (MPARAM) &params, NULL);
	break;
case EN_OMGPI:
	point.x = 0;
	point.y = 0;
	hps = WinGetClipPS (handle, 0, PSF_CLIPSIBLINGS);
	GpiErase (hps);
	GpiCharStringAt (hps, &point, strlen (pch), (PCH) pch);
	break;
	}
return;
}

/* ************************* baseIOWindow ***************************** */
void baseIOWindow::setInterface (HWND hwndDialog, int ID)
{
id = ID;
handle = WinWindowFromID (hwndDialog, ID);
}

/* ********************* numericEntryWindow ************************* */
template<class TYPE> char * numericEntryWindow<TYPE>::read()
{char		work[30];

fromScreen(work);
sscanf (work, convertSpec, value);
return (char *) NULL;
}

template<class TYPE> void numericEntryWindow<TYPE>::write (WINDOW_OUTPUT_METHOD method)
{char		work[30];

sprintf (work, convertSpec, value);
toScreen(work, method);
return;
}

/* *************************** validatedEntryWindow ************************ */
template<class TYPE> char * validatedEntryWindow<TYPE>::read()
{char		work[30];
TYPE		temp;

fromScreen(work);
sscanf (work, convertSpec, &temp);
if (temp < min)
	return limitErrorString;
if (temp > max)
	return limitErrorString;
*value = temp;
return (char *) NULL;
}

template<class TYPE> void validatedEntryWindow<TYPE>::write(WINDOW_OUTPUT_METHOD method)
{char		work[30];

sprintf (work, convertSpec, value);
toScreen(work, method);
return;
}

/* ****************************** radioButtonWindow *********************** */
char * radioButtonWindow::read()
{int		i;

i = (int)WinSendMsg (handle, BM_QUERYCHECKINDEX, NULL, NULL);
value = *(identList+i);
return NULL;
}

void radioButtonWindow::write(WINDOW_OUTPUT_METHOD method)
{HWND	hwn;
MRESULT	result;
BOOL	rc;

hwn = WinWindowFromID (parent, value);
if (method == EN_OMSend)
	result = WinSendMsg (hwn, BM_SETCHECK, (MPARAM) TRUE, NULL);
else
	rc = WinPostMsg (hwn, BM_SETCHECK, (MPARAM) TRUE, NULL);
return;
}

void radioButtonWindow::write (WINDOW_OUTPUT_METHOD method, int val)
{
value = val;
write (method);
return;
}

void radioButtonWindow::createIdentArray (HWND hwndDialog)
{
HWND	hwndWork;
int		*idents = NULL;
int		*pi;
int		identsRemaining = 0;
int		identsUsed = 0;
int		currentIdent,
		i;
char	chWork[30];
LONG	retLen;

parent = hwndDialog;
hwndWork = WinEnumDlgItem (hwndDialog, handle, EDI_FIRSTGROUPITEM);
while (1)
	{
	if ( !identsRemaining)
		{idents = (int *)realloc ((void *) idents, 
		  (identsUsed+20)*sizeof (int *));
		identsRemaining = 20;
		}
	retLen = WinQueryClassName (hwndWork, sizeof(chWork), chWork);
	if (strcmp ("#3", chWork) )
		goto l2;
	currentIdent = WinQueryWindowUShort (hwndWork, QWS_ID);
/* the enumerate dialog item call wraps to the beginning of the
	group after the last item has been returned.  The only way out
	of this loop is to see if the current ident is already in the array */
	for (i=0, pi=idents;	i<identsUsed;	++pi, ++i)
		{
		if (*pi == currentIdent)
			goto l4;
		}
	*(idents+identsUsed) = currentIdent;
	++identsUsed;
	--identsRemaining;
	
l2:
	
	hwndWork = WinEnumDlgItem (hwndDialog, hwndWork, EDI_NEXTGROUPITEM);
	}

l4:

identList = (int*) realloc (idents, identsUsed*sizeof(int *));
return;
}

/* *************************** checkBoxWindow **************************** */
char * checkBoxWindow::read ()
{
value = (int)WinSendMsg (handle, BM_QUERYCHECK, NULL, NULL);
return NULL;
}

void	checkBoxWindow::write (WINDOW_OUTPUT_METHOD method)
{
if (method == EN_OMSend)
	WinSendMsg (handle, BM_SETCHECK, (MPARAM) value, NULL);
else
	WinPostMsg (handle, BM_SETCHECK, (MPARAM) value, NULL);
return;
}

void	checkBoxWindow::write (WINDOW_OUTPUT_METHOD method, int val)
{
value = val;
write (method);
}

/* ***************************** listBoxTextWindow *********************** */
char * listBoxTextWindow :: read (void)
{int		i;

i = (INT)WinSendMsg (handle, LM_QUERYSELECTION, (MPARAM) LIT_FIRST, NULL);
if (i == LIT_NONE)
	return "No item selected";
WinSendMsg (handle, LM_QUERYITEMTEXT, MPFROM2SHORT (i, 100), (MPARAM) value);
return NULL;
}

/* ****************************** listBoxIndexWindow ********************** */
char * listBoxIndexWindow :: read (void)
{int		i;

i = (INT)WinSendMsg (handle, LM_QUERYSELECTION, (MPARAM) LIT_FIRST, NULL);
if (i == LIT_NONE)
	return "No item selected";
value = i;
return NULL;
}

/* ************************** resourceLoop ************************** */
/* A class to control interation through each dialog in a DLGTEMPLATE
   structure.
*/
class resourceLoop
	{
	char *		offChar;			// character pointer to the DLGTEMPLATE
									// It is a char * so that the methods of
									// the class will not have to perform 
									// repeated casts.
public:
	int			maxItemOffset;		// Stop processing when the offset of a
									// dialog item exceeds this value
	DLGTITEM *	pdlgItem;
	char *		pcontrol;
	char *		ppres;
	char *		ptext;
	
	void		init (DLGTEMPLATE * ptemplate);
	int			setIter();
	};

/* Initialize data members for subsezuent iteration */
void resourceLoop::init(DLGTEMPLATE * ptemplate)
{
offChar = (char *) ptemplate;
maxItemOffset = ptemplate->cbTemplate;
pdlgItem = ptemplate->adlgti;
}

/* Set data members for each iteration.  NOTE:
 1.	The header of the DLGTEMPLATE does not seem to have enough information
 	to calculate maxItemoffset.  I have observed that the class names, control
	data and text are all stored after the array of DLGITEMs.  I therefore
	compare maxItemOffset to the offsets of these data and adjust it as
	required.
*/
int resourceLoop::setIter()		// returns 0 if the pdlgItem points past
								// the last control in the DLGTEMPLATE, else 1

{
	int			i;
if (pdlgItem->cchClassName && (pdlgItem->offClassName<maxItemOffset))
	maxItemOffset = pdlgItem->offClassName;
if (pdlgItem->cchText && (pdlgItem->offText<maxItemOffset))
	maxItemOffset = pdlgItem->offText;
if (maxItemOffset > pdlgItem->offPresParams)
	maxItemOffset = pdlgItem->offPresParams;
	
if(maxItemOffset > pdlgItem->offCtlData)
	maxItemOffset = pdlgItem->offCtlData;
if(pdlgItem->offCtlData == 0xffff)
	pcontrol = NULL;
else
	pcontrol = offChar + pdlgItem->offCtlData;

if(pdlgItem->offPresParams == 0xffff)
	ppres = NULL;
else
	ppres = offChar + pdlgItem->offPresParams;

if ( (pdlgItem->cchText== 0) || (pdlgItem->offText==0xffff) )
	ptext = NULL;
else
	{if (maxItemOffset > pdlgItem->offText)
		maxItemOffset = pdlgItem->offText;
	ptext = offChar + pdlgItem->offText;
	}
i =  (pdlgItem < (DLGTITEM *) (offChar+maxItemOffset))? 1: 0;
return i;

}

/* ********************** controlsToBaseIOWindow ************************* */
/*  Accept a null terminated list of baseIOWindow * and a resource.  For each 
	baseIOWindow *, which is found in the resource initialize the window 
	handle and class members.  

 	Radio buttons are a special case since input and output are not symetric
 	at the system level.  i.e. when you test which button in a group is set
	you get the zero-based index of the selected button, but in order to set
	a button you must provide either a window handle or a control ID.  I
	solve this problem by setting up an array of control ID's for each group
	of radio buttons.  This array is indexed by the offset of the button in
	its group as returned by the inquiry as to which button is set.
	
	This enables read and write for radio buttons to be symetric (i.e. once
	you have read a value, if you use that same value to write to the button
	group, the selected button sill not change).  The user can assure that
	the IDs of controls do not change.  However, the value of the name will
	in general be defined by a resource editor and is subject to change.
	Therefore, the values used for reading and writing must not be stored
	in an archive such as a disk file or a profile since editing the resource
	and recompilation may change the value of the ID.  Therefore I have
	provided data types for managing conversion between a control ID and
	either a string or an integer.
*/
int controlsToBaseIOWindow (	//	returns the number of baseIOWindow *s 
								//	which were not found in the resource
								//	It should be zero
	HWND 			parent, 
	HMODULE			modHandle,
	int				resourceID,
	baseIOWindow 	**base,
	int				winCount)

{int			count,
				groupAllocated,
				*groupData,
				groupUsed,
				i,
				id,
				notFound,
				offset,
				style,
				wClass;
baseIOWindow	*win;
radioButtonWindow *	groupButton;
HWND			hwnd;
char			className[51],
				*pch;
resourceLoop	rl;
APIRET			rc;

qsort (base, winCount, sizeof (baseIOWindow *), compareWindowInfo);
notFound = winCount;
groupData = NULL;
rc = DosGetResource (modHandle, RT_DIALOG, resourceID, (PPVOID)&offset);
if (rc)
	return notFound;
groupButton = NULL;
for (rl.init( (DLGTEMPLATE *)offset);	rl.setIter();	++rl.pdlgItem)
	{id = rl.pdlgItem->id;
	hwnd = WinWindowFromID (parent, id);
	if (hwnd == 0)
		continue;
	//The format of class names is defined in the toolkit on-line docs
	WinQueryClassName (hwnd, 50, (PCH)className);
	if(className[0]== '#')
		{for (i=0, pch=className+1;	*pch;	++pch)
			{i *= 10;
			i += *pch-'0';
			}
		wClass = i + 0xffff0000;
		}
	else
		wClass = (int) className;
	style = WinQueryWindowULong (hwnd, QWL_STYLE);
	if (style & WS_GROUP)
		{if ((groupData!=NULL) && (groupButton!=NULL) )
			/* The preceding group was a radio button group.  Trim the
			   group data array and store the address of the array in the 
			   structure of the first button of the group. */
			{if (groupButton->identList)
				free (groupButton->identList);
			groupButton->identList = (int *) realloc (
				(void *) groupData, groupUsed*sizeof (int));
			groupData = NULL;
			groupButton = NULL;
			}
		if (groupData != NULL)
			free ((void *) groupData);
		groupData = (int *) malloc (20*sizeof(int));
		groupAllocated = 20;
		groupUsed = 0;
		}								/* Start of group */
	if (groupData)
		/*	We are processing a radio button group.  Store the control ID in
			in the group data array  */
		{if (groupUsed >= groupAllocated)
			{groupAllocated += 20;
			groupData = (int *) realloc ((void *)groupData,
					groupAllocated*sizeof(int));
			}
		*(groupData+groupUsed++) = id;
		}
	win = idToBaseIO (base, winCount, id);
	if (win == NULL)
		continue;
	win->handle = hwnd;
	win->wClass = wClass;
	if (win->wClass == (int)WC_BUTTON)
		{if ( ((style&0x7)!=BS_AUTORADIOBUTTON) && ((style&0x7)!=BS_RADIOBUTTON))
			continue;
		if (groupButton == NULL)
			{groupButton = (radioButtonWindow *)win;
			groupButton->parent = parent;
			}
		}
	}				/* for each id in resource */
if (groupData)
	{if (groupButton)
		groupButton->identList = (int *) realloc (groupData,
			groupUsed * sizeof (int));
	else
		free (groupData);
	}
return 0;
}

/* ************************* compareWindowInfo ****************************/
int _Optlink compareWindowInfo (const void *p1, const void *p2)
{
return (*((baseIOWindow **)p1))->id - (*((baseIOWindow **)p2))->id;
}

/* ************************* resourceToWindow ************************** */

/* This procedure somewhat duplicates the functionality of the WinCreateDlg.
   In addition to that functionality it allows the caller to specify a table 
   which defines class redifinitions.  If the class of a control in the
   dialog template matches a class in the translation table the new class
   is substituted for the old and the control is created.  This procedure
   was originally created so that dialogs generated by the Guidelines tool
   could be used directly.  Guidelines seems to register its own classes
   and the layout tool uses them.  A more direct approach is to run the .rc
   file generated by the tool through a sed(1) script which changes the
   Guidelines specific classes to predefined classes.  
   
   This approach might eliminate the need for the following procedure 
   altogether except that a possible enhancement would be to add two members
   to the baseIOClass then accept a baseIOClass ** as an input, then 
   use the group information in the DLGTEMPLATE to generate fields which
   support the creation of logic in the frame window procedure which allow
   a user to navigate the controls with tab and cursor keys just as in
   a dialog.
*/
int			resourceToWindow (HWND						hwndParent,
						HWND							hwndOwner,
						HMODULE							modHandle,
						int								resourceID,
						CLASS_CONVERSION_DEFINITION		*convert,
						int								conversionItemCount,
						HWND							hwndFrame,
						int								frameID
						)

{APIRET			rc;
PVOID			offset;
DLGTITEM		*pframeDlgItem;
HWND			hwnd;
int				aveHeight,
				aveWidth,
				convertCount,
				i;
int				notFound;
char			*pclass;
CLASS_CONVERSION_DEFINITION		*pconvert;
RECTL			rectl;
resourceLoop	rl;

	{FONTMETRICS		fm;
	HPS					hps;
	
	hps = WinBeginPaint (hwndOwner, 0L, NULL);
	GpiQueryFontMetrics (hps, sizeof (fm), &fm);
	aveWidth = fm.lAveCharWidth;
	aveHeight = fm.lMaxBaselineExt;
	WinEndPaint (hps);
	}
rc = DosGetResource (modHandle, RT_DIALOG, resourceID, &offset);
if (rc)
	return notFound;
for (rl.init( (DLGTEMPLATE *)offset);	rl.setIter();	++rl.pdlgItem)
	
	{char		*pWindowClass;
	
	/* If cchClassName == 0, then the offset to the class name is the
	   low 16 bits of some predefined class */
	pWindowClass = (char *) rl.pdlgItem->offClassName;
	if (!rl.pdlgItem->cchClassName)
		pWindowClass += 0xffff0000;
	else			// Not a predefined window class
		pWindowClass += (int)offset;
	if (rl.pdlgItem->id == frameID)
		{pframeDlgItem = rl.pdlgItem;
		rectl.xLeft = (aveWidth*pframeDlgItem->x)/4;
		rectl.yBottom = (aveHeight*pframeDlgItem->y)/8;
		rectl.xRight = (aveWidth*pframeDlgItem->cx)/4 + rectl.xLeft;
		rectl.yTop = (aveHeight*pframeDlgItem->cy)/8 + rectl.yBottom;
		WinCalcFrameRect (hwndFrame, &rectl, FALSE);
		WinSetWindowPos (hwndFrame, HWND_BOTTOM, rectl.xLeft, rectl.yBottom, 
			rectl.xRight-rectl.xLeft, rectl.yTop-rectl.yBottom,
			SWP_SIZE | SWP_MOVE | SWP_SHOW);
		continue;
		}
	for (convertCount=0, pconvert=convert;	convertCount<conversionItemCount;
			++convertCount, ++pconvert)
		{if (rl.pdlgItem->cchClassName != pconvert->srcCount)
			continue;
		if (!rl.pdlgItem->cchClassName)
			{if ( (ULONG)pWindowClass != ((ULONG) pconvert->srcClass))
				continue;
			}
		else								/* not a predefined control */
			{pclass = (char *) offset + rl.pdlgItem->offClassName;
			if(strcmp(pWindowClass, (char *)pconvert->srcClass))
				continue;
			}
		pWindowClass = (char *)pconvert->destClass;
		break;
		}	//for (each item in conversion table)
	hwnd = WinCreateWindow (hwndParent, (PSZ)pWindowClass, 
		(PSZ)rl.ptext, rl.pdlgItem->flStyle, 
		(aveWidth*rl.pdlgItem->x)/4, (aveHeight*rl.pdlgItem->y)/8, 
		(aveWidth*rl.pdlgItem->cx)/4, (aveHeight*rl.pdlgItem->cy)/8, hwndOwner, 
		HWND_TOP, rl.pdlgItem->id, rl.pcontrol, rl.ppres);
	}		// for (each control in dialog)
#if 0
rectl.xLeft = (aveWidth*pframeDlgItem->x)/4;
rectl.yBottom = (aveHeight*pframeDlgItem->y)/8;
rectl.xRight = (aveWidth*pframeDlgItem->cx)/4 + rectl.xLeft;
rectl.yTop = (aveHeight*pframeDlgItem->cy)/8 + rectl.yBottom;
WinCalcFrameRect (hwndFrame, &rectl, FALSE);
WinSetWindowPos (hwndFrame, HWND_BOTTOM, rectl.xLeft, rectl.yBottom, 
	rectl.xRight-rectl.xLeft, rectl.yTop-rectl.yBottom,
	SWP_SIZE | SWP_MOVE | SWP_SHOW | SWP_NOADJUST);
#endif
DosFreeResource (offset);
return notFound;
}

/* ****************************** idToBaseIO *************************** */
baseIOWindow * idToBaseIO (baseIOWindow **base, int count, int id)

{baseIOWindow 		key,
					*pkey;
baseIOWindow		**found;

pkey = &key;
key.id = id;
found = (baseIOWindow **)bsearch (&pkey, base, count, sizeof (baseIOWindow *),
		compareWindowInfo);
if (!found)
	return (baseIOWindow *)found;
return *found;
}

/* ******************************rbIDToInt ******************************* */
void rbIDToInt (radioButtonWindow *rb, RB_ID_TO_INT *table, int * where)

{

for (	;	table->id;	++table)
	if (table->id == rb->value)
		goto l2;
return;

l2:

*where = table->value;
return;
}

/* ********************************* rbIDToString ************************ */
void rbIDToString (radioButtonWindow *rb, RB_ID_TO_TEXT *table, char * where)
{
for (	;	table->id;	++table)
	if (table->id == rb->value)
		goto l2;
return;

l2:

strcpy (where, table->value);
return;
}

/* ******************************** rbIDFromInt ***************************** */
void rbIDFromInt (radioButtonWindow *rb, RB_ID_TO_INT *table, int * where)

{for (	;	table->id;	++table)
	if (table->value == *where)
		goto  l2;
return;

l2:

rb->value = table->id;
return;
}

/* ******************************** rbIDFromString ************************* */
void rbIDFromString (radioButtonWindow *rb, RB_ID_TO_TEXT *table, char * where)

{for (	;	table->id;	++table)
	if (!strcmp (table->value, where))
		goto  l2;
return;

l2:

rb->value = table->id;
return;
}

