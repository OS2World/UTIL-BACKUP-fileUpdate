/*************************************************************************** 
 *                                                                         *
 *    File:         fileUp.hpp                                             *
 *                                                                         *
 *    Description:  This file contains the function prototypes and data    *
 *                  structures used within the fileUp.HPP file.            *
 *                                                                         *
 *    Copyright:                                                           *
 *    Date:         09 Jan 2000                                            *
 *                                                                         *
 *    Note:         Generated by the Guidelines code generation facility.  *
 *                                                                         *
 ***************************************************************************/

#ifndef HPP_FILEUP
#define HPP_FILEUP


/*************************************************************************** 
 *    Include files.                                                       *
 ***************************************************************************/

#include "D:\OS2APPS\GUIDELINES\sys\guirun.h"

/*************************************************************************** 
 *    Menu identifiers.                                                    *
 ***************************************************************************/

#define IDM_MI_FILE          2000                /* Sub Menu               */
#define IDM_MI_SOURCE_NEWER  2001                /* Menu Item              */
#define IDM_MI_DIFFERENTFILES 2002               /* Sub Menu               */
#define IDM_MI_EXCLUDE       2003                /* Menu Item              */
#define IDM_APPLY_EXCLUSION  2004                /* Menu Item              */
#define IDM_MI_HIDDEN        2005                /* Menu Item              */
#define IDM_MI_SYSTEM        2006                /* Menu Item              */
#define IDM_MI_SHOW          2007                /* Sub Menu               */
#define IDM_MI_SUBDIRS       2008                /* Sub Menu               */
#define IDM_MI_NO_SUBDIRS    2009                /* Menu Item              */
#define IDM_MI_ALL_SUBDIRS   2010                /* Menu Item              */
#define IDM_MI_SELECT_SUBDIRS 2011               /* Menu Item              */
#define IDM_MI_SIZE          2012                /* Menu Item              */
#define IDM_MI_DATE          2013                /* Menu Item              */
#define IDM_DIRECTORIES      2014                /* Sub Menu               */
#define IDM_SOURCEDIR        2015                /* Menu Item              */
#define IDM_DESTINATIONDIR   2016                /* Menu Item              */
#define IDM_MI_MODE          2017                /* Sub Menu               */
#define IDM_DIR_DIR          2018                /* Menu Item              */
#define IDM_DIR_ZIP          2019                /* Menu Item              */
#define IDM_ZIP_DIR          2020                /* Menu Item              */
#define IDM_MENUITEM2        2021                /* Sub Menu               */
#define IDM_MI_SEL_ALL       2022                /* Menu Item              */
#define IDM_MI_SEL_CURRENT_FIRST 2023            /* Menu Item              */
#define IDM_MI_SEL_CURRENT_LAST 2024             /* Menu Item              */
#define IDM_MI_CLEAR_ALL     2025                /* Menu Item              */
#define IDM_MI_CLEAR_CURRENT_FIRST 2026          /* Menu Item              */
#define IDM_MI_CLEAR_CURRENT_LAST 2027           /* Menu Item              */
#define IDM_MI_OPERATE       2028                /* Sub Menu               */
#define IDM_UPDATE_FILES     2029                /* Menu Item              */
#define IDM_COMPARE_FILES    2030                /* Menu Item              */
#define IDM_CONFIGURE        2031                /* Menu Item              */


/*************************************************************************** 
 *    Window/control identifiers.                                          *
 ***************************************************************************/

#define IDD_FRAME1           3000                /* Frame                  */
#define IDD_DIRECTORYDIALOG  3001                /* Dialog                 */
#define IDD_CB_DRIVE         3002                /* Combobox               */
#define IDD_STATIC1          3003                /* Static Text            */
#define IDD_STATIC2          3004                /* Static Text            */
#define IDD_CB_DIRECTORY     3005                /* Combobox               */
#define IDD_PB_OK            3006                /* Push Button            */
#define IDD_PB_CANCEL        3007                /* Push Button            */
#define IDD_EXCLUDEDIALOG    3008                /* Dialog                 */
#define IDD_CB_EXCLUDE       3009                /* Combobox               */
#define IDD_PB_ADD           3010                /* Push Button            */
#define IDD_PB_DELETE        3011                /* Push Button            */
#define IDD_FILE_COPY        3012                /* Dialog                 */
#define IDD_MLE1             3013                /* Multi-Line Entry       */
#define IDD_SELECT_DIRS_DLG  3014                /* Dialog                 */
#define IDD_SUBDIR_NAMES     3015                /* Listbox                */
#define IDD_DLG_CONFIGURE    3016                /* Dialog                 */
#define IDD_CB_APPLY_EXCLUDES 3017               /* Checkbox               */
#define IDD_RB_DIR_DIR       3018                /* Radio Button           */
#define IDD_RB_DIR_ZIP       3019                /* Radio Button           */
#define IDD_RB_ZIP_DIR       3020                /* Radio Button           */
#define IDD_STATIC3          3021                /* Group Box              */
#define IDD_RB_ALL           3022                /* Radio Button           */
#define IDD_RB_SRC_NEWER     3023                /* Radio Button           */
#define IDD_RB_DIFFERENT     3024                /* Radio Button           */
#define IDD_CB_SHOW_SIZE     3025                /* Checkbox               */
#define IDD_CB_SHOW_DATE     3026                /* Checkbox               */
#define IDD_STATIC4          3027                /* Group Box              */
#define IDD_RB_NO_SUBDIR     3028                /* Radio Button           */
#define IDD_RB_ALL_SUBDIR    3029                /* Radio Button           */
#define IDD_RB_SELECT_SUBDIR 3030                /* Radio Button           */
#define IDD_CB_HIDDEN_FILES  3031                /* Checkbox               */
#define IDD_CB_SYSTEM_FILES  3032                /* Checkbox               */
#define IDD_CB_UPDATE_EXCLUDES 3033              /* Checkbox               */
#define IDD_CB_UPDATE_SRC_DIR 3034               /* Checkbox               */
#define IDD_CB_UPDATE_DEST_DIR 3035              /* Checkbox               */
#define IDD_ERROR_LOG        3036                /* Dialog                 */
#define IDD_LB_ERROR_LOG     3037                /* Listbox                */
#define IDD_PUSHBUTTON1      3038                /* Push Button            */


/*************************************************************************** 
 *    Global variables.                                                    *
 ***************************************************************************/

#ifndef RC_INVOKED
extern WINDOW  wFrame1;
extern WINDOW  wDirectoryDialog;
extern WINDOW  wExcludeDialog;
extern WINDOW  wFILE_COPY;
extern WINDOW  wSELECT_DIRS_DLG;
extern WINDOW  wDLG_CONFIGURE;
extern WINDOW  wERROR_LOG;
#endif /* !RC_INVOKED */


/*************************************************************************** 
 *    Function Prototypes.                                                 *
 ***************************************************************************/

#ifndef RC_INVOKED
PMENTRY Frame1WndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
PMENTRY DirectoryDialogWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
PMENTRY ExcludeDialogWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
PMENTRY FILE_COPYWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
PMENTRY SELECT_DIRS_DLGWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
PMENTRY DLG_CONFIGUREWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
PMENTRY ERROR_LOGWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
#endif /* !RC_INVOKED */



#endif /* HPP_FILEUP */

