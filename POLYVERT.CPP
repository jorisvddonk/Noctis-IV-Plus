/*

	Editor per oggetti poligonali complessi.
	----------------------------------------
	Strettamente personale, mi servir… per varie cose.
	I programmi di PolyVert non sono scherzi, tutt'altro che
	giocattolini, richiedono un computer che come minimo deve avere
	un Pentium: qualsiasi velocit… di clock, anche 60, ma un Pentium.
	Questo perch‚ tutto il livello di dettaglio si basa sul numero di
	poligoni usati: niente dettagli superficiali o effetti particolari
	di rimescolamento dei pixels, tutti sani poligoni indipendenti
	come piacciono a me.

	Elenco comandi da mouse:

		Spostamento del mouse: cambio angolazione.
		Click Sinistro + Spostamento: cambio coords. x/y origine.
		Click Destro + Spostamento: allontana / avvicina l'oggetto.

	Elenco comandi da tastiera:

		Cursore: sposta il cursore 3d su x/y.
		Ctrl+CrUp,CrDown: aumenta/diminuisce passo di griglia.
		Pag Up/Dn.: sposta il cursore 3d su z.
		Ctrl+Pag Up/Dn.: sposta l'origine su z.
		Barra spaziatrice: fissa un vertice.
		Backspace: cancella un vertice o un poligono.
		Invio: fissa un poligono.
		+: incrementa intensit… del colore.
		-: decrem. intensit… colore.
		c: riduce contrasto di colori.
		C: aumenta contrasto colori.
		l: riduce luminosit… colori.
		L: aumenta luminosit… colori.
		Shift+Invio: salva l'oggetto sul file "POLYVERT.NCC".
		Barra rovescia: sposta il cursore sul vertice pi— vicino,
				d… inoltre la possibilit… di spostare tutti
				i vertici ad esso sovrapposti tramite lo
				spostamento del cursore.
		p: porta in primopiano il poligono selezionato,
		   ovvero in fondo alla lista (viene disegnato per ultimo).
		b: porta in sfondo il poligono selezionato,
		   ovvero in cima alla lista (viene disegnato per primo).
		t: porta tutti i poligoni a tre vertici, aggiungendo quelli
		   che servono per dividere quelli a 4 vertici;
		   Š utile per approssimare forme "piegate" a 4 vertici,
		   per poi poterle far tracciare al codice 3d correttamente
		   sotto forma di due poligoni a 3 vertici.
		s: seleziona il poligono pi— vicino al cursore,
		   e lo evidenzia in giallo: da ora in poi, lo si potr…
		   spostare solidalmente al cursore.
		   se un poligono Š selezionato, lo deseleziona.
		punto (.): porta al cursore il colore del poligono
			   selezionato.
		duepunti (:): porta al poligono selezionato il colore
			      del cursore.
		h: da chiarire a cosa serva.
		   mi Š stato utile per portare un oggetto studiato con
		   una certa origine dal punto 0;0;0 al punto in cui si
		   trovava il cursore; per questo Š servito, ma se lo
		   ripeto, ovviamente, mi si risposta di crx;cry;crz
		   unit… di misura.
		o: fissa l'origine (intesa come riferimento nella
		   rotazione di comodit… dell'oggetto) sul cursore.
		q: abbandona il programma (quit). prima salvare!

	Descrizione del formato NCC:

		La sigla sta per Numero vertici, Coordinate, Colore.
		Struttura del file:

	  Inizio-> 2 bytes: n=numero di poligoni di cui Š composto l'ogg.
		   n bytes: nr. vertici per ogni poligono.
		   16 * n:  coordinata x di ogni vertice. max. 4 vertici.
		   16 * n:  y (16 * n = 4 vertici * 4 bytes * n).
		   16 * n:  z.
		   n bytes: intensit… colore di ogni poligono (da 1 a 62).
	  Fine->   Eof.

*/

#define larghezza 250
#include <dos.h>
#include "defs.h"

#include "assembly.h"
#include "tdpolygs.h"

#include <stdio.h>
#include <time.h>
#include "gfx.h"
#include <io.h>

int n = 0;
int backgr = 0;
int arrowcol = 0;
int showhelp = 0;
float aux;

float far *x, *y, *z;
unsigned char far *nv, *color;

char digimap[65*5] = {							// we need this for the showing of the help.
	 0, 0, 0, 0, 0, // 32. spazio vuoto
	 2, 2, 2, 0, 2, // 33. punto esclamativo
	 5, 0, 0, 0, 0, // 34. virgolette
	 0, 0, 3, 5, 5, // 35. cancelletto (rappresentato da una piccola n.)
	 2, 2, 6, 2, 2, // 36. sistro
	 1, 4, 2, 1, 4, // 37. percento
	 0, 0, 2, 0, 0, // 38. e commericale (non Š possibile visualizzarla)
	 0, 2, 2, 0, 0, // 39. apice
	 4, 2, 2, 2, 4, // 40. parentesi tonda aperta
	 1, 2, 2, 2, 1, // 41. parentesi tonda chiusa
	 0, 0, 7, 2, 2, // 42. asterisco
	 0, 2, 7, 2, 0, // 43. segno pi—
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
	 1, 3, 7, 3, 1, // 92. barra sinistrorsa
	 2, 2, 6, 0, 0, // 93. parentesi quadra chiusa
	 2, 2, 2, 2, 2, // 94. ordinale femminile
	 0, 0, 0, 0, 7, // 95. sottolinea
	 1, 2, 0, 0, 0  // 96. accento
};

void wrouthud (int x, int y, int l, char *text, int color = 191) // we need this for the showing of the help.
{
	int j, i, n;
	int spot;

	n = 0; if (!l) l = 32767;
	spot = y * 320 + x;

	while (text[n] && n < l) {
		j = (text[n] - 32) * 5;
		for (i = 0; i < 5; i++) {
			if (digimap[j + i] & 1) adapted[spot+0] = color - adapted[spot+0];
			if (digimap[j + i] & 2) adapted[spot+1] = color - adapted[spot+1];
			if (digimap[j + i] & 4) adapted[spot+2] = color - adapted[spot+2];
			spot += 320;
		}
		spot -= 320 * 5;
		spot += 4;
		n++;
	}
}

void main ()
{
	int c, p, v = 0, i = 0, ink = 32;
	int fh, d = 5, psel = -1;
	int mp = -1, mv = -1;

	float rx, ry, rz, k;
	float orx = 0, ory = 0, orz = 0;
	float crx = 0, cry = 0, crz = 0;
	float pcrx = 0, pcry = 0, pcrz = 0;
	float vx[4], vy[4], vz[4], chase = 300;

	int mx2d = 160, my2d = 100;
	float dx, dy, md;

	adapted = (unsigned char far *) farmalloc (64000);

	x = (float far *) farmalloc (16000);
	y = (float far *) farmalloc (16000);
	z = (float far *) farmalloc (16000);

	nv = (unsigned char far *) farmalloc (1000);
	color = (unsigned char far *) farmalloc (1000);

	_320_200_256 ();

	initscanlines ();

	
	tavola_colori (range8088,   0, 64,  0, 63,  0);
	tavola_colori (range8088,  64, 64, 63,  0,  0);
	tavola_colori (range8088, 128, 64, 63, 63,  0);
	tavola_colori (range8088, 192, 64,  8, 16, 63);

	fh = _open ("POLYVERT.NCC", 0);
	if (fh>-1) {
		_read (fh, &n, 2);
		_read (fh, nv, n);
		_read (fh, x, 16*n);
		_read (fh, y, 16*n);
		_read (fh, z, 16*n);
		_read (fh, color, n);
		_close (fh);
	}
	
	/*
		Word n: Number of polygons
		Uchar[n] nv: Number of vertices for each polygon
		float[n*4] x: X coordinates for each vertex (You always have 4 vertices in the file per polygon, even if nv says 3. The 4th is just ignored by NIV in that case.)
		float[n*4] y: Y coordinates for each vertex 
		float[n*4] z: Z coordinates for each vertex 
		Uchar[n] color:	
		
		The first polygon has x,y,z 0-3, the second polygon has 4-7, etc.
	
		
	*/

	uneg = 1;
	do {
		if (backgr == 0) {
		pclear (adapted, 0);
		} else if (backgr == 1) {
		pclear (adapted, 32);
		} else if (backgr == 2) {
		pclear (adapted, 230);
		} else if (backgr == 3) {
		pclear (adapted, 120);
		} else if (backgr == 4) {
		pclear (adapted, 191);
		}
		mpul = 0; mouse_input ();
		if (mpul) {
			if (mpul==1) {
				beta -= mdltx;
				alfa -= mdlty;
				change_angle_of_view ();
			}
			else {
				chase += mdltx;
				chase += mdlty;
				if (chase<0) chase = 0;
			}
		}
		else {
			mx2d += mdltx;
			my2d += mdlty;
			if (mx2d < 0) mx2d = 0;
			if (my2d < 0) my2d = 0;
			if (mx2d > 319) mx2d = 319;
			if (my2d > 199) my2d = 199;
		}
		cam_x = orx;
		cam_y = ory;
		cam_z = orz;
		Forward (-chase);
		for (c=0, p=0; p<n; c+=4, p++) {
			if (p!=psel) {
				poly3d (x+c, y+c, z+c, nv[p], color[p]);			//draw polygon "the normal way"
			} else
				poly3d (x+c, y+c, z+c, nv[p], 254);					//draw polygon in blue (selected polygon).
		}
		for (c=0; c<v; c++) {
			vx[0] = x[n*4+c]; vy[0] = y[n*4+c]-1; vz[0] = z[n*4+c];
			vx[1] = x[n*4+c]-1; vy[1] = y[n*4+c]; vz[1] = z[n*4+c];
			vx[2] = x[n*4+c]; vy[2] = y[n*4+c]+1; vz[2] = z[n*4+c];
			vx[3] = x[n*4+c]+1; vy[3] = y[n*4+c]; vz[3] = z[n*4+c];
			poly3d (vx, vy, vz, 4, 193 - (clock()%4) * 16);
		}
		if (mp>-1) {
			vx[0] = x[mp*4+mv]; vy[0] = y[mp*4+mv]-3; vz[0] = z[mp*4+mv];
			vx[1] = x[mp*4+mv]-3; vy[1] = y[mp*4+mv]; vz[1] = z[mp*4+mv];
			vx[2] = x[mp*4+mv]; vy[2] = y[mp*4+mv]+3; vz[2] = z[mp*4+mv];
			vx[3] = x[mp*4+mv]+3; vy[3] = y[mp*4+mv]; vz[3] = z[mp*4+mv];
			poly3d (vx, vy, vz, 4, 254 - (clock()%4) * 8);
		}
		vx[0] = crx; vy[0] = cry; vz[0] = crz;
		vx[1] = crx+10; vy[1] = cry+8; vz[1] = crz-10;
		vx[2] = crx+4; vy[2] = cry+14; vz[2] = crz-4;
		if (arrowcol == 1) {
		poly3d (vx, vy, vz, 3, 0+ink); //draw arrow green
		} else if (arrowcol == 0) {
		poly3d (vx, vy, vz, 3, 64+ink);//draw arrow red (normal)
		}
		if (tasto_premuto()) {
			pcrx = crx;
			pcry = cry;
			pcrz = crz;
			i = attendi_pressione_tasto();
			if (i==0) {
				i = attendi_pressione_tasto();
				switch (i) {
					case 77: crx+=d; break;
					case 75: crx-=d; break;
					case 80: cry+=d; break;
					case 72: cry-=d; break;
					case 73: crz+=d; break;
					case 81: crz-=d; break;
					case 141: if (d<5) d++; break;
					case 145: if (d>1) d--; break;
					case 0x84: orz+=d; break;
					case 0x76: orz-=d; break;
					case 0x3B:
						showhelp = showhelp + 1;
						if (showhelp == 2)
						showhelp = 0;
					 break;
				}
				i = 0;
			}
			else {
				switch (i) {
					case 'v':c = mp;
						 mp = -1;
						 mv = -1;
						 if (c>-1) break;
						 md = 1000;
						 for (c=0; c<n; c++) {
							for (p=0; p<nv[c]; p++) {
								if (getcoords (x[4*c+p], y[4*c+p], z[4*c+p])) {
									dx = mx2d - _x_;
									dy = my2d - _y_;
									dx = sqrt (dx*dx+dy*dy);
									if (dx < md) {
										md = dx;
										mp = c;
										mv = p;
									}
								}
							}
						 }
						 crx = x[4*mp+mv];
						 cry = y[4*mp+mv];
						 crz = z[4*mp+mv];
						 break;
					case 'o':orx = crx;
						 ory = cry;
						 orz = crz;
						 break;
					case 'h':for (c=0; c<n; c++) {
							for (p=0; p<nv[c]; p++) {
								x[4*c+p] += crx;
								y[4*c+p] += cry;
								z[4*c+p] += crz;
							}
						 }
						 break;
					case 'p':if (psel>-1&&psel<n-1) {
							for (p=0; p<4; p++) {
								x[4*n+p] = x[4*psel+p];
								y[4*n+p] = y[4*psel+p];
								z[4*n+p] = z[4*psel+p];
							}
							nv[n] = nv[psel];
							color[n] = color[psel];
							for (c=psel; c<n; c++) {
								for (p=0; p<4; p++) {
									asm finit; // boh? ma lo sapete che non vuol saperne di funzionare
										   // se non lo inizializzo, il coprocessore?
										   // che gli ci sia rimasto qualche flag alterato?
										   // mah. un'ora ci ho perso a cercare un errore.
										   // un errore che non c'Š.
										   // dio tribudellone.
									x[4*c+p] = x[4*(c+1)+p];
									y[4*c+p] = y[4*(c+1)+p];
									z[4*c+p] = z[4*(c+1)+p];
								}
								nv[c] = nv[c+1];
								color[c] = color[c+1];
							}
						 }
						 psel = n-1;
						 break;
					case 'b':if (psel>0) {
							for (p=0; p<4; p++) {
								x[4*n+p] = x[4*psel+p];
								y[4*n+p] = y[4*psel+p];
								z[4*n+p] = z[4*psel+p];
							}
							nv[n] = nv[psel];
							color[n] = color[psel];
							for (c=psel; c>0; c--) {
								for (p=0; p<4; p++) {
									x[4*c+p] = x[4*(c-1)+p];
									y[4*c+p] = y[4*(c-1)+p];
									z[4*c+p] = z[4*(c-1)+p];
								}
								nv[c] = nv[c-1];
								color[c] = color[c-1];
							}
							for (p=0; p<4; p++) {
								x[p] = x[4*n+p];
								y[p] = y[4*n+p];
								z[p] = z[4*n+p];
							}
							nv[0] = nv[n];
							color[0] = color[n];
						 }
						 psel = 0;
						 break;
					case '+':if (ink<62) ink++; break;
					case '-':if (ink>1) ink--; break;
					case 32: if (v<4) {
							 x[n*4+v] = crx;
							 y[n*4+v] = cry;
							 z[n*4+v] = crz;
							 v++;
						 }
						 break;
					case 92: c = mp;
						 mp = -1;
						 mv = -1;
						 if (c>-1) break;
						 k = 1E99;
						 for (c=0; c<n; c++) {
							for (p=0; p<nv[c]; p++) {
								rx = x[4*c+p] - crx;
								ry = y[4*c+p] - cry;
								rz = z[4*c+p] - crz;
								rx = rx*rx + ry*ry + rz*rz;
								if (rx<k) {
									k = rx;
									mp = c;
									mv = p;
								}
							}
						 }
						 crx = x[4*mp+mv];
						 cry = y[4*mp+mv];
						 crz = z[4*mp+mv];
						 break;
					case  8: if (v)
							v--;
						 else {
							 if (n) {
								if (n==psel) psel = -1;
								n--;
							 }
						 }
						 break;
					case 13: if (ctrlkeys[0]&3) {
							fh = _creat ("POLYVERT.NCC", 0);
							if (fh>-1) {
								_write (fh, &n, 2);
								_write (fh, nv, n);
								_write (fh, x, 16*n);
								_write (fh, y, 16*n);
								_write (fh, z, 16*n);
								_write (fh, color, n);
								_close (fh);
							}
						 }
						 else {
							 if (n==1000) break;
							 if (v<3) break;
							 nv[n] = v;
							 color[n] = ink;
							 v = 0;
							 n++;
						 }
						 break;
					case 'C':for (c=0; c<n; c++) {
							aux = color[c] - 32;
							aux *= 1.1;
							aux += 32;
							if (aux<1) aux = 1;
							if (aux>62) aux = 62;
							color[c] = aux;
						 }
						 break;
					case 'z':
						backgr = backgr + 1;
						if (backgr == 5)
						backgr = 0;
						 break;
					case 'x':
						arrowcol = arrowcol + 1;
						if (arrowcol == 2)
						arrowcol = 0;
						 break;
					case 'c':for (c=0; c<n; c++) {
							aux = color[c] - 32;
							aux *= 0.9;
							aux += 32;
							if (aux<1) aux = 1;
							if (aux>62) aux = 62;
							color[c] = aux;
						 }
						 break;
					case 'L':for (c=0; c<n; c++) {
							aux = color[c];
							aux ++;
							if (aux>62) aux = 62;
							color[c] = aux;
						 }
						 break;
					case 'l':for (c=0; c<n; c++) {
							aux = color[c];
							aux --;
							if (aux<1) aux = 1;
							color[c] = aux;
						 }
						 break;
					case 'n':
						psel++;
						if (psel>=n) psel=-1;
						break;
					case 's':if (psel>-1) { psel = -1; break; }
						 k = 1E9;
						 psel = -1;
						 for (c=0; c<n; c++) {
							rx = 0; ry = 0; rz = 0;
							for (p=0; p<nv[c]; p++) {
								rx += x[4*c+p];
								ry += y[4*c+p];
								rz += z[4*c+p];
							}
							rx /= nv[c];
							ry /= nv[c];
							rz /= nv[c];
							xx = rx - crx;
							yy = ry - cry;
							zz = rz - crz;
							if (xx*xx+yy*yy+zz*zz<k) {
								k = xx*xx+yy*yy+zz*zz;
								psel = c;
							}
						 }
						 break;
					case '.':if (psel>-1) ink = color[psel];
						 break;
					case ':':if (psel>-1) color[psel] = ink;
						 break;
					case 't':for (c=0; c<n; c++) {
							if (nv[c]==4&&n<1000) {
								nv[c] = 3;
								for (p=n; p>c; p--) {
									color[p+1] = color[p];
									x[4*p+4] = x[4*p+0];
									x[4*p+5] = x[4*p+1];
									x[4*p+6] = x[4*p+2];
									x[4*p+7] = x[4*p+3];
									y[4*p+4] = y[4*p+0];
									y[4*p+5] = y[4*p+1];
									y[4*p+6] = y[4*p+2];
									y[4*p+7] = y[4*p+3];
									z[4*p+4] = z[4*p+0];
									z[4*p+5] = z[4*p+1];
									z[4*p+6] = z[4*p+2];
									z[4*p+7] = z[4*p+3];
									nv[p+1] = nv[p];
								}
								c++;
								n++;
								color[c] = color[c-1];
								x[4*c+0] = x[4*c-2];
								x[4*c+1] = x[4*c-1];
								x[4*c+2] = x[4*c-4];
								y[4*c+0] = y[4*c-2];
								y[4*c+1] = y[4*c-1];
								y[4*c+2] = y[4*c-4];
								z[4*c+0] = z[4*c-2];
								z[4*c+1] = z[4*c-1];
								z[4*c+2] = z[4*c-4];
								nv[c] = 3;
							}
						 }
						 break;
				}
			}
			if (mp>-1) {
				rx = x[4*mp+mv];
				ry = y[4*mp+mv];
				rz = z[4*mp+mv];
				x[4*mp+mv] = crx;
				y[4*mp+mv] = cry;
				z[4*mp+mv] = crz;
				if (!(ctrlkeys[0]&64)) {
					for (c=0; c<n; c++) {
						for (p=0; p<nv[c]; p++) {
							if (x[4*c+p]==rx) {
							 if (y[4*c+p]==ry) {
							  if (z[4*c+p]==rz) {
								x[4*c+p] = crx;
								y[4*c+p] = cry;
								z[4*c+p] = crz;
							  }
							 }
							}
						}
					}
				}
			}
			if (psel>-1) {
				xx = crx - pcrx;
				yy = cry - pcry;
				zz = crz - pcrz;
				for (p=0; p<nv[psel]; p++) {
					x[4*psel+p] += xx;
					y[4*psel+p] += yy;
					z[4*psel+p] += zz;
				}
			}
		}
		sprintf (temptesto, "CRH %d; %d; %d", (int)crx, (int)cry, (int)crz);
		Testo (5, 5, temptesto, 127, NORMALE);
		sprintf (temptesto, "PLG %d;", psel);
		if (psel != -1)	Testo (5, 15, temptesto, 127, NORMALE); 	// only draw polygon indicator text when needed.
		if (mp>-1) {
			rx = x[4*mp+mv];
			ry = y[4*mp+mv];
			rz = z[4*mp+mv];
			k = 25;
			for (c=0; c<n; c++) {
				for (p=0; p<nv[c]; p++) {
					if (x[4*c+p]==rx) {
					 if (y[4*c+p]==ry) {
					  if (z[4*c+p]==rz) {
						sprintf (temptesto, "VTX %d OF PLG %d", p, c);
						Testo (5, k, temptesto, 127, NORMALE);
						k += 10;
					  }
					 }
					}
				}
			}
		}

		if (showhelp == 1) {
			areaclear (adapted, 13, 14, 275, 21, 0, 0, 0);
			wrouthud (14, 15, NULL, "                           POLYVERT+ KEYLIST");
			areaclear (adapted, 13, 21, 275, 193, 0, 0, 72);
			areaclear (adapted, 13, 21, 275, 22, 0, 0, 254);
			wrouthud (14, 23, NULL, "SPACEBAR - PLACE VERTEX");
			wrouthud (14, 29, NULL, "ENTER - CREATE POLYGON (AFTER VERTEXES ARE PLACED)");
			wrouthud (14, 35, NULL, "BACKSPACE - CANSEL VERTEX OR LATEST POLYGON");
			wrouthud (14, 41, NULL, "V - SELECT VERTEX AT MOUSE");
			wrouthud (14, 47, NULL, "BACKSLASH - SELECT VERTEX AT CURSOR");
			wrouthud (14, 53, NULL, "+ - BRIGHTEN COLOUR OF CURSOR");
			wrouthud (14, 59, NULL, "- - DARKEN COLOUR OF CURSOR");
			wrouthud (14, 65, NULL, ". - CHANGE CURSOR COLOR TO COLOR OF SELECTED POLYGON");
			wrouthud (14, 71, NULL, ": - CHANGE SELECTED POLYGON COLOR TO CURSOR COLOR");
			wrouthud (14, 77, NULL, "C - DECREASE CONTRAST");
			wrouthud (14, 83, NULL, "SHIFT + C - INCREASE CONTRAST");
			wrouthud (14, 89, NULL, "L - DECREASE LUMINOSITY");
			wrouthud (14, 95, NULL, "SHIFT + L - INCREASE LUMINOSITY");
			wrouthud (14, 101, NULL, "S - SELECT POLYGON CLOSEST TO CURSOR");
			wrouthud (14, 107, NULL, "P - PUT SELECED POLYGON ABOVE THE OTHERS / SELECT LATEST POLYGON");
			wrouthud (14, 113, NULL, "B - PUT SELECTED POLYGON BELOW THE OTHERS / SELECT FIRST POLYGON");
			wrouthud (14, 119, NULL, "T - SPLIT 4-VERTEXED POLYGON INTO TWO 3-VERTEXED POLYGONS");
			wrouthud (14, 125, NULL, "H - WARP MODEL AWAY FROM THE CURSOR (CHANGE ALL COORDINATES)");
			wrouthud (14, 131, NULL, "O - SET CAMERA'S ORIGIN TO CURSOR'S LOCATION");
			areaclear (adapted, 13, 137, 275, 138, 0, 0, 254);
			wrouthud (14, 139, NULL, "SHIFT + ENTER - SAVE TO POLYVERT.NCC");
			wrouthud (14, 145, NULL, "Q - QUIT POLYVERT WITHOUT SAVING");
			areaclear (adapted, 13, 151, 275, 152, 0, 0, 254);
			wrouthud (14, 153, NULL, "CURSOR KEYS/PGUP-PGDOWN - MOVE CURSOR");
			wrouthud (14, 159, NULL, "CTRL + UP/DOWN - ALTER PRECISION OF CURSOR(!)");
			wrouthud (14, 165, NULL, "LEFT MOUSE BUTTON + MOUSE MOVEMENT - ZOOM IN/OUT");
			wrouthud (14, 171, NULL, "RIGHT MOUSE BUTTON + MOUSE MOVEMENT - TURN CAMERA AROUND");
			areaclear (adapted, 13, 177, 275, 178, 0, 0, 254);
			wrouthud (14, 179, NULL, "Z - CHANGE BACKGROUND COLOR       N - NEXT POLY.");
			wrouthud (14, 185, NULL, "X - CHANGE CURSOR COLOR");
			areaclear (adapted, 13, 191, 275, 192, 0, 0, 254);
			areaclear (adapted, 13, 192, 275, 200, 0, 0, 0);
			wrouthud (14, 193, NULL, "F1 - SHOW/HIDE THIS KEYLIST");
		}
		pcopy (adaptor, adapted);
		if (backgr == 0) {
		adaptor[320*my2d + mx2d] = 191; //plot the mouse BRIGHT
		} else {
		adaptor[320*my2d + mx2d] = 0; //plot the mouse DARK
		}
	} while (i!='q');
	_80_25_C ();
}
