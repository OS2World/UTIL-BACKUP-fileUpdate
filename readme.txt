My program fileUpdate attempts to simplify back ups of the files
in a directory.  The backup can be either another directory or a
zip file.  The program will not create the initial zip archive
used for backup but will show the files in the archive and add to
it.  
There is no documentation, now will there ever be any unless
someone else writes it.  Just click on the menus and see what
happens.  The only menu pick which changes any files is the
Operate/Update pick.  
You compare the files in the source and destination directories
with the Operate/Compare menu pick.  Any discrepancies between
files are noted in the fileup$$.log file which is written in the
root directory of the current disk.
Status of the program:  I have been using the basic features of 
the program for some time now.  It has crashed occasionally
(although not lately), but has never destroyed any files.  This
is not a guarantee of course.  The file compare function and
configuration dialog are new, so you may have some problems
there.  
There is an issue with the long commands (read directories, copy
files and compare files).  During these operations I pop up a
multi-line edit box to display the name of the file or directory
which is currently being processed.  There is also a cancel
button to terminate the operation.  The trouble is that you can
not see the cursor while the directory names are updating.  This
effect seems related to the behavior of the MLE box, since it is
more pronounced when I am reading directories from my hard disk
than from the relatively slower Zip drive.  I do make the Cancel
button the default, so you can terminate the operation so long as
the dialog has the focus, but I have no idea how to make the
cursor behave The way I want it to.  If anyone has any thoughts
on the matter I would love to hear them.
You are welcome to make suggestions for new functionality, but
keep in mind that this program is not a file manager.  It is
intended only for maintaining a backup set of files for your
directories.  As such I will probably ignore any suggestions
which move the program in the direction of file management.
Bug reports are, of course, welcome.  Well, not actually welcome,
but expected.  In the event that I decide that your bug is
actually a feature bear in mind that you do have the source code.

There are four features of potential interest to developers
The first is the class heapManagerOB.  I have found it useful in
cases where potentially large blocks of memory have to be
allocated which are released either all at once or sub-blocks are
released in the reverse of the order in which they were
allocated.  This object has the advantage that you do not
continually have to reallocate memory, and that you are somewhat
unlikely to have a memory leak.
The functions in resourceToWin are very useful in cases where you
have a lot of values being written to and read from window
controls, but there is not much interaction between windows. 
i.e., clicking one button does not disable another window.  The
use I have made of it here is not a very good illustration of the
power of this class which is best shown by using the top level
function to create windows directly from a resource, then reading
and writing using arrays of pointers to the baseIOWindow which
each class is derived from.  BTW, one huge problem that I am
unable to solve is the correct relationship between the objects
and the windows which they represent.
The splitWindowOB class seemed like a good idea when I was
writing it - but I am not so certain now.  At any event it does
manage a user drawn client successfully.  Of course if it is ever
to be truly useful it really needs to be extended a lot.
Finally, I am kind of proud of a feature which you are not likely
to notice unless you read the makefile fairly carefully.  That
is, I used the Guidelines tool to generate my dialog windows and
then use a sed script to eliminate the statements in the
Guidelines-generated code which will not allow it to be compiled
by VACPP.

