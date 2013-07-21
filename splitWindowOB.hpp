typedef char * PANEL_FORMAT_FUNC (int panel, int line);
struct panelDescriptor
	{int		charsInPanel,
				panelFirstPixel;
	};

class splitWindowOB {
	HAB			hab;
	HWND		hwndHScrollBar,
				hwndVScrollBar;
	HWND		hwndClient,
				hwndFrame;
	LONG		CharHeight;
	LONG		CharWidth;
	LONG		CharMaxDescender;
	int			mostRecentSelectedLine;
	unsigned	numNonScrollingLines;
	unsigned	numPanels;
	PANEL_FORMAT_FUNC	* fmtPanel;
	panelDescriptor *	panelDef;

	RECTL	ClientRect,
				FrameRect;
	int		linesInWindow;
	int		scrollingLinesInWindow;
	int		clientTopPixel;
	int		clientWidth;
	int		charsInPane;
	int		rightPaneFirstPixel;
	int		firstFileLineOnScreen;
	int		totalDisplayLines;			// total number of lines defined by user
	int		firstDisplayChar,
			maxDisplayLineLength,
			maxFileNameLength,
			maxHorizOffset;
	char	*lineSelection;				// One character is allocated from the
										// heap for each line.  If the char
										// corresponding to a line is non-zero
										// then that line is selected.

	void invalidLines (PRECTL prectl, int *first, int *last);
	int	lineFromYOrd (int y);
	void setVScrollBar (void);
	void showLines (HWND hwnd, HPS hps, PRECTL prectl);

public:
	void assignSelectedField (int first, int last, int sel);
	int		buttonClick (MPARAM mp1, MPARAM mp2);
	void HScrollMsg (MPARAM mp1, MPARAM mp2);
	void newDisplayList (int lines, int maxLineLen);
	void newDisplayWidth (int maxLineLen);
	int	queryLatestSelection (void) 	{return (mostRecentSelectedLine);}
	int querySelect (int line);
	void refreshClient (void);
	void resize (void);
	void scrollScreenHoriz (int count);
	void scrollScreenVert (int count);
	void selectionTimerProc (int init, int action);
	void setHScrollBar (void);
	void VScrollMsg (MPARAM mp1, MPARAM mp2);
	splitWindowOB (HWND hwF, HWND hwC, char * fontNameSize, int nFrames,
		int nFixedL, PANEL_FORMAT_FUNC * pFormat);
	~splitWindowOB ();
};

