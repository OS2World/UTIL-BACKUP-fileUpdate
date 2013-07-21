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

/* define the Output Methods used by baseIOWindow */
typedef enum {EN_OMPost, EN_OMSend, EN_OMGPI} WINDOW_OUTPUT_METHOD;

typedef struct _CLASS_CONVERSION_DEFINITION
	{int		srcCount;
	PSZ			srcClass;
	PSZ			destClass;
	} CLASS_CONVERSION_DEFINITION;
typedef struct {
	int			id;
	int			value;
	}	RB_ID_TO_INT;
typedef struct {
	int			id;
	char		*value;
	}	RB_ID_TO_TEXT;


int		_Optlink compareWindowInfo (const void *p1, const void *p2);

class baseIOWindow {
	public:
		HWND	handle;
		int		id;
		int		wClass;

		baseIOWindow () 	{id=0, handle=0;}		
		baseIOWindow (int ident) 	{ id=ident; handle=0;}
		void setInterface (HWND hwndDialog, int ID);
		BOOL postMessage (ULONG msg, MPARAM mp1, MPARAM mp2) 	{
			return WinPostMsg (handle, msg, mp1, mp2);}
		MRESULT sendMessage (ULONG msg, MPARAM mp1, MPARAM mp2)	{
			return WinSendMsg (handle, msg, mp1, mp2);}
		virtual char * read () 	{return NULL;}
		virtual void write (WINDOW_OUTPUT_METHOD method)	{return;}
	};

/* the numeric entry classes are derived from this class */
class baseEntry {
public:
	char	changed;

	baseEntry () 	{changed = 0;}
	int		queryChanged () {return (int)changed;};
	void	setChanged () {changed = 1; return;};
	void	clearChanged () {changed = 0; return;};
	void fromScreen (HWND handle, char *pch)
		{WinQueryWindowText (handle, 99, (PCH) pch);
		return;
		};
	void toScreen (HWND handle, char *pch, WINDOW_OUTPUT_METHOD method);
	};

template <class TYPE> class numericEntryWindow : public baseIOWindow, 
public baseEntry {
	TYPE *	value;
	char *	convertSpec;
public:
	numericEntryWindow (int ID, TYPE * val, char *spec)
		{id = ID; value = val; convertSpec = spec;}
	char *	read();
	void	write (WINDOW_OUTPUT_METHOD method=EN_OMSend);
	};

class textEntryWindow : public baseIOWindow, public baseEntry {
	char *	value;
public:
	textEntryWindow (int ID, char *val)
			{id = ID; value = val;}
	char * read () {fromScreen (handle, value); return (char *)NULL;}
	void	write (WINDOW_OUTPUT_METHOD method=EN_OMSend) {
			toScreen (handle, value, method); return;}
	};

template <class TYPE> class convertWithLimits {
	char *	convertSpec;
	TYPE	min,
			max;
public:
	convertWithLimits (char *spec, TYPE mn, TYPE mx)
			{convertSpec = spec; min=mn; max = mx;}
	};

template <class TYPE> class validatedEntryWindow
	: public baseIOWindow, public baseENTRY {
	TYPE *		value;
	convertWithLimits<TYPE>	*convert;

	validatedEntryWindow (int ID, TYPE * val, convertWithLimits<TYPE> * conv)
			{id = ID, value = val; convert = conv;}
	};

class radioButtonWindow : public baseIOWindow
	{
public:
	HWND	parent;
	int *	identList;
	int 	value;

	radioButtonWindow (int ID)
			{id = ID; identList=NULL;}
	~radioButtonWindow () 	{if (identList!= NULL) free (identList);}
	char *	read();
	void	write (WINDOW_OUTPUT_METHOD method=EN_OMSend);
	void	write (WINDOW_OUTPUT_METHOD method, int val);
	void	createIdentArray (HWND hwndDialog);
	};

class checkBoxWindow : public baseIOWindow
	{
public:
	int 	value;

	checkBoxWindow (int ID)
			{id = ID;}
	char *	read ();
	void	write (WINDOW_OUTPUT_METHOD method=EN_OMSend);
	void	write (WINDOW_OUTPUT_METHOD method, int val);
	};

class listBoxTextWindow : public baseIOWindow
	{
public:
	char *		value;
	
	listBoxTextWindow (int ID, char * val) {id = ID; value=val;}
	char * read (void);
	void	write(WINDOW_OUTPUT_METHOD method=EN_OMSend) {return;}
	};

class listBoxIndexWindow : public baseIOWindow
	{
public:
	int		value;
	
	listBoxIndexWindow (int ID) :baseIOWindow (ID)
		{	}
	char *	read (void);
	void	write (WINDOW_OUTPUT_METHOD method=EN_OMSend) {return;}
	};

int			resourceToWindow (HWND						hwndParent,
						HWND							hwndOwner,
						HMODULE							modHandle,
						int								resourceID,
						CLASS_CONVERSION_DEFINITION		*convert,
						int								conversionItemCount,
						HWND							hwndFrame,
						int								frameID
						);

baseIOWindow * idToBaseIO (baseIOWindow **base, int count, int id);

int controlsToBaseIOWindow (
	HWND 			parent, 
	HMODULE			modHandle,
	int				resourceID,
	baseIOWindow 	**base,
	int				winCount);

void rbIDToInt (radioButtonWindow *rb, RB_ID_TO_INT *table, int * where);
void rbIDToString (radioButtonWindow *rb, RB_ID_TO_TEXT *table, char * where);
void rbIDFromInt (radioButtonWindow *rb, RB_ID_TO_INT *table, int * where);
void rbIDFromString (radioButtonWindow *rb, RB_ID_TO_TEXT *table, char * where);

