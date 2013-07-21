#define INCL_WINMESSAGEMGR
#define INCL_WIN
#define INCL_WINDIALOGS
#define INCL_GPILCIDS
#define INCL_GPIPRIMITIVES

#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "splitWindowOB.hpp"



/* ************************ assignSelectedField *************************** */
void splitWindowOB::assignSelectedField (int first, int last, int sel)
{char *				psel;
int					i,
					test;
RECTL				rect;

if (first < 0)
	first = 0;
if (first > last)
	{i = first;
	first = last;
	last = i;
	}
if (last >= totalDisplayLines)
	last = totalDisplayLines - 1;
for (i=first, psel=lineSelection+first;	i<=last;	++i, ++psel)
	*psel = sel;
if (first < firstFileLineOnScreen)
	first = firstFileLineOnScreen;
test = firstFileLineOnScreen + totalDisplayLines - 2;
if (last > test)
	last = test;
rect.xLeft = 0;
rect.xRight = clientWidth;
rect.yTop = clientTopPixel - CharHeight*(first-firstFileLineOnScreen+1);
rect.yBottom = rect.yTop - CharHeight*(last+first+1);
WinInvalidateRect (hwndClient, &rect, FALSE);
WinSendMsg (hwndClient, WM_PAINT, NULL, NULL);
return;
}

/* *************************** buttonClick ************************ */
int splitWindowOB::buttonClick (MPARAM mp1, MPARAM mp2)
{int			i,
				line;
RECTL			rectl;
char			*psel;
		
line = lineFromYOrd ( SHORT2FROMMP(mp1));
if (line < numNonScrollingLines)
	return line;
i = firstFileLineOnScreen + line - numNonScrollingLines;
mostRecentSelectedLine = i;
psel = lineSelection + i;
rectl.xLeft = 0;
rectl.xRight = clientWidth;
rectl.yBottom = clientTopPixel - (line+1)*CharHeight;
rectl.yTop = rectl.yBottom + CharHeight-1;
*psel = *psel ^ 1;
WinInvalidateRect (hwndClient, &rectl, FALSE);
return line + firstFileLineOnScreen;
}

/* *************************** HScrollMsg ***************************** */
void splitWindowOB::HScrollMsg (MPARAM mp1, MPARAM mp2)
{

switch (SHORT2FROMMP (mp2))
	{
		
case SB_LINELEFT:
	scrollScreenHoriz (-1);
	break;
case SB_LINERIGHT:
	scrollScreenHoriz (1);
	break;
case SB_PAGELEFT:
	scrollScreenHoriz (-charsInPane+5);
	break;
case SB_PAGERIGHT:
	scrollScreenHoriz (charsInPane-5);
	break;
case SB_SLIDERTRACK:
	scrollScreenHoriz (SHORT1FROMMP (mp2) - firstDisplayChar);
	break;
case SB_SLIDERPOSITION:
	break;
	}
return;
}
/* *************************** invalidLines **************************** */
void splitWindowOB::invalidLines (PRECTL prectl, int *first, int *last)
{int		diff;

*first = (clientTopPixel - prectl->yTop)/CharHeight;
diff = clientTopPixel - prectl->yBottom;
*last = diff/CharHeight;
if (diff > *last*CharHeight)
	++*last;
if (*last >= linesInWindow)
	*last = linesInWindow - 1;
return;
}

/* ***************************** lineFromYOrd ***************************** */
int splitWindowOB::lineFromYOrd (int y)
{int		i;

i =  (clientTopPixel - 3- y)/CharHeight;		// I have no idea why I need
												// to subtract 3
if (i < 0)
	i=0;
else if (i >= linesInWindow)
	i = linesInWindow - 1;
return i;
}

/* ***************************** newDisplayList ************************ */
void splitWindowOB::newDisplayList (int lines, int maxLineLen)
{int		i;
char *		pch;

if (lineSelection)
	free (lineSelection);
mostRecentSelectedLine = 0;
lineSelection = (char *) malloc (lines);
memset (lineSelection, 0, lines);
maxDisplayLineLength = maxLineLen;
totalDisplayLines = lines;
firstFileLineOnScreen = 0;
firstDisplayChar = 0;
setVScrollBar ();
setHScrollBar ();
return;
}

/* ****************************** newDisplayWidth *********************** */
void splitWindowOB::newDisplayWidth (int maxLineLen)
{

maxDisplayLineLength = maxLineLen;
firstDisplayChar = 0;
setHScrollBar ();
return;
}

/* ***************************** querySelect ***************************** */
int splitWindowOB::querySelect (int line)
{
return *(lineSelection+line);
}

/* **************************** refreshClient **************************** */
void splitWindowOB::refreshClient (void)
{POINTL		pointl;
int			x, y;
RECTL		rect;
HPS			hps;
int			i;
panelDescriptor *	ppanel;
	
hps = WinBeginPaint (hwndClient, 0, &rect);
WinFillRect (hps, &rect, 0);
for (i=1, ppanel = panelDef+1;	i<numPanels;	++i, ++ppanel)
	{x = ppanel->panelFirstPixel;
	if ( (x>=rect.xLeft) && (x<=rect.xRight) )
		{pointl.x = x;
		pointl.y = rect.yBottom;
		GpiMove (hps, &pointl);
		pointl.y = rect.yTop;
		GpiLine (hps, &pointl);
		}
	}
showLines (hwndClient, hps, &rect);
WinEndPaint ( hps);
return;
}

/* ****************************** resize *************************** */
void splitWindowOB::resize (void)
{RECTL			rectl;
int				pixelsInPane;
int				i;
panelDescriptor *	ppanel;
int					pixel;
		
WinQueryWindowRect (hwndFrame, &FrameRect);
WinQueryWindowRect (hwndClient, &ClientRect);
i = (ClientRect.yTop - ClientRect.yBottom);
linesInWindow = i/CharHeight;
scrollingLinesInWindow = linesInWindow;
if (i != linesInWindow/CharHeight)
	++linesInWindow;
clientTopPixel = ClientRect.yTop;
pixelsInPane = (ClientRect.xRight - ClientRect.xLeft)/numPanels;
charsInPane = pixelsInPane/CharWidth;
pixel = 0;
for (i=0, ppanel=panelDef;	i<numPanels;	++i, ++ppanel)
	{ppanel->charsInPanel = charsInPane;
	ppanel->panelFirstPixel = pixel;
	pixel += pixelsInPane;
	}
clientWidth = ClientRect.xRight;
setVScrollBar ();
setHScrollBar ();
scrollScreenVert (0);		// adjust firstFileLineOnScreen
scrollScreenHoriz (0);
return;
}

/* ************************** scrollScreenHoriz ************************** */
void splitWindowOB::scrollScreenHoriz (int count)
{
if (maxHorizOffset < 0)
	{firstDisplayChar = 0;
	return;
	}
firstDisplayChar += count;
if (firstDisplayChar < 0)
	firstDisplayChar = 0;
else if (firstDisplayChar > maxHorizOffset)
	firstDisplayChar = maxHorizOffset;
WinSendMsg (hwndHScrollBar, SBM_SETSCROLLBAR, (MPARAM) firstDisplayChar,
	MPFROM2SHORT (0, maxHorizOffset) );
WinInvalidateRect (hwndClient, NULL, FALSE);
return;
}

/* *************************** scrollScreenVert ***************************** */
void splitWindowOB::scrollScreenVert (int count)
{int			firstLine,
				test;

firstLine = firstFileLineOnScreen;
test = totalDisplayLines - scrollingLinesInWindow + numNonScrollingLines;
if (test <= 0)
	{firstFileLineOnScreen = 0;
	return;
	}
if ( !firstFileLineOnScreen && count<0 )
	return;
if ( (firstFileLineOnScreen>=test) && (count>0) )
	return;
firstFileLineOnScreen = firstFileLineOnScreen + count;
if (firstFileLineOnScreen < 0)
	firstFileLineOnScreen = 0;
else if (firstFileLineOnScreen > test)
		firstFileLineOnScreen = test;
count = firstFileLineOnScreen - firstLine;
RECTL		clipRect,
			scrollRect,
			updateRect;
HPS			hps;

scrollRect.xLeft = 0;
scrollRect.yBottom = 0;
scrollRect.xRight = clientWidth;
scrollRect.yTop = clientTopPixel - CharHeight*numNonScrollingLines;
clipRect = scrollRect;
WinScrollWindow (hwndClient, 0, count*CharHeight, &scrollRect, &clipRect, 
		NULLHANDLE, &updateRect, SW_INVALIDATERGN);
WinSendMsg (hwndVScrollBar, SBM_SETSCROLLBAR, (MPARAM) firstFileLineOnScreen,
	MPFROM2SHORT (0, test) );
return;
}

/* ************************** selectionTimerProc ************************** */
void splitWindowOB::selectionTimerProc (int init, int action)
/* Process the ticks of the timer which is started when the user
   starts to select a group of files with the cursor.  init is non-zero
   when the timer is first started.  action  is 1 if the user wants to 
   set the selection or 0 if the selection is to clear the selected field
   for each line selected.
   The strategy depends on the relationship between the line selected
   when the procedure is initialized, the line the cursor was on at
   the previus call, and the line the currently on.  The following 
   diagram shows the important cases:

       Case 1                   Case 2              Case 3

       ------  current      ------  previous


       ------- previous     ------  current         ------  previous



   ================================================================ firstLine


                                                     ------ current

   In Case 1 simply perform the requested action from the previous
   line to the current.
   In Case 2 invert the requested action from previous up to but
   not including current.
   In Case 3 invert from previous up to not including firstLine, then 
   perform the requested act from firstLine up to including current.
   
   
*/
{static int			firstLine,
					previousLine;

int					absPreviousOffset,
					cAction,				// The complement of action
					line,
					normalizedCurrentOffset,
					sign;
SWP					swp;
POINTL				pos;
RECTL				frameRectWorld;

WinQueryPointerPos (HWND_DESKTOP, &pos);
WinQueryWindowPos (hwndFrame, &swp);
frameRectWorld.xLeft = swp.x;
frameRectWorld.yBottom = swp.y;
frameRectWorld.xRight = swp.x + swp.cx;
frameRectWorld.yTop = swp.y + swp.cy;
WinQueryWindowPos (hwndClient, &swp);
line = lineFromYOrd (pos.y - frameRectWorld.yBottom - swp.y);
if ( line < numNonScrollingLines)
	line = numNonScrollingLines;
line = line + firstFileLineOnScreen - numNonScrollingLines;
if (init)
	{firstLine = line;
	previousLine = line;
	assignSelectedField (line, line, action);
	return;
	}
if ( !WinPtInRect (hab, &frameRectWorld, &pos))
	return;
if (pos.y > frameRectWorld.yBottom+swp.y+swp.cy)
	{scrollScreenVert ( -1);
	line = firstFileLineOnScreen;
	}
else if (pos.y < frameRectWorld.yBottom+swp.y)
	{scrollScreenVert ( 1);
	line = firstFileLineOnScreen + linesInWindow - numNonScrollingLines - 1;
	}
if (line == previousLine)
	return;
sign = previousLine>=firstLine?	1:	-1;
if ( sign*(line-previousLine) > 0)
	{assignSelectedField (previousLine+sign, line, action);
	}
else
	{cAction = action? 0: 1;
	assignSelectedField (previousLine, line+sign, cAction);
	if ( sign*(line-firstLine) < 0)
		assignSelectedField (firstLine, line, action);
	}
mostRecentSelectedLine = line;
previousLine = line;
}

/* ***************************** setHScrollBar ************************ */
void splitWindowOB::setHScrollBar (void)
{
maxHorizOffset = maxDisplayLineLength - charsInPane;
WinSendMsg (hwndHScrollBar, SBM_SETTHUMBSIZE, 
	MPFROM2SHORT (charsInPane, maxDisplayLineLength),	(MPARAM) 0);
WinSendMsg (hwndHScrollBar, SBM_SETSCROLLBAR, (MPARAM) 0,
	MPFROM2SHORT (0, maxHorizOffset) );
}

/* ***************************** setVScrollBar ************************** */
void splitWindowOB::setVScrollBar (void)
{
WinSendMsg (hwndVScrollBar, SBM_SETTHUMBSIZE, 
	MPFROM2SHORT (scrollingLinesInWindow-numNonScrollingLines, totalDisplayLines),	(MPARAM) 0);
WinSendMsg (hwndVScrollBar, SBM_SETSCROLLBAR, (MPARAM) firstFileLineOnScreen,
	MPFROM2SHORT (0, totalDisplayLines-(scrollingLinesInWindow-numNonScrollingLines)) );
return;
}

/* ****************************** showLines ****************************** */
void splitWindowOB::showLines (HWND hwnd, HPS hps, PRECTL prectl)
{
int			i, j, panel, 
			firstInvalidLine,
			lastInvalidLine,
			line,
			offset,
			outputLen;
char			*work;
CHARBUNDLE		bundle;
POINTL			point;
panelDescriptor *ppanel;

invalidLines (prectl, &firstInvalidLine, &lastInvalidLine);
bundle.lBackColor = CLR_BLUE;
for (panel=0, ppanel=panelDef; panel<numPanels;	++panel, ++ppanel)
	{
	for (i=firstInvalidLine;	i<=lastInvalidLine;	++i)
		{
		point.y = clientTopPixel - (i+1)*CharHeight + CharMaxDescender;
		if (i < numNonScrollingLines)
			{work = fmtPanel (panel, i);
			offset = 0;
			line = i;
			}
		else
			{
			if ( (i+firstFileLineOnScreen-numNonScrollingLines) >= totalDisplayLines)
				break;
			line = i + firstFileLineOnScreen;
			work = fmtPanel (panel, line);
			offset = firstDisplayChar;
			if ( *(lineSelection + line - numNonScrollingLines))
				GpiSetColor (hps, CLR_BLUE);
			else
				GpiSetColor (hps, CLR_DEFAULT);
			}
		point.x = ppanel->panelFirstPixel+1;
		outputLen = strlen (work) - firstDisplayChar;
		if (outputLen > ppanel->charsInPanel)
			outputLen = ppanel->charsInPanel;
		if (outputLen > 0)
			GpiCharStringAt (hps, &point, outputLen, work+firstDisplayChar);
		}		// for each invalid line
	}			// for each panel
return;
}

/* **************************** splitWindowOB **************************** */
splitWindowOB::splitWindowOB (HWND hwF, HWND hwC, char *fontNameSize,
	int nFrames, int fixedL, PANEL_FORMAT_FUNC * formatPanelStr)
{HPS			hps;
FONTMETRICS		fontMetrics;
RECTL			rectl;

mostRecentSelectedLine = 0;
hab = WinQueryAnchorBlock (hwC);
hwndClient = hwC;
hwndFrame = hwF;
fmtPanel = formatPanelStr;
WinSetPresParam (hwndClient, PP_FONTNAMESIZE, 
		(ULONG) strlen (fontNameSize)+1, (PVOID) fontNameSize);
hps = WinBeginPaint (hwndClient, 0L, &rectl);
GpiQueryFontMetrics (hps, sizeof(fontMetrics), &fontMetrics);
CharHeight = fontMetrics.lMaxBaselineExt;
CharWidth = fontMetrics.lMaxCharInc;
CharMaxDescender = fontMetrics.lMaxDescender;
WinEndPaint (hps);
hwndVScrollBar = WinWindowFromID (hwndFrame, FID_VERTSCROLL);
hwndHScrollBar = WinWindowFromID (hwndFrame, FID_HORZSCROLL);
numNonScrollingLines = fixedL;
numPanels = nFrames;
panelDef = (panelDescriptor *) malloc (numPanels * sizeof (panelDescriptor));
lineSelection = NULL;
totalDisplayLines = 0;
WinPostMsg (hwndClient, WM_SIZE, NULL, NULL);

return;
}

/* ************************** ~splitWindowOB ************************* */
splitWindowOB::~splitWindowOB ( )
{
if (panelDef)
	free (panelDef);
if (lineSelection)
	free (lineSelection);
return;
}

/* *************************** VScrollMsg ***************************** */
void splitWindowOB::VScrollMsg (MPARAM mp1, MPARAM mp2)
{
switch (SHORT2FROMMP (mp2))
	{
		
case SB_LINEUP:
	scrollScreenVert (-1);
	break;
case SB_LINEDOWN:
	scrollScreenVert (1);
	break;
case SB_PAGEUP:
	scrollScreenVert (2-linesInWindow);
	break;
case SB_PAGEDOWN:
	scrollScreenVert (linesInWindow-2);
	break;
case SB_SLIDERTRACK:
	scrollScreenVert (SHORT1FROMMP (mp2) - firstFileLineOnScreen);
	break;
	} 
return;
}
