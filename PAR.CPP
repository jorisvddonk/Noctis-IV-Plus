/*

	Noctis galactic guide / PAR command.
	GOES Net Module.

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>

//////////////////////////////////////////////////////////////////////////////

char   *situation_file  = "..\\DATA\\Current.BIN";

char   sync               = 1;		// 0
char   anti_rad           = 1;          // 1
char   pl_search          = 0;          // 2
char   field_amplificator = 0;          // 3
char   ilight             = 63;         // 4
char   ilightv            = 1;          // 5
char   charge             = 3;          // 6
char   revcontrols        = 0;          // 7
char   ap_targetting      = 0;          // 8
char   ap_targetted       = 0;          // 9
char   ip_targetting      = 0;          // 10
char   ip_targetted       = -1;		// 11
char   ip_reaching        = 0;          // 12
char   ip_reached         = 0;          // 13
char   ap_target_spin     = 0;          // 14
char   ap_target_r        = 0;          // 15
char   ap_target_g        = 0;          // 16
char   ap_target_b        = 0;          // 17
char   nearstar_spin      = 0;          // 18
char   nearstar_r         = 0;          // 19
char   nearstar_g         = 0;          // 20
char   nearstar_b         = 0;          // 21
char   gburst	          = 0;          // 22
char   menusalwayson      = 1;		// 23
char   depolarize  	  = 0;          // 24
int    sys                = 4;		// 25
int    pwr                = 15000;	// 27
int    dev_page           = 0;		// 29
int    ap_target_class    = 0;		// 31
int    f_ray_elapsed	  = 0;		// 33
int    nearstar_class     = 0;		// 35
int    nearstar_nop       = 0;		// 37
float  pos_x              = 0;		// 39
float  pos_y              = 0;		// 43
float  pos_z              = -500;	// 47
float  user_alfa          = 0;		// 51
float  user_beta          = 0;		// 55
float  navigation_beta    = 0;		// 59
float  ap_target_ray      = 0;		// 63
float  nearstar_ray       = 1000;	// 67
double dzat_x             = +3797120;
double dzat_y             = -4352112;
double dzat_z             = -925018;
double ap_target_x        = 0;
double ap_target_y        = 1E9;
double ap_target_z        = 0;
double nearstar_x         = 0;
double nearstar_y         = 1E9;
double nearstar_z         = 0;
double helptime           = 0;
double ip_target_initial_d= 0;
double requested_approach_coefficient = 1;
double current_approach_coefficient = 1;
double reaction_time      = 0.01;
char   fcs_status[11]     = "STANDBY";
int    fcs_status_delay   = 0;
int    psys               = 4;
double ap_target_initial_d= 0;
double requested_vimana_coefficient = 1;
double current_vimana_coefficient = 1;
double vimana_reaction_time = 0.01;
char   lithium_collector  = 0;
char   autoscreenoff	  = 0;
char   ap_reached         = 0;
int    lifter		  = 0;
double secs		  = 0;

void unfreeze ()
{
	int fh = _open (situation_file, 0);
	if (fh>-1) {
		_read (fh, &sync, 244);
		_close (fh);
	}
}

//////////////////////////////////////////////////////////////////////////////

unsigned char far *adaptor = (unsigned char far *) 0xA0000000;

char digimap[65*5] = {
	 0, 0, 0, 0, 0, // 32. spazio vuoto
	 2, 2, 2, 0, 2, // 33. punto esclamativo
	 5, 0, 0, 0, 0, // 34. virgolette
	 0, 0, 3, 5, 5, // 35. cancelletto (rappresentato da una piccola n.)
	 2, 2, 6, 2, 2, // 36. sistro
	 1, 4, 2, 1, 4, // 37. percento
	 0, 0, 2, 0, 0, // 38. e commericale (non � possibile visualizzarla)
	 0, 2, 2, 0, 0, // 39. apice
	 4, 2, 2, 2, 4, // 40. parentesi tonda aperta
	 1, 2, 2, 2, 1, // 41. parentesi tonda chiusa
	 0, 0, 7, 2, 2, // 42. asterisco
	 0, 2, 7, 2, 0, // 43. segno pi�
	 0, 0, 0, 2, 1, // 44. virgola
	 0, 0, 7, 0, 0, // 45. segno meno
	 0, 0, 0, 0, 2, // 46. punto
	 0, 4, 2, 1, 0, // 47. barra destrorsa
	 7, 5, 5, 5, 7, // 48. 0
	 3, 2, 2, 2, 7, // 49. 1
	 7, 4, 7, 1, 7, // 50. 2
	 7, 4, 6, 4, 7, // 51. 3
	 4, 6, 5, 7, 4, // 52. 4
	 7, 1, 7, 4, 7, // 53. 5
	 7, 1, 7, 5, 7, // 54. 6
	 7, 4, 4, 4, 4, // 55. 7
	 7, 5, 7, 5, 7, // 56. 8
	 7, 5, 7, 4, 4, // 57. 9
	 0, 2, 0, 2, 0, // 58. duepunti
	 0, 2, 0, 2, 1, // 59. punto e virgola
	 4, 2, 1, 2, 4, // 60. minore
	 0, 7, 0, 7, 0, // 61. uguale
	 1, 2, 4, 2, 1, // 62. maggiore
	 7, 4, 6, 0, 2, // 63. punto interrogativo
	 0, 2, 0, 0, 0, // 64. a commerciale (non visualizzabile)
	 7, 5, 7, 5, 5, // 65. A
	 7, 5, 3, 5, 7, // 66. B
	 7, 1, 1, 1, 7, // 67. C
	 3, 5, 5, 5, 3, // 68. D
	 7, 1, 3, 1, 7, // 69. E
	 7, 1, 3, 1, 1, // 70. F
	 7, 1, 5, 5, 7, // 71. G
	 5, 5, 7, 5, 5, // 72. H
	 2, 2, 2, 2, 2, // 73. I
	 4, 4, 4, 5, 7, // 74. J
	 5, 5, 3, 5, 5, // 75. K
	 1, 1, 1, 1, 7, // 76. L
	 7, 7, 5, 5, 5, // 77. M
	 5, 7, 7, 5, 5, // 78. N
	 7, 5, 5, 5, 7, // 79. O
	 7, 5, 7, 1, 1, // 80. P
	 7, 5, 5, 1, 5, // 81. Q
	 7, 5, 3, 5, 5, // 82. R
	 7, 1, 7, 4, 7, // 83. S
	 7, 2, 2, 2, 2, // 84. T
	 5, 5, 5, 5, 7, // 85. U
	 5, 5, 5, 5, 2, // 86. V
	 5, 5, 7, 7, 5, // 87. W
	 5, 5, 2, 5, 5, // 88. X
	 5, 5, 7, 2, 2, // 89. Y
	 7, 4, 2, 1, 7, // 90. Z
	 0, 0, 6, 2, 2, // 91. parentesi quadra aperta
	 0, 1, 2, 4, 0, // 92. barra sinistrorsa
	 2, 2, 6, 0, 0, // 93. parentesi quadra chiusa
	 2, 2, 2, 2, 2, // 94. ordinale femminile
	 0, 0, 0, 0, 7, // 95. sottolinea
	 1, 2, 0, 0, 0  // 96. accento
};

void areaclear (unsigned char far *dest, int x, int y,
		int x2, int y2, int l, int h, unsigned char pattern)
{
	unsigned p;

	if (x<0) x = 0;
	if (y<0) y = 0;

	if (x2>=320) x2 = 319;
	if (y2>=200) y2 = 199;

	if (x2>0) l = x2 - x;
	if (y2>0) h = y2 - y;

	if (x+l>=320) l = 320 - x;
	if (y+h>=200) h = 200 - y;

	if (l<1 || h<1) return;

	p = 320 * y + x;

	asm {	push es
		pusha
		pushf
		cld
		les di, dword ptr dest
		add di, p
		mov al, pattern
		db 0x66; shl ax, 8
		mov al, pattern
		db 0x66; shl ax, 8
		mov al, pattern
		db 0x66; shl ax, 8
		mov al, pattern }
ac_nextl: asm {	push di
		mov cx, l
		shr cx, 2
		jcxz ac_bytes
		db 0xf3 // rep stosd
		db 0x66
		db 0xab }
ac_bytes: asm { mov cx, l
		and cx, 3
		jcxz ac_endl
		rep stosb }
ac_endl:  asm { pop di
		add di, 320
		dec h
		jnz ac_nextl
		popf
		popa
		pop es }
}

void wrouthud (unsigned x, unsigned y, unsigned l, char *text)
{
	int j, i, n;
	unsigned spot;

	n = 0; if (!l) l = 32767;
	spot = y * 320 + x;

	while (text[n] && n < l) {
		j = (text[n] - 32) * 5;
		for (i = 0; i < 5; i++) {
			if (digimap[j + i] & 1) adaptor[spot+0] = 191 - adaptor[spot+0];
			if (digimap[j + i] & 2) adaptor[spot+1] = 191 - adaptor[spot+1];
			if (digimap[j + i] & 4) adaptor[spot+2] = 191 - adaptor[spot+2];
			spot += 320;
		}
		spot -= 320 * 5;
		spot += 4;
		n++;
	}
}

void warn (char *text, int line)
{
	int w2 = strlen(text) * 2;

	areaclear (adaptor, 160 - w2 - 1, 100 + line * 8, 0, 0, w2*2 + 2, 8, 127);
	wrouthud (160 - w2, 100 + line * 8 + 1, NULL, text);
}

//////////////////////////////////////////////////////////////////////////////

char msgbuffer[77];
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

int	sts;
char	query;

long	round;
int 	i, fh, gh;
int 	analyzed_sectors_range;
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

double 	laststar_x, laststar_y, laststar_z;

char isthere (double star_id)
{
	char warnbuffer[41];

	double laststar_id;

	double sidlow = star_id - idscale;
	double sidhigh = star_id + idscale;

	unsigned sx, sy, sz;

	unsigned visible_sectors_x = analyzed_sectors_range;
	unsigned visible_sectors_y = analyzed_sectors_range;
	unsigned visible_sectors_z = analyzed_sectors_range;

	long   	advance = 100000;
	long   	sect_x, sect_y, sect_z;

	long   	align_m = 100000 * visible_sectors_z;
	long   	align_e = 100000 * visible_sectors_y;

	sect_x = (dzat_x - visible_sectors_x*50000) / 100000; sect_x *= 100000;
	sect_y = (dzat_y - visible_sectors_y*50000) / 100000; sect_y *= 100000;
	sect_z = (dzat_z - visible_sectors_z*50000) / 100000; sect_z *= 100000;

	asm {	mov ax, visible_sectors_x
		mov sx, ax }
e_while:asm {	mov ax, visible_sectors_y
		mov sy, ax }
m_while:asm {	mov ax, visible_sectors_z
		mov sz, ax }
i_while:asm {	db 0x66, 0xBB, 0x50, 0xC3, 0x00, 0x00 // mov ebx, 50000
		db 0x66; mov ax, word ptr sect_x
		db 0x66; mov dx, word ptr sect_z
		db 0x66; add ax, dx
		db 0x66; mov cx, ax
		db 0x66; mov dx, ax
		db 0x66, 0x81, 0xE2, 0xFF, 0xFF, 0x01, 0x00 // and edx, 0x0001FFFF
		db 0x66; add dx, word ptr sect_x
		db 0x66; sub dx, bx
		jnz i_cont_x }  // |
		asm jmp i_next; // | excludes stars with x coordinate = 0
i_cont_x: asm {                 // |
		db 0x66; mov word ptr laststar_x, dx
		db 0x66; imul dx
		db 0x66; add dx, ax
		db 0x66; add cx, dx
		db 0x66, 0x81, 0xE2, 0xFF, 0xFF, 0x01, 0x00 // and edx, 0x0001FFFF
		db 0x66; add dx, word ptr sect_y
		db 0x66; sub dx, bx
		jnz i_cont_y }  // |
		asm jmp i_next; // | excludes stars with y coordinate = 0
i_cont_y: asm {                 // |
		db 0x66; mov word ptr laststar_y, dx
		db 0x66; mov ax, cx
		db 0x66; imul dx
		db 0x66; add dx, ax
		db 0x66, 0x81, 0xE2, 0xFF, 0xFF, 0x01, 0x00 // and edx, 0x0001FFFF
		db 0x66; add dx, word ptr sect_z
		db 0x66; sub dx, bx
		jnz i_cont_z }  // |
		asm jmp i_next; // | excludes stars with z coordinate = 0
i_cont_z: asm {                 // |
		db 0x66; mov word ptr laststar_z, dx
		fild dword ptr laststar_x
		fst  laststar_x
		fmul idscale
		fild dword ptr laststar_y
		fst  laststar_y
		fmul idscale
		fild dword ptr laststar_z
		fst  laststar_z
		fmul idscale
		fmulp
		fmulp
		fst laststar_id
		fcomp sidlow
		fstsw ax
		sahf
		jb i_next
		fld laststar_id
		fcomp sidhigh
		fstsw ax
		sahf
		jb y_end }
i_next:	asm {	db 0x66; mov ax, word ptr advance
		db 0x66; add word ptr sect_z, ax
		dec sz
		jz i_end
		jmp i_while }
i_end:	  asm { db 0x66; mov dx, word ptr align_m
		db 0x66; sub word ptr sect_z, dx
		db 0x66; add word ptr sect_y, ax
		dec sy
		jz m_end
		jmp m_while }
m_end:	  asm { db 0x66; mov dx, word ptr align_e
		db 0x66; sub word ptr sect_y, dx
		db 0x66; add word ptr sect_x, ax
		dec sx
		jz e_end
		db 0x66; pusha }
		if (sts > 100) {
			sprintf (warnbuffer, "SLICE %d OF %d", visible_sectors_x - sx, visible_sectors_x);
			warn (warnbuffer, 0);
			while (kbhit()) {
				if (getch() == 27) {
					asm { db 0x66; popa }
					return (0);
				}
			}
		}
	  asm { db 0x66; popa
		jmp e_while }
e_end:	  return (0);
y_end:	  return (1);
}

void calc_parsis_for ()
{
	double 	star_id;
	int 	planet_nr;

	if (object_label[21] == 'S') {
		star_id = object_id;
		if (isthere (star_id)) {
			sprintf (textbuffer, "X=%1.0f", laststar_x);
			msg (textbuffer);
			sprintf (textbuffer, "Y=%1.0f", -laststar_y);
			msg (textbuffer);
			sprintf (textbuffer, "Z=%1.0f", laststar_z);
			msg (textbuffer);
		}
		else {
			msg (object_label);
			msg ("IS OUT OF RANGE");
		}
	}
	else {
		planet_nr  = object_label[23] - '0';
		planet_nr += 10 * (object_label[22] - '0');
		star_id    = object_id - planet_nr;
		if (isthere (star_id)) {
			sprintf (textbuffer, "X=%1.0f", laststar_x);
			msg (textbuffer);
			sprintf (textbuffer, "Y=%1.0f", -laststar_y);
			msg (textbuffer);
			sprintf (textbuffer, "Z=%1.0f", laststar_z);
			msg (textbuffer);
		}
		else {
			msg (object_label);
			msg ("IS OUT OF RANGE");
		}
	}
}

void main ()
{
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
		msg ("PAR OBJECTNAME");
		msg ("PAR OBJECTNAME:RANGE");
		msg ("^^^^^^^^^^^^^^^^^^^^^");
		msg ("PLEASE RUN AGAIN,");
		msg ("SPECIFYING PARAMETERS");
		msg (divider);
		msg ("(PAR REVISION 6011/2)");
		return;
	}
	else {
		msg ("GOES STARMAP ANALYSIS");
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

	if (parbuffer[i] != ':')
		analyzed_sectors_range = 100;
	else {
		sts = atoi (parbuffer + i + 1);
		if (sts <= 2 || sts > 10000)
			analyzed_sectors_range = 100;
		else
			analyzed_sectors_range = sts;
	}

	unfreeze ();
	if (sts > 100) warn ("SCANNING THE GALAXY: ESC TO STOP", -1);

	strupr (parbuffer);
	strupr (objectname);
	objectname[i] = 0;
	query = find (objectname);
	if (query) {
		if (query==1) msg ("SUBJECT: STAR;");
		if (query==2) msg ("SUBJECT: PLANET;");
		sprintf (outbuffer, "NAME: %s", subjectname);
		msg (outbuffer);
		calc_parsis_for ();
	}

	_close (fh);
}