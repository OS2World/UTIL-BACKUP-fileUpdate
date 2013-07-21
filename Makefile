#C_DEBUG = -Ti+ -Tm -DENABLE_DEBUG
C_DEBUG = -O+
LINK_DEBUG = -CO
CFLAGS = -Q+ -Gm+ -DRC_INVOKED $(C_DEBUG) -Gd-
%.obj : %.cpp; icc $(CFLAGS) -c $<

OBJECTS = fileUpdate$O directoryDlg$O heapManagerOB$O splitWindowOB$O\
	resToWnd$O

fileUpdate.exe: $(OBJECTS) fileUpGen.res
	ilink -NOLOGO $(OBJECTS) /PM:PM $(LINK_DEBUG)
	rc fileUpGen.res fileUpdate.exe

testWP.exe:	testWP.obj
	ilink -NOLOGO -MAP testWP.obj /PM:PM $(LINK_DEBUG) somtk.lib

testWP.obj:	testWP.cpp
	icc $(CFLAGS) -c $<

fileUpdate$O:	fileUpdate.hpp fileUpGen.hpp directoryDlg.hpp \
	heapManagerOB.hpp splitWindowOB.hpp
splitWindowOB$O:	splitWindowOB.hpp
directoryDlg$O: directoryDlg.hpp
heapManagerOB$O:	heapManagerOB.hpp


fileUpGen.res: fileUpGen.rc fileUpGen.hpp fileUpGen.dlg
	rc -r fileUpGen.rc 

fileUpGen.hpp: fileUp.hpp 
	sed -f outputrc.sed <fileUp.hpp >fileUpGen.hpp

fileUpGen.rc: fileUp.rc
	sed -f outputrc.sed <fileUp.rc >fileUpGen.rc

fileUpGen.dlg: fileUp.dlg
	sed -f outputrc.sed <fileUp.dlg >fileupGen.dlg
