/*

	Modulo principale di noctis.
	Funzioni di supervisione per il modulo di base.

*/

#include "defs.h"                //NICE defines
#include "noctis-d.h"
#include "noctis-0.h"
#include "noctis-2.h"
const double deg = M_PI / 180;

extern char exitflag;
extern char entryflag;
extern void planetary_main ();
extern float tiredness;

/* Roba importata da noctis-0.cpp per alleggerirlo */

char   nsnp 		= 1; // Nearstar-Not-Prepared
char   manual_target    = 0;
char   mt_string_char	= 0;
char   mt_coord		= 0;
char   manual_x_string [11];
char   manual_y_string [11];
char   manual_z_string [11];

/* Carica i parametri di viaggio per il pilota automatico. */

void fix_remote_target ()
{
	double dxx, dyy, dzz;

	status ("TGT FIXED", 105);
	dxx = dzat_x - ap_target_x;
	dyy = dzat_y - ap_target_y;
	dzz = dzat_z - ap_target_z;
	ap_target_initial_d = sqrt (dxx*dxx + dyy*dyy + dzz*dzz);
	requested_vimana_coefficient = 1000 * ap_target_initial_d;
	current_vimana_coefficient = 1000 * ap_target_initial_d;
	vimana_reaction_time = 0.01;
	ap_reached = 0;
	dxx = sqrt (ap_target_x * ap_target_x + ap_target_z * ap_target_z);
	dxx += fabs(ap_target_y * 30);
	if (dxx > 2E9) {
		status ("OUTOFRANGE", 105);
		ap_targetted = 0;
	}
	else
		ap_targetted = 1;
}

void fix_local_target ()
{
	double dxx, dyy, dzz;

	status ("TGT FIXED", 105);
	planet_xyz (ip_targetted);
	dxx = dzat_x - plx;
	dyy = dzat_y - ply;
	dzz = dzat_z - plz;
	ip_target_initial_d = sqrt (dxx*dxx + dyy*dyy + dzz*dzz);
	requested_approach_coefficient = 1000 * ip_target_initial_d;
	current_approach_coefficient = 1000 * ip_target_initial_d;
	reaction_time = 0.01;
}

/* Lampada alogena (ovvero il laser a diffusione interno alle zattere). */

void alogena ()
{
	float x[3], y[3], z[3];
	float lon, dlon, dlon_2;
	int pcol;

	dlon = M_PI / 5;

	if (fabs(cam_z)>1500) dlon *= fabs(cam_z) / 1500;

	dlon_2 = dlon / 2;

	pcol = 72 + ilightv;
	if (pcol > 100) pcol = 100;

	x[2] = -10; y[2] = 10; z[2] = -10;
	for (lon = 0; lon < 2*M_PI - dlon_2;) {
		x[0] = 15*sin(lon); y[0] = 15*cos(lon); z[0] = (y[0]-x[0])/2;
		lon += dlon; x[1] = 15*sin(lon); y[1] = 15*cos(lon); z[1] = (y[1]-x[1])/2;
		poly3d (x, y, z, 3, pcol);
		pcol += 2;
	}

	if (ilightv==1 && !elight)
		lens_flares_for (cam_x, cam_y, cam_z, -10, 10, -10, -50000, 2, 1, 0, 1, 1);
}

/* Quadranti di selezione del computer di bordo. */

void qsel (float *x, float *y, float *z, unsigned n, unsigned char c)
{
	setfx (1);

	x[0] -= 10; y[0] -= 10;
	x[1] += 10; y[1] -= 10;
	x[2] += 10; y[2] += 10;
	x[3] -= 10; y[3] += 10;

	poly3d (x, y, z, n, c);

	chgfx (0);

	x[0] += 10; y[0] += 10;
	x[1] -= 10; y[1] += 10;
	x[2] -= 10; y[2] -= 10;
	x[3] += 10; y[3] -= 10;

	poly3d (x, y, z, n, 68);

	resetfx ();
}

/* Tutti i riflessi sulle superfici riflettenti. */

void reflexes ()
{
	float x[4], y[4], z[4];

	setfx (1);

	lbxf++;

/*	if (depolarize) goto norefs;

	if (ilight) {
		x[0] = 3200; y[0] = -425; z[0] = -100;
		x[1] = 1000; y[1] = -415; z[1] = -100;
		x[2] = 3200; y[2] = -400; z[2] = -100;
		poly3d (x, y, z, 3, 1);
		x[0] = 3200; y[0] = -400; z[0] = -100;
		x[1] = 3175; y[1] = -425; z[1] = -100;
		x[2] = 3200; y[2] = +300; z[2] = -100;
		poly3d (x, y, z, 3, 5);
	}

	if (pos_z<-1750||beta<-45||beta>45) {
		x[0] = -1200; y[0] = -525; z[0] = -1800;
		x[1] = +1350; y[1] = -525; z[1] = -1800;
		x[2] = +1300; y[2] = -575; z[2] = -1850;
		poly3d (x, y, z, 3, 1);
		x[0] = +1350; y[0] = -525; z[0] = -1800;
		x[1] = +1350; y[1] = -525; z[1] = -4000;
		poly3d (x, y, z, 3, 1);
		x[0] = -1200; y[0] = +450; z[0] = -1800;
		x[1] = +1350; y[1] = +450; z[1] = -1800;
		x[2] = +1300; y[2] = +500; z[2] = -1850;
		poly3d (x, y, z, 3, 1);
		x[0] = +1350; y[0] = +450; z[0] = -1800;
		x[1] = +1350; y[1] = +450; z[1] = -4000;
		poly3d (x, y, z, 3, 1);
	} */

norefs:	if (ap_targetting||ip_targetting) goto noevid;

	if (ilight)
		setfx (1);
	else
		setfx (0);

	z[0] = 0;
	z[1] = 0;
	z[2] = 0;
	z[3] = 0;

	if (s_control) {
		x[0] = -66*30 - 10; y[0] = 50 * (s_control - 3) - 30;
		x[1] = -46*30;      y[1] = 50 * (s_control - 3) - 30;
		x[2] = -46*30;      y[2] = 50 * (s_control - 2) - 25;
		x[3] = -66*30 - 10; y[3] = 50 * (s_control - 2) - 25;
		qsel (x, y, z, 4, 1);
	}

	if (sys!=4) {
		x[0] = -45.65*30; y[0] = - 125;
		x[1] = -45.45*30; y[1] = - 125;
		x[2] = -45.45*30; y[2] =    75;
		x[3] = -45.65*30; y[3] =    75;
		if (stspeed)
			poly3d (x, y, z, 4, 32);
		else {
			if (ilight)
				poly3d (x, y, z, 4, 8);
			else
				poly3d (x, y, z, 4, 80);
		}
		if (s_command) {
			x[0] = 27*30*s_command - 72*30 + 10; y[0] = -130;
			x[1] = x[0] + 26*30 + 10;    	     y[1] = -130;
			x[2] = x[1]; 			     y[2] =  -75;
			x[3] = x[0]; 			     y[3] =  -75;
			qsel (x, y, z, 4, 1);
		}
	}

noevid:	lbxf--;

	setfx (0);
}

/* Schemi aggiuntivi per lo schermo del computer. */

void frame (float x, float y, float l, float h,
	    float borderwidth, unsigned char color)
{ // disegna una cornice rettangolare.
	float vx[4], vy[4], vz[4] = { 0, 0, 0, 0 };

	float x0 = cam_x;
	float y0 = cam_y;

	setfx (4);

	vx[0] = -l-borderwidth; vy[0] = -borderwidth;
	vx[1] = +l+borderwidth;	vy[1] = vy[0];
	vx[2] = +l+borderwidth; vy[2] = +borderwidth;
	vx[3] = -l-borderwidth; vy[3] = vy[2];

	cam_x = x0 - x;
	cam_y = y0 - y - h; poly3d (vx, vy, vz, 4, color);
	cam_y = y0 - y + h; poly3d (vx, vy, vz, 4, color);

	vx[0] = -borderwidth; vy[0] = -h-borderwidth;
	vx[1] = +borderwidth; vy[1] = vy[0];
	vx[2] = +borderwidth; vy[2] = +h+borderwidth;
	vx[3] = -borderwidth; vy[3] = vy[2];

	cam_y = y0 - y;
	cam_x = x0 - x - l; poly3d (vx, vy, vz, 4, color);
	cam_x = x0 - x + l; poly3d (vx, vy, vz, 4, color);

	cam_x = x0;
	cam_y = y0;

	resetfx ();
}

/* Croce di puntamento delle stelle. */

void pointer_cross_for (double xlight, double ylight, double zlight)
{
	double xx, yy, zz, z2, rx, ry, rz;

	xx = xlight - dzat_x;
	yy = ylight - dzat_y;
	zz = zlight - dzat_z;

	rx = xx * opt_pcosbeta + zz * opt_psinbeta;
	z2 = zz * opt_tcosbeta - xx * opt_tsinbeta;
	rz = z2 * opt_tcosalfa + yy * opt_tsinalfa;
	ry = yy * opt_pcosalfa - z2 * opt_psinalfa;

	if (rz>1) {
		rx /= rz; rx += x_centro;
		ry /= rz; ry += y_centro - 2;
		if (rx>10&&ry>10&&rx<310&&ry<190) {
			_SI = 320*(int)ry + rx;
			asm {	les di, dword ptr adapted
				add di, si
				mov al, 126
				mov es:[di-320*7-7], al
				mov es:[di-320*6-6], al
				mov es:[di-320*5-5], al
				mov es:[di-320*4-4], al
				mov es:[di+320*4+4], al
				mov es:[di+320*5+5], al
				mov es:[di+320*6+6], al
				mov es:[di+320*7+7], al
				mov es:[di-320*7+7], al
				mov es:[di-320*6+6], al
				mov es:[di-320*5+5], al
				mov es:[di-320*4+4], al
				mov es:[di+320*4-4], al
				mov es:[di+320*5-5], al
				mov es:[di+320*6-6], al
				mov es:[di+320*7-7], al }
		}
	}
}

/* Scrive sullo schermo del computer di bordo: una linea. */

void cline (int line, char *text)
{
	asm {   push ds
		push ds
		pop es
		lea di, ctb
		shl line, 7
		add di, line
		add di, 20
		lds si, dword ptr text }
byter:	asm {	cmp byte ptr [si], 0
		je endline
		movsb
		jmp byter }
endline:asm {   mov point, di
		pop ds }
}

/* Prosegue la linea suddetta. */

void other (char *text)
{
	asm {   push ds
		push ds
		pop es
		mov di, point
		lds si, dword ptr text }
byter:	asm {	cmp byte ptr [si], 0
		je endline
		movsb
		jmp byter }
endline:asm {   mov point, di
		pop ds }
}

/* Scrive il titolo di un controllo di sistema (sono 4 in tutto). */

void control (int line, char *text)
{
	asm {   push ds
		push ds
		pop es
		lea di, ctb
		dec line
		shl line, 7
		add di, line
		lds si, dword ptr text }
byter:	asm {	cmp byte ptr [si], 0
		je endline
		movsb
		jmp byter }
endline:asm 	pop ds
}

/* Scrive il titolo di un comando. */

void command (int nr, char *text)
{
	asm {   push ds
		push ds
		pop es
		lea di, ctb
		add di, 20
		dec nr
		mov ax, nr
		mov dx, 27
		mul dx
		add di, ax
		mov ax, 27
		lds si, dword ptr text }
byter:	asm {	cmp byte ptr [si], 0
		je endline
		movsb
		dec ax
		jnz byter }
endline:asm {	test ax, ax
		jz endwork
		mov byte ptr es:[di], 32
		inc di
		dec ax
		jnz endline }
endwork:asm  	pop ds
}

/* Cancella tutto lo schermo del computer di bordo. */

void clear_onboard_screen ()
{
	asm {
		push ds
		pop es
		lea di, ctb
		mov cx, 256
		xor ax, ax
		cld
	    rep stosw
	}
}

/* Gruppo di gestione del sistema operativo di bordo. */

unsigned char reset_signal = 55;		// Segnale di reset (=55).
char force_update = 0;				// Forza il refresh schermi.
char active_screen = 0;				// Schermo attualmente attivo.
unsigned char osscreen[2][7*21+1]; 		// Matrici degli schermi GOES.
int osscreen_cursor_x[2] = { 0, 0 };		// Posizione cursori (x).
int osscreen_cursor_y[2] = { 0, 0 };		// Posizione cursori (y).
char osscreen_textbuffer[7*21+1] = "";		// Buffer degli schermi.

void mslocate (int screen_id, int cursor_x, int cursor_y)
{ // Rilocazione cursore (multischermo).

	osscreen_cursor_x[screen_id] = cursor_x;
	osscreen_cursor_y[screen_id] = cursor_y;
}

void mswrite (int screen_id, char *text)
{ // Scrittura caratteri (multischermo).

	int  i, c = 0;
	char symbol;

	while ((symbol = text[c]) != 0) {
		if (symbol>=32 && symbol<=96) {
			i = 21 * osscreen_cursor_y[screen_id];
			i += osscreen_cursor_x[screen_id];
			osscreen[screen_id][i] = symbol;
			osscreen_cursor_x[screen_id]++;
			if (osscreen_cursor_x[screen_id] >= 21) {
				osscreen_cursor_x[screen_id] = 0;
				osscreen_cursor_y[screen_id]++;
			}
		}
		switch (symbol) {
			case 13:
				osscreen_cursor_x[screen_id] = 0;
				osscreen_cursor_y[screen_id]++;
				break;
			case 9:
				osscreen_cursor_x[screen_id] /= (3*4);
				osscreen_cursor_x[screen_id]++;
				osscreen_cursor_x[screen_id] *= (3*4);
				if (osscreen_cursor_x[screen_id] >= 21) {
					osscreen_cursor_x[screen_id] = 0;
					osscreen_cursor_y[screen_id]++;
				}
				break;
		}
		c++;
	}
}

char gnc_pos = 0;			// Numero carattere in command line.
long goesfile_pos = 0;			// Posizione sul file output di GOES.
char goesnet_command[120] = "_";	// Command line di GOES Net.
char *comm = "..\\DATA\\COMM.BIN";	// File di comunicazione dei moduli.
int roofspeed;
/* Congela la situazione (all'uscita dal programma o al run di un modulo). */

void freeze ()
{
	int fh = _creat (situation_file, 0);
	if (fh==-1) return;

	_write (fh, &sync, old_currentbin_length);
	_write (fh, &gnc_pos, 1);
	_write (fh, &goesfile_pos, 4);
	_write (fh, goesnet_command, 120);
	WRITEFILE (fh, &lastSnapshot, sizeof(lastSnapshot));				//36
	WRITEFILE (fh, &option_mouseLook, sizeof(option_mouseLook));				//40
	WRITEFILE (fh, &roofspeed, sizeof(roofspeed));
	_close (fh);
}

void run_goesnet_module ()
{ // Esecuzione di un modulo eseguibile della GOES Net.

	int ch;
	unsigned bqw = QUADWORDS;

	// Salva la situazione perch� alcuni moduli ne hanno bisogno.
	freeze ();

	// Libera circa 60 kilobytes per il lancio del modulo eseguibile.
	farfree (adapted);

	// Verifica comandi residenti.
	if (!memcmp (goesnet_command, "CLR", 3)) {
		remove (goesoutputfile);
		goto solong;
	}

	ch = _creat (goesoutputfile, 0);
	if (ch > -1) {
		_write (ch, "(UNKNOWN MODULE)", 16);
		_close (ch);
	}

	// Cancella l'ultimo carattere (che � il _ cursore) dalla command
	// line, poi aggiunge la redirezione sul file "goesfile.txt"
	goesnet_command[gnc_pos] = 0;
	strcat (goesnet_command, " >");
	strcat (goesnet_command, goesoutputfile);
	system (goesnet_command);

	// Ri-alloca l'area temporaneamente liberata.
solong:	adapted = (unsigned char far  *) farmalloc (sc_bytes);

	if (!adapted) {
		_80_25_C ();
		printf ("Sorry, GOES Net crashed.\n");
		printf ("System integrity compromised: any key to quit.\n\n");
		attendi_pressione_tasto();
		exit (0xFF);
	}
	else {
		QUADWORDS = 80 * 10;
		pclear (adaptor, 0);
		QUADWORDS = 16000 - 80 * 12;
		pcopy (adapted + 2*320, adaptor);
		QUADWORDS = bqw;
		// Reagisce alla presenza di dati nel file di comunicazione.
		ch = _open (comm, 0);
		if (ch > -1) {
			if (filelength(ch) == 2) {
				if (ap_reached) {
					if (pwr > 15000) {
						if (_read (ch, &ip_targetted, 2) == 2) {
							ip_targetted--;
							fix_local_target ();
							ip_targetting = 0;
							ip_reached = 0;
							ip_reaching = 1; // partenza automatica
						}
					}
				}
				else
					status ("NEED RECAL", 75);
			}
			if (filelength(ch) == 24) {
				_read (ch, &ap_target_x, 8);
				_read (ch, &ap_target_y, 8);
				if (_read (ch, &ap_target_z, 8) == 8) {
					ap_targetting = 0;
					extract_ap_target_infos ();
					fix_remote_target ();
					if (lithium_collector || manual_target)
						status ("CONFLICT", 50);
					else {
						if (pwr > 15000) {
							stspeed = 1; // partenza automatica
							nsnp = 1;
							ip_reached = 0;
							ip_targetted = -1;
						}
					}
				}
			}
			_close (ch);
			remove (comm);
		}
	}

	force_update = 1;
	goesfile_pos = 0;
}

/* Gruppo di tracciamento dello schermo del computer di bordo. */

void squaredot (float x, float y, float size, unsigned char color)
{ // questo � un quadratino, un "pixel" del computer di bordo.
	float vx[4], vy[4], vz[4] = { 0, 0, 0, 0 };

	vx[0] = x-size;
	vx[1] = x+size;
	vx[2] = x+size;
	vx[3] = x-size;

	vy[0] = y-size;
	vy[1] = y-size;
	vy[2] = y+size;
	vy[3] = y+size;

	poly3d (vx, vy, vz, 4, color);
}

long pp[32]= { 0x00000001, 0x00000002, 0x00000004, 0x00000008,
	       0x00000010, 0x00000020, 0x00000040, 0x00000080,
	       0x00000100, 0x00000200, 0x00000400, 0x00000800,
	       0x00001000, 0x00002000, 0x00004000, 0x00008000,
	       0x00010000, 0x00020000, 0x00040000, 0x00080000,
	       0x00100000, 0x00200000, 0x00400000, 0x00800000,
	       0x01000000, 0x02000000, 0x04000000, 0x08000000,
	       0x10000000, 0x20000000, 0x40000000, 0x80000000 };

void digit_at (char digit, float x, float y, float size, unsigned char color, char shader)
{ // questo � un carattere alfanumerico...
	unsigned char huge *prev_txtr = txtr;

	float vx[4], vy[4], vz[4] = { 0, 0, 0, 0 };

	float size_x_left = size * -1.5;
	float size_y_left = size * -2.0;
	float size_x_right= size * +4.0;
	float size_y_right= size * +8.0;

	long prev_xs = XSIZE;
	long prev_ys = YSIZE;

	int  n, m, d, i;

	char pixel_color = color % 64;
	char map_base = (color >> 6) << 6;

	if (reset_signal > 100) {
		pixel_color -= (reset_signal - 100);
		if (pixel_color < 0) pixel_color = 0;
	}

	if (digit>32 && digit<=96) {
		txtr = p_surfacemap;
		d = (digit - 32) * 36;
		for (n = 0; n < 36; n++) {
			i = 256 * n - 5;
			txtr[i-1] = 0; // evita aliasing a fine scanline.
			for (m = 0; m < 32; m++) {
				if (digimap2[n+d] & pp[m])
					txtr[i] = pixel_color;
				else
					txtr[i] = 0;
				i++;
			}
			if (shader) pixel_color--;
		}
		txtr[256 * 36 - 6] = 0; // evita aliasing a fine matrice.
		vx[3] = x + size_x_left;
		vx[0] = x + size_x_right;
		vx[1] = x + size_x_right;
		vx[2] = x + size_x_left;
		vy[3] = y + size_y_left;
		vy[0] = y + size_y_left;
		vy[1] = y + size_y_right;
		vy[2] = y + size_y_right;
		setfx (2);
		XSIZE = 512; YSIZE = 576;
		polymap (vx, vy, vz, 4, map_base);
		XSIZE = prev_xs; YSIZE = prev_ys;
		txtr = prev_txtr;
		resetfx ();
	}
}

void screen ()
{
	float x, y;
	int   c, p, t = 0;

	#define blinkscolor 127
	#define screencolor 127

	if (ap_targetting) return;

	if (pwr <= 15000 && !charge) {
		fast_srand (secs);
		c = fast_random(3) + 2;
		if (clock() % c) return;
	}

	x = cam_x;
	y = cam_y;

	for (p=-2; p<2; p++)
		for (c=-64; c<64; c++) {
			cam_x = x - c*30;
			if (c<-44) {
				if (s_control || menusalwayson) {
					cam_x += 50;
					cam_y = y - p*50;
				}
				else {
					t += 19;
					c += 19;
					goto passby;
				}
			}
			if (c==-44) {
				if (s_command || menusalwayson) {
					if (p==-2 || menusalwayson)
						cam_y = y - p*50;
					else {
						t += 108;
						break;
					}
				}
				else {
					if (!infoarea || p==-2) {
						t += 108;
						break;
					}
					else
						cam_y = y - p*46 - 12;
				}
			}
			if (ctb[t]>='A'&&ctb[t]<='Z')
				digit_at (ctb[t], -6, -16, 4, blinkscolor - 12*(clock()%6), 1);
			else {
				if (ctb[t]>='a'&&ctb[t]<='z')
					digit_at (ctb[t] - 32, -6, -16, 4, screencolor, 1);
				else
					digit_at (ctb[t], -6, -16, 4, screencolor, 1);
			}
		passby:	t++;
		}

	cam_x = x;
	cam_y = y;
}

/* Disegna la mappa di superficie al momento in cui si vuole sbarcare. */

void show_planetary_map ()
{
	char 	is_moon;
	int 	lat, lon, i, j, p;

	if (nearstar_p_owner[ip_targetted] > -1)
		is_moon = 1;
	else
		is_moon = 0;

	lon = landing_pt_lon - 22;
	for (i = 0; i < 48; i++) {
		while (lon > 359) lon -= 360;
		while (lon <   0) lon += 360;
		lat = landing_pt_lat - 14;
		for (j = 0; j < 32; j++) {
			p = ((j + 9) << 8) + i + 14;
			if (lat > 0 && lat < 120) {
				ptr = lat * 360 + lon;
				if (is_moon)
					p_surfacemap[p] = s_background[ptr] + 128;
				else
					p_surfacemap[p] = p_background[ptr] + 192;
			}
			else
				p_surfacemap[p] = 4;
			lat++;
		}
		lon++;
	}
}

/* Disegna l'astrozattera. (Quella che si sta usando, vista dall'interno.)
   Assume anche il compito di decifrare i comandi da tastiera per GOESNet. */

int goesk_a = -1;
int goesk_e = -1;

void vehicle (float opencapcount)
{
	int   n, c, i, j, k, screenfile;
	char  short_text[11];
	unsigned char chcol;

	float backup_cam_x, backup_cam_z;
	float backup_beta = beta;
	float chry;

	float vx[4], vy[4], vz[4];
	float osscreen_x[4], osscreen_z[4];
	float osscreen_y[4] = { -20*15,  14*15,  14*15, -20*15 };

	if (elight) {
		_fmemset (osscreen[0], 0, 7*21);
		_fmemset (osscreen[1], 0, 7*21);
	}

	// Tracciamento delle cupole panoramiche.
	// Quando non si stanno chiudendo per formare la capsula.

	if (!opencapcount) {
		cam_z += 3100;
			cam_y -= 550;
				polycupola (0, 0);
				setfx(0); cupola (0, 8); resetfx();
			cam_y += 550;
			if (!ontheroof) {
				polycupola (+1, 0);
				setfx(0); cupola (+1, 8); resetfx();
			}
		cam_z -= 3100;
	}

	// Resto dello scafo.
	// Se si � in terrazza, ci si ferma qui, dopo aver disegnato
	// la cupola superiore, che dev'essere sovraimpressa allo scafo.

	if (depolarize) {
		setfx (2);
		drawpv (vehicle_handle, 2, 3, 0.0, 0.0, 0.0, 0);
		resetfx ();
	}
	else
		drawpv (vehicle_handle, 2, 2, 0.0, 0.0, 0.0, 0);

	if (ontheroof) {
		cam_z += 3100;
			polycupola (+1, 0);
			setfx(0);
			cupola (+1, 8);
			resetfx();
		cam_z -= 3100;
		return;
	}

	// Intercettazione tasti (prioritaria) per GOESNet.

	if (force_update || (active_screen == 0 && tasto_premuto())) {
		if (!force_update) {
			goesk_a = -1;
			c = attendi_pressione_tasto();
			if (!c) {
				c = attendi_pressione_tasto();
				goesk_e = c;
				if (c == 0x47) {
					goesnet_command[0] = '_';
					goesnet_command[1] = 0;
					gnc_pos = 0;
					goesk_e = -1;
				}
			}
			else {
				switch (c) {
				       case 27:	goesk_a = c; // passa la keystroke
						break;
				       case 8:  if (gnc_pos > 0) {
							goesnet_command[gnc_pos - 1] = '_';
							goesnet_command[gnc_pos] = 0;
							gnc_pos--;
						}
						break;
				       case 13: run_goesnet_module ();
						n = 0;
						if (!memcmp (goesnet_command, "CAST", 4)) {
							i = 0;
							while (i < gnc_pos) {
								if (goesnet_command[i] == ':') {
									n = i + 1;
									goto reset_cli;
								}
								i++;
							}
						}
					   reset_cli:
						goesnet_command[n] = '_';
						goesnet_command[n+1] = 0;
						gnc_pos = n;
				}
				// trasformazione virgolette in apici:
				if (c == 34) c = 39;
				// trasformazione minuscole in maiuscole:
				if (c >= 'a' && c <= 'z') c -= 32;
				// controllo caratteri non validi:
				if (c == 36) goto non_valido; // dollaro
				if (c == 38) goto non_valido; // e commerciale
				if (c == 60) goto non_valido; // minore
				if (c == 62) goto non_valido; // maggiore
				if (c == 95) goto valido;     // sottolinea
				// inserimento caratteri validi:
				if (c >= 32 && c <= 90 && gnc_pos < 83) {
				    valido:
					goesnet_command[gnc_pos] = c;
					goesnet_command[gnc_pos+1] = '_';
					goesnet_command[gnc_pos+2] = 0;
					gnc_pos++;
				}
			    non_valido:
			}
		}
		_fmemset (osscreen[0]+3*21, 0, 4*21);
		mslocate (0, 0, 3); mswrite (0, goesnet_command);
	}

	// Intercettazione tasti (prioritaria) per lo schermo "STARMAP TREE".

	if (force_update || (active_screen == 1 && tasto_premuto())) {
		if (!force_update) {
		    krep1:
			c = attendi_pressione_tasto();
			goesk_a = c;
			if (!c)	{
				goesk_a = -1;
				c = attendi_pressione_tasto();
				goesk_e = c;
				switch (c) {
				     case 0x4F:
				     case 0x76:
				     case 0x91: screenfile = _open (goesoutputfile, 0);
						if (screenfile > -1) {
							goesfile_pos = filelength(screenfile) - 7*21;
							if (goesfile_pos < 0) goesfile_pos = 0;
							_close (screenfile);
						}
						goesk_e = -1;
						break;
				     case 0x47:
				     case 0x84:
				     case 0x8D: goesfile_pos = 0;
						goesk_e = -1;
						break;
				       case 80: goesfile_pos += 21;
						goesk_e = -1;
						break;
				       case 72: goesfile_pos -= 21;
						if (goesfile_pos < 0) goesfile_pos = 0;
						goesk_e = -1;
						break;
				       case 0x51: goesfile_pos += 21*7;
						goesk_e = -1;
						break;
				       case 0x49: goesfile_pos -= 21*7;
						if (goesfile_pos < 0) goesfile_pos = 0;
						goesk_e = -1;
						break;
				}
			}
			if (tasto_premuto()) goto krep1;
		}
		_fmemset (osscreen[1], 0, 21*7);
		screenfile = _open (goesoutputfile, 0);
		if (screenfile > -1) {
			lseek (screenfile, goesfile_pos, SEEK_SET);
			n = _read (screenfile, osscreen[1], 7*21);
			osscreen[1][n] = 0;
			_close (screenfile);
		}
	}

	// Intercettazione tasti (prioritaria) per la planetary map.

	if (active_screen == 2 && tasto_premuto()) {
	    krep2:
		c = attendi_pressione_tasto();
		goesk_a = c;
		if (!c)	{
			goesk_a = -1;
			c = attendi_pressione_tasto();
			goesk_e = c;
			if (landing_point) {
				switch (c) {
				       case 77: landing_pt_lon++;
						if (landing_pt_lon >= 360) landing_pt_lon -= 360;
						goesk_e = -1;
						break;
				       case 75: landing_pt_lon--;
						if (landing_pt_lon < 0) landing_pt_lon += 360;
						goesk_e = -1;
						break;
				       case 80: landing_pt_lat++;
						if (landing_pt_lat > 119) landing_pt_lat = 119;
						goesk_e = -1;
						break;
				       case 72: landing_pt_lat--;
						if (landing_pt_lat < 1) landing_pt_lat = 1;
						goesk_e = -1;
						break;
				     case 0x74: landing_pt_lon += 3;
						if (landing_pt_lon >= 360) landing_pt_lon -= 360;
						goesk_e = -1;
						break;
				     case 0x73: landing_pt_lon -= 3;
						if (landing_pt_lon < 0) landing_pt_lon += 360;
						goesk_e = -1;
						break;
				     case 0x91: landing_pt_lat += 3;
						if (landing_pt_lat > 119) landing_pt_lat = 119;
						goesk_e = -1;
						break;
				     case 0x8D: landing_pt_lat -= 3;
						if (landing_pt_lat < 1) landing_pt_lat = 1;
						goesk_e = -1;
				}
			}
		}
		else {
			if (landing_point) {
				if (c == 13) {
					land_now = 1;
					goesk_a = -1;
				}
				if (c == 27) {
					landing_point = 0;
					status ("CANCELLED", 50);
					goesk_a = -1;
				}
				/* Unit� di debugging dell'albedo
				unsigned char far *ov=(unsigned char far*)objectschart;
				if (c == 'b') {
					ov[(18 + 60*360) / 2] += 4;
					p_background[18 + 60*360] ++;
				}
				if (c == 'd') {
					ov[(18 + 60*360) / 2] -= 4;
					p_background[18 + 60*360] --;
				}*/
			}
		}
		if (tasto_premuto()) goto krep2;
	}

	// Tracciamento degli schermi di GOESNet.
	// Si tratta dei primi due schermi sulla paratia destra.

	H_MATRIXS = 6; V_MATRIXS = 3;
	change_txm_repeating_mode ();
	txtr = p_surfacemap + 256 * 8 + 16;

	osscreen_z[0] = -104*15;
	osscreen_z[1] = -104*15;
	osscreen_z[2] = -154*15;
	osscreen_z[3] = -154*15;
	osscreen_x[0] = +236*15;
	osscreen_x[1] = +236*15;
	osscreen_x[2] = +236*15;
	osscreen_x[3] = +236*15;

	vx[0] = 236*15; vx[1] = 236*15; vx[2] = 236*15; vx[3] = 236*15;
	vy[0] = -22*15; vy[1] = -23*15; vy[2] = -23*15; vy[3] = -22*15;
	vz[0] =-104*15; vz[1] =-104*15; vz[2] =-108*15; vz[3] =-108*15;

	n = 0; while (n < 2) {
		poly3d (osscreen_x, osscreen_y, osscreen_z, 4, 68);
		beta += 90; if (beta > 359) beta -= 360;
		change_angle_of_view ();
		backup_cam_x = cam_x;
		backup_cam_z = cam_z;
		cam_z = backup_cam_x - 236*15;
		chry = -18*15;
		chcol = 152;
		k = 0;
		for (j = 0; j < 7; j++) {
			cam_x = - backup_cam_z - 105*15;
			for (i = 0; i < 21; i++) {
				c = osscreen[n][k + i];
				if (c > 48 && c < 91)
					digit_at (c, 0, chry, 5.5, chcol, 0);
				else {
					if (c == '(') chcol = 191;
					if (c != '$' && c != '[' && c != ']'
					 && c != '*' && c != '&' && c != '_')
						digit_at (c, 0, chry, 5.5, chcol, 0);
					else
						digit_at (c, 0, chry, 6.5, 138, 0);
					if (c == ')') chcol = 152;
				}
				cam_x -= 2.35*15;
			}
			k += 21;
			chry += 4.5*15;
		}
		cam_x = backup_cam_x;
		cam_z = backup_cam_z;
		beta = backup_beta;
		change_angle_of_view ();
		if (n == active_screen)
			poly3d (vx, vy, vz, 4, 63);
		else
			poly3d (vx, vy, vz, 4, 00);
		cam_z += 54 * 15;
		n++;
	}

	// Tracciamento della mappa planetaria.
	// Dopo gli schermi di GOES.

	H_MATRIXS = 3; V_MATRIXS = 2;
	change_txm_repeating_mode ();

	if (landing_point) {
		show_planetary_map ();
		polymap (osscreen_x, osscreen_y, osscreen_z, 4, 0);
		sprintf (short_text, "LQ %03d:%03d", landing_pt_lon, landing_pt_lat);
		status (short_text, 10);
	}
	else
		poly3d (osscreen_x, osscreen_y, osscreen_z, 4, 4);

	#define surface_crosshair_x_shift	+25
	#define surface_crosshair_y_shift	-10
	#define surface_crosshair_x_spacing	+11
	#define surface_crosshair_y_spacing	+10

	setfx (2);
	stick3d (osscreen_x[0], osscreen_y[0], osscreen_z[0] - 27 * 15 - surface_crosshair_x_spacing + surface_crosshair_x_shift,
		 osscreen_x[0], osscreen_y[1], osscreen_z[0] - 27 * 15 - surface_crosshair_x_spacing + surface_crosshair_x_shift);
	stick3d (osscreen_x[0], osscreen_y[0], osscreen_z[0] - 27 * 15 + surface_crosshair_x_spacing + surface_crosshair_x_shift,
		 osscreen_x[0], osscreen_y[1], osscreen_z[0] - 27 * 15 + surface_crosshair_x_spacing + surface_crosshair_x_shift);
	stick3d (osscreen_x[0], osscreen_y[0] + 17 * 15 - surface_crosshair_y_spacing + surface_crosshair_y_shift, osscreen_z[0],
		 osscreen_x[0], osscreen_y[0] + 17 * 15 - surface_crosshair_y_spacing + surface_crosshair_y_shift, osscreen_z[2]);
	stick3d (osscreen_x[0], osscreen_y[0] + 17 * 15 + surface_crosshair_y_spacing + surface_crosshair_y_shift, osscreen_z[0],
		 osscreen_x[0], osscreen_y[0] + 17 * 15 + surface_crosshair_y_spacing + surface_crosshair_y_shift, osscreen_z[2]);
	resetfx();

	if (active_screen == 2)
		poly3d (vx, vy, vz, 4, 63);
	else
		poly3d (vx, vy, vz, 4, 00);

	// Fine tracciamento schermi.

	cam_z -= 2 * 54 * 15;
	txtr = p_background;
	if (force_update) force_update = 0;

	// Tracciamento della lampada interna.

	cam_x -= 3395;
	cam_y +=  480;
	cam_z +=  200;
	alogena ();
	cam_x += 3395;
	cam_y -=  480;
	cam_z -=  200;

	// Tracciamento delle cupole panoramiche.
	// Quando si stanno chiudendo per formare la capsula.

	if (opencapcount) {
		chry = cam_y;
		cam_z += 3100;
			cam_y = chry + opencapcount * 9.55 - 550;
				polycupola (-opencapcount/85, 0);
				setfx(0); cupola (-opencapcount/85, 8); resetfx();
			cam_y = chry - opencapcount * 9.55;
			if (!ontheroof) {
				polycupola (+1, 0);
				setfx(0); cupola (+1, 8); resetfx();
			}
		cam_z -= 3100;
		cam_y = chry;
	}
}

/* Disegna un'astrozattera, vista dall'esterno. */

void other_vehicle_at (double ovhx, double ovhy, double ovhz)
{
	cam_x = - ovhx;
	cam_y = - ovhy;
	cam_z = - ovhz;

	cam_z += 3100;
	setfx (2);
	if (ovhy>-375) cupola (+1, 8);
	if (ovhy<+375) cupola (-1, 8);
	resetfx ();
	cam_z -= 3100;

	cam_x = 0;
	cam_y = 0;
	cam_z = 0;

	drawpv (vehicle_handle, 0, 0, ovhx, ovhy, ovhz, 1);

	cam_x = - ovhx;
	cam_y = - ovhy;
	cam_z = - ovhz;

	cam_z += 3100;
	setfx (2);
	if (ovhy>+375) cupola (+1, 8);
	if (ovhy<-375) cupola (-1, 8);
	resetfx ();
	cam_z -= 3100;

	lens_flares_for (cam_x, cam_y, cam_z, 3225, 0, 0, -5e5, 3, 1, 1, 1, 1);
	lens_flares_for (cam_x, cam_y, cam_z, -3225, 0, 0, -5e5, 3, 1, 1, 1, 1);
	lens_flares_for (cam_x, cam_y, cam_z, 3225, 0, -6150, -5e5, 3, 1, 1, 1, 1);
	lens_flares_for (cam_x, cam_y, cam_z, -3225, 0, -6150, -5e5, 3, 1, 1, 1, 1);
}


/* Fine roba importata da noctis-0.cpp */

/* Variabili globali di uso generico. */
int moviedelay;
char moviestat = 0;
int moviefsec = 1;
extern int movieexists;
extern char movie;
extern int movienr;
extern char movieflashoff;
extern int moviefscap;
extern int moviestime;
extern int moviedeck;
int moviepaused=0;

unsigned char far * ctrlkeys     = (unsigned char far *) 0x00000417;

char   aso_countdown = 100;	// contatore per la funzione "autoscreenoff"
long   tgt_label_pos = -1;	// posizione etichetta target selezionato
int    tgts_in_show = 0;	// targets correntemente visualizzati

/* Dati gestione cartografia. */

char   targets_in_range = 0;
long   sm_consolidated  = 0;

char   target_name[4][24];

char   iptargetstring[11];
char   iptargetchar = 0;
char   iptargetplanet;
char   iptargetmoon;

double ap_target_id = 12345, ap_target_previd = 54321;
double current_planet_id = 12345, prev_planet_id = 54321;
char   labstar = 0, labplanet = 0, labstar_char = 0, labplanet_char = 0;
long   star_label_pos = -1, planet_label_pos = -1;

double star_id = 12345;
char   star_label[25] = "UNKNOWN STAR / CLASS ...";
char   star_no_label[24] = "UNKNOWN STAR / CLASS ...";

double planet_id = 12345;
char   planet_label[25] = "NAMELESS PLANET / N. ...";
char   planet_no_label[24] = "NAMELESS PLANET / N. ...";
char   moon_no_label[24] =   "NAMELESS MOON #../../...";

char   *sr_message = "SYSTEM RESET";

void update_star_label ()
{
			if (ap_targetted == -1)
				strcpy (star_label, "- DIRECT PARSIS TARGET -");
			else {
				ap_target_id = ap_target_x / 100000 * ap_target_y / 100000 * ap_target_z / 100000;
				if (ap_target_id != ap_target_previd) {
					ap_target_previd = ap_target_id;
					star_label_pos = search_id_code (ap_target_id, 'S');
					if (star_label_pos != -1) {
						smh = _open (starmap_file, 0);
						lseek (smh, star_label_pos, SEEK_SET);
						_read (smh, &star_id, 32);
						_close (smh);
					}
					else
						memcpy (star_label, star_no_label, 24);
					srand (ap_target_id); sprintf (star_label+21, "S%02d", random (star_classes));
				}
			}
}

void update_planet_label ()
{
			current_planet_id = nearstar_identity + ip_targetted + 1;
			if (current_planet_id != prev_planet_id) {
				prev_planet_id = current_planet_id;
				planet_label_pos = search_id_code (current_planet_id, 'P');
				if (planet_label_pos != -1) {
					smh = _open (starmap_file, 0);
					lseek (smh, planet_label_pos, SEEK_SET);
					_read (smh, &planet_id, 32);
					_close (smh);
				}
				else {
					if (nearstar_p_owner[ip_targetted] == -1)
						memcpy (planet_label, planet_no_label, 24);
					else {
						memcpy (planet_label, moon_no_label, 24);
						sprintf (planet_label+15, "%02d", nearstar_p_moonid[ip_targetted] + 1);
						sprintf (planet_label+18, "%02d", nearstar_p_owner[ip_targetted] + 1);
						planet_label[17] = '/';
						planet_label[20] = '&';
					}
				}
				sprintf (planet_label+21, "P%02d", ip_targetted + 1);
			}
}

/* Controllo di volo (Flight Control System). */

void fcs ()
{
	int n;

	if (ip_targetted != -1) {
		cline (1, "local target: ");
		if (nearstar_p_owner[ip_targetted] > -1) {
			other ("moon #");
			other (alphavalue(nearstar_p_moonid[ip_targetted] + 1));
			other (" of ");
			n = nearstar_p_owner[ip_targetted];
		}
		else
			n = ip_targetted;
		other (ord[n + 1]);
		other (" planet. ");
		other (planet_description[nearstar_p_type[ip_targetted]]);
	}

	if (ap_targetted) {
		if (ap_targetted == 1) {
			cline (2, "remote target: class ");
			other (alphavalue(ap_target_class));
			other (" star; ");
			other (star_description[ap_target_class]);
		}
		else
			cline (2, "direct parsis target: non-star type.");
	}
	else
		cline (2, "no remote target selected");

	cline (3, "current range: elapsed ");
	xx = (float) pwr - 15000; if (xx<0) xx = 0;
	other (alphavalue(xx));
	other (" kilodyams, remaining lithium: ");
	other (alphavalue(charge));
	other (" grams.");

	command (1, "set remote target");

	if (stspeed)
		command (2, "stop vimana flight");
	else
		command (2, "start vimana flight");

	if (landing_point)
		command (4, "cancel landing request");
	else
		command (4, "deploy surface capsule");

	if (ip_targetted==-1||ip_reached)
		command (3, "set local target");
	else {
		if (ip_reaching)
			command (3, "stop fine approach");
		else {
			command (3, "start fine approach");
			command (4, "clear local target");
		}
	}
}

/* Comandi dell'FCS. */

void fcs_commands ()
{
	switch (s_command) {
		case 1: if (stspeed || manual_target) {
				status ("CONFLICT", 50);
				break;
			}
			status ("TGT-REMOTE", 50);
			ap_targetting = 1;
			ap_targetted = 0;
			break;
		case 2:	if (stspeed) {
				stspeed = 0;
				status ("IDLE", 50);
			}
			else {
				if (lithium_collector || manual_target) {
					status ("CONFLICT", 50);
					break;
				}
				if (pwr > 15000) {
					stspeed = 1;
					if (ap_targetted) {
						nsnp = 1;
						ap_reached = 0;
						ip_reached = 0;
						ip_targetted = -1;
					}
				}
			}
			break;
		case 3: if (ip_reached||ip_targetted==-1) {
				if (ap_reached) {
					status ("TGT-LOCAL", 50);
					ip_targetted = -1;
					ip_targetting = 1;
					ip_reaching = 0;
					ip_reached = 0;
					iptargetchar = 0;
				}
				else
					status ("NEED RECAL", 75);
			}
			else {
				if (ip_reaching) {
					status ("IDLE", 50);
					ip_targetted = -1;
					ip_reaching = 0;
					ip_reached = 1;
				}
				else {
					if (pwr > 15000) {
						ip_reaching = 1;
						status ("CONFIRM", 50);
					}
				}
			}
			break;
		case 4: if (!ip_reaching&&ip_targetted!=-1) {
				if (!ip_reached) {
					ip_targetted = -1;
					status ("TGT REJECT", 50);
				}
				else {
					landing_point = 1 - landing_point;
					if (landing_point) {
						if (nearstar_p_type[ip_targetted] == 0
						  ||nearstar_p_type[ip_targetted] == 6
						  ||nearstar_p_type[ip_targetted] >= 9) {
							status ("IMPOSSIBLE", 50);
							landing_point = 0;
						}
						else {
							status ("SURFACE", 50);
							landing_pt_lon = 0;
							landing_pt_lat = 60;
						}
					}
					else
						status ("IDLE", 50);
				}
			}
			else
				status ("ERROR", 50);
	}
}

/* Dispositivi di bordo: men� principale e quattro sottomen�. */

void devices ()
{
	double 	parsis_x, parsis_y, parsis_z;
	int 	n, sp;
	long 	lsecs;

	switch (dev_page) {
		case 0: // sub menu
			command (1, "navigation instruments");
			command (2, "miscellaneous");
			command (3, "galactic cartography");
			command (4, "emergency functions");
			cline (3, "SELECT ARGUMENT");
			break;
		case 1: // navigation status
			if (field_amplificator) {
				command (1, "STARFIELD AMPLIFICATOR");
				cline (1, "starfield amplification active, ");
			}
			else {
				command (1, "starfield amplificator");
				cline (1, "starfield amplification disabled, ");
			}
			if (anti_rad) {
				command (4, "FORCE RADIATIONS LIMIT");
				other ("high-radiation fields are avoided.");
			}
			else {
				command (4, "force radiations limit");
				other ("high-radiation fields are ignored.");
			}
			if (sync) {
				if (ip_targetted!=-1 && ip_reached) {
					sp = 0;
					for (n = 0; n < ip_targetted; n++) {
						if (nearstar_p_type[n] != -1)
							sp++;
					}
					cline (2, "tracking status: performing ");
					if (sync==1) {
						other ("fixed-point chase.");
						command (3, "fixed-point chase");
					}
					if (sync==2) {
						other ("far chase.");
						command (3, "far chase");
					}
					if (sync==3) {
						other ("syncrone orbit.");
						command (3, "syncrone orbit");
					}
					if (sync==4) {
						other ("high-speed orbit.");
						command (3, "high-speed orbit");
					}
					if (sync==5) {
						other ("near chase.");
						command (3, "near chase");
					}
				}
				else {
					cline (2, "tracking status: disconnected.");
					if (sync==1)
						command (3, "fixed-point chase");
					if (sync==2)
						command (3, "far chase");
					if (sync==3)
						command (3, "syncrone orbit");
					if (sync==4)
						command (3, "high-speed orbit");
					if (sync==5)
						command (3, "near chase");
				}
			}
			else {
				cline (2, "tracking status: inactive.");
				command (3, "drive tracking mode");
			}
			if (pl_search) {
				command (2, "LOCAL PLANETS FINDER");
				xx = nearstar_x - dzat_x;
				yy = nearstar_y - dzat_y;
				zz = nearstar_z - dzat_z;
				xx = sqrt (xx*xx+yy*yy+zz*zz);
				if (xx<20000) {
					if (nearstar_nop) {
						cline (3, "planet finder report: system has ");
						other (alphavalue(nearstar_nop));
						other (" ");
						if (nearstar_class==9)
							other ("proto");
						if (nearstar_nop==1)
							other ("planet, and ");
						else
							other ("planets, and ");
						other (alphavalue(nearstar_nob - nearstar_nop));
						other (" minor bodies. ");
						other (alphavalue(nearstar_labeled));
						other (" labeled out of ");
						other (alphavalue(nearstar_nob));
						other (".");
					}
					else
						cline (3, "planet finder report: there are no major bodies in this system.");
				}
				else
					cline (3, "planet finder report: no stellar systems within remote sensors range.");
			}
			else
				command (2, "local planets finder");
			break;
		case 2: // miscellaneous devices status
			if (ilightv==1)
				command (1, "internal light on");
			else
				command (1, "internal light off");
			command (2, "remote target data");
			command (3, "local target data");
			command (4, "environment data");
			if (data == 1) command (2, "REMOTE TARGET DATA");
			if (data == 2) command (3, "LOCAL TARGET DATA");
			if (data == 3) command (4, "ENVIRONMENT DATA");
			break;
		case 3: // galactic cartography status
			if (labstar)
				command (1, "assign star label");
			else {
				if (star_label_pos > -1)
					command (1, "remove star label");
				else
					command (1, "label star as...");
			}
			if (labplanet)
				command (2, "assign planet label");
			else {
				if (planet_label_pos > -1)
					command (2, "remove planet label");
				else
					command (2, "label planet as...");
			}
			if (targets_in_range)
				command (3, "quit targets in range");
			else
				command (3, "show targets in range");
			if (manual_target)
				command (4, "(enter coordinates)");
			else
				command (4, "set target to parsis");
			cline (1, "epoc ");
			other (alphavalue((long)epoc)); other (" triads ");
			lsecs = secs; lsecs -= lsecs % 1000000L; lsecs /= 1000000L;
			other (alphavalue(lsecs)); other (",");
			lsecs = secs; lsecs %= 1000000L; lsecs -= lsecs % 1000; lsecs /= 1000;
			other (alphavalue(lsecs)); other (",");
			lsecs = secs; lsecs %= 1000;
			other (alphavalue(lsecs));
			cline (2, "parsis universal coordinates: ");
			asm {	fld dzat_x
				frndint
				fstp parsis_x
				fld dzat_y
				frndint
				fstp parsis_y
				fld dzat_z
				frndint
				fstp parsis_z }
			other (alphavalue(parsis_x));
			other (";");
			other (alphavalue(-parsis_y));
			other (";");
			other (alphavalue(parsis_z));
			cline (3, "heading pitch: ");
			other (alphavalue((int)(sin(deg*navigation_beta)*+100)));
			other (";");
			other (alphavalue((int)(cos(deg*navigation_beta)*-100)));
			break;
		case 4: // emergency functions status
			command (1, "reset onboard system");
			command (2, "send help request");
			if (lithium_collector)
				command (3, "stop scoping lithium");
			else
				command (3, "scope for lithium");
			command (4, "clear status");
			if (gburst == -1) {
				cline (1, "NOTE: there are no emergencies at the moment.");
				cline (2, "help request not sent.");
				break;
			}
	}
}

/* Comandi dei dispositivi di bordo. */

char dummy_identity[8] = "Removed:";
char comp_data[32];

void dev_commands ()
{
	int n;
	float dist;

	switch (dev_page) {
		case 0:	dev_page = s_command;
			switch (s_command) {
				case 1: status ("NAVIGATION", 50); break;
				case 2: status ("SUPPORTS", 50); break;
				case 3: status ("CARTOGRAFY", 50); break;
				case 4: status ("EMERGENCY", 50);
			}
			break;
		case 1: switch (s_command) {
				case 1: field_amplificator = 1 - field_amplificator;
					if (field_amplificator)
						status ("ACTIVE", 50);
					else
						status ("INACTIVE", 50);
					break;
				case 2: pl_search = 1 - pl_search;
					if (pl_search)
						status ("ACTIVE", 50);
					else
						status ("INACTIVE", 50);
					break;
				case 3: sync++;
					sync %= 6;
					if (!sync) {
						status ("IDLE", 50);
						ip_reaching = 0;
						ip_reached = 1;
					}
					else {
						status ("ACQUIRED", 50);
						if (ip_reached) {
							ip_reaching = 1;
							ip_reached = 0;
						}
					}
					break;
				case 4: anti_rad = 1 - anti_rad;
					if (anti_rad)
						status ("ACTIVE", 50);
					else
						status ("INACTIVE", 50);
			}
			break;
		case 2: switch (s_command) {
				case 1: ilightv = -ilightv;
					if (ilightv > 0)
						status ("ON", 50);
					else
						status ("OFF", 50);
					break;
				case 2: if (data == 1)
						datasheetdelta = -2;
					else {
						data = 1;
						datasheetdelta = +2;
					}
					break;
				case 3: if (data == 2)
						datasheetdelta = -2;
					else {
						data = 2;
						datasheetdelta = +2;
					}
					break;
				case 4: if (data == 3)
						datasheetdelta = -2;
					else {
						data = 3;
						datasheetdelta = +2;
					}
					break;
			}
			break;
		case 3: // galactic cartography commands
			switch (s_command) {
				case 1: if (ap_targetted==1 && !ap_targetting && !labplanet) {
						labstar = 1 - labstar;
						if (labstar) {
							if (star_label_pos > -1) {
								labstar = 0;
								if (star_label_pos >= sm_consolidated) {
									smh = _open (starmap_file, 4);
									if (smh > -1) {
										lseek (smh, star_label_pos, SEEK_SET);
										_write (smh, &dummy_identity[0], 8);
										_close (smh);
										ap_target_previd = 12345;
										status ("REMOVED", 50);
										star_label_pos = -1;
										nearstar_labeled--;
									}
									else
										status ("INT. ERROR", 50);
								}
								else
									status ("DENIED", 50);
							}
							else {
								status ("PROMPT", 50);
								labstar_char = 0;
								star_id = ap_target_id;
								for (n = 0; n < 21; n++)
									star_label[n] = 32;
							}
						}
						else {
							smh = _open (starmap_file, 4);
							if (smh == -1) {
								smh = _creat (starmap_file, 0);
								sm_consolidated = 4; _write (smh, &sm_consolidated, 4);
							}
							if (smh > -1) {
								lseek (smh, 4, SEEK_SET);
								while (_read (smh, comp_data, 32) == 32)
									if (memcmp (comp_data, dummy_identity, 8)) {
									if (!memicmp (comp_data+8, star_label, 20)) {
										status ("EXTANT", 50);
										ap_target_previd = 12345;
										star_label_pos = -1;
										_close (smh);
										return;
									}}
								lseek (smh, 0, SEEK_END);
								star_label_pos = tell(smh);
								_write (smh, &star_id, 32);
								_close (smh);
								status ("ASSIGNED", 50);
								nearstar_labeled++;
							}
							else
								status ("INT. ERROR", 50);
						}
					}
					else
						status ("CONFLICT", 50);
					break;
				case 2: if (ip_targetted!=-1 && !labstar) {
						labplanet = 1 - labplanet;
						if (labplanet) {
							if (planet_label_pos > -1) {
								labplanet = 0;
								if (planet_label_pos >= sm_consolidated) {
									smh = _open (starmap_file, 4);
									if (smh > -1) {
										lseek (smh, planet_label_pos, SEEK_SET);
										_write (smh, &dummy_identity[0], 8);
										_close (smh);
										prev_planet_id = 12345;
										status ("REMOVED", 50);
										planet_label_pos = -1;
										nearstar_labeled--;
									}
									else
										status ("INT. ERROR", 50);
								}
								else
									status ("DENIED", 50);
							}
							else {
								status ("PROMPT", 50);
								labplanet_char = 0;
								planet_id = nearstar_identity + ip_targetted + 1;
								for (n = 0; n < 21; n++)
									planet_label[n] = 32;
							}
						}
						else {
							smh = _open (starmap_file, 4);
							if (smh == -1) {
								smh = _creat (starmap_file, 0);
								sm_consolidated = 4; _write (smh, &sm_consolidated, 4);
							}
							if (smh > -1) {
								lseek (smh, 4, SEEK_SET);
								while (_read (smh, comp_data, 32) == 32)
									if (memcmp (comp_data, dummy_identity, 8)) {
									if (!memicmp (comp_data+8, planet_label, 20)) {
										status ("EXTANT", 50);
										prev_planet_id = 12345;
										planet_label_pos = -1;
										_close (smh);
										return;
									}}
								lseek (smh, 0, SEEK_END);
								planet_label_pos = tell(smh);
								_write (smh, &planet_id, 32);
								_close (smh);
								status ("ASSIGNED", 50);
								nearstar_labeled++;
							}
							else
								status ("INT. ERROR", 50);
						}
					}
					else
						status ("CONFLICT", 50);
					break;
				case 3:	targets_in_range = 1 - targets_in_range;
					if (targets_in_range) {
						targets = 0;
						target_line = 0;
						topmost_target = 0;
						tgt_collect_lastpos = 4;
						collecting_targets = 1;
						memset (&target_name[0], 32, 4 * 24);
						collect_targets ();
						tgts_in_show = 0;
					}
					break;
				case 4: // target parsis
					manual_target = 1 - manual_target;
					if (manual_target) {
						mt_coord = 0;
						mt_string_char = 0;
						manual_x_string[0] = 0;
						status ("TGT MANUAL", 100);
						ap_targetted = 0;
						stspeed = 0;
					}
					else
						status ("TGT REJECT", 100);
					break;
			}
			break;
		case 4: switch (s_command) {
				case 1: // reset onboard system
					reset_signal = 150;
					break;
				case 2: // help request
					if (pwr <= 15000 && !charge) {
						dist = fabs(dzat_x + dzat_y + dzat_z) * 0.0001;
						if (dist > 1800) dist = 1800;
						fast_srand (clock());
						helptime = (flandom() * dist) + secs + 60;
						status ("HELP REQ.", 50);
						gburst = 63;
					}
					else {
						status ("ERROR", 50);
						gburst = -1;
					}
					break;
				case 3: // collect lithium
					if (!lithium_collector) {
						if (ap_reached && ap_targetted == 1) {
							if (nearstar_class == 5 || (nearstar_class == 6 && nearstar_ray > 4))
								lithium_collector = 1;
							else
								status ("UNSUITABLE", 50);
						}
						else
							status ("NEED RECAL", 75);
					}
					else {
						lithium_collector = 0;
						status ("IDLE", 50);
					}
					break;
				case 4: // clear status
					status ("READY", 50);
					gburst = 0;
			}
	}
}

/* Preferenze. */

void prefs ()
{
	if (autoscreenoff)
		command (1, "auto screen sleep on");
	else
		command (1, "auto screen sleep off");

	if (revcontrols)
		command (2, "reverse pitch controls");
	else
		command (2, "normal pitch controls");

	if (menusalwayson)
		command (3, "menus always onscreen");
	else
		command (3, "auto-hidden menus");

	if (depolarize)
		command (4, "polarize");
	else
		command (4, "depolarize");
}

/* Comandi di impostazione delle opzioni preferenziali. */

void toggle_option (char *option_flag)
{
	*option_flag = 1 - *option_flag;
	if (*option_flag)
		status ("ACQUIRED", 50);
	else
		status ("DISABLED", 50);
}

void pfs_commands ()
{
	switch (s_command) {
		case 1: toggle_option (&autoscreenoff); break;
		case 2: toggle_option (&revcontrols);	break;
		case 3: toggle_option (&menusalwayson); break;
		case 4: toggle_option (&depolarize);
	}
}

/* Comandi impartiti al computer di bordo.
   Tutti tranne "disattiva schermo". */

void commands ()
{
	switch (sys) {
		case 1: fcs_commands (); break;
		case 2: dev_commands (); break;
		case 3: pfs_commands (); break;
	}
}

/* Scongela la situazione, riproducendola in tutto e per tutto,
   e facendola evolvere al momento attuale. */

void unfreeze ()
{
	int 	fh;
	double	elapsed, dpwr;

	/* Lettura della mappa stellare consolidata. */

	smh = _open (starmap_file, 4);
	if (smh > -1) {
		_read (smh, &sm_consolidated, 4);
		if (!sm_consolidated) {
			lseek (smh, 0, SEEK_SET);
			sm_consolidated = filelength(smh);
			_write (smh, &sm_consolidated, 4);
		}
		_close (smh);
	}
	else
		sm_consolidated = 0;

	/* Lettura della situazione precedente. */

	fh = _open (situation_file, 0);
	if (fh>-1) {
		_read (fh, &sync, 245);
		_read (fh, &gnc_pos, 1);
		_read (fh, &goesfile_pos, 4);
		_read (fh, goesnet_command, 120);
		READFILE (fh, &lastSnapshot, sizeof(lastSnapshot));				//36
		READFILE (fh, &option_mouseLook, sizeof(option_mouseLook));				//40
		READFILE (fh, &roofspeed, sizeof(roofspeed));
		strcpy(fcs_status_extended, fcs_status_extended);
		_close (fh);
	}
	else
		return;

	/* Risincronizzazione della situazione in relazione ai
	   precedenti eventi (evoluzione nascosta della situazione). */

	elapsed = secs;
	getsecs ();
	elapsed = secs - elapsed;

	if (helptime && secs>helptime+20) {
		helptime = 0;
		charge = 4;
		gburst = 0;
	}

	/* Ricostruzione del sistema stellare attuale. */

	npcs = -12345;
	prepare_nearstar ();

	if (lithium_collector) {
		while (elapsed >= 30 && charge < 120) {
			elapsed -= 30;
			charge++;
		}
		if (charge==120)
			pwr = 20000;
		else {
			srand (secs);
			pwr = random(5000) + 15000;
		}
	}

	/* Aggiornamento consumi supplementari. */

	dpwr = pwr;

	if (ilightv==1) dpwr -= elapsed / 84;
	if (pl_search) dpwr -= elapsed / 155;
	if (field_amplificator) dpwr -= elapsed / 41;

	if (ip_targetted != -1 && ip_reached) {
		if (sync==1) // fixed-point chase
			dpwr -= elapsed / 29;
		if (sync==2) // far chase
			dpwr -= elapsed / 18;
		if (sync==3) // syncrone orbit
			dpwr -= elapsed / 58;
		if (sync==4) // vimana orbit
			dpwr -= elapsed / 7;
		if (sync==5) // near chase
			dpwr -= elapsed / 33;
	}

	while (dpwr < 15000) {
		if (charge) {
			dpwr += 5000;
			charge--;
		}
		else
			dpwr = 15000;
	}

	pwr = dpwr;
}

/* Gestione errori hardware. */

#pragma warn -par
int caught_somewhere_in_time (int errval, int ax, int bp, int si)
{
	hardresume (_HARDERR_FAIL);
	return (2);
}
#pragma warn +par

/* Programma principale. */

float starmass_correction[star_classes] = {
       1.886, // classe 0
	1.50, // classe 1
     8000.40, // classe 2
	0.05, // classe 3
	2.44, // classe 4
	3.10, // classe 5
	9.30, // classe 6
       48.00, // classe 7
	1.00, // classe 8
	1.00, // classe 9
	0.07, // classe 10
    15000.00  // classe 11
};

extern unsigned _stklen = 0x1800;

void main ()
{
	float satur, DfCoS;
     //	float user_drawing_range;
	char *planetd[] = { "INTERNALLY HOT",
				   "CRATERIZED NO ATMOSPHERE",
				   "THICK ATMOSPHERE",
				   "FELISIAN",
				   "CREASED NO ATMOSPHERE",
				   "THIN ATMOSPHERE",
				   "LARGE NOT CONSISTENT",
				   "ICY",
				   "QUARTZ",
				   "SUBSTELLAR OBJECT",
				   "COMPANION STAR" };
	long  ir,  ig,  ib,  ire  = 0, ige  = 0, ibe  = 0;
	long  ir2, ig2, ib2, ir2e = 0, ig2e = 0, ib2e = 0;
	long  ir3, ig3, ib3, ir3e = 0, ig3e = 0, ib3e = 0;

	int   c, p_mpul = 0;
	char  sky_palette_ok = 0;
	char  select, lrv, right_dblclick = 0;
	float right_dblclick_dir;

	double dpz, ras, rap, dasp, eclipse;
	double dxx, dyy, dzz, l_dsd, p_dsd, stz, ang;

	int opencapcount = 0;
	int opencapdelta = 0;
	int holdtomiddle = 0;

	char leftturn, rightturn, arrowcolor, farstar = 0;

	char temp_distance_buffer[16];

	unsigned pqw;
	float hold_z;

	float tmp_float;
	long p1, p2, p3, p4;

	harderr (caught_somewhere_in_time);

	for (ir = 0; ir < 200; ir ++) m200[ir] = ir * 200;

	if (!test_and_init_mouse()) {
		printf ("\nMouse not present or mouse driver not installed.\n");
		return;
	}

	n_offsets_map = (unsigned char far  *) farmalloc (om_bytes);
	n_globes_map  = (char far  *) farmalloc ((unsigned)gl_bytes+(unsigned)gl_brest);
	s_background  = (unsigned char far  *) farmalloc (st_bytes);
	p_background  = (unsigned char far  *) farmalloc (pl_bytes);
	p_surfacemap  = (unsigned char huge *) farmalloc (ps_bytes);
	objectschart  = (quadrant far       *) farmalloc (oc_bytes);
	ruinschart    = (unsigned char far  *) objectschart; // oc alias
	pvfile        = (unsigned char far  *) farmalloc (pv_bytes);
	adapted       = (unsigned char far  *) farmalloc (sc_bytes);
	txtr          = (unsigned char huge *) p_background; // txtr alias
	digimap2      = (unsigned long far *) &n_globes_map[gl_bytes]; // font alias

	reach_your_dir ();

	if (pvfile&&adapted&&n_offsets_map&&n_globes_map
	    &&p_background&&s_background&&p_surfacemap
	    &&objectschart&&lens_flares_init()) {
		lrv = loadpv (vehicle_handle, vehicle_ncc, 15, 15, 15, 0, 0, 0, 0, 1);
		if (lrv < 1) {
			printf ("\nLoad error.\n");
			return;
		}
		load_QVRmaps ();
		load_starface ();
		load_digimap2 ();
	}
	else {
		printf ("\nNot enough free conventional memory to run.");
		printf ("\nType MEM and hit ENTER to check it out.");
		printf ("\n550 KB are needed to run Noctis!\n");
		return;
	}

	//tweakedVGA (X320Y200C4YPAL); // INIZIALIZZAZIONE GRAFICA.
	_320_200_256 (); // INIZIALIZZAZIONE GRAFICA.

	initscanlines ();
	init_FP_segments ();

	unfreeze ();

	pclear (adapted, 0);

	QUADWORDS -= 1440;
	pqw = QUADWORDS;

	mouse_input ();
	mpul = 0;

	clock_t right_dblclick_timing = 0;

	dpp  = 210;
	change_camera_lens ();

	//   0..64  veicolo, selezioni computer, artefatti. BLU COBALTO, ma dipende dalla luce delle stelle.
	//  64..128 cosmo, sfondo galattico, cieli sereni e "suplucsi effect". dal BLU ELETTRICO al BIANCO.
	// 128..192 stelle (continue sfumature cicliche) o lune (non costanti)
	// 192..256 pianeti (non costanti)
	tavola_colori (range8088, 0, 64, 16, 32, 63);
	tavola_colori (tmppal, 0, 256, 64, 64, 64);

	int resolve = 64;

	// causa il recupero dell'eventuale contenuto dello schermo
	// di output della GOES command net
	force_update = 1;

	// recupero della situazione di superficie
	sfh = _open (surface_file, 0);
	if (sfh > -1) {
		// lettura precedenti coordinate di sbarco
		_read (sfh, &landing_pt_lon, 2);
		_read (sfh, &landing_pt_lat, 2);
		_close (sfh);
		// recupero labels del pianeta e della stella-bersaglio
		update_star_label ();
		update_planet_label ();
		// risincronizzazione istantanea della posizione della navicella
		getsecs ();
		planet_xyz (ip_targetted);
		dzat_x = plx;
		dzat_y = ply;
		dzat_z = plz;
		// calcolo della distanza dalla stella primaria
		dxx = dzat_x - nearstar_x;
		dyy = dzat_y - nearstar_y;
		dzz = dzat_z - nearstar_z;
		dsd = sqrt (dxx*dxx + dyy*dyy + dzz*dzz) + 1;
		// rielaborazione superficie planetaria
		from_vehicle ();
		landing_point = 1; planets (); landing_point = 0;
		// ripresa del ciclo di esplorazione planetaria
		entryflag = 1; planetary_main();
		// termine esplorazione
		opencapcount = 86;
		opencapdelta = -2;
		holdtomiddle = 1;
		pp_gravity = 1;
		QUADWORDS = 16000;
		pclear (adapted, 0);
		pclear (adaptor, 0);
		QUADWORDS = pqw;
		if (exitflag) goto allstop;
	}

	do {
		sync_start ();
		
		//
		// Controllo del flag che indica quando ci si trova
		// sulla "terrazza panoramica", il tetto dello stardrifter.
		//
		pos_y += lifter;
		if (lifter > 0) {
			lifter--;
			if (lifter > 65)
				user_alfa += 0.11 * (40 - user_alfa);
			else
				user_alfa -= 0.25 * user_alfa;
			step = 0.5 * lifter;
		}
		if (lifter < 0) {
			lifter++;
			if (pos_y > -325)
				user_alfa += 0.12 * (user_alfa - 40);
			if (pos_y < -325 && pos_y > -715)
				step = - pos_y;
		}
		if (pos_y > 0) {
			lifter = 0;
			pos_y = 0;
		}
		if (pos_y < -750) {
			lifter = 0;
			pos_y = -750;
		}
		if (pos_y < -500)
			ontheroof = 1;
		else
			ontheroof = 0;
		if (!lifter && ontheroof) {
			DfCoS = pos_z + 3100;
			DfCoS = sqrt(pos_x*pos_x+DfCoS*DfCoS);
			if (DfCoS + step < 1100) lifter = + 75;
		}
		//
		// Risposta al reset del sistema operativo di bordo.
		// La variabile "reset_signal" controlla la procedura
		// di reset, che reimposta tutti i parametri operativi
		// allo stato originario.
		//
		// Impostanto reset_signal a 150 si resetta tutto il sistema.
		// Impostanto reset_signal a 60 si resetta la rete GOES.
		//
		if (reset_signal) {
			switch (reset_signal) {
			       case 150: elight 		= 1;
					 ilight			= 0;
					 ilightv		= 0;
					 status ("----------", 100);
					 break;
			       case 140: ip_targetted 		= -1;
					 ip_reaching		= 0;
					 break;
			       case 130: ap_targetted 		= 0;
					 stspeed 		= 0;
					 break;
			       case 120: gburst			= 0;
					 sync			= 1;
					 anti_rad		= 1;
					 pl_search		= 0;
					 field_amplificator 	= 0;
					 break;
			       case 115: sys			= 4;
					 psys			= 4;
					 break;
			       case 110: lithium_collector	= 0;
					 autoscreenoff		= 0;
					 break;
			       case 101: status ("_^*^-!_$[]", 100);
					 ap_reached		= 0;
					 ip_reached		= 0;
					 landing_point		= 0;
					 break;
				case 75: elight 		= 0;
					 ilightv 		= 1;
					 break;
				case 55: mslocate (0, 0, 0);
					 mswrite (0, "G.O.E.S. COMMAND NET:");
					 break;
				case 35: mslocate (0, 0, 1);
					 mswrite (0, "REVISION ID 6011/0200");
					 break;
				case 25: mslocate (0, 0, 2);
					 mswrite (0, "SESSION ID ");
					 fast_srand (secs * 18);
					 sprintf (temp_distance_buffer, "%05lu%05lu", fast_random (0x7FFF), fast_random (0x7FFF));
					 mswrite (0, temp_distance_buffer);
					 break;
				case 10: status ("STANDBY", 100);
					 break;
			}
			reset_signal--;
		}
		//
		// Controlla il timer di sistema.
		//
		getsecs ();
		//
		// Accensione luci d'emergenza.
		// Comportamento dell'astrozattera in mancanza di litio.
		//
		if (pwr <= 15000 && !charge) {
			elight = 1;
			sync = 0;
			anti_rad = 0;
			pl_search = 0;
			field_amplificator = 0;
			ip_targetted = -1;
			ap_reached = 0;
			datasheetdelta = -100;
		}
		else {
			if (elight && !reset_signal) {
				elight = 0;
				ilight = 0;
				ilightv = 1;
				reset_signal = 200;
			}
		}
		//
		// Input del mouse per gli spostamenti dell'utente.
		//
		
		p_mpul = mpul; mpul = 0;
		mouse_input ();
		if (mpul&2) {
			shift += 3*mdltx;
			dlt_alfa -= (float) mdlty / 8;
		}
		else {
			step -= 3*mdlty;
			if (abs(mdlty)>7) dlt_alfa = -user_alfa / 6;
			dlt_beta -= (float) mdltx / 3;
		}
		//
		// Input del mouse per il doppio click sinistro e destro.
		//
		if (ontheroof) goto nop;
		if ((mpul&1)&&!(p_mpul&1))
			select = 1;
		else
			select = 0;
		if ((mpul&2)&&!(p_mpul&2)&&!right_dblclick) {
			if (!right_dblclick_timing)
				right_dblclick_timing = clock();
			else {
				if (clock()-right_dblclick_timing<9) {
					right_dblclick = 1;
					right_dblclick_dir = user_beta;
				}
				else
					right_dblclick_timing = clock();
			}
		}
		if (right_dblclick) {
			if (ap_targetting) {
				ap_targetting = 0;
				right_dblclick = 0;
				extract_ap_target_infos ();
				fix_remote_target ();
				goto nop;
			}
			if (ip_targetting) {
				ip_targetting = 0;
				right_dblclick = 0;
				if (ip_targetted != -1)
					fix_local_target ();
				else
					status ("NO TARGET", 50);
				goto nop;
			}
			if (!holdtomiddle) {
				if (right_dblclick_dir > -135 && right_dblclick_dir < -45) {
					user_beta += 90;
					user_beta /= 1.5;
					xx = pos_x - 2900;
					pos_x -= xx * 0.25;
					if (landing_point)
						zz = pos_z + 104*15 + 1980;
					else
						zz = pos_z + 1940;
					pos_z -= zz * 0.25;
					if (fabs(xx)<25&&fabs(zz)<25&&fabs(user_beta)<1) {
						right_dblclick_timing = 0;
						right_dblclick = 0;
					}
					user_beta -= 90;
				}
				else {
					user_beta /= 1.5;
					zz = pos_z + 500;
					pos_z -= zz * 0.25;
					if (sys!=4) {
						if (fabs(zz)<25&&fabs(user_beta)<1) {
							right_dblclick_timing = 0;
							right_dblclick = 0;
						}
					}
					else {
						xx = pos_x + 1700;
						pos_x -= xx * 0.25;
						if (fabs(zz)<25&&fabs(xx)<25&&fabs(user_beta)<1) {
							right_dblclick_timing = 0;
							right_dblclick = 0;
						}
					}
				}
			}
		}
		//
		// Variazione angoli visivi.
		//
	   nop:	user_alfa += dlt_alfa;
		dlt_alfa /= 1.5; if (fabs(dlt_alfa)<0.25) dlt_alfa = 0;
		if (user_alfa<-44.9) {
			user_alfa = -44.9;
			dlt_alfa = 0;
		}
		if (user_alfa>44.9) {
			user_alfa = 44.9;
			dlt_alfa = 0;
		}
		user_beta += dlt_beta;
		dlt_beta /= 1.5; if (fabs(dlt_beta)<0.25) dlt_beta = 0;
		if (user_beta>180) user_beta -= 360;
		if (user_beta<-180) user_beta += 360;
		//
		// Variazione della posizione dell'utente nella navicella.
		//
		alfa = user_alfa;
		beta = user_beta - 90;
		change_angle_of_view ();
		p_Forward (shift);
		beta = user_beta;
		change_angle_of_view ();
		p_Forward (step);
		shift /= 1.5; if (fabs(shift)<0.5) shift = 0;
		step /= 1.25; if (fabs(step)<0.5) step = 0;
		if (pos_x<-3100) pos_x = -3100;
		if (pos_x>+3100) pos_x = +3100;
		if (pos_z>- 300) pos_z = - 300;
		if (pos_z<-5800) pos_z = -5800;
		//
		// Background nero, che verr� reso nebuloso...
		//
		if (!stspeed)
			pclear (adapted + 2880, 0);
		else
			pfade (adapted, 180, 8);
		//
		// gestione stella vicina
		//
		from_vehicle ();
		dxx   = dzat_x - nearstar_x;
		dyy   = dzat_y - nearstar_y;
		dzz   = dzat_z - nearstar_z;
		l_dsd = sqrt (dxx*dxx + dyy*dyy + dzz*dzz) + 1;
		satur = (12 * dsd) / nearstar_ray;
		fast_srand (nearstar_z);
		ir = fast_random(31) + 29;
		if (satur < ir) satur = ir;
		if (satur > 63) satur = 63;
		//
		if (l_dsd < 100 * nearstar_ray)
			whiteglobe (adapted, nearstar_x, nearstar_y, nearstar_z,
				    3 * nearstar_ray, 0.3);
		for (ir = 0; ir < nearstar_nop; ir++) {
			if (nearstar_p_type[ir] == 10) {
				planet_xyz (ir);
				p_dsd = nearstar_p_qsortdist[ir];
				fast_srand (ir + nearstar_x);
				whiteglobe (adapted, plx, ply, plz,
				    3 * nearstar_p_ray[ir], 0.15 - fast_flandom() * 0.3);
				if (p_dsd>5*nearstar_p_ray[ir]&&p_dsd<1000*nearstar_p_ray[ir])
					lens_flares_for (dzat_x, dzat_y, dzat_z, plx, ply, plz,
							(10 * nearstar_p_ray[ir]) / p_dsd,
							1 + (0.001 * p_dsd), 1, 0, 3, 0);
			}
		}
		if (l_dsd > 6 * nearstar_ray) {
			if (nearstar_class!=5&&nearstar_class!=6&&nearstar_class!=10) {
				if (nearstar_class!=11||gl_start<90) {
					if (l_dsd>5*nearstar_ray&&l_dsd<1000*nearstar_ray) {
						lens_flares_for (dzat_x, dzat_y, dzat_z,
								 nearstar_x, nearstar_y, nearstar_z,
								 (10 * nearstar_ray) / l_dsd, 1 + (0.001 * l_dsd), 1, 0, 3, 0);
					}
				}
			}
			psmooth_grays (adapted+2880);
		}
		//
		mask_pixels (adapted+2880, 64);
		//
		if (l_dsd < 8 * nearstar_ray) {
			if (farstar) {
				farstar = 0;
				load_starface ();
			}
			glass_bubble = 0;
			if (nearstar_spin) {
			 gl_start += nearstar_spin;
			 gl_start %= 360;
			 globe (gl_start, adapted, s_background, n_globes_map, gl_bytes,
				nearstar_x, nearstar_y, nearstar_z, nearstar_ray, 64, satur);
			}
			else {
			 globe ((clock()/360)%360, adapted, s_background,
				n_globes_map, gl_bytes, nearstar_x, nearstar_y,
				nearstar_z, nearstar_ray, 64, satur);
			}
		}
		else
			farstar = 1;
		//
		if (l_dsd > 100 * nearstar_ray && l_dsd < 1550 * nearstar_ray) {
			ir = ((1600 * nearstar_ray) - l_dsd) / (100 * nearstar_ray);
			if (ir < 0) ir = 0;
			ir += 0x30;
			far_pixel_at (nearstar_x, nearstar_y,
				      nearstar_z, 0, ir);
			far_pixel_at (nearstar_x, nearstar_y,
				      nearstar_z, 0, ir);
			far_pixel_at (nearstar_x, nearstar_y,
				      nearstar_z, 0, ir);
		}
		//
		// Riflessi del protagonista sulla vetrata (rimossi).
		/*
		if (!stspeed) {
			if (ilight) {
				from_user ();
				if (cam_z>-1000) {
					if (alfa>-10&&alfa<10)
						user_drawing_range = 55;
					else
						user_drawing_range = 70;
				}
				else
					user_drawing_range = 40;
				if (beta>-user_drawing_range&&beta<user_drawing_range) {
					cam_x = -4*beta;
					cam_y = 10*alfa;
					cam_z = 0;
					Forward (2*pos_z);
					alfa *= 2; beta *= 2;
					change_angle_of_view ();
					flares = 2;
					user ();
					flares = 0;
				}
			}
		}
		*/
		// Controllo gestore (indicando i comandi con lo sguardo).
		//
		active_screen = -1;
		from_user ();
		leftturn  = 0;
		rightturn = 0;
		infoarea  = 0;
		s_control = 0;
		s_command = 0;
		if (ontheroof) goto jpr;
		do {
			zz = fabs (cam_z);
			xx = fabs (cam_x);
			Forward (zz/2);
		} while (zz>25&&xx<3000);
		if (zz<25) {
			if (cam_x<-44*30) {
				if (cam_x>-68*30) {
					s_control = (cam_y + 25) / 50 + 3;
					if (s_control<1) s_control = 1;
					if (s_control>4) s_control = 4;
					if (select) {
						if (!ap_targetting&&!ip_targetting) {
							aso_countdown = 100;
							sys = s_control;
							dev_page = 0;
						}
					}
				}
			}
			else {
				if (cam_y<-50) {
					if (cam_x<68*30) {
						s_command = (cam_x + 44*30) / (27*30) + 1;
						if (s_command<1) s_command = 1;
						if (s_command>4) s_command = 4;
						if (select) {
							if (!ap_targetting&&!ip_targetting) {
								aso_countdown = 100;
								commands ();
								goto jpr;
							}
						}
					}
				}
				else
					infoarea = 1;
			}
		}
		if (select && pwr > 15000) {
			if (revcontrols) {
				if (cam_x > 2500) {
					dlt_nav_beta += 1.5;
					status ("PITCH - R", 25);
				}
				if (cam_x < -2500) {
					dlt_nav_beta -= 1.5;
					status ("PITCH - L", 25);
				}
			}
			else {
				if (cam_x > 2500) {
					dlt_nav_beta -= 1.5;
					status ("PITCH - L", 25);
				}
				if (cam_x < -2500) {
					dlt_nav_beta += 1.5;
					status ("PITCH - R", 25);
				}
			}
		}
		if (cam_x>2500)
			rightturn = 1;
		if (cam_x<-2500)
			leftturn = 1;
		//
		// Rotazione della navicella.
		// Attivazione schermi.
		//
	   jpr: if (!elight) {
			// Paratia destra:
			if (user_beta > -135 && user_beta < -45 && pos_z < -104*15 && pos_z > -262*15 && pos_x > 172*15)
				active_screen = (pos_z + 104*15) / (-54*15);
			// Paratia sinistra:
			//if (user_beta > +45 && user_beta < +135 && pos_z < -104*15 && pos_z > -154*15 && pos_x < -172*15)
				//active_screen = (pos_z + 104*15) / (-54*15) + 2;
		}
		navigation_beta += dlt_nav_beta;
		dlt_nav_beta /= 1.1; if (fabs(dlt_nav_beta)<0.5) dlt_nav_beta = 0;
		if (navigation_beta>=360) navigation_beta -= 360;
		if (navigation_beta<0) navigation_beta += 360;
		//
		// Tracciamento pianeti.
		//
		from_vehicle ();
		planets ();
		//
		// Controllo sulle richieste d'aiuto.
		//
		if (helptime && secs>helptime) {
			if (gburst) {
				status ("HELP CAME!", 50);
				gburst = 0;
			}
			if (secs<helptime+120) {
				stz = 0;
				if (secs<helptime+20)
					stz = pow (helptime + 20 - secs, 2) * 2000;
				if (secs>helptime+100)
					stz = pow (helptime + 100 - secs, 2) * 2000;
				if (!stz&&charge<3) charge = 3;
				other_vehicle_at ((stz + 16000) * cos (secs / 10),
						  4000 * sin (secs / 100),
						  (stz + 16000) * sin (secs / 10));
			}
			else
				helptime = 0;
		}
		
		//
		// Tracciamento della navicella.
		//
		from_user ();
		vehicle (opencapcount);
		
		//
		// Tracciamento riflessi, aggiornamento dello schermo
		// del gestore, tracciamento dello schermo del gestore,
		// reazione visiva agli eventi interni alla navicella.
		//
		if (ontheroof) goto ext_1;
		from_user ();
		if (!opencapcount) reflexes ();
		if (!(clock()%10)) {
			clear_onboard_screen ();
			control (1, "flight control drive");
			control (2, "onboard devices");
			control (3, "preferences");
			control (4, "disable display");
			switch (sys) {
				case 1: control (1, "FLIGHT CONTROL DRIVE");
					if (sys!=psys) status ("FCS MENU", 50);
					fcs (); break;
				case 2:	control (2, "ONBOARD DEVICES");
					if (sys!=psys) status ("SELECT SUB", 50);
					devices (); break;
				case 3:	control (3, "PREFERENCES");
					if (sys!=psys) status ("PREFS MENU", 50);
					prefs (); break;
				case 4:	control (4, "DISABLE DISPLAY");
					if (sys!=psys) status ("SCREEN OFF", 50);
			}
			psys = sys;
		}
		//
		if (!ap_targetting&&!ip_targetting) {
			setfx (4);
			dxx = pos_z / 88;
			if (dxx < -16) dxx = -16;
			entity = dxx;
			screen ();
			setfx (0);
		}
		//
		if (leftturn) {
			arrowcolor = 127 - 16 * (clock()%4);
			digit_at ('-', -2900, -50, 12, arrowcolor, 0);
			if (revcontrols)
				digit_at ('>', -3000, -50, 12, arrowcolor, 0);
			else
				digit_at ('<', -3000, -50, 12, arrowcolor, 0);
		}
		if (rightturn) {
			arrowcolor = 127 - 16 * (clock()%4);
			digit_at ('-', +2900, -50, 12, arrowcolor, 0);
			if (revcontrols)
				digit_at ('<', +3000, -50, 12, arrowcolor, 0);
			else
				digit_at ('>', +3000, -50, 12, arrowcolor, 0);
		}
		//
		// ***** H.U.D. OUTER LAYER *****
		// Fornisce informazioni sullo strato esterno dell'H.U.D.
		// Qualsiasi glifo verr� in seguito trattato con dithering.
		//
		if (active_screen != -1) goto nohud_1;
		//
		// Informazioni e schemi addizionali sull'H.U.D.
		// Tracciamento label della stella selezionata.
		// Tracciamento distanza dalla stella selezionata.
		//
		if (ap_targetting || ap_targetted) {
			alfa = 0; beta = 0; change_angle_of_view ();
			cam_x = 450; cam_y = 250; cam_z = -750;
			for (c = 0; c < 24; c++) {
				if (labstar && c == labstar_char) digit_at ('_', -6, -15, 5, 127 - 2 * (clock()%32), 0);
				digit_at (star_label[c], -6, -15, 5, 127, 1);
				cam_x -= 40;
			}
			dxx = dzat_x - ap_target_x;
			dyy = dzat_y - ap_target_y;
			dzz = dzat_z - ap_target_z;
			l_dsd = sqrt (dxx*dxx + dyy*dyy + dzz*dzz) * 5E-5;
			if (ap_reached && ap_target_id == nearstar_identity) l_dsd *= 0.01;
			sprintf (temp_distance_buffer, "%01.2f", l_dsd);
			cam_x = 450; cam_y = -180; cam_z = -750;
			c = 0; while (temp_distance_buffer[c] != 0) {
				digit_at (temp_distance_buffer[c], -6, -15, 5, 127, 1);
				cam_x -= 40;
				c++;
			}
			cam_x -= 40; digit_at ('L', -6, -15, 5, 112, 1);
			cam_x -= 40; digit_at ('.', -6, -15, 5, 112, 1);
			cam_x -= 40; digit_at ('Y', -6, -15, 5, 112, 1);
			cam_x -= 40; digit_at ('.', -6, -15, 5, 112, 1);
		}
		//
		// Tracciamento label del pianeta selezionato,
		// tracciamento distanza dal pianeta selezionato,
		// aggiornamento nome del pianeta-bersaglio.
		//
		if (ip_targetted!=-1) {
			update_planet_label ();
			alfa = 0; beta = 0; change_angle_of_view ();
			cam_x = 450; cam_y = 180; cam_z = -750;
			for (c = 0; c < 24; c++) {
				if (labplanet && c == labplanet_char) digit_at ('_', -6, -15, 5, 127 - 2 * (clock()%32), 0);
				digit_at (planet_label[c], -6, -15, 5, 112, 1);
				cam_x -= 40;
			}
			planet_xyz (ip_targetted);
			dxx = dzat_x - plx;
			dyy = dzat_y - ply;
			dzz = dzat_z - plz;
			l_dsd = sqrt (dxx*dxx + dyy*dyy + dzz*dzz) * 1E-2;
			sprintf (temp_distance_buffer, "%01.2f", l_dsd);
			cam_x = 450; cam_y = -250; cam_z = -750;
			c = 0; while (temp_distance_buffer[c] != 0) {
				digit_at (temp_distance_buffer[c], -6, -15, 5, 120, 1);
				cam_x -= 40;
				c++;
			}
			cam_x -= 40; digit_at ('D', -6, -15, 5, 105, 1);
			cam_x -= 40; digit_at ('Y', -6, -15, 5, 105, 1);
			cam_x -= 40; digit_at ('A', -6, -15, 5, 105, 1);
			cam_x -= 40; digit_at ('M', -6, -15, 5, 105, 1);
			cam_x -= 40; digit_at ('S', -6, -15, 5, 105, 1);
		}
		//
		// Messaggio di reset, lampeggiante.
		//
		if (reset_signal && (reset_signal % 10) < 5) {
			alfa = 0; beta = 0; change_angle_of_view ();
			cam_x = 300; cam_y = 0; cam_z = -750;
			c = 0; while (sr_message[c] != 0) {
				digit_at (sr_message[c], -6, -15, 8, 127, 1);
				cam_x -= 60;
				c++;
			}
		}
		//
		// Tracciamento dello stato attuale dell'FCS.
		//
       nohud_1: alfa = 0; beta = 0; change_angle_of_view ();
		cam_x = -512; cam_y = -275; cam_z = -750;
		c = strlen(fcs_status_extended) - 1;
		while (c >= 0) {
			digit_at (fcs_status_extended[c], -6, -15, 6, 120, 1);
			cam_x += 45;
			c--;
		}
		//
		// Link alla funzione di ricerca dei targets in real-time.
		//
		if (collecting_targets) {
			status ("SCANNING..", 100);
			collect_targets ();
			if (!collecting_targets) {
				if (targets)
					status ("DONE!", 100);
				else
					status ("NO TARGETS", 100);
			}
		}
		//
		// Visualizzazione/aggiornamento tabella "targets in range".
		//
		if (targets_in_range) {
			if (update_targets) {
				tgts_in_show = 0;
				c = topmost_target;
				while (targets && c < targets && tgts_in_show < 3) {
					tgt_label_pos = search_id_code (targets_table_id[c], 'S');
					if (tgt_label_pos > -1) {
						smh = _open (starmap_file, 0);
						if (smh > -1) {
							lseek (smh, tgt_label_pos + 8, SEEK_SET);
							_read (smh, &target_name[tgts_in_show], 24);
							_close (smh);
							tgts_in_show++;
						}
					}
					c++;
				}
				update_targets = 0;
			}
			cam_x = 175; cam_y = 40; cam_z = -500;
			frame (225, 48, 285, 96, 2, 90);
			cam_y = 8 + 40;
			c = 0;
			while (c < tgts_in_show) {
				cam_x = 35 + 175;
				if (c == target_line)
					frame (226+35, 0, 277, 30, 1, 120);
				for (ir = 0; ir < 24; ir++) {
					digit_at (target_name[c][ir], -5, -10, 3.5, 174, 1);
					cam_x -= 23;
				}
				cam_y -= 55;
				c++;
			}
		}
       ext_1:	//
		// Anti-aliasing e dithering (error-diffusion).
		// E` un procedimento molto peculiare, che fornisce effetti
		// straordinariamente belli su uno schermo che, di per s�,
		// � poco risolutivo, sia fisicamente che cromaticamente.
		//
		QUADWORDS -= 240;
		psmooth_64 (adapted, 200);
		psmooth_64 (adapted, 200);
		QUADWORDS += 240;
		//
		// Tracciamento di tutte le stelle visibili.
		//
		from_vehicle ();
		sky (0x405C);
		//
		// ***** H.U.D. INNER LAYER *****
		// Fornisce informazioni sullo strato interno dell'H.U.D.
		// Qualsiasi glifo non verr� trattato con dithering.
		//
		if (datasheetscroll) {
			areaclear (adapted, 11, 85, 0, 0, 1 + datasheetscroll, 9, 72);
			areaclear (adapted, 11, 95, 0, 0, 1 + datasheetscroll, 40, 112);
			c = (datasheetscroll / 4) - 1;
			if (c > 0) {
			switch (data) {
				case 1: // remote target data
					if (ap_targetted) {
						if (ap_targetted == 1) {
							wrouthud (14, 87, c, star_label);
							tmp_float = 1e-3 * qt_M_PI * ap_target_ray * ap_target_ray * ap_target_ray;
							tmp_float *= starmass_correction[ap_target_class];
							if (nearstar_class == 8 || nearstar_class == 9) {
								fast_srand ((long)ap_target_x%32000);
								switch (fast_random(5)) {
									case 0: tmp_float /= 1 + 5 * fast_flandom(); break;
									case 1: tmp_float /= 1 + fast_flandom(); break;
									case 2: tmp_float *= 1 + fast_flandom(); break;
									case 3: tmp_float *= 1 + 20 * fast_flandom(); break;
									case 4: tmp_float *= 1 + 50 * fast_flandom();
								}
							}
							wrouthud (14, 97, c,   "                 RADIUS:");
							wrouthud (14, 97, c, "PRIMARY MASS:");
							sprintf (outhudbuffer, "                %1.4f", ap_target_ray);
							FitOutHudBuffer(16, 24);
							wrouthud (14, 103, c, outhudbuffer);
							sprintf (outhudbuffer, "%1.8f", tmp_float);
							FitOutHudBuffer(0, 8);
							sprintf (outhudbuffer+strlen(outhudbuffer), " BAL M");

							wrouthud (14, 103, c, outhudbuffer);
								
							wrouthud (14, 109, c, "              CENTIDYAMS");
							tmp_float /= 0.38e-4 * ap_target_ray;
							if (ap_target_class == 6) tmp_float *= 0.0022;
							//wrouthud (14, 113, c, "SURFACE TEMPERATURE:");
							wrouthud (14, 116, c, "SURFACE TEMPERATURE:");
							sprintf (outhudbuffer, "%1.0f@K&%1.0f@C&%1.0f@F", tmp_float + 273.15, tmp_float, tmp_float*1.8 + 32);
							//wrouthud (14, 119, c, outhudbuffer);
							wrouthud (14, 122, c, outhudbuffer);
							sprintf (outhudbuffer, "MAJOR BODIES: %d EST.", starnop (ap_target_x, ap_target_y, ap_target_z));
							wrouthud (14, 129, c, outhudbuffer);
						}
						else {
							wrouthud (14, 87, c, "DIRECT PARSIS TARGET");
						}
					}
					else
						wrouthud (14, 87, c, "REMOTE TARGET NOT SET");
					break;
				case 2: // local target data
					if (ip_targetted != -1) {
						wrouthud (14, 87, c, planet_label);
						wrouthud (14, 97, c, "PERIOD OF ROTATION:");
						if (nearstar_p_qsortindex[nearstar_nob-1] == ip_targetted) {
							if (nearstar_p_rtperiod[ip_targetted] > 0) {
								p1 = nearstar_p_rtperiod[ip_targetted]; p1 *= 360;
								p2 = p1 / 1000; p2 /= 1000;
								p3 = p1 / 1000; p3 %= 1000;
								p4 = p1 % 1000;
								sprintf (outhudbuffer, "TRIADS %03ld:%03ld:%03ld", p2, p3, p4);
								wrouthud (14, 103, c, outhudbuffer);
							}
							else {
								if (ip_reaching || ip_reached) {
									if (nearstar_p_type[ip_targetted] != 10)
										wrouthud (14, 103, c, "COMPUTING...");
									else
										wrouthud (14, 103, c, "NOT RESOLVABLE");
								}
								else
									wrouthud (14, 103, c, "TOO FAR TO ESTIMATE");
							}
						}
						else
							wrouthud (14, 103, c, "TOO FAR TO ESTIMATE");
					
						wrouthud (14, 110, c, "PERIOD OF REVOLUTION:");
						tmp_float = rtp (ip_targetted);
						p1 = tmp_float * 1e-9;
						p2 = tmp_float * 1e-6; p2 %= 1000;
						p3 = tmp_float * 1e-3; p3 %= 1000;
						p4 = (long)(tmp_float) % 1000;
						if (p1 < 2)
							sprintf (outhudbuffer, "%ld EPOCS, %03ld:%03ld:%03ld", p1, p2, p3, p4);
						else {
							if (p1 < 2047)
								sprintf (outhudbuffer, "%ld EPOCS, %03ld:%03ld:???", p1, p2, p3);
							else
								sprintf (outhudbuffer, "%ld EPOCS, %03ld:???:???", p1, p2);
						}
						wrouthud (14, 116, c, outhudbuffer);						
						sprintf (outhudbuffer, "       %s", planetd[nearstar_p_type[ip_targetted]]);						
						wrouthud (14, 123, c, outhudbuffer);
						wrouthud (14, 123, c, "RADIUS:");
						
						sprintf (outhudbuffer, "%1.4f", nearstar_p_ray[ip_targetted]);
						FitOutHudBuffer(16, 24);
						sprintf (outhudbuffer+strlen(outhudbuffer), " CENTIDYAMS");
						wrouthud (14, 129, c, outhudbuffer);
					}
					else
						wrouthud (14, 87, 21, "LOCAL TARGET NOT SET");
					break;
				case 3: // environment data
					wrouthud (14, 87, c, "EXTERNAL ENVIRONMENT");
					fast_srand (secs/2);
					tmp_float = 16 - dsd * 0.044;
					tmp_float *= fabs(tmp_float);
					tmp_float -= (tmp_float + 273.15) * eclipse;
					if (tmp_float < -269) tmp_float = fast_flandom() - 269;
					sprintf (outhudbuffer, "TEMP. %1.2f@K", tmp_float + 273.15);
					wrouthud (14, 97, c, outhudbuffer);
					sprintf (outhudbuffer, "      %1.2f@C", tmp_float);
					wrouthud (14, 103, c, outhudbuffer);
					sprintf (outhudbuffer, "      %1.2f@F", tmp_float*1.8 + 32);
					wrouthud (14, 109, c, outhudbuffer);
					srand (nearstar_identity);
					if (nearstar_class == 6 || nearstar_class == 5) {
						ir = random (50);
						if (nearstar_class == 5) {
							ir -= 125 / dsd;
							if (ir <= 0) ir = 1;
						}
						else
							ir -= 25 / dsd;
					}
					else
						ir = 0;
					sprintf (outhudbuffer, "LI+ IONS: %ld MTPD EST.", ir);
					wrouthud (14, 119, c, outhudbuffer);
					tmp_float = 50 + random (10) - random (10);
					tmp_float *= (1 - eclipse);
					tmp_float *= 100 / dsd;
					if (nearstar_class == 11) {
						if (gl_start < 90)
							tmp_float *= 75 + random (50);
						else
							tmp_float *= 50;
					}
					if (nearstar_class == 10) tmp_float *= 0.25;
					if (nearstar_class == 9) tmp_float *= 3;
					if (nearstar_class == 8) tmp_float *= 1.5;
					if (nearstar_class == 7) tmp_float *= 25;
					if (nearstar_class == 6) tmp_float *= 0.01;
					if (nearstar_class == 5) tmp_float *= 0.1;
					if (nearstar_class == 4) tmp_float *= 10;
					if (nearstar_class == 3) tmp_float *= 0.5;
					if (nearstar_class == 2) tmp_float *= 18;
					if (nearstar_class == 1) tmp_float *= 5;
					srand (secs);
					tmp_float *= 1
						   + (float)(random(100)) * 0.001
						   - (float)(random(100)) * 0.001;
					sprintf (outhudbuffer, "RADIATION: %1.1f KR", tmp_float);
					wrouthud (14, 126, c, outhudbuffer);
					break;
			}}
		}
		datasheetscroll +=
			datasheetdelta;
		if (datasheetscroll > 100)
			datasheetscroll = 100;
		if (datasheetscroll < 0) {
			datasheetscroll = 0;
			data = 0;
		}
		//
		// Croce di puntamento planetaria.
		//
		if ((ip_targetted != -1 && !ip_reached) || ip_targetting) {
			planet_xyz (ip_targetted);
			if (far_pixel_at (plx, ply, plz, 0, 1)) {
				asm {   les di, dword ptr adapted
					add di, vptr
					sub di, 640
					mov al, 126
					mov es:[di-7], al
					mov es:[di-6], al
					mov es:[di-5], al
					mov es:[di-4], al
					mov es:[di+4], al
					mov es:[di+5], al
					mov es:[di+6], al
					mov es:[di+7], al
					mov es:[di-320*7], al
					mov es:[di-320*6], al
					mov es:[di-320*5], al
					mov es:[di-320*4], al
					mov es:[di+320*4], al
					mov es:[di+320*5], al
					mov es:[di+320*6], al
					mov es:[di+320*7], al }
			}
		}
		//
		// Croce di puntamento della stella-bersaglio,
		// aggiornamento nome della stella-bersaglio,
		// spostamenti interstellari suplucsi.
		//
		dxx = dzat_x - ap_target_x;
		dyy = dzat_y - ap_target_y;
		dzz = dzat_z - ap_target_z;
		l_dsd = sqrt (dxx*dxx + dyy*dyy + dzz*dzz);
		if (ap_targetting || l_dsd>10000) {
			pointer_cross_for (ap_target_x,
					   ap_target_y,
					   ap_target_z);
		}
		if (ap_targetting || ap_targetted) {
			update_star_label ();
			if (stspeed == 1) {
				if (ap_targetted == -1)
					ras = 25000;
				else {
					if (anti_rad)
						ras = 44 * ap_target_ray;
					else
						ras = 1.5 * ap_target_ray;
					if (l_dsd < 20000 && nsnp) {
						prepare_nearstar ();
						nsnp = 0;
					}
				}
				if (l_dsd < ras) {
					status ("CALIBRATED", 50);
					ap_reached = 1;
					stspeed = 0;
				}
				else {
					if (l_dsd > 0.9999 * ap_target_initial_d) {
						requested_vimana_coefficient =   0.001 * l_dsd;
						status ("CHARGING", 0);
						vimana_reaction_time = 0.1;
						goto ap_drive_mode;
					}
					if (l_dsd < 7500 + ras) {
						requested_vimana_coefficient =   0.005 * l_dsd;
						status ("PARKING", 0);
						vimana_reaction_time = 0.01;
						goto ap_drive_mode;
					}
					if (l_dsd < 15000 + ras) {
						requested_vimana_coefficient =   0.005 * l_dsd;
						status ("LINKING", 0);
						vimana_reaction_time = 0.0025;
						goto ap_drive_mode;
					}
					if (l_dsd < 0.9990 * ap_target_initial_d) {
						requested_vimana_coefficient = 0.00001 * l_dsd;
						status ("DRIVING", 0);
						vimana_reaction_time = 0.05;
						goto ap_drive_mode;
					}
					requested_vimana_coefficient = 0.0002 * l_dsd;
					status ("IGNITION", 0);
					if (vimana_reaction_time != 0.08)
						vimana_reaction_time = 0.08;
				ap_drive_mode:
					current_vimana_coefficient +=
						(requested_vimana_coefficient -
						 current_vimana_coefficient)
						 * vimana_reaction_time;
					if (current_vimana_coefficient < 10)
						current_vimana_coefficient = 10;
					dzat_x -= dxx / current_vimana_coefficient;
					dzat_y -= dyy / current_vimana_coefficient;
					dzat_z -= dzz / current_vimana_coefficient;
					pwr    -= l_dsd * 1E-5;
				}
			}
		}
		//
		// Sincronizzazione della navicella con i moti planetari,
		// spostamenti interplanetari suplucsi.
		//
	   resynctoplanet:
		if (ip_targetted != -1 && pwr > 15000) {
			planet_xyz (ip_targetted);
			dxx = dzat_x - plx;
			dyy = dzat_y - ply;
			dzz = dzat_z - plz;
			if (ip_reached && sync) {
				if (moviestat) {
					status ("MOVIEMAKER", 0);
				}
				else {
					status ("TRACKING", 0);
				}
				if (sync==1) { // fixed-point chase
					ang = (double)deg * (double)navigation_beta;
					hold_z = 1.8;
				}
				if (sync==2) { // far chase
					ang = (double)deg * (double)navigation_beta;
					hold_z = 5.4;
				}
				if (sync==3) { // syncrone orbit
					ang = (double)nearstar_p_rotation[ip_targetted] * (double)deg;
					hold_z = 1.8 + 0.1 * nearstar_p_ray[ip_targetted];
				}
				if (sync==4) { // vimana orbit
					ang = 7 * secs * (double)deg;
					hold_z = 3.6;
				}
				if (sync==5) { // near chase
					ang = (double)deg * (double)navigation_beta;
					hold_z = 1.2;
				}
				dxx += hold_z * nearstar_p_ray[ip_targetted] * sin (ang);
				dzz -= hold_z * nearstar_p_ray[ip_targetted] * cos (ang);
				if (_delay < 5) {
					dzat_x -= dxx * 0.05;
					dzat_y -= dyy * 0.05;
					dzat_z -= dzz * 0.05;
				}
				else {
					dzat_x -= dxx;
					dzat_y -= dyy;
					dzat_z -= dzz;
				}
			}
			if (ip_reaching) {
				l_dsd = sqrt (dxx*dxx + dyy*dyy + dzz*dzz);
				if (l_dsd > 0.99999 * ip_target_initial_d) {
					requested_approach_coefficient =  25 * l_dsd;
					status ("WARMING UP", 0);
					reaction_time = 0.001;
					goto ip_drive_mode;
				}
				if (l_dsd < 25 && ip_target_initial_d > 500) {
					requested_approach_coefficient =  50 * l_dsd;
					status ("REFINING", 0);
					reaction_time = 0.0002;
					goto ip_drive_mode;
				}
				if (l_dsd < 100 && ip_target_initial_d > 500) {
					requested_approach_coefficient =  15 * l_dsd;
					status ("BREAKING", 0);
					reaction_time = 0.0003;
					goto ip_drive_mode;
				}
				if (l_dsd < 0.99500 * ip_target_initial_d) {
					requested_approach_coefficient = 0.05 * l_dsd;
					status ("APPROACH", 0);
					reaction_time = 0.025;
					goto ip_drive_mode;
				}
				requested_approach_coefficient = 1.5 * l_dsd;
				status ("IGNITION", 0);
				reaction_time = 0.05;
			ip_drive_mode:
				current_approach_coefficient +=
					(requested_approach_coefficient -
					 current_approach_coefficient)
					 * reaction_time;
				if (current_approach_coefficient < 10)
					current_approach_coefficient = 10;
				dzat_x -= dxx / current_approach_coefficient;
				dzat_z -= dzz / current_approach_coefficient;
				dzat_y -= dyy / (0.5 * current_approach_coefficient);
				pwr    -= l_dsd * 0.5E-5;
				if (l_dsd < 2*nearstar_p_ray[ip_targetted]) {
					status ("STANDBY", 0);
					ip_reaching = 0;
					ip_reached = 1;
				}
			}
		}
		//
		// Gestione del consumo di litio.
		// Consumi supplementari, gestione ricariche.
		//
		additional_consumes ();
		//
		// Calcolo della distanza dalla stella pi� vicina,
		// per il controllo su radiazioni, eclissi, temperatura.
		//
		dxx = dzat_x - nearstar_x;
		dyy = dzat_y - nearstar_y;
		dzz = dzat_z - nearstar_z;
		dsd = sqrt (dxx*dxx + dyy*dyy + dzz*dzz) + 1;
		//
		// Allontanamento d'emergenza della navicella dalla stella.
		//
		if (dsd < (0.44+(double)(2*anti_rad))*nearstar_ray) {
			status ("CORRECTION", 100);
			dzat_x += (dxx / dsd) * 0.1;
			dzat_y += (dyy / dsd) * 0.1;
			dzat_z += (dzz / dsd) * 0.1;
		}
		//
		// Manovre di approvvigionamento litio dallo spazio
		// attorno alle stelle di classe 5 o a certe di classe 6.
		// Le stelle di classe 6 sono pi� difficili da sfruttare,
		// ma danno i migliori risultati. Quelle di classe 5 sono
		// sempre adatte, ma con scarsi risultati.
		//
		if (lithium_collector) {
			srand (nearstar_identity);
			ir = random (50);
			if (nearstar_class == 5) {
				ir -= 125 / dsd;
				if (ir <= 0) ir = 1;
			}
			else
				ir -= 25 / dsd;
			

			if (ir > 0) {
				if (pwr >= 20000 && charge >= 120) {
					status ("! FULL !", 100);
					lithium_collector = 0;
				}
				else {
					pwr += ir;
					if (pwr >= 20000) {
						if (charge < 120) {
							pwr = 15001;
							charge++;
						}
						else
							pwr = 20000;
					}
					status ("SCOPING...", 25);
				}
			}
			else {
				status ("GET CLOSER", 100);
				if (!reset_signal) lithium_collector = 0;
			}
		}
		//
		// Controllo eclissi.
		//
		stz = dzz * cos (deg * navigation_beta)
		    - dxx * sin (deg * navigation_beta)
		    - fabs (dyy) / 2;
		if (stz>dsd) stz = dsd;
		if (stz<0) stz = 0;
		stz /= 1.25;
		ras = (105 * nearstar_ray) / dsd;
		if (ras>66) ras = 66;
		if (ras<1) ras = 1;
		eclipse = 0;
		for (c=0; c<nearstar_nob; c++) {
		 if (nearstar_p_type[c]!=-1) {
			planet_xyz (c);
			dxx = dzat_x - plx;
			dyy = dzat_y - ply;
			dzz = dzat_z - plz;
			dpz = sqrt (dxx*dxx + dyy*dyy + dzz*dzz) + 0.001;
			if (dpz<10*nearstar_p_ray[c]) {
				watch (dzat_x, dzat_y, dzat_z,
				       nearstar_x, nearstar_y, nearstar_z);
				change_angle_of_view ();
				if (xy (dzat_x, dzat_y, dzat_z, plx, ply, plz)) {
					dasp = sqrt (delta_x * delta_x + delta_y * delta_y);
					rap = (105 * nearstar_p_ray[c]) / dpz;
					if (rap>66) rap = 66;
					eclipse = (dasp + ras - rap) / (2 * ras);
					if (eclipse>1) eclipse = 1;
					if (eclipse<0) eclipse = 0;
					eclipse = 1 - eclipse;
				}
			}
		 }
		}
		//
		// Reazione alle eclissi (oscura le tinte della navicella).
		// E abbassa la temperatura interna. Solo un po', perch� �
		// contrastata dalla climatizzazione.
		//
		//
		fast_srand (secs/2);
		pp_temp = 90 - dsd * 0.33;
		pp_temp -= 44;
		pp_temp *= fabs(pp_temp * 0.44);
		pp_temp -= (pp_temp + 273.15) * eclipse;
		if (pp_temp < -269) pp_temp = fast_flandom() - 269;
		if (!ontheroof) {
			if (pp_temp < 0) pp_temp = 0;
			if (pp_temp > 40) pp_temp = 40;
			while (pp_temp < 14) pp_temp += fast_flandom() * 2.5;
			while (pp_temp > 32) pp_temp -= fast_flandom() * 2.5;
			l_dsd = (24 * nearstar_ray) / (dsd - stz);
			l_dsd -= l_dsd * eclipse; pp_temp += l_dsd;
			if (pp_temp > 40) pp_temp = 40;
			while (pp_temp > 38) pp_temp -= fast_flandom() * 2.5;
			pp_pressure = 1;
		}
		else
			pp_pressure = 0;
		//
		ilight += ilightv;
		if (ilight<0) ilight = 0;
		if (ilight>63) ilight = 63;
		l_dsd = (15 * nearstar_ray * nearstar_r) / (dsd - stz);
		l_dsd -= l_dsd * eclipse;
		if (elight)
			ir3 = ilight + 30 - clock()%30 + l_dsd;
		else
			ir3 = ilight / 4 + l_dsd;
		if (ir3>nearstar_r+16) ir3 = nearstar_r+16;
		if (nearstar_class==11&&gl_start<90) ir = 5 * ir3;
		if (ir3>63) ir3 = 63;
		l_dsd = (7 * nearstar_ray * nearstar_g) / (dsd - stz);
		l_dsd -= l_dsd * eclipse;
		if (elight)
			ig3 = (ilight + 30 - clock()%30) / 2 + l_dsd;
		else
			ig3 = ilight / 2 + l_dsd;
		if (ig3>nearstar_g+32) ig3 = nearstar_g+32;
		if (nearstar_class==11&&gl_start<90) ig3 = 5 * ig3;
		if (ig3>63) ig3 = 63;
		l_dsd = (7 * nearstar_ray * nearstar_b) / (dsd - stz);
		l_dsd -= l_dsd * eclipse;
		if (elight)
			ib3 = (ilight + 30 - clock()%30) / 4 + l_dsd;
		else
			ib3 = ilight + l_dsd;
		if (nearstar_class==11&&gl_start<90) ib3 = 5 * ib3;
		if (ib3>63) ib3 = 63;
		if (gburst > 0 && gburst < 5) {
			ir3 += 8 * gburst;
			ig3 += 8 * gburst;
			ib3 += 8 * gburst;
		}
		if (ir3 != ir3e || ig3 != ig3e || ib3 != ib3e) {
			tavola_colori (range8088, 0, 64, ir3, ig3, ib3);
			ir3e = ir3; ig3e = ig3; ib3e = ib3;
		}
		//
		// Controllo del flag di richiesta di atterraggio.
		//
		if (land_now) {
			land_now = 0;
			landing_point = 0;
			holdtomiddle = 1;
			opencapdelta = 2;
			right_dblclick = 0;
			status ("UNLOCKING", 50);
		}
		//
		// Avanzamento contatore per le richieste d'aiuto.
		//
		if (gburst > 0) {
			gburst--;
			if (!gburst) {
				gburst = 63;
				status ("SIGNAL", 50);
			}
		}
		//
		// Countdown per il delay dei messaggi di stato dell'FCS.
		//
		if (fcs_status_delay)
			fcs_status_delay--;
		//
		// Countdown per il delay della funzione auto-sleep.
		//
		if (autoscreenoff) {
			aso_countdown--;
			if (aso_countdown <= 0) {
				aso_countdown = 100;
				sys = 4;
			}
		}
		//
		// Il protagonista sta sempre in una tutina... � normale.
		// Vede le cose attraverso uno scafandro, non ingombrante
		// ma pur sempre uno scafandro. La funzione "surrounding"
		// disegna i bordi dello scafandro, illuminati in relazione
		// all'ambiente circostante.
		//
		surrounding (0, 180);
		// riduzione stanchezza (continua, eventualmente
		// dall'ultima volta che si � scesi in superficie)
		// e variazioni nelle pulsazioni, pi� verosimili...
		fast_srand (secs/2);
		tiredness *= 0.9977;
		pp_pulse = (1 + tiredness) * 118;
		pp_pulse += fast_flandom () * 8;
		pp_pulse -= fast_flandom () * 8;
		// se si sta per scendere o si � appena risaliti,
		// si deve trattenere il player nel mezzo della navicella,
		// in quanto si suppone che sia bloccato nella capsula.
		if (holdtomiddle || lifter) {
			pos_x *= 0.75;
			hold_z = pos_z + 3100;
			pos_z -= hold_z * 0.25;
		}
		// effetto di apertura della capsula:
		// quando � totalmente aperta, si pu� scendere.
		// LQ significa Last Quadrant (ultimo quadrante visitato)
		if (opencapdelta < 0) {
			opencapcount += opencapdelta;
			if (opencapcount <= 0) {
				opencapdelta = 0;
				holdtomiddle = 0;
				sprintf (temp_distance_buffer, "LQ %03d:%03d", landing_pt_lon, landing_pt_lat);
				status (temp_distance_buffer, 100);
			}
		}
		// effetto di chiusura della capsula:
		// quando � totalmente sigillata, scotty beam me down.
		// al ritorno, comincia a riaprire la capsula...
		if (opencapdelta > 0) {
			opencapcount += opencapdelta;
			if (opencapcount >= 85) {
				entryflag = 0; planetary_main ();
				if (exitflag) goto allstop;
				opencapdelta = -2;
				holdtomiddle = 1;
				pp_gravity = 1;
				resolve = 0;
				_delay = 13; // solo al ritorno da superficie
				goto resynctoplanet;
			}
		}
		//
		// Page swap.
		//
		QUADWORDS = 16000;
		if (_delay == 13) _delay = 0;
		if (!_delay)
			pcopy (adaptor, adapted);
		else {
			pcopy (adapted, adapted);
			if (_delay>0 && _delay<10) _delay--;
		}
		QUADWORDS = pqw;
		if (resolve == 1) {
		 while (resolve <= 63) {
			tavola_colori (return_palette, 0, 256,
				       resolve, resolve, resolve);
			ir = clock(); while (ir == clock());
			resolve+=4;
		 }
		}
		if (resolve == 0)
			resolve = 1;
		//
		// Questa sezione controlla i pixels a ciclo continuo
		// che raffigurano le correnti convettive interne alle stelle,
		// ma entra in azione solo se la stella � abbastanza vicina,
		// altrimenti non si vede che un globo bianco: i colori
		// delle stelle non sono propriamente colori, ma solo tenui
		// sfumature; in pratica, essendo quasi tutte ben oltre il
		// livello di saturazione dell'occhio, le stelle appaiono
		// in genere tutte bianche, a meno che non le si avvicini
		// davvero moltissimo.
		//
		if (dsd < 1000 * nearstar_ray) {
			ir = nearstar_r;
			ig = nearstar_g;
			ib = nearstar_b;
			c = nearstar_class;
			if (c == 8) {
				fast_srand (nearstar_identity);
				srand (fast_random(0x7FFF));
				c = random (star_classes);
				ir = class_rgb[c * 3 + 0];
				ig = class_rgb[c * 3 + 1];
				ib = class_rgb[c * 3 + 2];
			}
			switch (c) {
				case 0:
					ir2 = 64;
					ig2 = 54;
					ib2 = 28;
					break;
				case 1:
					ir2 = 36;
					ig2 = 50;
					ib2 = 64;
					break;
				case 2:
					ir2 = 24;
					ig2 = 32;
					ib2 = 48;
					break;
				case 3:
					ir2 = 64;
					ig2 = 24;
					ib2 = 12;
					break;
				case 4:
					ir2 = 64;
					ig2 = 40;
					ib2 = 32;
					break;
				case 5:
					ir2 = 28;
					ig2 = 20;
					ib2 = 12;
					break;
				case 6:
					ir2 = 32;
					ig2 = 32;
					ib2 = 32;
					break;
				case 7:
					ir2 = 32;
					ig2 = 44;
					ib2 = 64;
					break;
				case 8:
					ir2 = 64;
					ig2 = 60;
					ib2 = 32;
					break;
				case 9:
					fast_srand (nearstar_identity);
					ir2 = 32 + fast_random(31);
					ig2 = 32 + fast_random(31);
					ib2 = 16 + fast_random(31);
					break;
				case 10:
					ir2 = 32;
					ig2 = 26;
					ib2 = 22;
					break;
				case 11:
					ir2 = 36;
					ig2 = 48;
					ib2 = 64;
			}
			satur = (6.4 * dsd) / nearstar_ray;
			if (satur > 44) satur = 44;
			if (ir<satur) ir = satur;
			if (ig<satur) ig = satur;
			if (ib<satur) ib = satur;
			if (ir2<satur) ir2 = satur;
			if (ig2<satur) ig2 = satur;
			if (ib2<satur) ib2 = satur;
		}
		else {
			ir = 48;
			ig = 56;
			ib = 64;
			ir2 = 24;
			ig2 = 32;
			ib2 = 40;
		}
		if (ire == ir && ige == ig && ibe == ib &&
		    ir2e == ir2 && ig2e == ig2 && ib2e == ib2) {
			if (!sky_palette_ok) {
				sky_palette_ok = 1;
				goto last_sky_palette_redefinition;
			}
		}
		else {
			sky_palette_ok = 0;
			last_sky_palette_redefinition:
			if (ire < ir) ire++; if (ire > ir) ire--;
			if (ige < ig) ige++; if (ige > ig) ige--;
			if (ibe < ib) ibe++; if (ibe > ib) ibe--;
			if (ir2e < ir2) ir2e++; if (ir2e > ir2) ir2e--;
			if (ig2e < ig2) ig2e++; if (ig2e > ig2) ig2e--;
			if (ib2e < ib2) ib2e++; if (ib2e > ib2) ib2e--;
			shade (tmppal, 64+00, 24, 0, 0, 0, ir2e, ig2e, ib2e);
			shade (tmppal, 64+24, 16, ir2e, ig2e, ib2e, ire, ige, ibe);
			shade (tmppal, 64+40, 24, ire, ige, ibe, 64, 70, 76);
			tavola_colori (tmppal + 3*64, 64, 64, 63, 63, 63);
		}
		if (!farstar) {
			for (ir = 0; ir < 64800; ir++) {
				ig = (s_background[ir] + 1) % 64;
				ib = (s_background[ir] >> 6) << 6;
				s_background[ir] = ig + ib;
			}
			//QUADWORDS = 16200;
			//pcycle (s_background, 1);
			//QUADWORDS = pqw;
		}

		// added roofspeed flag as a switchable option ('s' key) (neuzd)
		if (!ontheroof || !roofspeed){
		   sync_stop ();
		}

		//
		// Hook per la gestione dei moti delle caratteristiche
		// di superficie dei pianeti (come le macchie che si
		// spostano e la turbolenza atmosferica): l'hook aggiorna
		// l'aspetto della superficie una volta ogni 5 minuti.
		//
		if (ip_targetted!=-1&&ip_reached) {
			if ((long)secs%300 == 0)
				npcs = -12345;
		}
		//
		// Input da tastiera: snapshot, fine sessione,
		// selezione bersagli, attribuzione labels etc...
		//
		if (movie) {
			moviedelay = moviedelay - 1;
			if (moviedelay <= 0) {
				snapshot (0, 0);
				moviedelay = moviefsec;
			}
		}
		if (ontheroof) {
			if (tasto_premuto()) {
				c = attendi_pressione_tasto();
				if (!c) {
					c = attendi_pressione_tasto();
					// moved as I changed the controls to allow movie (neuzd)
					/*if (c=='*' || c=='m'){
	                	snapshot (0, 1);
	                }
	                if (c=='b') {
						snapshot (0, 0);
					}
					*/
	                if (c==0x3D){								// f3 - moviestat
						if (moviestat == 0){
							moviestat = 1;
							
						}
						else {
							moviestat = 0;
						}
					}
					if (c==144 && moviestat && !movie) {					//add 1 to moviedeck
						if (moviedeck < 999) {
							moviedeck++;
							/*sprintf (snapfilename, "..\\MOVIES\\%02i\\00000001.BMP", moviedeck);
							FILE * tmpFile = fopen(snapfilename, "r");
							if (tmpFile == NULL) {
								movieexists = 0;
							} else {
								movieexists = 1;
							}*/
							movieexists = 0;
							movienr = 0;
						}
					}
					if (c==142 && moviestat && !movie ) {					//substract 1 from moviedeck
						if (moviedeck > 1) {
							moviedeck--;
							/*sprintf (snapfilename, "..\\MOVIES\\%02i\\00000001.BMP", moviedeck);
							FILE * tmpFile = fopen(snapfilename, "r");
							if (tmpFile == NULL) {
								movieexists = 0;
							} else {
								movieexists = 1;
							}*/	
							movieexists = 0;
							movienr = 0;
						}
					}
				}
				else
				{
					if (c=='*' || c=='m'){
	                	snapshot (0, 1);
	                }
	                if (c=='b') {
						snapshot (0, 0);
					}
					if (c==13) {
						if (movie == 0 && moviestat) {
							movie = 1;
							moviefscap = 0;
							moviestime = gtime;
							moviestat = 0;
							movienr = 0;
							moviepaused = 0;
						}
						else if (movie == 1) {
	                                           movie = 0;
	                                           movieexists = 0;
	                                           moviedeck++;
	                                           moviepaused = 0;
	                                          }
					}
					if (c=='f' && moviestat && !movie) {
						if (movieflashoff == 1) movieflashoff = 0;
						else movieflashoff = 1;
					}
					// Added a pause feature when filming a movie (neuzd)
					if (c == 'p')
					{
						if (movie)
						{
							movie = 0;
							moviestat = 1;
							moviepaused = 1;
							status ("PAUSE REC",100);
						}
						else
						{
							if (moviestat && moviepaused)
							{
								movie = 1;
								moviestat = 0;
								moviepaused = 0;
								status ("RESUME REC",100);
							}
						}
					}
				}
			}
			else
				c = 0;
			
			goto endmain;
		}
		
		//
		// non saprei...
		// un'istruzione di debug,
		// o la lascio come "codice segreto"?
		// fa istantaneamente il pieno di litio.
		//
		/*ir = ctrlkeys[0];
		ir &= 0x70;
		if (ir == 0x70) {
			charge = 120;
			pwr = 20000;
			ctrlkeys[0] = 0x20;
		}*/
		//
		// Comandi da tastiera.
		//
		if (goesk_e != -1) {
			c = goesk_e; goesk_e = -1;
			goto goesk_e_reentry;
		}
		if (goesk_a != -1) {
			c = goesk_a; goesk_a = -1;
			goto goesk_a_reentry;
		}
		if (active_screen == -1 && tasto_premuto()) {
		 while (tasto_premuto()) {
			c = attendi_pressione_tasto();
			if (!c) {
				c = attendi_pressione_tasto();
				if (c==0x3D){								// f3 - moviestat
					if (moviestat == 0){
						moviestat = 1;
						
						char tempsnapfile[24];
						if (movieexists == 0)
						{								//let's check if the moviedeck exists already.... But only if the moviedeck has been changed lately.
							//wrouthud (10,90, NULL, "CHECKING!!!");
							sprintf (tempsnapfile, "..\\MOVIES\\%03i\\00000001.BMP", moviedeck);
							FILE * tmpFile = fopen(tempsnapfile, "r");
							if (tmpFile == NULL) {
								movieexists = 1;
							} else {
								movieexists = 2;
							}
							fclose(tmpFile);
						}
						char  short_text[11];
						if (movieexists == 2)
						{
							sprintf (short_text, "%i EXISTS", moviedeck);
							status(short_text,100);
						}
						else
						{
							sprintf (short_text, "MD %i FREE", moviedeck);
							status(short_text,100);
						}
					}
					else {
						moviestat = 0;
						status("MVMENU OFF",100);
					}
				}
				if (c==144 && moviestat && !movie) {					//add 1 to moviedeck
					if (moviedeck < 999) {
						moviedeck++;
						char tempsnapfile[24];
						sprintf (tempsnapfile, "..\\MOVIES\\%03i\\00000001.BMP", moviedeck);
						FILE * tmpFile = fopen(tempsnapfile, "r");
						if (tmpFile == NULL) {
							movieexists = 1;
						} else {
							movieexists = 2;
						}
						fclose(tmpFile);
						char  short_text[11];
						if (movieexists == 2)
						{
							sprintf (short_text, "%i EXISTS", moviedeck);
							status(short_text,100);
						}
						else
						{
							sprintf (short_text, "MD %i FREE", moviedeck);
							status(short_text,100);
						}
						//movieexists = 0;
						movienr = 0;
					}
				}
				if (c==142 && moviestat && !movie ) {					//substract 1 from moviedeck
					if (moviedeck > 1) {
						moviedeck--;
						char tempsnapfile[24];
						sprintf (tempsnapfile, "..\\MOVIES\\%03i\\00000001.BMP", moviedeck);
						FILE * tmpFile = fopen(tempsnapfile, "r");
						if (tmpFile == NULL) {
							movieexists = 1;
						} else {
							movieexists = 2;
						}
						fclose(tmpFile);
						char  short_text[11];
						if (movieexists == 2)
						{
							sprintf (short_text, "%i EXISTS", moviedeck);
							status(short_text,100);
						}
						else
						{
							sprintf (short_text, "MD %i FREE", moviedeck);
							status(short_text,100);
						}
						//movieexists = 0;
						movienr = 0;
					}
				}
				
				goesk_e_reentry:
				if (targets_in_range) {
					if (c == 80) {
						if (target_line < 2) {
							if (topmost_target + target_line < targets - 1)
								target_line ++;
						}
						else {
							if (topmost_target < targets - 3) {
								topmost_target ++;
								update_targets = 1;
							}
						}
					}
					if (c == 72) {
						if (target_line > 0)
							target_line --;
						else {
							if (topmost_target > 0) {
								topmost_target --;
								update_targets = 1;
							}
						}
					}
					goto endmain;
				}
				if (c==75) {
					dlt_nav_beta += 1.5;
					status ("PITCH - R", 25);
				}
				if (c==77) {
					dlt_nav_beta -= 1.5;
					status ("PITCH - L", 25);
				}
				if (c==72)
					lifter = -100;
				
			}
			else {
				goesk_a_reentry:
				if (c=='*' || c=='m' && !(labstar || labplanet) ) {
					snapshot (0, 1);
					goto endmain;
				}
				if (c=='b' && !(labstar || labplanet) ) {
					snapshot (0, 0);
					goto endmain;
				}
				if (c=='+' && moviestat && moviefsec < 999 && !movie)
				{
					moviefsec++;
					char  short_text[11];
					sprintf (short_text, "CAPT %i FR", moviefsec);
					status(short_text,100);
				}
				if (c=='-' && moviestat && moviefsec > 1 && !movie )
				{
					moviefsec--;
					char  short_text[11];
					sprintf (short_text, "CAPT %i FR", moviefsec);
					status(short_text,100);
				}
				if (c == 13) {
					if (!ip_targetting && !labstar && !labplanet && !targets_in_range && !manual_target)
					{
						if (movie == 0 && moviestat) {
							status ("RECORDING",50);
							movie = 1;
							moviefscap = 0;
							moviestime = gtime;
							moviestat = 0;
							movienr = 0;
							moviepaused = 0;
						}
						else if (movie == 1)
						{
							status ("STOP REC",50);
	                        movie = 0;
	                        movieexists = 0;
	                        moviedeck++;
	                        moviepaused = 0;
	                        //movieexists = 2;
	                    }
                	}
				}
				if (c=='f' && moviestat && !movie) {
					if (movieflashoff == 1)
					{
						movieflashoff = 0;
						status("BLK FLASH", 100);
					}
					else
					{
						movieflashoff = 1;
						status("NO FLASH", 100);
					}
				}
				if (c=='s')
				{
					if (roofspeed)
					{
						roofspeed = 0;
						status("NO ROOFSPD",50);
					}
					else
					{
						roofspeed = 1;
						status("ROOFSPEED",50);
					}
					
				}
				// Added a pause feature when filming a movie (neuzd)
				if (c == 'p')
				{
					if (movie)
					{
						movie = 0;
						moviestat = 1;
						moviepaused = 1;
						status ("PAUSE REC",100);
					}
					else
					{
						if (moviestat && moviepaused)
						{
							movie = 1;
							moviestat = 0;
							moviepaused = 0;
							status ("RESUME REC",100);
						}
					}
				}
				if (data) {
					if (c == 27) {
						c = 0;
						datasheetdelta = -2;
						goto endmain;
					}
				}
				if (ap_targetting) {
					if (c == 27) {
						c = 0;
						ap_targetting = 0;
						ap_targetted = 0;
						status ("CANCELLED", 50);
						goto endmain;
					}
				}
				if (labstar) {
					if (c == 27) {
						c = 0;
						labstar = 0;
						ap_target_previd = -1;
						goto endmain;
					}
					if (c >= 32 && c <= 126 && labstar_char < 20) {
						if (c >= 'a' && c <= 'z') c -= 32;
						star_label[labstar_char] = c;
						labstar_char++;
					}
					if (c == 8 && labstar_char > 0) {
						labstar_char--;
						star_label[labstar_char] = 32;
					}
					if (c == 13) {
						dev_page = 3;
						s_command = 1;
						dev_commands ();
					}
					goto endmain;
				}
				if (labplanet) {
					if (c == 27) {
						c = 0;
						labplanet = 0;
						prev_planet_id = -1;
						goto endmain;
					}
					if (c >= 32 && c <= 126 && labplanet_char < 20) {
						if (c >= 'a' && c <= 'z') c -= 32;
						planet_label[labplanet_char] = c;
						labplanet_char++;
					}
					if (c == 8 && labplanet_char > 0) {
						labplanet_char--;
						planet_label[labplanet_char] = 32;
					}
					if (c == 13) {
						dev_page = 3;
						s_command = 2;
						dev_commands ();
					}
					goto endmain;
				}
				if (targets_in_range) {
					if (c == 27) {
						c = 0;
						targets_in_range = 0;
						goto endmain;
					}
					if (c == 13) {
						if (!collecting_targets && topmost_target + target_line < targets) {
							ap_target_x = targets_table_px[topmost_target + target_line];
							ap_target_y = targets_table_py[topmost_target + target_line];
							ap_target_z = targets_table_pz[topmost_target + target_line];
							extract_ap_target_infos ();
							fix_remote_target ();
						}
						goto endmain;
					}
				}
				if (ip_targetting) {
					if (c == 27) {
						c = 0;
						ip_targetted = -1;
						ip_targetting = 0;
						status ("CANCELLED", 50);
						goto endmain;
					}
					if (c == 8 && iptargetchar > 0) {
						iptargetchar--;
						iptargetstring[iptargetchar] = 0;
						status (iptargetstring, 100);
					}
					if (((c >= '0' && c <= '9') || c == '/') && iptargetchar < 10) {
						if (c == '/') {
							if (iptargetchar == 0) goto endmain;
							ir = 0;
							while (ir < iptargetchar) {
								if (iptargetstring[ir] == '/') goto endmain;
								ir++;
							}
						}
						iptargetstring[iptargetchar] = c;
						iptargetstring[iptargetchar+1] = 0;
						iptargetchar++;
						status (iptargetstring, 100);
					}
					if (c == 13) {
						if (iptargetchar == 0) goto endmain;
						ir = 0;
						while (ir < iptargetchar) {
							if (iptargetstring[ir] == '/') {
								iptargetstring[ir] = 0;
								iptargetmoon = atoi (iptargetstring);
								iptargetstring[ir] = '/';
								if (iptargetstring[ir + 1] != 0) {
									iptargetplanet = atoi (iptargetstring + ir + 1);
									goto searchmoon;
								}
								status ("NOT EXTANT", 100);
								goto endmain;
							}
							ir++;
						}
						iptargetplanet = atoi (iptargetstring);
						if (iptargetplanet != 0 && iptargetplanet <= nearstar_nop) {
							ip_targetted = iptargetplanet - 1;
							fix_local_target ();
							ip_targetting = 0;
						}
						status ("NOT EXTANT", 100);
						goto endmain;
				    searchmoon:	ir = 0;
						while (ir < nearstar_nob) {
							if (nearstar_p_owner[ir] == iptargetplanet-1 && nearstar_p_moonid[ir] == iptargetmoon-1) {
								ip_targetted = ir;
								fix_local_target ();
								ip_targetting = 0;
								goto endmain;
							}
							ir++;
						}
						status ("NOT EXTANT", 100);
					}
					goto endmain;
				}
				if (manual_target) {
					if (c == 27) {
						c = 0;
						manual_target = 0;
						ap_targetted = 0;
						status ("CANCELLED", 50);
						goto endmain;
					}
					if (c == 8 && mt_string_char > 0) {
						mt_string_char--;
						switch (mt_coord) {
							case 0: manual_x_string[mt_string_char] = 0;
								break;
							case 1: manual_y_string[mt_string_char] = 0;
								break;
							case 2: manual_z_string[mt_string_char] = 0;
						}
					}
					if ((c >= '0' && c <= '9' && mt_string_char < 10) || (c == '-' && mt_string_char == 0)) {
						switch (mt_coord) {
							case 0: manual_x_string[mt_string_char] = c;
								manual_x_string[mt_string_char+1] = 0;
								break;
							case 1: manual_y_string[mt_string_char] = c;
								manual_y_string[mt_string_char+1] = 0;
								break;
							case 2: manual_z_string[mt_string_char] = c;
								manual_z_string[mt_string_char+1] = 0;
						}
						mt_string_char++;
					}
					if (c == 13) {
						switch (mt_coord) {
							case 0: ap_target_x = atol (manual_x_string);
								manual_y_string[0] = 0; break;
							case 1: ap_target_y = - atol (manual_y_string);
								manual_z_string[0] = 0; break;
							case 2: ap_target_z = atol (manual_z_string);
						}
						mt_string_char = 0;
						mt_coord ++;
						if (mt_coord > 2) {
							manual_target = 0;
							fix_remote_target ();
							ap_targetted = -1;
						}
					}
					switch (mt_coord) {
						case 0: sprintf (temp_distance_buffer, "%s", manual_x_string);
							break;
						case 1: sprintf (temp_distance_buffer, "%s", manual_y_string);
							break;
						case 2: sprintf (temp_distance_buffer, "%s", manual_z_string);
					}
					if (mt_coord <= 2)
						status (temp_distance_buffer, 100);
					goto endmain;
				}
				if (!ap_targetting&&!ip_targetting) {
					aso_countdown = 100;
					switch (c) {
						case '5': sys = 1; dev_page = 0; break;
						case 'r': sys = 2; dev_page = 0; break;
						case 'd': sys = 3; dev_page = 0; break;
						case 'x': sys = 4; dev_page = 0; break;
						case '6': s_command = 1; commands (); break;
						case '7': s_command = 2; commands (); break;
						case '8': s_command = 3; commands (); break;
						case '9': s_command = 4; commands (); break;
					}
				}
				if (c=='+' && surlight < 63 && !moviestat) surlight++;
				if (c=='-' && surlight > 10 && !moviestat) surlight--;
				//if (c=='+' && surlight < 63) surlight++;
				//if (c=='-' && surlight > 10) surlight--;
			}
	endmain: 
		
		
		
		}
		}
		else
			c = 0;
	} while ((c != 27) || stspeed || ip_reaching || lifter);

	remove (surface_file);

    allstop:
	_80_25_C ();

	freeze ();
}