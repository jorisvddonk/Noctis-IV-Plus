/*

	Noctis galactic guide / OUTBOX command.
	GOES Net Module.

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>

char msgbuffer[77];
char *divider = "&&&&&&&&&&&&&&&&&&&&&";

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
int 	fh, gh, ph;
char	*map = "..\\DATA\\STARMAP.BIN";
char	*guide = "..\\DATA\\GUIDE.BIN";
char	*packet = "..\\DATA\\OUTBOX.ZIP";

char	outbuffer[40];
char	textbuffer[40];
char	parbuffer[160];
char	nullbuffer[128];
char	objectname[21];
char	subjectname[21];

double	mblock_subject;
char	mblock_message[77];

long	starmap_size, sm_out;
long	guide_size, gg_out;

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
	printf ("\n\t- GOES NET onboard microsystem, EPOC 6011 REVISION 3\n");
	return;

	startup:

	warn ("EXPORTING ARCHIVES...", 0);

	msg ("     GOES OUTBOX     ");
	msg (divider);

	fh = _open (map, 0);
	if (fh == -1) {
		msg ("STARMAP NOT AVAILABLE");
		return;
	}

	gh = _open (guide, 0);
	if (gh == -1) {
		msg ("GUIDE NOT AVAILABLE");
		_close (fh);
		return;
	}

	ph = _creat (packet, 0);
	if (fh == -1) {
		msg ("CANNOT CREATE DATA");
		msg ("PACKET: DISK ACCESS");
		msg ("DENIED OR DISK FULL.");
		_close (fh);
		_close (gh);
		return;
	}

	_write (ph, "STARMAP_", 8);

	sm_out = 0;
	_read (fh, &starmap_size, 4);
	lseek (fh, starmap_size, SEEK_SET);
	while (_read (fh, &object_id, 8) && _read (fh, object_label, 24) == 24) {
		if (memcmp (&object_id, "Removed:", 8)) {
			_write (ph, &object_id, 8);
			round = _write (ph, object_label, 24);
			if (round != 24) {
				msg ("ERROR: DISK FULL?");
				_close (fh);
				_close (gh);
				_close (ph);
				remove (packet);
				return;
			}
			else
				sm_out++;
		}
	}

	_write (ph, "GUIDE___", 8);

	gg_out = 0;
	_read (gh, &guide_size, 4);
	lseek (gh, guide_size, SEEK_SET);
	while (_read (gh, &mblock_subject, 8) && _read (gh, mblock_message, 76) == 76) {
		if (memcmp (&mblock_subject, "Removed:", 8)) {
			_write (ph, &mblock_subject, 8);
			round = _write (ph, mblock_message, 76);
			if (round != 76) {
				msg ("ERROR: DISK FULL?");
				_close (fh);
				_close (gh);
				_close (ph);
				remove (packet);
				return;
			}
			else
				gg_out++;
		}
	}

	_close (fh);
	_close (gh);
	_close (ph);

	msg ("OUTGOING LABELS:");
	if (sm_out == 0)
		strcpy (textbuffer, "   NONE");
	else
		sprintf (textbuffer, "   %ld", sm_out);
	msg (textbuffer);
	msg ("OUTGOING COMMENTS:");
	if (gg_out == 0)
		strcpy (textbuffer, "   NONE");
	else
		sprintf (textbuffer, "   %ld", gg_out);
	msg (textbuffer);

	if (sm_out + gg_out > 0) {
		msg ("NOW PLEASE SEND FILE");
		msg ("  DATA/ OUTBOX.ZIP");
		msg ("TO THE ACTUAL G.O.E.S");
		msg ("NETWORK KEEPER.");
		msg (divider);
		msg ("SEE THE MANUAL OR THE");
		msg ("G.O.E.S. HOMEPAGE FOR");
		msg ("THE DESTINATION EMAIL");
		msg ("ADDRESS...");
		msg (divider);
		msg ("THANK YOU.");
	}
}