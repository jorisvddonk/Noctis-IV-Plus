/*

	Noctis galactic guide / REP command.
	GOES Net Module.

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>

char msgbuffer[40];
char *divider = "&&&&&&&&&&&&&&&&&&&&&";

void msg (char *string)
{
	int x;

	strcpy (msgbuffer, string);
	msgbuffer[21] = 0;
	printf (msgbuffer);

	x = strlen(msgbuffer);
	while (x < 21) {
		printf (" ");
		x++;
	}
}

double 	object_id = 12345;
char   	object_label[25];
double 	s_object_id = 12345;
char   	s_object_label[25];

double	subject_id = 12345;
double  idscale = 0.00001;

long	round;
int 	i, mlen, fh, gh;
char	*file = "..\\DATA\\STARMAP.BIN";
char	*guide = "..\\DATA\\GUIDE.BIN";

char	outbuffer[40];
char	textbuffer[40];
char	parbuffer[160];
char	nullbuffer[128];
char	objectname[21];
char	subjectname[21];

double	mblock_subject;
char	mblock_message[77];

char find (char *starname)
{
	int p, n, ctc, found;
	ctc = strlen (starname);
	if (ctc > 20 || ctc <= 0) {
		msg ("INVALID OBJECT NAME.");
		return(0);
	}
	n = 0;
	found = 0;
	lseek (fh, 4, SEEK_SET);
	while (_read (fh, &s_object_id, 8) && _read (fh, &s_object_label, 24) == 24) {
		if (memcmp (&s_object_id, "Removed:", 8)) {
			if (!memcmp (s_object_label, starname, ctc)) {
				n++;
				memcpy (object_label, s_object_label, 24);
				object_id = s_object_id;
				memcpy (subjectname, object_label, 20);
				subject_id = object_id;
				if (object_label[21] == 'S') found = 1;
				if (object_label[21] == 'P') found = 2;
				p = 20;
				while (p >= 0) {
					if (s_object_label[p] != 32) {
						if (s_object_label[p] == starname[p])
							return (found);
						else
							break;
					}
					p--;
				}
			}
		}
	}
	if (!n)
		msg ("OBJECT NOT FOUND.");
	if (n > 1) {
		msg ("AMBIGUOUS SEARCH KEY:");
		msg ("PLEASE EXPAND NAME...");
		msg (divider);
		msg ("POSSIBLE RESULTS ARE:");
		msg (divider);
		lseek (fh, 4, SEEK_SET);
		while (_read (fh, &s_object_id, 8) && _read (fh, &s_object_label, 24) == 24) {
		if (memcmp (&s_object_id, "Removed:", 8) && !memcmp (s_object_label, starname, ctc)) {
			s_object_label[21] = 0;
			msg (s_object_label);
		}}
		msg (divider);
		found = 0;
	}
	return (found);
}

void main ()
{
	long guide_consolidated;
	long rec, rectorep;
	char query;
	int  is;

	asm {	xor	ax, ax
		mov	es, ax
		cmp	byte ptr es:[0x449], 0x13
		je	startup }

	printf ("\nGalactic Organization of Explorers and Stardrifters (G.O.E.S)\n");
	printf ("-------------------------------------------------------------\n");
	printf ("This is a GOES NET module and must be run from a stardrifter.\n");
	printf ("Please use the onboard computer console to run this module.\n");
	printf ("\n\t- GOES NET onboard microsystem, EPOC 6011 REVISION 2\n");
	return;

	startup:
	if (_argc<2) {
		msg ("________USAGE________");
		msg ("REP OBJNAME:X:NOTES");
		msg ("^^^^^^^^^^^^^^^^^^^^^");
		msg ("PLEASE RUN AGAIN,");
		msg ("SPECIFYING PARAMETERS");
		return;
	}
	else {
		msg (" GOES GALACTIC GUIDE ");
		msg (divider);
	}

	fh = _open (file, 4);
	if (fh == -1) {
		msg ("STARMAP NOT AVAILABLE");
		return;
	}

	i = 2;
	strcpy (parbuffer, _argv[1]);
	while (i < _argc) {
		strcat (parbuffer, " ");
		strcat (parbuffer, _argv[i]);
		i++;
	}

	i = 0;
	while (parbuffer[i]) {
		if (parbuffer[i] == '_')
			parbuffer[i] = 32;
		i++;
	}

	i = 0;
	while (i < 21 && parbuffer[i] != ':' && parbuffer[i] != 0) {
		objectname[i] = parbuffer[i];
		i++;
	}

	if (parbuffer[i] != ':') {
		msg ("SYNTAX ERROR:");
		msg ("MISSING COLON BETWEEN");
		msg ("OBJECT NAME AND NOTES");
	}
	else {
		i++;
		is = i;
		while (parbuffer[i] != ':' && parbuffer[i] != 0) i++;
		if (parbuffer[i] != ':') {
			msg ("SYNTAX ERROR:");
			msg ("MISSING COLON BETWEEN");
			msg ("RECORD NR. AND NOTES");
		}
		else {
			if (parbuffer[i+1] == 0) {
				msg ("VOID MESSAGE:");
				msg ("WAS NOT SENT.");
			}
			else {
				parbuffer[i] = 0;
				rectorep = atol (parbuffer + is);
				objectname[is - 1] = 0;
				strupr (parbuffer);
				strupr (objectname);
				query = find (objectname);
				if (query) {
					msg ("TRANSFER SUCCEDED:");
					msg (divider);
					if (query==1) msg ("SUBJECT: STAR;");
					if (query==2) msg ("SUBJECT: PLANET;");
					sprintf (outbuffer, "NAME: %s", subjectname);
					msg (outbuffer);
					msg ("CORRECTION SENT;");
					gh = _open (guide, 4);
					if (gh == -1) {
						msg ("DATABASE ERROR;");
						msg ("MESSAGE REJECTED.");
						msg ("(ERROR CODE 1001)");
					}
					else {
						rec = 0;
						_read (gh, &guide_consolidated, 4);
						while (_read (gh, &mblock_subject, 8) && _read (gh, mblock_message, 76) == 76) {
							if (mblock_subject > subject_id - idscale && mblock_subject < subject_id + idscale) {
								rec++;
								if (rec == rectorep) {
									lseek (gh, -84L, SEEK_CUR);
									if (tell(gh) >= guide_consolidated) {
										mlen = strlen (parbuffer + i + 1);
										_write (gh, &subject_id, 8);
										_write (gh, parbuffer + i + 1, 76);
										msg ("CORRECTION ACCEPTED.");
									}
									else {
										msg ("CORRECTION REJECTED:");
										msg ("MESSAGE IS PROTECTED.");
									}
									goto done;
								}
							}
						}
						msg ("CORRECTION REJECTED:");
						msg ("NO SUCH RECORD!");
					    done:
						_close (gh);
					}
				}
			}
		}
	}

	_close (fh);
}