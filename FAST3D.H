/*

	Grafica ad alta precisione a bastoncini luminosi,
	ovverosia con un'evanescente fosforescenza.
	Visualizzazione full-screen.

	Colegabile a: Text3d.h per testi 3d a bastoncini.

	30 Ottobre 1994 / 4 Febbraio 1996.
	Alessandro Ghignola & Fottifoh.

*/

#ifndef __SCANLINE_H
#include <scanline.h>
#endif

#ifndef __ALLOC_H
#include <alloc.h>
#endif

#ifndef __MATH_H
#include <math.h>
#endif

unsigned char far * adaptor = (unsigned char far *) 0xA0000000;
unsigned char far * adapted = (unsigned char far *) 0x80000000;
unsigned char far * fake_adaptor = (unsigned char far *) 0x80000000;

void _320_200_256 () // inizializza grafica a 320x200x256 colori.
{
	asm {
		push ax
		mov ax, 0x13
		int 0x10
		pop ax
	}
}

void _80_25_C () // modo grafico 80x25 testo a colori.
{
	asm {
		push ax
		mov ax, 3
		int 0x10
		pop ax
	}
}

int attendi_pressione_tasto () // aspetta un tasto e d� qual'�.
{
	unsigned char ritorno;

	asm {
		push ax
		mov ah, 8
		int 0x21
		mov ritorno, al
		pop ax
	}

	return (ritorno);
}

int tasto_premuto () // torna 1 se c'� un tasto premuto da estrarre.
{
	asm {
		push ax
		mov ah, 0xb
		int 0x21
		cmp al, 0xff
		pop ax
		jne Stop
	}

	return (1);

	Stop:
	return (0);
}

unsigned char tmppal[768];

void tavola_colori (unsigned char *nuova_tavolozza,
		    unsigned colore_di_partenza, unsigned nr_colori,
		    char filtro_rosso, char filtro_verde, char filtro_blu)
{
	int c, cc = 0;
	unsigned temp;
	nr_colori *= 3;
	colore_di_partenza *= 3;

	c = colore_di_partenza;
	while (cc<nr_colori) {
		tmppal[c] = nuova_tavolozza[cc];
		cc++;
		c++;
	}

	c = colore_di_partenza;
	while (c<nr_colori+colore_di_partenza) {
		temp = tmppal[c];
		temp *= filtro_rosso;
		temp /= 63;
		tmppal[c] = temp;
		c++;
		temp = tmppal[c];
		temp *= filtro_verde;
		temp /= 63;
		tmppal[c] = temp;
		c++;
		temp = tmppal[c];
		temp *= filtro_blu;
		temp /= 63;
		tmppal[c] = temp;
		c++;
	}

	asm {
		push si
		push ax
		push cx
		push dx
		mov cx, colore_di_partenza
		add cx, nr_colori
		lea si, tmppal
		mov ax, seg tmppal
		mov ds, ax
		mov dx, 0x3c8
		mov al, 0
		out dx, al
		inc dx
	}
dzap:	asm {
		mov al, [si]
		out dx, al
		inc si
		loop dzap
		pop dx
		pop cx
		pop ax
		pop si
	}
}

/* Lettura del mouse e ritorno nelle variabili indicate. */

int mdltx = 0, mdlty = 0, mx = 0, my = 0, mpul = 0;

void mouse_input ()
{
	asm {
		push ax
		push bx
		push cx
		push dx
		mov ax, 0xb
		int 0x33
		push cx
		push dx
		mov ax, 5
		int 0x33
		pop dx
		pop cx
		mov mdltx, cx
		mov mdlty, dx
		add mx, cx
		add my, dx
		cmp ax, 0
		je  non_imp
		mov mpul, ax
	}
non_imp:asm {
		pop dx
		pop cx
		pop bx
		pop ax
	}
}

// Ultraveloce copia di pagina grafica.

void pcopy (unsigned char far *dest, unsigned char far *sorg)
{
	asm {
		push si
		push di
		push ds
		push es
		push cx
		pushf
		cld
		lds si, dword ptr sorg
		les di, dword ptr dest
		mov cx, 16000
		db 0xf3 // macro: rep movsd
		db 0x66
		db 0xa5
		popf
		pop cx
		pop es
		pop ds
		pop di
		pop si
	}
}

// Ultraveloce cancella pagina grafica.

void pclear (unsigned char far *target, unsigned char pattern)
{
	asm {
		push di
		push es
		push cx
		push ax
		les di, dword ptr target
		mov cx, 16000
		mov al, pattern
		mov ah, pattern
		db 0x66 // macro: shl eax, 16
		db 0xc1
		db 0xe0
		db 0x10
		mov al, pattern
		mov ah, pattern
		db 0xf3 // macro: rep stosd
		db 0x66
		db 0xab
		pop ax
		pop cx
		pop es
		pop di
	}
}

const double Pi = 3.141592654;
//const double Deg = M_PI / 180;

float *tcos, *tsin;
float *tcosx, *tsinx;
float *tcosy, *tsiny;

double cam_x = 0;
double cam_y = 0;
double cam_z = 0;

int alfa  = 0;
int beta  = 0;

double kk;
double rx, ry, rz;
double x2, y2, z2;
double ox, oy, oz;

int zbasex = 200, zbasey = 150;
int nav_zbasex = 318, nav_zbasey = 198;
int lowerbound_y = -97, upperbound_y = 97;
int lowerbound_x = -157, upperbound_x = 157;

#define lwx 3
#define lwy 3
#define upx 317
#define upy 197

int x_centro = 160;
int y_centro = 100;

double uneg = 1;
double mindiff = 0.01;

// Inizializzazione.

void init ()
{
	/* Caricamento delle tabelle con i risultati delle funzioni
	   trigonometriche, spesso molto lente nell'esecuzione. */

	int c;
	double ang;

	tsin  = (float *) malloc (2880);
	tcos  = (float *) malloc (2880);

	tsinx = (float *) malloc (2888);
	tsiny = (float *) malloc (2888);
	tcosx = (float *) malloc (2888);
	tcosy = (float *) malloc (2888);

	if (!tcosy) return;

	for (ang=0, c=0; c<720; ang+=Pi/180, c++) {
		tsin[c] = sin (ang);
		tcos[c] = cos (ang);
	}

	for (ang=0, c=0; c<361; ang+=Pi/180, c++) {
		tsinx[c] = sin (ang) * zbasex; tcosx[c] = cos (ang) * zbasex;
		tsiny[c] = sin (ang) * zbasey; tcosy[c] = cos (ang) * zbasey;
	}
	for (ang=0, c=361; c<722; ang+=Pi/180, c++) {
		tsinx[c] = sin (ang) * nav_zbasex; tcosx[c] = cos (ang) * nav_zbasex;
		tsiny[c] = sin (ang) * nav_zbasey; tcosy[c] = cos (ang) * nav_zbasey;
	}
}

// Tracciamento linee 2d.

unsigned ptr;
unsigned long global_x, global_y;

void Segmento (unsigned long x, unsigned long y,
	       unsigned long x2, unsigned long y2)
{
	long a, b, L, temp;

	if (x==x2) {
		if (y>y2) {
			temp = y;
			y = y2;
			y2 = temp;
		}
		y2++;
		_DI = riga[y] + x;
		_AX = riga[y2];
		asm {   les si, dword ptr adapted
			add ax, si
			add si, di }
	clu:	asm {	cmp si, ax
			jnb _ret
			cmp byte ptr es:[si], 32
			jnb tz2
			add word ptr es:[si-1], 0x0402
			add byte ptr es:[si+1], 2
			add word ptr es:[si+319], 0x0201
			inc byte ptr es:[si+321]
			add word ptr es:[si-321], 0x0201
			inc byte ptr es:[si-319] }
	tz2:	asm {	add si, 320
			jmp clu }
	_ret:	return;
	}

	a = x2-x;
	if (a<0) {
		temp = x2; x2 = x; x = temp;
		temp = y2; y2 = y; y = temp;
		a = x2-x;
	}

	L = a;
	b = y2-y;
	if (b>0) {
		if (b>L)
			L=b;
	}
	else {
		if (-b>L)
			L=-b;
	}
	L++;

	x2 *= 65536;
	a *= 65536; a /= L;
	b *= 65536; b /= L;

	global_y = y * 65536;
	global_x = x * 65536;

	/* Traccia il segmento tramite DMA. */

	asm lea si, global_x[2];
	asm lea bx, global_y[2];
	asm les ax, dword ptr adapted;

	do {
	asm {   mov di, word ptr [bx]
		shl di, 1
		mov di, word ptr riga[di]
		add di, word ptr [si]
		cmp byte ptr es:[di+4], 32
		jnb tz
		add word ptr es:[di+3], 0x0402
		add byte ptr es:[di+5], 2
		add word ptr es:[di+323], 0x0201
		inc byte ptr es:[di+325]
		add word ptr es:[di-317], 0x0201
		inc byte ptr es:[di-315] }
	tz: 	global_y += b;
		global_x += a;
	} while (global_x<x2);
}

// Tracciamento linee 3d.

char explode = 0;

void Line3D (double p_x, double p_y, double p_z,
	     double x, double y, double z)
{
	long   fx, fy, lx, ly;
	double p_rx, p_ry, p_rz, p_z2, diff;

	if (explode) {
		p_rx = (p_x + x) / 2;
		p_ry = (p_y + y) / 2;
		p_rz = (p_z + z) / 2;
		kk = (10 / (double)explode) - 0.1;
		rx = (p_rx - ox) * kk;
		ry = (p_ry - oy) * kk;
		rz = (p_rz - oz) * kk;
		p_x += rx; p_y += ry; p_z += rz;
		x += rx; y += ry; z += rz;
	}

	p_x -= cam_x;
	p_y -= cam_y;
	p_z -= cam_z;

	p_z2 = p_z * tcos[beta] - p_x * tsin[beta];
	p_rz = p_z2 * tcos[alfa] + p_y * tsin[alfa];

	x -= cam_x;
	y -= cam_y;
	z -= cam_z;

	z2 = z * tcos[beta] - x * tsin[beta];
	rz = z2 * tcos[alfa] + y * tsin[alfa];

	if (rz<uneg&&p_rz<uneg) return;

	p_rx = p_x * tcosx[beta] + p_z * tsinx[beta];
	p_ry = p_y * tcosy[alfa] - p_z2 * tsiny[alfa];

	rx = x * tcosx[beta] + z * tsinx[beta];
	ry = y * tcosy[alfa] - z2 * tsiny[alfa];

	/* Conversione punti alle spalle dell'osservatore rispetto al piano
	   dello schermo. */

	if (rz<uneg) {
		diff = rz-p_rz;
		if (diff<-mindiff || diff>mindiff) {
			kk = (uneg-p_rz) / diff;
			rx = kk * (rx-p_rx) + p_rx;
			ry = kk * (ry-p_ry) + p_ry;
		}
		rz = uneg;
	}
	else {
		if (p_rz<uneg) {
			diff = p_rz-rz;
			if (diff<-mindiff || diff>mindiff) {
				kk = (uneg-rz) / diff;
				p_rx = kk * (p_rx-rx) + rx;
				p_ry = kk * (p_ry-ry) + ry;
			}
			p_rz = uneg;
		}
	}

	// Prospettiva.

	fx = rx / rz; fy = ry / rz;
	lx = p_rx / p_rz; ly = p_ry / p_rz;

	/* Ottimizza il tracciamento del "Segmento", tagliando via le parti
	   che di sicuro non si vedono. Questa parte non � efficace se
	   i punti di partenza e di arrivo fanno s� che il segmento non
	   intersechi alcun lato dell'area visibile: ma in questo peculiare
	   caso le linee vengono escluse dai controlli successivi. */

	if (fx<lowerbound_x) {
		diff = fx-lx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (lowerbound_x-lx) / diff;
			fy = kk * (fy-ly) + ly;
			fx = lowerbound_x;
		}
	}
	if (lx<lowerbound_x) {
		diff = lx-fx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (lowerbound_x-fx) / diff;
			ly = kk * (ly-fy) + fy;
			lx = lowerbound_x;
		}
	}
	if (fy<lowerbound_y) {
		diff = fy-ly;
		if (diff<-mindiff || diff>mindiff) {
			kk = (lowerbound_y-ly) / diff;
			fx = kk * (fx-lx) + lx;
			fy = lowerbound_y;
		}
	}
	if (ly<lowerbound_y) {
		diff = ly-fy;
		if (diff<-mindiff || diff>mindiff) {
			kk = (lowerbound_y-fy) / diff;
			lx = kk * (lx-fx) + fx;
			ly = lowerbound_y;
		}
	}
	if (fx>upperbound_x) {
		diff = fx-lx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (upperbound_x-lx) / diff;
			fy = kk * (fy-ly) + ly;
			fx = upperbound_x;
		}
	}
	if (lx>upperbound_x) {
		diff = lx-fx;
		if (diff<-mindiff || diff>mindiff) {
			kk = (upperbound_x-fx) / diff;
			ly = kk * (ly-fy) + fy;
			lx = upperbound_x;
		}
	}
	if (fy>upperbound_y) {
		diff = fy-ly;
		if (diff<-mindiff || diff>mindiff) {
			kk = (upperbound_y-ly) / diff;
			fx = kk * (fx-lx) + lx;
			fy = upperbound_y;
		}
	}
	if (ly>upperbound_y) {
		diff = ly-fy;
		if (diff<-mindiff || diff>mindiff) {
			kk = (upperbound_y-fy) / diff;
			lx = kk * (lx-fx) + fx;
			ly = upperbound_y;
		}
	}

	if (fx==lx&&fy==ly) return; // Esclude le linee costituite da un punto solo.

	if (fy<lowerbound_y||ly<lowerbound_y) return; // Esclude le linee che mai e poi mai si vedranno.
	if (fy>upperbound_y||ly>upperbound_y) return;
	if (fx<lowerbound_x||lx<lowerbound_x) return;
	if (fx>upperbound_x||lx>upperbound_x) return;

	Segmento (fx+x_centro, fy+y_centro, lx+x_centro, ly+y_centro);
}

// Conversioni x, y, z -> share_x, share_y

int share_x;
int share_y;

int C32 (double x, double y, double z)
{
	x -= cam_x;
	y -= cam_y;
	z -= cam_z;

	z2 = z * tcos[beta] - x * tsin[beta];
	rz = z2 * tcos[alfa] + y * tsin[alfa];

	if (rz<uneg) return (0); // Il punto non si vede. Inutile continuare.

	ry = y * tcosy[alfa] - z2 * tsiny[alfa];
	share_y = ry / rz;

	if (share_y<lowerbound_y)
		return (0);
	else {
		if (share_y>upperbound_y)
			return (0);
	}

	rx = x * tcosx[beta] + z * tsinx[beta];
	share_x = rx / rz;

	if (share_x<lowerbound_x)
		return (0);
	else {
		if (share_x>upperbound_x)
			return (0);
	}

	share_x += x_centro;
	share_y += y_centro;

	return(1);
}

/* Traccia una linea relativamente alle coordinate dell'origine di un
   Crystal Pixel. */

void rel (double sx, double sy, double sz, double fx, double fy, double fz)
{
	sx += ox; fx += ox;
	sy += oy; fy += oy;
	sz += oz; fz += oz;

	Line3D (sx, sy, sz, fx, fy, fz);
}

/* Traccia un rettangolo relativamente alle coordinate dell'origine di un
   Crystal Pixel. Lunghezza e larghezza sono date da 2*hx e 2*hz. */

void rectrel (double cx, double cy, double cz, double hx, double hz, char orient)
{
	cx += ox; cy += oy; cz += oz;

	if (orient==0) {
		Line3D (cx-hx, cy-hz, cz, cx+hx, cy-hz, cz);
		Line3D (cx+hx, cy-hz, cz, cx+hx, cy+hz, cz);
		Line3D (cx+hx, cy+hz, cz, cx-hx, cy+hz, cz);
		Line3D (cx-hx, cy+hz, cz, cx-hx, cy-hz, cz);
		return;
	}

	if (orient==1) {
		Line3D (cx-hx, cy, cz+hz, cx+hx, cy, cz+hz);
		Line3D (cx+hx, cy, cz+hz, cx+hx, cy, cz-hz);
		Line3D (cx+hx, cy, cz-hz, cx-hx, cy, cz-hz);
		Line3D (cx-hx, cy, cz-hz, cx-hx, cy, cz+hz);
		return;
	}

	Line3D (cx, cy-hz, cz-hx, cx, cy-hz, cz+hx);
	Line3D (cx, cy-hz, cz+hx, cx, cy+hz, cz+hx);
	Line3D (cx, cy+hz, cz+hx, cx, cy+hz, cz-hx);
	Line3D (cx, cy+hz, cz-hx, cx, cy-hz, cz-hx);
}

/* Traccia un parallelepipedo relativamente alle coordinate dell'origine
   di un Crystal Pixel. L'altezza e data da 2*hy, la lunghezza da 2*hx
   e la profondit� da 2*hz. */

void boxrel (double cx, double cy, double cz,
	     double hx, double hy, double hz)
{
	cx += ox; cy += oy; cz += oz;
	Line3D (cx-hx, cy-hy, cz+hz, cx-hx, cy+hy, cz+hz);
	Line3D (cx+hx, cy-hy, cz+hz, cx+hx, cy+hy, cz+hz);
	Line3D (cx+hx, cy-hy, cz-hz, cx+hx, cy+hy, cz-hz);
	Line3D (cx-hx, cy-hy, cz-hz, cx-hx, cy+hy, cz-hz);
	Line3D (cx-hx, cy-hy, cz+hz, cx+hx, cy-hy, cz+hz);
	Line3D (cx+hx, cy-hy, cz+hz, cx+hx, cy-hy, cz-hz);
	Line3D (cx+hx, cy-hy, cz-hz, cx-hx, cy-hy, cz-hz);
	Line3D (cx-hx, cy-hy, cz-hz, cx-hx, cy-hy, cz+hz);
	Line3D (cx-hx, cy+hy, cz+hz, cx+hx, cy+hy, cz+hz);
	Line3D (cx+hx, cy+hy, cz+hz, cx+hx, cy+hy, cz-hz);
	Line3D (cx+hx, cy+hy, cz-hz, cx-hx, cy+hy, cz-hz);
	Line3D (cx-hx, cy+hy, cz-hz, cx-hx, cy+hy, cz+hz);
}

// Fa un puntino. Coordinate relative al pixel.

void xrel (double px, double py, double pz)
{
	if (C32 (ox+px, oy+py, oz+pz)) {
		_DI = share_x+riga[share_y];
		asm {
			les si, dword ptr adapted
			add si, di
			cmp byte ptr es:[si], 32
			jnb tz
			add byte ptr es:[si], 8
			add byte ptr es:[si+1], 4
			add byte ptr es:[si-1], 4
			add byte ptr es:[si+319], 2
			add byte ptr es:[si-319], 2
			add byte ptr es:[si+320], 4
			add byte ptr es:[si-320], 4
			add byte ptr es:[si+321], 2
			add byte ptr es:[si-321], 2
		}
		tz:
	}
}