#include "defs.h"
#include "noctis-d.h"
#include "noctis-0.h"
#include "noctis-2.h"
// <temp> Temporary to make the code easier to mod later on
#define FLYING_LANDER 1
#define option_flying_lander 1
// </temp>

const double deg = M_PI / 180;

int	opencapcount;
float	refx, refy, refz;
float	sp_x, sp_y, sp_z;

/* Funzioni e variabili globali di tracciamento e gestione
   delle superfici planetarie, in poligonale (h! ce la far�?)

	Si, ce l'ho fatta, ahem... dunque...
	per tracciare in tempo utile una superficie di 40000 quadranti,
	ovvero 80000 triangoli, su una matrice di 200x200 (un po' tantino
	in effetti) ho applicato un procedimento di, boh...
	"focalizzazione" diciamo. Pi� probabilmente pu� chiamarsi
	"depth culling"... inclusi i riflessi, la funzione "fragment"
	passa al vaglio 160.000 poligoni per fotogramma, disegnandone
	comunque circa un decimo. E' importante ottimizzare quella,
	quando si vuole ottimizzare qualcosa... */

char 	sctype; // tipo di scenario.
#define OCEAN	1
#define PLAINS	2
#define DESERT	3
#define ICY	4

float dsd1, dsd2;     // distanza dal sole (pri/sec)
float nray1, nray2;   // raggio del sole (pri/sec)
float latitude;	      // latitudine (0..90, 90=poli)
float exposure;	      // longitudine relativa al centro dell'area diurna.
float sun_x, sun_y, sun_z; // coordinate del "sole" locale.
int   sh_delta;		// shift del puntatore di confronto per lo shading.

// dati riguardanti il "sole" primario se ci si trova
// attorno a un sole secondario (sistemi multipli di classe 8)
char jumping;
char jetpack;
extern char blinkhudlights;
extern char blinkhudlights_stay;

int   pri_crepzone;
int   pri_nightzone;
int   pri_sun_x_factor;
float pri_latitude, pri_exposure;
float pri_x, pri_y, pri_z;

char  mirror = 0;     // effetto specchio d'acqua (agisce su "fragment").
char  waves_in = 0;   // flag di presenza delle onde sui mari.
char  waves_out = 0;  // flag di produzione di onde sui mari.
long  T_SCALE;	      // scala della texture, passata in H/V_MATRIXS.

float base_pp_temp;
float base_pp_pressure;

float hpoint (long px, long pz)
{ // Trova l'altezza di un punto sulla superficie.
  // Per migliorare la risoluzione, usa un procedimento d'interpolazione
  // bi-lineare, con 16384 gradazioni intermedie, tante quante le unit�
  // logiche che compongono il lato di un quadrante; considera anche
  // la divisione dei quadranti in due triangoli isosceli.
	long cpos;

	float h1, h2, h3, h4, icx, icz, py;

	cpos   = m200[pz>>14] + (px>>14);

	h1     = - ((long)(p_surfacemap[cpos])     << 11);
	h2     = - ((long)(p_surfacemap[cpos+1])   << 11);
	h3     = - ((long)(p_surfacemap[cpos+201]) << 11);
	h4     = - ((long)(p_surfacemap[cpos+200]) << 11);

	icx    = px & 16383;
	icz    = pz & 16383;

	if (icx+icz<16384) {
		py  = h1 + (h2 - h1) * (icx * qid);
		py += (h4 - h1) * (icz * qid);
	}
	else {
		py  = h3 + (h4 - h3) * ((16384-icx) * qid);
		py += (h2 - h3) * ((16384-icz) * qid);
	}

	return (py);
}

/*

	-----------------------------------------------------------------
	Collezione di funzioni interdipendenti per il tracciamento degli
	oggetti che si possono trovare sulla superficie dei vari pianeti.
	-----------------------------------------------------------------

*/

char  groundflares = 0;		// tipo di tracciamento del suolo.
long  mushscaling = 8191;	// range variabilit� (in bitmask) "greenmush"
float treescaling = 4096;	// scalatura alberi, di solito mushscaling/2
float treespreads = 0.75;	// scalatura rami ad ogni ricorsione
float treepeaking = 1.25;	// passa come "distance_from_perfection"
float branchwidth = 0.15;	// larghezza dei rami rispetto alla lunghezza
float rootheight  = 0.50;	// altezza del tronco rispetto a "treescaling"
char  rootshade   = 0x00;	// colore di base del tronco
char  treeflares  = 0x00;	// tipo di tracciamento rami.
char  leafflares  = 0x00;       // tipo di tracciamento foglie.

float rockscaling = 500;	// dimensioni delle rocce.
float rockpeaking = 250;	// altezza delle rocce.
int   rockdensity = 15;		// densit� gruppi di rocce (bitmask).
char  quartz	  = 0;		// traccia quarzi trasparenti, se impostato.

int   detail_seed = 12345;

void greenmush (float x, float y, float z,
		unsigned char mask_1, unsigned char mask_2,
		long scaling, unsigned char colorgrade,
		unsigned char colormask, char noseed)
{ // produce una serie di gruppi di minuscole sagome verdi, studiate per
  // fornire l'impressione delle fronde degli alberi pi� distanti.

	long	correction;
	int	n1, m1, n2, m2;

	correction = scaling >> 1;
	x += correction;
	y += correction;
	z += correction;

	if (!noseed)
		fast_srand (((long)x>>14) + ((long)y>>14) + ((long)z>>14));

	m1 = 1 + fast_random (mask_1);
	for (n1 = 0; n1 < m1; n1++)
		if (getcoords (x - fast_random (scaling),
			       y - fast_random (scaling),
			       z - fast_random (scaling))) {
			m2 = 1 + fast_random (mask_2);
			for (n2 = 0; n2 < m2; n2++) {
				_DI = riga[_y_+fast_random(7)]+_x_+fast_random(7);
				_CL = colorgrade + fast_random (colormask);
				asm {	les ax, adapted
					mov es:[di+4], cl
					mov es:[di+5], cl
					mov es:[di+3], cl
					mov es:[di+324], cl
					mov es:[di-316], cl
					mov es:[di-636], cl }
			}
		}
}

void build_fractal_tree (float x, float y, float z,
			 float scaling, float reduction, float globalwidth,
			 long layers, long divisions, float distance_from_perfection,
			 unsigned char rootcolormask, unsigned char leafcolormask,
			 float branchdetail, char isrootnode, char occurrence)
{ // funzione ricorsiva: eventualmente traccia l'intero albero pseudo-casuale,
  // con una struttura multilivello, ma va usata con parsimonia perch� �
  // ovviamente una cosa piuttosto laboriosa in termini di tempo.
  //
  // serve qualche spiegazione per i parametri, che sono davvero tantini...
  //
  // P(x;y;z)    - origine del tronco (punto medio della base del tronco)
  // scaling     - altezza del tronco (determina anche la lunghezza dei rami)
  // reduction   - coefficiente di riduzione della lunghezza dei rami,
  //		   calcolata rispetto a quella del tronco livello per livello
  // globalwidth - coefficiente che determina la larghezza dei rami,
  //		   calcolata rispetto alla loro lunghezza
  // layers	 - numero di processi ricorsivi di suddivisione del tronco
  //		   una buona tattica per disegnare ciuffi d'erba � porre
  //		   questo parametro E il successivo entrambi a zero...
  // divisions   - maschera delle ramificazioni della cima di ogni ramo
  // branchdetail- step di rotazione nel tracciamento dei rami
  //		   (p.es. 120 traccia 360/120 = 3 poligoni per ramo)...
  //		   il minimo livello di dettaglio � 360, un poligono per ramo,
  //		   180 fa i rami piatti ma visibili da ogni lato,
  //		   e infine 120 approssima piuttosto bene...
  // isrootnode  - chiamare la funzione con questo parametro impostato a 1
  //		   per ottenere un albero normale, anche se si possono
  //		   agevolmente disegnare dei cespugli semplicemente ponendo
  //		   questo flag a zero, poich� un cespuglio pu� essere anche
  //		   approssimato, in effetti, come un albero senza tronco
  // occurrence  - � un contatore, va semplicemente posto a zero.
  //
  // rootcolormask � il colore di base per il tronco e per i rami.
  // leafcolormask � il colore di base per le foglie.
  //
  // distance_from_perfection, infine, � un coefficiente in gradi, che
  // esprime di quanto i rami "figli" possono divaricarsi ad ogni ulteriore
  // suddivisione dei rami "padri": in pratica, distance_from_perfection
  // rappresenta l'irregolarit� generale dell'albero - ad esempio, per le
  // latifoglie bisognerebbe impostare questo parametro ad un valore alto
  // (diciamo all'incirca 1.25), mentre per qualcosa di pi� simile ad una
  // conifera questo valore andrebbe abbassato all'incirca a 0.3 .. 0.4
  // (ponendolo a zero si otterrebbero rami in una colonna verticale).

	int   subdivs;
	char  polycolor;
	char  pf = flares;

	float x2, y2, z2;
	float fx[4], fy[4], fz[4];
	float widthscale1, widthscale2;
	float b_angle, b_angle_delta, range, rotation, rlimit;
	float rot2, rot3;

	long hm, vm, lseed;
	unsigned char huge *previoustexture;

	widthscale1 = scaling * globalwidth;
	widthscale2 = reduction * scaling * globalwidth;

	if (isrootnode) {
		subdivs = 1;
		range = scaling * distance_from_perfection * 0.2;
	}
	else {
		subdivs = 1 + fast_random (divisions);
		range = scaling * distance_from_perfection * 0.5;
	}

	rlimit = 360 - branchdetail;
	b_angle = 0;
	if (subdivs) {
		b_angle_delta = (float)(2*M_PI) / (float)subdivs;
		b_angle = b_angle_delta * 0.5;
	}

	lseed = x + y + z + detail_seed;
	while (subdivs) {
		fast_srand (lseed);
		lseed += 3;
		//
		if (layers) {
			flares = treeflares;
			hm = H_MATRIXS; vm = V_MATRIXS;
			H_MATRIXS = 3; V_MATRIXS = 8;
			change_txm_repeating_mode();
			previoustexture = txtr;
			x2 = x + cos(b_angle) * range;
			z2 = z + sin(b_angle) * range;
			if (isrootnode)
				y2 = y - (fast_flandom() * rootheight + 0.1) * scaling;
			else
				y2 = y - (fast_flandom() + 0.25) * scaling * 0.25;
			fy[0] = y;
			fy[1] = y;
			fy[2] = y2;
			fy[3] = y2;
			polycolor = 9 * occurrence;
			for (rotation = 0; rotation <= rlimit; rotation += branchdetail) {
				fx[0] = x  + lft_cos[rotation               ] * widthscale1;
				fz[0] = z  + lft_sin[rotation               ] * widthscale1;
				fx[1] = x  + lft_cos[rotation + branchdetail] * widthscale1;
				fz[1] = z  + lft_sin[rotation + branchdetail] * widthscale1;
				fx[2] = x2 + lft_cos[rotation + branchdetail] * widthscale2;
				fz[2] = z2 + lft_sin[rotation + branchdetail] * widthscale2;
				fx[3] = x2 + lft_cos[rotation               ] * widthscale2;
				fz[3] = z2 + lft_sin[rotation               ] * widthscale2;
				if (isrootnode) {
					fy[0] = hpoint (fx[0], fz[0]);
					fy[1] = hpoint (fx[1], fz[1]);
				}
				if (facing (fx, fy, fz))
					polymap (fx, fy, fz, 4, polycolor + rootcolormask);
				txtr += 48;
				polycolor += 4;
			}
			H_MATRIXS = hm; V_MATRIXS = vm;
			change_txm_repeating_mode();
			txtr = previoustexture;
			build_fractal_tree (x2, y2, z2, scaling * reduction, reduction,
					    globalwidth, layers - 1, divisions, distance_from_perfection,
					    rootcolormask, leafcolormask, branchdetail, 0, occurrence + 1);
		}
		else {
			flares = leafflares;
			hm = H_MATRIXS; vm = V_MATRIXS;
			H_MATRIXS = 1; V_MATRIXS = 3;
			change_txm_repeating_mode();
			previoustexture = txtr;
			x2 = x + fast_flandom() * range - fast_flandom() * range;
			z2 = z + fast_flandom() * range - fast_flandom() * range;
			fy[0] = y;
			fy[1] = y;
			fy[2] = y - fast_flandom() * scaling;
			polycolor = fast_random (31);
			rot2 = 0; rot3 = 0;
			for (rotation = 0; rotation <= rlimit; rotation += branchdetail) {
				rot2 = rotation + 72; if (rot2 > 359) rot2 -= 360;
				rot3 = rotation + 36; if (rot3 > 359) rot3 -= 360;
				fx[0] = x  + lft_cos[rotation]	* widthscale1;
				fz[0] = z  + lft_sin[rotation]	* widthscale1;
				fx[1] = x  + lft_cos[rot2]	* widthscale1;
				fz[1] = z  + lft_sin[rot2]	* widthscale1;
				fx[2] = x2 + lft_cos[rot3]	* range + wdircos;
				fz[2] = z2 + lft_sin[rot3]	* range + wdirsin;
				polymap (fx, fy, fz, 3, polycolor + leafcolormask);
				greenmush (fx[2], fy[2], fz[2], 15, 3, 1023, 223, 31, 0);
				polycolor += 2;
			}
			H_MATRIXS = hm; V_MATRIXS = vm;
			change_txm_repeating_mode();
			txtr = previoustexture;
		}
		//
		subdivs--;
		b_angle += b_angle_delta;
	}

	flares = pf;
}

#define FORCE_LAT	11
#define FORCE_CON	22
#define GIANT_TREE	333


void albero (float x, float y, float z, long depth)
{ // disegna alberi ove richiesti.

	fast_srand (x+y+z+3);
	int treetype = fast_random(511);

	if (treetype == GIANT_TREE) {
		if (depth > 11) {
			greenmush (x, y - treescaling * 0.5, z, 07, 15, mushscaling, 223, 31, 0);
			return;
		}
		if (depth > 07) {
			build_fractal_tree (x, y, z,
					    2*treescaling, treespreads, 1.5*branchwidth,
					    3, 2, 1.5*treepeaking, rootshade, 0xC0, 120, 1, 0);
			return;
		}
		if (depth > 04) {
			build_fractal_tree (x, y, z,
					    2*treescaling, treespreads, 1.5*branchwidth,
					    4, 3, 1.5*treepeaking, rootshade, 0xC0, 120, 1, 0);
			return;
		}
		build_fractal_tree (x, y, z,
				    2*treescaling, treespreads, 1.5*branchwidth,
				    4, 3, 1.5*treepeaking, rootshade, 0xC0, 72, 1, 0);
		return;
	}

	if (depth > 20) {
		greenmush (x, y - treescaling * 0.5, z, 03, 07, mushscaling, 223, 31, 0);
		return;
	}

	if (depth > 10) {
		greenmush (x, y - treescaling * 0.7, z, 07, 15, mushscaling, 223, 31, 0);
		return;
	}

	if (depth > 03) {
		greenmush (x, y - treescaling * 0.9, z, 15, 31, mushscaling, 223, 31, 0);
		return;
	}

	treetype >>= 3;

	if (treetype == FORCE_LAT) {
		build_fractal_tree (x, y, z,
				    treescaling, treespreads, branchwidth,
				    2, 3, 2*treepeaking, 0x80, 0xC0, 120, 1, 0);
		return;
	}

	if (treetype == FORCE_CON) {
		build_fractal_tree (x, y, z,
				    treescaling, treespreads, branchwidth,
				    2, 2, 0.5*treepeaking, 0x80, 0x40, 120, 1, 0);
		return;
	}

	build_fractal_tree (x, y, z,
			    treescaling, treespreads, branchwidth,
			    2, 2, treepeaking, rootshade, 0xC0, 120, 1, 0);

}

void cespuglio (float x, float y, float z, long depth)
{ // disegna un cespuglio.

	// da 48 mt in poi: ammasso di foglie.
	if (depth >= 3) {
		greenmush (x, y, z, 7, 7, mushscaling, 209, 31, 0);
		return;
	}

	switch (depth) {
		case 2: // 32 -- 48 mt: visibili i ramoscelli pi� grandi.
			build_fractal_tree (x, y, z, 3000, 0.75, 0.15, 1, 1, 1.5, 0x00, 0xC0, 180, 0, 0);
			break;
		case 1: // 16 -- 32 mt: visibili il 50% delle ramificazioni.
			build_fractal_tree (x, y, z, 3000, 0.75, 0.15, 1, 2, 1.5, 0x00, 0xC0, 120, 0, 0);
			break;
		case 0: //  0 -- 16 mt: cespuglio completo.
			build_fractal_tree (x, y, z, 3000, 0.75, 0.15, 1, 3, 1.5, 0x00, 0xC0, 120, 0, 0);
	}

}

void ciuffo (float x, float y, float z, long depth)
{ // disegna un ciuffo d'erba.

	// da 64 mt in poi, non � visibile.
	if (depth >= 4) return;

	switch (depth) {
		case 3: // 48 -- 64 mt: qualche macchietta.
			greenmush (x, y, z, 3, 7, 1023, 216, 31, 0);
			break;
		case 2: // 32 -- 48 mt: visibile un filo d'erba.
			build_fractal_tree (x, y, z, 1000, 1.00, 0.25, 0, 0, 1.0, 0x00, 0xC0, 120, 0, 0);
			break;
		case 1: // 16 -- 32 mt: visibili il 50% dei fili d'erba.
			build_fractal_tree (x, y, z, 1000, 1.00, 0.25, 0, 7, 1.0, 0x00, 0xC0,  90, 0, 0);
			break;
		case 0: //  0 -- 16 mt: un ciuffo completo.
			build_fractal_tree (x, y, z, 1000, 1.00, 0.25, 0, 7, 1.0, 0x00, 0xC0,  60, 0, 0);
	}

}

void roccia (float x, float y, float z, long depth)
{ // disegna una pietra fatta sulla base di un tetraedo per risparmiare tempo.

	float tx[4], ty[4], tz[4];
	float px[3], pz[3];
	float rs = rockscaling;

	char  rc[3], rcolor;
	int   cdown;

	// da 160 mt in poi: nulla di visibile.
	if (depth >= 8) return;

	// questo significa che non devono mai essere visualizzate rocce.
	if (!rockdensity) return;

	fast_srand (detail_seed);

	// e questo che IN questo quadrante non ci sono rocce.
	cdown = fast_random (rockdensity);
	if (!cdown) return;

	// da 32 a 160 mt: solo un triangolino.
	if (depth > 2) {
		tx[0] = x; tx[1] = x; tx[2] = x + rockscaling - fast_flandom () * rockscaling;
		tz[0] = z; tz[1] = z; tz[2] = z + rockscaling - fast_flandom () * rockscaling;
		ty[0] = y - 100 - fast_flandom () * rockpeaking;
		ty[1] = y - 100 - fast_flandom () * rockpeaking;
		ty[2] = y - 100 - fast_flandom () * rockpeaking;
		rcolor = fast_random (64 + 7);
		if (facing (tx, ty, tz))
			poly3d (tx, ty, tz, 3, rcolor);
		return;
	}

	// da 16 a 32 mt: tre triagolini disposti a tetraedo, senza base.
	// da 0 a 16 mt: con texture, e sassolini multipli se necessari.

	rockscaling *= 5;
	setfx (quartz);

rockrep:px[0] = x - fast_flandom () * rockscaling;
	pz[0] = z - fast_flandom () * rockscaling;
	px[1] = x;
	pz[1] = z + fast_flandom () * rockscaling;
	px[2] = x + fast_flandom () * rockscaling;
	pz[2] = z - fast_flandom () * rockscaling;

	rcolor = fast_random (64);
	rc[0] = rcolor + fast_random( 7);
	rc[1] = rcolor + fast_random(15);
	rc[2] = rcolor + fast_random(31);

	tx[2] = x; tz[2] = z;
	ty[2] = hpoint (x, z) - 100 - fast_flandom () * rockpeaking;

	tx[0] = px[0]; tx[1] = px[1];
	tz[0] = pz[0]; tz[1] = pz[1];
	ty[0] = hpoint (tx[0], tz[0]);
	ty[1] = hpoint (tx[1], tz[1]);
	if (!facing (tx, ty, tz)) {
		if (depth < 2) {
			tx[3] = tx[2]; ty[3] = ty[2]; tz[3] = tz[2];
			polymap (tx, ty, tz, 4, rc[0]);
		}
		else
			poly3d (tx, ty, tz, 3, rc[0]);
	}
	tx[0] = px[1]; tx[1] = px[2];
	tz[0] = pz[1]; tz[1] = pz[2];
	ty[0] = hpoint (tx[0], tz[0]);
	ty[1] = hpoint (tx[1], tz[1]);
	if (!facing (tx, ty, tz)) {
		if (depth < 2) {
			tx[3] = tx[2]; ty[3] = ty[2]; tz[3] = tz[2];
			polymap (tx, ty, tz, 4, rc[1]);
		}
		else
			poly3d (tx, ty, tz, 3, rc[1]);
	}
	tx[0] = px[2]; tx[1] = px[0];
	tz[0] = pz[2]; tz[1] = pz[0];
	ty[0] = hpoint (tx[0], tz[0]);
	ty[1] = hpoint (tx[1], tz[1]);
	if (!facing (tx, ty, tz)) {
		if (depth < 2) {
			tx[3] = tx[2]; ty[3] = ty[2]; tz[3] = tz[2];
			polymap (tx, ty, tz, 4, rc[2]);
		}
		else
			poly3d (tx, ty, tz, 3, rc[2]);
	}
	x = x + fast_flandom() * 1000 * cdown - fast_flandom () * 1000 * cdown;
	z = z + fast_flandom() * 1000 * cdown - fast_flandom () * 1000 * cdown;
	y = hpoint (x, z); rockscaling *= 0.5;
	cdown--; if (cdown>0) goto rockrep;

	rockscaling = rs;
	resetfx ();
}

/*

	-----------------------------------------------------------------
	Collezione di funzioni per il tracciamento e l'animazione delle
	forme di vita indigene dei pianeti abitabili.
	-----------------------------------------------------------------

*/

#define LFS 	100		// massimo numero di animali.
int   animals  =   0;		// animali attualmente visibili.
char  ani_type [LFS];		// tipologia
long  ani_seed [LFS];		// seme pseudo per le modifiche alla forma.
float ani_scale[LFS];		// scala.
float ani_x    [LFS];		// posizione (X)
float ani_quote[LFS];		// quota rispetto al suolo.
float ani_z    [LFS];		// posizione (Z)
float ani_pitch[LFS];		// direzione in cui si spostano.
float ani_speed[LFS];		// velocit� attuale.
float tgt_quote[LFS];		// quota che vogliono raggiungere.
float tgt_speed[LFS];		// velocit� che vogliono raggiungere.
float tgt_pitch[LFS];		// direzione che vogliono acquisire.
char  ani_lcount[LFS];		// contatempo di vicinanza.
unsigned ani_sqc[LFS];		// sub-quadrant coordinates (attuali).
char  ani_mtype[LFS];		// tipo di movimento.
	#define FELINE_LIKE	0
	#define RABBIT_LIKE	1
        #define KANGAROO_LIKE	2

#define BIRD	1               // definizione tipologia (per classi).
#define REPTIL	4
#define MAMMAL  5

// dati di definizione - classe uccelli - relativo PVfile: "birdy_ncc"

const int bird_wings_center_p = 1;
const int bird_wings_center_v = 0;

pvlist bird_wing1[3] = { {  0, 1,1,1,0 }, {  1, 1,1,1,0 }, {0xFFF,0,0,0,0} };
pvlist bird_wing2[3] = { {  2, 1,1,1,0 }, {  3, 1,1,1,0 }, {0xFFF,0,0,0,0} };

const int bird_legs_center_p  = 18;
const int bird_legs_center_v  = 1;

pvlist bird_legs[3]  = { { 18, 0,0,1,0 }, { 19, 0,1,0,0 }, {0xFFF,0,0,0,0} };

// dati di definizione - classe mammiferi - relativo PVfile: "mammal_ncc"

pvlist mamm_ears[5] = {
	{ 42, 0,1,0,0 },
	{ 45, 0,0,1,0 },
	{ 43, 1,0,0,0 },
	{ 44, 0,0,1,0 },
	{0xFFF,0,0,0,0}
};

const int mamm_wrap_center_p  = 16;
const int mamm_wrap_center_v  = 2;

pvlist mamm_reartoto[19] = {
	{  7, 0,0,1,1 },
	{  8, 1,1,1,1 },
	{  9, 1,1,1,1 },
	{ 14, 1,1,1,1 },
	{ 18, 0,1,1,0 },
	{ 12, 1,1,1,1 },
	{ 19, 1,0,0,1 },
	{ 21, 0,0,1,1 },
	{ 10, 1,1,1,1 },
	{ 15, 1,1,1,1 },
	{ 13, 1,1,1,1 },
	{ 11, 1,1,1,1 },
	{ 46, 1,1,1,1 },
	{ 47, 1,1,1,1 },
	{ 50, 1,1,1,1 },
	{ 51, 1,1,1,1 },
	{ 48, 1,1,1,0 },
	{ 49, 1,1,1,0 },
	{0xFFF,0,0,0,0}
};

pvlist mamm_legs[15] = {
	{  0, 1,1,1,1 }, // F-L
	{  2, 1,1,0,0 },
	{ 22, 1,0,0,0 },
	{  1, 1,1,1,1 }, // F-R
	{  3, 1,1,0,0 },
	{ 23, 0,1,0,0 },
	{  8, 0,0,1,1 }, // R-L
	{ 10, 1,1,1,1 },
	{ 14, 0,1,1,0 },
	{ 15, 1,1,1,1 },
	{ 12, 0,0,1,1 }, // R-R
	{ 13, 1,1,1,1 },
	{  9, 0,0,1,1 },
	{ 11, 1,1,1,1 },
	{0xFFF,0,0,0,0}
};

const int mamm_tail_center_p  = 46;
const int mamm_tail_center_v  = 1;

pvlist mamm_tail[7] = {
	{ 46, 1,1,1,1 },
	{ 47, 1,1,1,1 },
	{ 50, 1,1,1,1 },
	{ 51, 1,1,1,1 },
	{ 48, 1,1,1,0 },
	{ 49, 1,1,1,0 },
	{0xFFF,0,0,0,0}
};

/* Funzione di tracciamento ed animazione delle forme di vita animali. */

void live_animal (int n)
{
	const double an_incl_prec = 50;

	double	incl;
	float	period;
	int	sqc_x, sqc_z;
	long 	tick = 18 * secs;
	float	dx, dy, dz, ax, ay, ay2, az;

	float	update_ratio, tendence_to_stop; // mammals

	char	perform_depth_sort = 0;
	char	texture_skin_map   = 0;
	float	animal_distance    = 0;

	float	quote    = tgt_quote[n];
	float	pitch    = tgt_pitch[n];
	float	velocity = tgt_speed[n];
	float	reaction = 0.5 / ani_scale[n];

	ax = ani_x[n]; az = ani_z[n];
	ay = hpoint (ani_x[n], ani_z[n]) - ani_quote[n];

	if (ani_lcount[n] < 0) {
		dx 	      = 1 / (float)(-ani_lcount[n]);
		ani_x[n]     += dx * (refx - ax);
		ani_z[n]     += dx * (refz - az);
		ani_quote[n] -= dx * ani_quote[n];
		if (ani_lcount[n] > -10) {
			stick3d (ax, ay, az, pos_x,      pos_y - 50, pos_z);
			stick3d (ax, ay, az, pos_x - 50, pos_y - 50, pos_z);
			stick3d (ax, ay, az, pos_x + 50, pos_y - 50, pos_z);
			stick3d (ax, ay, az, pos_x,      pos_y - 50, pos_z + 50);
			stick3d (ax, ay, az, pos_x,      pos_y - 50, pos_z - 50);
		}
		if (ani_lcount[n] < -1) {
			step += 2 * ani_lcount[n];
			ani_lcount[n]++;
		}
		goto inactive;
	}

	/* Comportamento in distanza. */

	if (ani_type[n] == BIRD) {
		if (quote >= 1500) {
			velocity = 800;
			fast_srand (n + (tick / 50));
			pitch   += 5 * fast_flandom() - 2.5;
			quote   += 1000 * fast_flandom() - 500;
			goto end_far;
		}
		if (quote > 750) {
			velocity = 400;
			quote   *= 0.5;
			fast_srand (n + (tick / 15));
			pitch   += 10 * fast_flandom() - 5;
			goto end_far;
		}
		if (quote > 250) {
			velocity = 0;
			quote    = 0;
			goto end_far;
		}
		fast_srand (n + (tick / 10));
		if (quote < 50)
			velocity = 0;
		else {
			velocity = 100 * fast_flandom();
			pitch += 10 * fast_flandom() - 5;
		}
		if (fast_random(7) == 3)
			quote = 1500 + 1000 * fast_flandom();
		else
			quote += 500 * fast_flandom() - 250;
	}

	if (ani_type[n] == MAMMAL) {
		//stick3d (ax, ay, az, ax, ay - 50000, az);
		fast_srand (n);
		update_ratio = fast_random (31) + 3;
		tendence_to_stop = fast_flandom () * 0.8;
		fast_srand (n + tick / update_ratio);
		if (fast_flandom() < tendence_to_stop) {
			velocity = 0;
			fast_srand (n + tick);
			if (fast_flandom() < 0.1 * tendence_to_stop) {
				pitch += 100 * fast_flandom();
				pitch -= 100 * fast_flandom();
			}
		}
		else {
			fast_srand (n + tick / 18);
			if (ani_mtype[n] == FELINE_LIKE) velocity = 350 + fast_flandom() * 350;
			if (ani_mtype[n] == RABBIT_LIKE) velocity = 200 + fast_flandom() * 200;
			if (ani_mtype[n] == KANGAROO_LIKE) velocity = 400 + fast_flandom() * 100;
			fast_srand (n + (tick / 5));
			dx = 300 * fast_flandom() - 5;
			pitch += dx / velocity;
		}
		quote = 0;
	}

    end_far:
	if (quote < 0) quote = 0;

	tgt_speed[n] = velocity;
	tgt_quote[n] = quote;
	tgt_pitch[n] = pitch;

	dx = velocity - ani_speed[n];
	dy = quote    - ani_quote[n];
	dz = pitch    - ani_pitch[n];

	ani_speed[n] += 3 * reaction * dx;
	ani_pitch[n] += 2 * reaction * dz;
	ani_quote[n] += 1 * reaction * dy;

	ani_x[n] -= ani_speed[n] * sin (deg * ani_pitch[n]);
	ani_z[n] -= ani_speed[n] * cos (deg * ani_pitch[n]);
	if (ani_x[n] < 0 || ani_x[n] > 3276800 || ani_z[n] < 0 || ani_z[n] > 3276800) {
		ani_x[n]     += ani_speed[n] * sin (deg * ani_pitch[n]);
		ani_z[n]     += ani_speed[n] * sin (deg * ani_pitch[n]);
		ani_pitch[n] += 180;
	}

	inactive:

	dx = ax - cam_x;
	dy = ay - cam_y;
	dz = az - cam_z;
	animal_distance = sqrt (dx*dx + dy*dy + dz*dz);

	if (animal_distance > 250000) {
		ani_x[n] = pos_x + 100000 * fast_flandom() - 100000 * fast_flandom();
		ani_z[n] = pos_z + 100000 * fast_flandom() - 100000 * fast_flandom();
		if (ani_type[n] == BIRD)
			ani_quote[n] = 25000 * fast_flandom();
		else
			ani_quote[n] = 0;
		tgt_quote[n] = ani_quote[n];
	}

	sqc_x = ani_x[n] / 16384;
	sqc_z = ani_z[n] / 16384;
	if (sqc_x < 0) sqc_x = 0;
	if (sqc_x > 199) sqc_x = 199;
	if (sqc_z < 0) sqc_z = 0;
	if (sqc_z > 199) sqc_z = 199;
	ani_sqc[n] = m200[sqc_z] + sqc_x;

	if (animal_distance > 150000) return;

	if (animal_distance <  75000) perform_depth_sort = 1;
	if (animal_distance <  12500) texture_skin_map   = 1 + (n % 2);

	/* Comportamento in vicinanza e tracciamento. */

	// impostazione texture per forme di vita.
	flares = 0;
	txtr = p_background;
	XSIZE = TEXTURE_XSIZE * 256;
	YSIZE = TEXTURE_YSIZE * T_SCALE;

	if (ani_type[n] == BIRD) {
		// preparazione forma di base:
		copypv (bird_result, bird_base);
		modpv (bird_result, -1, -1, ani_scale[n], ani_scale[n], ani_scale[n], 0, 0, 0, NULL);
		// modifiche alla forma di base:
		if (ani_lcount[n] < 0) {
			// b�, questo non � pi� vivo:
			// � stato catturato e giace legato
			// ad alcune cordicelle, mentre viene
			// trascinato dietro al player...
			dz = 180 / (float)ani_lcount[n];
			goto bird_trace;
		}
		if (ani_quote[n] < 500) {
			// si inclina all'indietro, prima di
			// atterrare o decollare. � normale.
			dz = -0.1 * fabs (250 - ani_quote[n]);
		}
		else
			dz = 0;
		if (ay < 0 || sctype != OCEAN) {
			if (ani_quote[n] < 50) {
				// modello di comportamento:
				// quando � a terra. ali chiuse o semiaperte.
				// non succede quando al posto della terraferma
				// c'� l'acqua...
				dy = 1 - (ani_quote[n] * reaction); if (dy < 0) dy = 0;
				modpv (bird_result, bird_wings_center_p, bird_wings_center_v, 1, 1, 1, 0, +45*dy, +75*dy, bird_wing1);
				modpv (bird_result, bird_wings_center_p, bird_wings_center_v, 1, 1, 1, 0, -45*dy, -75*dy, bird_wing2);
				goto bird_trace;
			}
		}
		modpv (bird_result, bird_legs_center_p, bird_legs_center_v, 1, 1, 1, -75, 0, 0, bird_legs);
		if (ani_scale[n] > 10) {
			// modello di comportamento:
			// in volo, grandi uccelli.
			modpv (bird_result, bird_wings_center_p, bird_wings_center_v, 1, 1, 1,
			       0, 0, fabs(10 - (tick % 20)) * -4.5, bird_wing1);
			modpv (bird_result, bird_wings_center_p, bird_wings_center_v, 1, 1, 1,
			       0, 0, fabs(10 - (tick % 20)) * +4.5, bird_wing2);
		}
		else {
			// modello di comportamento:
			// in volo, piccoli uccelli.
			modpv (bird_result, bird_wings_center_p, bird_wings_center_v, 1, 1, 1,
			       0, 0, fabs(3 - (tick % 6)) * -15, bird_wing1);
			modpv (bird_result, bird_wings_center_p, bird_wings_center_v, 1, 1, 1,
			       0, 0, fabs(3 - (tick % 6)) * +15, bird_wing2);
		}
		// visualizzazione:
		bird_trace:
		modpv (bird_result, bird_wings_center_p, bird_wings_center_v, 1, 1, 1,
		       dz, 0, 0, 0);
		modpv (bird_result, bird_wings_center_p, bird_wings_center_v, 1, 1, 1,
		       0, ani_pitch[n], 0, 0);
		drawpv (bird_result, texture_skin_map, 3, ax, ay
			- 9 * ani_scale[n], az, perform_depth_sort);
		// reazioni alla vicinanza.
		// b�, gli uccelli tendono a scappare,
		// a meno che non ci si avvicini ad essi
		// con molta cautela. tuttavia, si possono
		// catturare: concettualmente, � semplice,
		// dato che basta tender loro un agguato,
		// e saltargli addosso da molto vicino.
		// praticamente � piuttosto difficile...
		if (ani_lcount[n] >= 0) {
			if (animal_distance < 5000 && step > 250)
				tgt_quote[n] += 2500;
			if (animal_distance < 3000 && step > 100)
				tgt_quote[n] += 2000;
			if (animal_distance < 1000) {
				tgt_speed[n] = 500 * reaction;
				tgt_quote[n] = 250 * reaction;
				if (animal_distance < 500)
					ani_lcount[n] = -25;
			}
		}
	}

	if (ani_type[n] == MAMMAL) {
		// preparazione forma di base:
		copypv (mamm_result, mamm_base);
		modpv (mamm_result, -1, -1, ani_scale[n], ani_scale[n], ani_scale[n], 0, 0, 0, NULL);
		if (ay > -10 && sctype == OCEAN) {
			// nell'acqua...
			// se alcuni ci si avventurano, b�,
			// possono sempre nuotare...
			modpv (mamm_result, -1, -1, 1, 0.7, 1, 0, 0, 0, NULL);
			modpv (mamm_result, -1, -1, 1, 0.0, 1, 0, 0, 0, mamm_legs);
			period = fabs (fsecs - 0.5);
			modpv (mamm_result, -1, -1, 1, 1, 1, 15, 0, 50 * period, NULL);
		}
		else {
			// Sulla terraferma...
			if (ani_mtype[n] != FELINE_LIKE) {
				modpv (mamm_result, -1, -1, 2, 2, 0.75, 0, 0, 0, mamm_reartoto);
				modpv (mamm_result, -1, -1, 1, 1, 1, 60, 0, 0, NULL);
				modpv (mamm_result, mamm_tail_center_p, mamm_tail_center_v, 1, 1, 1, -100, 0, 0, mamm_tail);
				if (ani_mtype[n] != KANGAROO_LIKE)
					modpv (mamm_result, -1, -1, 0.33, 0.33, 0.33, 0, 0, 0, NULL);
			}
			ay2 = hpoint (ax - an_incl_prec * sin (deg * ani_pitch[n]),
				      az - an_incl_prec * cos (deg * ani_pitch[n]))
				      - ani_quote[n];
			incl = ay - ay2;
			incl /= an_incl_prec;
			if (incl < -1) incl = -1;
			if (incl > +1) incl = +1;
			incl = ((double)180 * atan(incl)) / M_PI;
			modpv (mamm_result, mamm_wrap_center_p,
			       mamm_wrap_center_v, 1, 1, 1, incl, 0, 0, NULL);
			if (ani_mtype[n] == FELINE_LIKE) ay -= ani_scale[n] * 16;
			if (ani_mtype[n] == RABBIT_LIKE) ay -= ani_scale[n] * 48;
			if (ani_mtype[n] == KANGAROO_LIKE) ay -= ani_scale[n] * 115;
			if (ani_speed[n] < 50) {
				// se sono fermi possono comunque
				// scondinzolare, mentre pensano
				// a cosa fare...
				fast_srand (4*n);
				if (fast_random(1)) {
					period = fabs (fsecs - 0.5);
					modpv (mamm_result, mamm_tail_center_p, mamm_tail_center_v,
					       1, 1, 1, 0, 240 * period - 60, 0, mamm_tail);
				}
			}
			else {
				// se corrono, si fa semplicemente
				// un'animazione del tronco per dare
				// l'idea che stiano appunto correndo.
				if (ani_mtype[n] == FELINE_LIKE)
					period = 45;
				if (ani_mtype[n] == RABBIT_LIKE)
					period = 60;
				if (ani_mtype[n] == KANGAROO_LIKE)
					period = 22;
				period *= fabs (fsecs - 0.5);
				period /= ani_scale[n];
				if (ani_mtype[n] == FELINE_LIKE)
					ay -= 35 * ani_scale[n] * period;
				if (ani_mtype[n] == RABBIT_LIKE)
					ay -= 50 * ani_scale[n] * period;
				if (ani_mtype[n] == KANGAROO_LIKE)
					ay -= 300 * ani_scale[n] * period;
				modpv (mamm_result, mamm_wrap_center_p, mamm_wrap_center_v,
				       1, 1, 1, - 50 * period, 0, 0, NULL);
				modpv (mamm_result, mamm_wrap_center_p, mamm_wrap_center_v,
				       1, 1, 1, 100 * period, 0, 0, mamm_reartoto);
			}
		}
		modpv (mamm_result, mamm_wrap_center_p, mamm_wrap_center_v,
		       1, 1, 1, 0, ani_pitch[n] + 180, 0, NULL);
		// visualizzazione:
		mamm_trace:
		drawpv (mamm_result, 1, 0, ax, ay, az, perform_depth_sort);
	}

    end_vicinity:
}

/*

	-----------------------------------------------------------------
	Collezione di funzioni interdipendenti per il tracciamento delle
	superfici di base che compongono l'orografia dei territori.
	Queste funzioni coordinano quelle che si occupano del
	tracciamento degli oggetti in superficie.
	-----------------------------------------------------------------

*/

#define  bk_lines_to_horizon	120
#define  culling_limit		50

void srf_detail (float x, float y, float z, long depth, char _class_)
{ // disegna un oggetto sulla superficie di un pianeta.

	switch (_class_) {

		case ROCKS: // rocce, sassi, massi, pietre, pietruzze etc...

			roccia (x, y, z, depth);

			break;

		case VEGET: // vegetali che non rientrano nella classe alberi.

			ciuffo (x, y + 150, z, depth);

			break;

		case TREES: // alberi, appunto. e cespugli quasi-alberi.

			if (y > -15000)
				cespuglio (x, y, z, depth);
			else
				albero (x, y, z, depth);

			break;

		case NOTHING: // una parte non coperta dalla texture (rovine).

			break;

	}

}

char gtx; // se attivo, traccia il livello del suolo con texture specifica
int  ipfx, ipfz; // centro di tracciamento (coordinate SQC dell'osservatore).
char nearest_fragment_already_traced; // flag di lavoro.

void fragment (long x, long z)
{ // traccia un quadrante della superficie.

	char  poly1, poly2;

	int   c1, count, id, cl;

	long  h1, h2, h3, h4;
	long  x2 = x + 1, z2 = z + 1;

	float vx1[4], vy1[4], vz1[4];
	float vx2[4], vy2[4], vz2[4];

	float px, py, pz, icx, icz;
	float hp1, hp2, hp3, hp4, hp5, hp6, hp7, hp8;

	unsigned char rch1, rch2, rch3, rch4;

	unsigned int *ani_sqc_temp;

	if (x == ipfx && z == ipfz) {
		// si assicura di tracciare una sola volta
		// il frammento pi� vicino (quello che sta
		// "sotto ai piedi" dell'esploratore),
		// per evitare eventuali ridondanze che
		// sarebbero alquanto gravi in termini di
		// velocit�.
		if (nearest_fragment_already_traced)
			return;
		else
			nearest_fragment_already_traced = 1;
	}
	else {
		// esegue un rapido calcolo per
		// dare un'occhiata alla profondit�
		// del frammento (distanza dall'osservatore)
		// calibrando i frammenti possibilmente visibili
		// rispetto a un range di 64 * radice di 2,
		// cio� 90. 64 infatti � il limite di visibilit�
		// corrente, applicato al valore della distanza
		// pi� preciso che viene calcolato dopo (depth).
		c1 = x - ipfx;
		if (c1 < 0) c1 = -c1;
		cl = z - ipfz;
		if (cl < 0) cl = -cl;
		if (c1 + cl > 90) return;
	}

	/* -1- Traccia il frammento di superficie del landscape. */

	// fissa le coordinate piane del frammento (parte 1).
	vx1[0] = x  << 14;
	vx1[1] = x2 << 14;
	vz1[0] = z  << 14;
	vz1[2] = z2 << 14;

	// trova la distanza del frammento dall'osservatore.
	float dx    = pos_x - (0.5 * (vx1[0] + vx1[1]));
	float dz    = pos_z - (0.5 * (vz1[0] + vz1[2]));
	float hpdep = sqrt (dx*dx + dz*dz);
	long  depth = (long)(hpdep) >> 14;

	// limita la visibilit� in diagonale, pi� che altro per fare
	// uno sfondo su pianta rotonda, invece che quadrata, perch� cos�
	// si ottiene una sorta di curvatura dell'orizzonte, ormai alla
	// Terra piatta non ci crede pi� nessuno...
	// Poi: in Noctis IV ho deciso di tracciare in massima precisione,
	// quindi la limitazione deve essere presente, altrimenti � lento.
	if (depth > 64) return;

	// fissa le coordinate piane del frammento (parte 2).
	vz1[1] = z  << 14;
	vx1[2] = x  << 14;
	vx1[3] = vx1[2]  ;
	vz1[3] = vz1[2]  ;
	vx2[0] = x2 << 14; vz2[0] = z  << 14;
	vx2[1] = x2 << 14; vz2[1] = z2 << 14;
	vx2[2] = x  << 14; vz2[2] = z2 << 14;
	vx2[3] = vx2[2]  ; vz2[3] = vz2[2]  ;

	// Considera pi� quadranti circostanti come i pi� vicini
	depth -= 1;
	if (depth < 0) depth = 0;

	// fissa le coordinate di elevazione.
	h1 = x  + m200[(unsigned)z];
	h2 = x2 + m200[(unsigned)z];
	h3 = x2 + m200[(unsigned)z2];
	h4 = x  + m200[(unsigned)z2];
	vy1[0] = - ((long)(p_surfacemap[h1]) << 11);
	vy1[1] = - ((long)(p_surfacemap[h2]) << 11);
	vy1[2] = - ((long)(p_surfacemap[h4]) << 11);
	vy2[0] = - ((long)(p_surfacemap[h2]) << 11);
	vy2[1] = - ((long)(p_surfacemap[h3]) << 11);
	vy2[2] = - ((long)(p_surfacemap[h4]) << 11);

	// componente di shading della superficie in questo punto
	if (sh_delta) {
		// luce incidente
		c1 = p_surfacemap[h1] - p_surfacemap[h1 + sh_delta];
	}
	else {
		// sole pressoch� a perpendicolo, luce diffusa
		fast_srand (h1 + global_surface_seed);
		c1 = 8 + fast_random (7);
	}

	// lieve effetto atmosferico sui quadranti a grande distanza
	// l'effetto "nebbia" � aumentato per via della riduzione di
	// visibilit� in diagonale (da 80 quadranti a 64) in conseguenza
	// della decisione di tracciare in massima precisione.
	// � passato da depth / 3 a depth / 2 (con uno shift, fra l'altro).
	if (c1 < 00) c1 = 00;
	c1 += depth >> 1;
	if (c1 > 32) c1 = 32;

	// depth culling dei territori lontani (per velocizzare).
	// Nei deserti, c'� in genere sabbia, e questo costituisce
	// una piccola eccezione al culling: i granelli pi� lontani
	// dovrebbero apparire con grana pi� fine di quelli vicini,
	// e il culling ha l'effetto collaterale di fare andare le
	// cose esattamente al contrario.
	if (sctype == DESERT) {
		if (depth >= 4)
			culling_needed = 0;
		else
			culling_needed = 1;
	}
	else {
		if (depth >= 4)
			culling_needed = 1;
		else
			culling_needed = 0;
	}

	// tracciamento del suolo.
	if (!mirror) {
		// verifica visibilit� poligoni.
		poly1 = 0;
		if (facing(vx1, vy1, vz1))
			if (gtx||(vy1[0]+vy1[1]+vy1[2]!=0))
				poly1 = 1;
		poly2 = 0;
		if (facing(vx2, vy2, vz2))
			if (gtx||(vy2[0]+vy2[1]+vy2[2]!=0))
				poly2 = 1;
		if (poly1 || poly2) {
			// impostazione parametri della texture del suolo.
			txtr = p_background;
			rch1 = ruinschart[h1];
			rch2 = ruinschart[h2];
			rch3 = ruinschart[h3];
			rch4 = ruinschart[h4];
			if (rch1 == AF1 || rch2 == AF1 || rch3 == AF1 || rch4 == AF1) {
				XSIZE = TEXTURE_XSIZE * 2;
				YSIZE = TEXTURE_YSIZE * 2;
				c1 %= 0x40; c1 += 0x40;
				flares = 0;
			}
			else {
				XSIZE = TEXTURE_XSIZE * T_SCALE;
				YSIZE = TEXTURE_YSIZE * T_SCALE;
				flares = groundflares;
			}
			// imposta il colore delle escrescenze erbose.
			if (groundflares == 8) {
				if (depth >= 16)
					flares = 0;
				else {
					escrescenze = c1 + 32 - (depth << 1);
					if (escrescenze > 32) escrescenze = 32;
				}
			}
			// traccia il suolo.
			if (poly1) polymap (vx1, vy1, vz1, 3, c1);
			if (poly2) polymap (vx2, vy2, vz2, 3, c1);
		}
		// traccia la capsuletta con cui si � discesi: essa
		// emette una sorta di "raggio intrinsecamente luminoso",
		// che � molto utile per ritrovarla, dato che il territorio
		// esplorabile � molto vasto (ci si pu� allontanare fino
		// a coprire un raggio di 3 chilometri dal punto di sbarco).
		if (landed&&atl_x==x&&atl_z==z) {
			px = cam_x; py = cam_y; pz = cam_z;
			hp1 = (atl_x << 14) + atl_x2;
			hp2 = (atl_z << 14) + atl_z2;
			cam_x -= hp1; cam_z -= hp2;
			cam_y -= hpoint (hp1, hp2) - 900;
			H_MATRIXS = 0;
			V_MATRIXS = 0;
			change_txm_repeating_mode();
			txtr = n_globes_map;
			cam_y += 515; polycupola (-1, 1); flares = 0; cupola (-1, 8);
			cam_y -=1030; polycupola (+1, 1); flares = 0; cupola (+1, 8);
			cam_y += 515;
			flares = 1;
			for (cl = 0; cl < nightzone*5 + 1; cl++) {
				/*-----------*/ stick3d (0, -2000, 0, 100, -2E6,   0);
				if (depth < 48) stick3d (0, -2050, 0,   0, -1E6,   0);
				if (depth < 24) stick3d (0, -2100, 0,   0, -5E5, 100);
			}
			cam_x = px; cam_y = py; cam_z = pz;
		}
	}
	else {
		// verifica visibilit� poligoni.
		poly1 = 0;
		vy1[0] = -vy1[0];
		vy1[1] = -vy1[1];
		vy1[2] = -vy1[2];
		vy1[3] = -vy1[3];
		if ((vy1[0]+vy1[1]+vy1[2]!=0)&&!facing(vx1, vy1, vz1))
			poly1 = 1;
		poly2 = 0;
		vy2[0] = -vy2[0];
		vy2[1] = -vy2[1];
		vy2[2] = -vy2[2];
		vy2[3] = -vy2[3];
		if ((vy2[0]+vy2[1]+vy2[2]!=0)&&!facing(vx2, vy2, vz2))
			poly2 = 1;
		if (poly1 || poly2) {
			// impostazione parametri della texture dei riflessi.
			txtr = p_background;
			rch1 = ruinschart[h1];
			rch2 = ruinschart[h2];
			rch3 = ruinschart[h3];
			rch4 = ruinschart[h4];
			if (rch1 == AF1 || rch2 == AF1 || rch3 == AF1 || rch4 == AF1) {
				XSIZE = TEXTURE_XSIZE * 2;
				YSIZE = TEXTURE_YSIZE * 2;
				c1 %= 0x40; c1 += 0x40;
				flares = 0;
			}
			else {
				if (hpdep < 49152L) {
					cl = 1536 - ((long)(hpdep) >> 5);
					cl += TEXTURE_XSIZE;
					XSIZE = cl;
					YSIZE = cl;
				}
				else {
					XSIZE = TEXTURE_XSIZE;
					YSIZE = TEXTURE_YSIZE;
				}
				flares = groundflares;
				txtr += (x << 3);
				c1 &= 0xC0;
			}
			// traccia i riflessi del suolo.
			if (poly1) polymap (vx1, vy1, vz1, 3, c1);
			if (poly2) polymap (vx2, vy2, vz2, 3, c1);
		}
		// evita riflessi di oggetti perch� sarebbe davvero troppo.
		return;
	}

	/* -2- tracciamento forme di vita animali. */

	// oltre 1 Km, n� animali n� oggetti sono visibili.
	if (depth > 40) return;

	// verifica la visibilit� delle superfici del frammento.
	// (approssima in questo modo, per�, soltanto in profondit�,
	// perch� potrebbe accadere, per i frammenti pi� vicini,
	// che la superficie del suolo non sia visibile, mentre gli
	// oggetti che vi si ergono o gli animali che vi camminano s�...)
	if (depth > 8 && poly1+poly2 == 0) return;

      /*for (id = 0; id < animals; id++)
			if (ani_sqc[id] == h1)
				live_animal (id);*/

	ani_sqc_temp = ani_sqc;
	asm {   mov cx, animals
		cmp cx, 0
		jle nat
		mov si, word ptr ani_sqc_temp
		mov ax, word ptr h1 }
   lfa: asm {	cmp word ptr [si], ax
		jne nah
		push ax
		push cx
		push si
		mov dx, animals
		sub dx, cx }
		live_animal (_DX);
	asm {	pop si
		pop cx
		pop ax }
   nah: asm {	add si, 2
		dec cx
		jnz lfa }

	/* -3- Traccia gli oggetti presenti sulla superficie. */

	// se non sono previsti oggetti su questa superficie, lascia...
   nat:	if ((count = objectschart[h1].nr_of_objects) == 0) return;

	// diminuisce in generale il numero di oggetti visibili
	// nella distanza... eh, b�, anche la velocit� vuole la
	// sua parte, � assurdo ma consigliabile. E si nota poco.
	
	// Extended viewfield of objects :: neuzd 
	//if (depth > 16) {
	if (depth > 255) {
		count >>= 1;
		if (!count) return;
	}

	// impostazione texture per gli oggetti.
	flares = 0;
	txtr = p_background;
	cl = T_SCALE >> 2;
	XSIZE = TEXTURE_XSIZE * cl;
	YSIZE = TEXTURE_YSIZE * cl;

	// ID del primo oggetto.
	id = 0;

	// calcola i vertici di riferimento della superficie.
	hp1 = vy1[0];
	hp2 = vy1[1];
	hp3 = vy2[1];
	hp4 = vy1[2];
	hp5 = (hp2 - hp1) * qid;
	hp6 = (hp4 - hp1) * qid;
	hp7 = (hp4 - hp3) * qid;
	hp8 = (hp2 - hp3) * qid;

	while (count) {
		// seleziona la tabella pseudo relativa al presente frammento.
		// (fornisce una base costante per tutti i valori estratti.)
		fast_srand (x * z * count);
		// estrae le coordinate degli oggetti dalla tabella,
		// calcolando l'origine cartesiana dell'oggetto come (px; py; pz),
		// usando misure in millimetri, con px e pz compresi tra 0 e 16383,
		// e con py che giace sul piano della superficie (livello del suolo).
		icx = fast_random (16383); px = vx1[0] + icx;
		icz = fast_random (16383); pz = vz1[0] + icz;
		if (icx+icz < 16384) {
			// il punto fa parte del triangolo #1.
			py  = hp1 + hp5 * icx;
			py += hp6 * icz;
		}
		else {
			// il punto fa parte del triangolo #2.
			py  = hp3 + hp7 * (16384 - icx);
			py += hp8 * (16384 - icz);
		}
		// disegna l'oggetto in questione...
		dz = px + pz;
		detail_seed = h1;
		switch (id) {
			case 0:	srf_detail (px, py, pz, depth, objectschart[h1].object0_class);
				break;
			case 1:	srf_detail (px, py, pz, depth, objectschart[h1].object1_class);
				break;
			case 2:	srf_detail (px, py, pz, depth, objectschart[h1].object2_class);
		}
		// passa al prossimo oggetto...
		id++; if (id > 2) id = 0;
		count--;
	}
}

void iperficie (int additional_quadrants)
{ // Traccia i poligoni della superficie, dirigendo la funzione precedente.
  // questa funzione � centrata su ipfx;ipfz, e si prende cura di tracciare
  // i poligoni nell'ordine corretto rispetto alla distanza.

	int 	  b = beta;
	int	  x, z;

	if (b<0)
		b += 360;

	nearest_fragment_already_traced = 0;

	if (b<45||b>=315) {
		for (z=199; z>=ipfz - additional_quadrants;) { // -dlz
			for (x=0; x<ipfx;) { // +dlx
				fragment (x, z);
				x++;
			}
			for (x=199; x>=ipfx;) { // -dlx
				fragment (x, z);
				x--;
			}
			z--;
		}
		goto backoff;
	}

	if (b>=135&&b<225) {
		for (z=0; z<=ipfz + additional_quadrants;) { // +dlz
			for (x=0; x<ipfx;) { // +dlx
				fragment (x, z);
				x++;
			}
			for (x=199; x>=ipfx;) { // -dlx
				fragment (x, z);
				x--;
			}
			z++;
		}
		goto backoff;
	}

	if (b>=45&&b<135) {
		for (x=0; x<=ipfx + additional_quadrants;) { // +dlx
			for (z=199; z>ipfz;) { // -dlz
				fragment (x, z);
				z--;
			}
			for (z=0; z<=ipfz;) { // +dlz
				fragment (x, z);
				z++;
			}
			x++;
		}
		goto backoff;
	}

	if (b>=225&&b<315) {
		for (x=199; x>=ipfx - additional_quadrants;) { // -dlx
			for (z=199; z>ipfz;) { // -dlz
				fragment (x, z);
				z--;
			}
			for (z=0; z<=ipfz;) { // +dlz
				fragment (x, z);
				z++;
			}
			x--;
		}
		goto backoff;
	}

    backoff:
	flares = 0;
	H_MATRIXS = 16;
	V_MATRIXS = 16;
	change_txm_repeating_mode();
}

/*

	-----------------------------------------------------------------
	Collezione di funzioni interdipendenti per la definizione dei
	profili orografici di un'area di un pianeta. I pianeti sono
	principalmente suddivisi in aree e quadranti, come segue:

		- il lato di un'area � di 6 Km e 552 metri;
		- il lato di ogni quadrante � di 32 mt. e 768 mm.

	La superficie esplorabile di un pianeta � costituita
	da 43200 aree, organizzate in una matrice di 360 x 120,
	una diversa dall'altra per caratteristiche orografiche
	e termografiche. Di un pianeta si pu� arrivare ad esplorare
	una superficie totale di 1.855.424 chilometri quadrati, il che
	significa, s�, molto meno della reale estensione di un pianeta,
	ma di certo abbastanza perch� ben pochi se ne possano accorgere.
	-----------------------------------------------------------------

*/

void round_hill (int cx, int cz, unsigned r, float h, float hmax, char allowcanyons)
{ // Una collina rotonda, o una montagna molto erosa (se la si fa grossa).
  // hmax entra in gioco se il flag "allowcanyons" � a zero:
  //      quando l'altezza puntuale supera "hmax", per allowcanyons=0
  //      la funzione costruisce un altopiano sulla sommit� della collina,
  //	  mentre allowcanyons=1 fa ignorare il parametro "hmax" e, quando
  //	  l'altezza supera il limite massimo globale (127), scava un canyon
  //	  al centro della collina.
	int x, z;

	float dx, dz, d;
	float y, v = (float)r / M_PI_2;

	for (x = cx-r; x < cx+r; x++)
		for (z = cz-r; z < cz+r; z++) {
			if (x>-1&&z>-1&&x<200&&z<200) {
				dx = x - cx;
				dz = z - cz;
				d  = sqrt (dx*dx + dz*dz);
				y  = cos (d / v) * h;
				if (y>=0) {
					y += p_surfacemap[200*(long)z+x];
					if (allowcanyons) {
						if (y>127)
							y = 254 - y;
					}
					else {
						if (y>hmax)
							y = hmax;
					}
					p_surfacemap[200*(long)z+x] = y;
				}
			}
		}
}

void smoothterrain (int rounding)
{ // Smussa il profilo del terreno.
	int n;
	while (rounding) {
		for (ptr = 0; ptr < 39799; ptr++) {
			n  = p_surfacemap[ptr];
			n += p_surfacemap[ptr + 1];
			n += p_surfacemap[ptr + 200];
			n += p_surfacemap[ptr + 201];
			p_surfacemap[ptr] = n >> 2;
		}
		rounding--;
	}
}

void rockyground (int roughness, int rounding, char level)
{ // Produce una superficie pi� o meno accidentata.
	for (ptr = 0; ptr < 40000; ptr++)
		p_surfacemap[ptr] = random (roughness);
	smoothterrain (rounding);
	for (ptr = 0; ptr < 40000; ptr++) {
		if (p_surfacemap[ptr] >= abs(level)) {
			p_surfacemap[ptr] += level;
			if (p_surfacemap[ptr] > 127)
				p_surfacemap[ptr] = 127;
		}
		else
			p_surfacemap[ptr] = 0;
	}
}

void std_crater (unsigned char huge *map, int cx, int cz, int r,
		 int lim_h, float h_factor, float h_raiser, long align)
{ // Un cratere.
	int x, z;

	float dx, dz, d, y, h, fr;

	h = (float)r * h_factor;
	r = abs (r); fr = r;

	for (x = cx-r; x < cx+r; x++)
		for (z = cz-r; z < cz+r; z++) {
			if (x>-1&&z>-1&&x<align&&z<align) {
				dx = x - cx;
				dz = z - cz;
				d  = sqrt (dx*dx + dz*dz);
				if (d <= fr) {
					y  = sin (M_PI*(d/fr)) * h;
					y  = pow (y, h_raiser);
					y += map[align*(long)z+x];
					if (y<0) y = 0;
					if (y>lim_h) y = lim_h;
					map[align*(long)z+x] = y;
				}
			}
		}
}

void srf_darkline (unsigned char huge *map, int length,
		   int x_trend, int z_trend, long align)
{ // Una crepa scura (versione principalmente per textures).
	int fx = random(align), fz = random(align);
	long mapsize = align*align;
	unsigned location;
	//
	while (length) {
		fx += random(3) + x_trend;
		fz += random(3) + z_trend;
		location = align*(long)fz+fx;
		if (location>0 && location<mapsize) map[location] >>= 1;
		length--;
	}
}

void felisian_srf_darkline (unsigned char huge *map, int length,
			    int x_trend, int z_trend, long align)
{ // Un crepaccio (versione principalmente per superfici).
	int fx = random(align), fz = random(align);
	int peak, deviation, variability;
	long mapsize = align*align;
	unsigned location;
	//
	deviation = random(25) - 50;
	variability = 2 + random(10);
	while (length) {
		fx += random(3) + x_trend;
		fz += random(3) + z_trend;
		deviation += random(variability) - (variability>>1);
		location = align*(long)fz+fx;
		if (location>0 && location<mapsize) {
			peak = map[location];
			peak += deviation;
			if (peak < 0) peak = 0;
			if (peak > 127) peak = 127;
			map[location] = peak;
			map[location+1] = peak;
			map[location-1] = peak;
			map[location+align] = peak;
			map[location-align] = peak;
		}
		length--;
	}
}

void asterism (unsigned char huge *map, int x, int y,
	       int base, int variation, int density,
	       int size, long align)
{ // Simile a un asterisco variabile. Viene usata per i ceppi d'erba.
	if (density <=0) return;

	float ad  = M_PI * 2 / (float)density;
	float ang = 0;

	float shift_d;
	long  shift_x;
	long  shift_y;
	long  shift_p;

	float color, var;

	while (ang < M_PI * 2) {
		shift_d  = (float)random(1000) / 1000;
		shift_d *= size;
		if (shift_d >= 1) {
			var = (float)variation / shift_d;
			color = base;
			while (shift_d > 0) {
				shift_x = cos (ang) * shift_d + x;
				shift_y = sin (ang) * shift_d + y;
				if (shift_x>0 && shift_y>0 && shift_x < align && shift_y < align) {
					shift_p = shift_y * align + shift_x;
					map[shift_p] = color;
				}
				color += var;
				shift_d--;
			}
		}
		ang += ad;
	}
}

/* Funzioni per la mappatura dei cieli planetari. */

void nebular_sky ()
{ // Cielo nebuloso, piuttosto alieno, con piccoli ammassi sparsi o striati.
	unsigned pqw = QUADWORDS;
	unsigned seed = random (10000);

	QUADWORDS = st_bytes / 4;

	asm {	les di, dword ptr s_background
		mov cx, st_bytes
		mov ax, seed }
rndpat:	asm {   add ax, cx
		xor dx, dx
		imul ax
		add ax, dx
		mov bl, al
		and bl, 0x3F
		mov es:[di], bl
		inc di
		dec cx
		jnz rndpat }

	lssmooth (s_background);

	if (random(2)) ssmooth (s_background);
	if (random(3)) psmooth_grays (s_background);

	QUADWORDS = pqw;
}

void cloudy_sky (int density, int smooths)
{ // Cielo con nuvole sparse, di tipo terrestre.
	int 	 n = random (density + albedo);
	float    x, y, cx, cy, r, b;
	unsigned p, pqw = QUADWORDS;

	QUADWORDS = st_bytes / 4;

	while (n>0) {
		cx = random (360);
		r = random (25) + 5;
		cy = random (50) + 25 + r;
		for (y = - r; y < r; y++)
			for (x = - 2*r; x < 2*r; x++) {
				if (sqrt(x*x*0.2+y*y)<r) {
					p = x + cx + 360 * (y+cy);
					if (p < st_bytes) {
						b = 1.4142 / sqrt((x+r)*(x+r)+(y+r)*(y+r));
						b *= 64; b += s_background[p];
						if (b>63) b = 63;
						s_background[p] = b;
					}
				}
			}
		n--;
	}

	while (smooths) {
		ssmooth (s_background);
		smooths--;
	}

	QUADWORDS = pqw;
}

/* Funzioni che costruiscono rovine sulle superfici dei pianeti "storici",
   quali Felysia, le lune abitabili di Fal Galmatrifal ed pianeti Feniani.
   Viene chiamata tramite "build_surface" e quindi i semi random sono stati
   gi� impostati rispetto al global_surface_seed. Gli stili possibili sono:
   ------------------------------------------------------------------------
   0 - grattacieli
   1 - edifici quadrati senza tetto (solo resti di mura)
   2 - piazzali e colonnati (soprattutto tipici di Felysia)
   3 - palazzi
   4 - edifici coloniali a forma di X (stile Feniano)
   5 - edifici coloniali con tetto a cupola (stile Suricrasiano) */

int average_of_y (int ic, int jc, int ra)
{
	int av = 0;
	int ai = 1;
	int ip, jp, pt;

	for (jp = jc - ra; jp < jc + ra; jp++) {
		for (ip = ic - ra; ip < ic + ra; ip++) {
			av += p_surfacemap[pt];
			ai++;
		}
	}
	av /= ai;

	return (av);
}

void make_ruins (char style1, char style2, char style3,
		 char style4, char style5, int density)
{
	int buildings = 0;

	switch (sctype) {
		case OCEAN:	buildings = random(4);
				if (!random(10)) buildings *= 2 + random(2);
				break;
		case PLAINS:	buildings = random(8);
				if (!random(5)) buildings *= 3 + random(2);
				break;
		case DESERT:	buildings = random(3);
				break;
		case ICY:	buildings = random(2);
	}

	int peak;
	char bstyle;
	unsigned ic, jc, ip, jp, ra, hr, pt, av, ai;

	buildings *= density;
	while (buildings) {
		ai = random(5);
		switch (ai) {
			case 0: bstyle = style1; break;
			case 1: bstyle = style2; break;
			case 2: bstyle = style3; break;
			case 3: bstyle = style4; break;
			case 4: bstyle = style5;
		}
		switch (bstyle) {
		// Stili Felisiani antichi.
		case 0:
			// grossi grattacieli
			hr = 10 + random(25);
			ic = 20 + random(160);
			jc = 20 + random(160);
			ra =  1 + random(3);
		    like0:
			av = average_of_y (ic, jc, ra);
			for (jp = jc - ra; jp < jc + ra; jp++) {
				for (ip = ic - ra; ip < ic + ra; ip++) {
					pt = m200[jp] + ip;
					p_surfacemap[pt] = av + hr;
					if (!random(5)) p_surfacemap[pt]--;
					ruinschart[pt] = AF1;
				}
			}
			break;
		case 1:
			// edifici crollati (senza tetto)
			hr = random(8);
			ic = 20 + random(160);
			jc = 20 + random(160);
			ra =  3 + random(5);
			for (ip = ic - ra; ip < ic + ra; ip++) {
				pt = m200[jc - ra] + ip;
				av = hr + random(2);
				p_surfacemap[pt] += av;
				if (av) ruinschart[pt] = AF1;
				pt = m200[jc + ra] + ip;
				av = hr + random(3);
				p_surfacemap[pt] += av;
				if (av) ruinschart[pt] = AF1;
			}
			for (jp = jc - ra; jp < jc + ra; jp++) {
				pt = m200[jp] + ic - ra;
				av = hr + random(2);
				p_surfacemap[pt] += av;
				if (av) ruinschart[pt] = AF1;
				pt = m200[jp] + ic + ra;
				av = hr + random(3);
				p_surfacemap[pt] += av;
				if (av) ruinschart[pt] = AF1;
			}
			break;
		case 2:
			// piazzali e colonnati
			hr = 5 + random(10);
			ic = 30 + random(140);
			jc = 30 + random(140);
			ra = 15 + random(15);
			av = average_of_y (ic, jc, ra);
			for (jp = jc - ra; jp < jc + ra; jp++) {
				for (ip = ic - ra; ip < ic + ra; ip++) {
					pt = m200[jp] + ip;
					p_surfacemap[pt] = av;
				}
			}
			ra /= 2;
			if (!random(3)) {
				ai = 2 + random(3);
				for (ip = ic - ra; ip < ic + ra; ip += ai) {
					pt = m200[jc - ra] + ip;
					p_surfacemap[pt] = av + hr + random(2);
					ruinschart[pt] = AF1;
					pt = m200[jc + ra] + ip;
					p_surfacemap[pt] = av + hr + random(3);
					ruinschart[pt] = AF1;
				}
			}
			if (!random(3)) {
				ai = 2 + random(3);
				for (jp = jc - ra; jp < jc + ra; jp += ai) {
					pt = m200[jp] + ic - ra;
					p_surfacemap[pt] = av + hr + random(2);
					ruinschart[pt] = AF1;
					pt = m200[jp] + ic + ra;
					p_surfacemap[pt] = av + hr + random(3);
					ruinschart[pt] = AF1;
				}
			}
			break;
		// Stili Felisiani coloniali.
		case 3:
			// palazzine e piccoli grattacieli
			hr = 10 + random(10);
			ic = 20 + random(160);
			jc = 20 + random(160);
			ra =  1 + random(2);
			goto like0;
		case 4:
			// edifici coloniali moderni, a forma di X
			hr =  1 + random(3);
			ic = 25 + random(150);
			jc = 25 + random(150);
			ra =  5 + random(5);
			if (!random(5)) ra *= 2;
			for (ip = ic - ra; ip < ic + ra; ip++) {
				pt = m200[jc] + ip;
				p_surfacemap[pt] += hr * (ra - abs(ip-ic) + 1);
				ruinschart[pt] = AF1;
			}
			for (jp = jc - ra; jp < jc + ra; jp++) {
				pt = m200[jp] + ic;
				p_surfacemap[pt] += hr * (ra - abs(jp-jc) + 1);
				ruinschart[pt] = AF1;
			}
			break;
		case 5:
			// edifici coloniali di Suricrasia (Ylastravenia)
			// sono quadrati, ma hanno una cupoletta sul tetto
			hr = 10 + random(10);
			ic = 20 + random(160);
			jc = 20 + random(160);
			ra = 12 + random(8);
			av = average_of_y (ic, jc, ra);
			for (jp = jc - ra; jp < jc + ra; jp++) {
				for (ip = ic - ra; ip < ic + ra; ip++) {
					pt = m200[jp] + ip;
					p_surfacemap[pt] = av + hr;
					ruinschart[pt] = AF1;
				}
			}
			ra *= 2;
			ra /= 3;
			bstyle = random(2);
			for (jp = jc - ra; jp < jc + ra; jp++) {
				for (ip = ic - ra; ip < ic + ra; ip++) {
					pt   = m200[jp] + ip;
					ai   = (jp - jc) * (jp - jc);
					ai  += (ip - ic) * (ip - ic);
					ai   = sqrt (ai);
					peak = 3 * hr * cos (M_PI_2 * (double)ai / (double)ra);
					if (peak > 0)
						p_surfacemap[pt] = av + peak + 1;
					if (bstyle)
						ruinschart[pt] = AF1;
					else
						ruinschart[pt] = 0;
				}
			}
		}
		buildings--;
	}
}

/* Funzione che costruisce la superficie del pianeta.
   Regolata dal seme global_surface_seed, per ottenere risultati coerenti. */

void build_surface ()
{
	int cx, cz, cr, n, incl;
	float hf, hr, ht;
	unsigned ptr1;

	char waswet;
	char frosty;
	char snowy;
	char liquid_water;

	snowy     = 0;
	frosty    = 0;
	waswet    = 0;
	waves_in  = 0;
	waves_out = 0;
	T_SCALE	  = 32;
	quartz    = 0;
	gtx	  = 1; // suolo con texture, per default

	_fmemset (txtr, 16, 65535);
	_fmemset (p_surfacemap, 0, ps_bytes);
	_fmemset (objectschart, 0, oc_bytes);

	// regolazione dei parametri generali: coerente con il layout
	// del pianeta, e indipendente dalla superficie.
	fast_srand (global_surface_seed);
	srand (global_surface_seed);

	// normalmente, la superficie non � trasparente o traslucida...
	groundflares = 0;

	// potrebbe esserlo? mah, per esempio in caso di ghiacci molto
	// particolari, o di interi pianeti fatti di sostanze trasparenti.
	// rarissimi, direi.
	cz = random (2);
	cx = random (100);
	if (cx > 97)
		groundflares = 2 + (2 * cz);
	if (cx > 45 && cx < 55) {
		if (nearstar_p_type[ip_targetted] == 3 && latitude > 75)
			groundflares = 2 + (2 * cz);
	}

	// normalmente, la superficie � pi� o meno rocciosa...
	liquid_water = 0;
	for (ptr = 0; ptr < oc_bytes; ptr ++) {
		objectschart[ptr].object0_class = ROCKS;
		objectschart[ptr].object1_class = ROCKS;
		objectschart[ptr].object2_class = ROCKS;
	}

	// gli alberi: quando � possibile la loro presenza,
	// vengono influenzati dalla latitudine della zona di sbarco.
	// � piuttosto logico che ci siano delle latifoglie ai climi pi�
	// miti, e delle conifere a quelli pi� rigidi.
	if (latitude > 45)
		treepeaking = flandom() * 0.9 + 0.1;
	else
		treepeaking = flandom() + 0.75;
	// un dettaglio raro ma possibile: alberi fibrosi, con il tronco
	// simile a un grosso stelo... tanto si parla di ambienti alieni...
	switch (random(3)) {
		case 0: rootshade = 0x00; break;
		case 1: rootshade = 0x80; break;
		case 2: rootshade = 0xC0; break;
	}
	// altro dettaglio parecchio insolito, che per� conviene sia
	// davvero molto raro: alberi trasparenti (e chi lo sa? magari...)
	switch (random(30)) {
		case 7:  treeflares = 1; break;
		case 8:  treeflares = 2; break;
		case 9:  treeflares = 4; break;
		default: treeflares = 0;
	}
	// pi� probabile... foglie trasparenti... credo sia pi� plausibile.
	switch (random(15)) {
		case 7:  leafflares = 1; break;
		case 8:  leafflares = 2; break;
		case 9:  leafflares = 4; break;
		default: leafflares = 0;
	}
	// gli altri parametri degli alberi, b�... sono piuttosto casuali.
	treescaling = 3000 + flandom() * 3000 - flandom() * 1500;
	treespreads = 0.75 + flandom() * 0.50 - flandom() * 0.50;
	branchwidth = 0.05 + flandom() * 0.15;
	rootheight  = 0.05 + flandom();

	mushscaling = 4095;
	if (treescaling > 4096) mushscaling = 8191;

	// bump mapping di superficie:
	// � spesso coperta d'escrescenze erbose nel caso si tratti di un
	// pianeta abitabile e lo scenario non sia un ghiacciaio/deserto...
	if (nearstar_p_type[ip_targetted] == 3) {
		if (sctype != ICY && sctype != DESERT) {
			if (random(4))
				groundflares = 8;
		}
	}

	// veniamo all'ambiente di superficie...
	// esso dipende strettamente dalle coordinate di sbarco.
	fast_srand (landing_pt_lat * landing_pt_lon);
	srand (landing_pt_lat * landing_pt_lon);

	switch (nearstar_p_type[ip_targetted]) {

	     // case 0: ATTUALMENTE non considerato: � un pianeta vulcanico.

		case 1: // rocciosi (stile luna)

			// terreno molto liscio ed estremamente arrotondato.
			// ma ci sono casi in cui � tutto l'opposto.
			n = random(5);
			if (n <= 2) rockyground (25, 4 + random (4), 0);
			if (n == 3) rockyground (5 + random(5), 1, 1);
			if (n == 4) rockyground (10, 2, -random(5));

			n = random(48) + 32 - albedo;
			if (n > 30) n = 30;
			if (n < 0) n = 0;
			while (n) {
				hf = (float)random(32) * 0.01;
				hr = (float)(random(20) + 5) * 0.075;
				std_crater (p_surfacemap,
					    random(200), random(200),
					    random(50) + 5, 127, hf, hr, 200);
				n--;
			}
			n = random(48) + 64 - albedo;
			if (n < 0) n = 0;
			hf = 0.35; if (nightzone) hf = 0.1;
			while (n) {
				cx = random (200);
				cz = random (200);
				cr = random (32) + 10;
				std_crater (txtr, cx, cz, cr, 31, hf, 1, 256);
				if (cr%2) std_crater (txtr, cx + cr/3, cz + cr/3, -cr, 31, hf, 1, 256);
				n--;
			}
			n = random(100);
			while (n) {
				srf_darkline (txtr, random(1000), -1, -1, 256);
				n--;
			}
			// molte piccole rocce aguzze, o nulla...
			rockdensity = (15 + 16 * random(2)) * random(2);
			rockscaling = 150 + random (500);
			rockpeaking = 100 + random (300);
			break;

		case 2: // con spessa atmosfera (pianeti "venusiani")
			rockyground (10, 1, 0);
			n = albedo + random (100);
			while (n) { // basse colline e frequenti altipiani,
				    // oceani e mari sono rari ma possibili...
				round_hill (random(200),
					    random(200),
					    random(100) + 50,
					    random( 50) + 10, 0, 1);
				n--;
			}
			// per i dettagli di superficie, due scenari possibili:
			switch (random(2)) {
				case 0: // questo nebuloso, indefinito.
					n = albedo + random(200) - random (100);
					hf = (float)random(10) * 0.02;
					if (n<0) n = 0;
					while (n) {
						cx = random (256);
						cz = random (256);
						cr = random (8)+8;
						if (random(2))
							std_crater (txtr, cx, cz, -cr, 31, hf, 1, 256);
						else
							std_crater (txtr, cx, cz, cr, 31, hf, 1, 256);
						n--;
					}
					break;
				case 1: // e questo con irregolarit� sparse...
					n = albedo + random(500);
					ptr = random (2000);
					while (n) {
						srf_darkline (txtr, random(ptr), -1, -1, 256);
						n--;
					}
			}
			// di solito poche grosse rocce erose
			rockscaling = 500 + random (500);
			rockdensity = 7 + 8 * random(2);
			rockpeaking = 50 + random (150);
			break;

		case 3: // abitabili

			gtx = 0; // pu� esserci acqua liquida,
				 // quindi non tracciare i poligoni
				 // al livello del suolo a meno che
				 // non sia un particolare scenario
				 // tra quelli descritti sotto.

			switch (sctype) {
				case OCEAN:
					waves_in = 1;
					waves_out = 1;
					// se non � proprio mare aperto,
					// si pu� considerare un paesaggio
					// "in riva al mare", usando il
					// codice delle PLAINS.
					if (albedo > 20) {
						waswet = 1;
						waves_in = 0;
						goto revert;
					}
					// albedo pi� basse, ma comunque pi�
					// alte di quella del mare aperto(16):
					// scogli sparsi che rompono le onde,
					// nel qual caso il mare � calmo...
					if (albedo > 16 && random(2)) {
						rockyground (10, random(2), -5);
						waves_in = 0;
					}
					// mentre in pieno oceano,
					// al limite potrebbe esserci
					// un'isoletta... raro, ma possibile.
					if (!random(3)) {
						cx = random(100) + 50;
						cz = random(100) + 50;
						round_hill (cx + random(15),
							    cz + random(15),
							    random(100) + 25,
							    random(10) + 1, 0, 1);
						round_hill (cx, cz,
							    random(100) + 25,
							    random(100) +  1, 0, 1);
						waswet = 1;
						goto addtrees;
					}
					// texture sabbiosa, direi...
					T_SCALE = 128;
					n = random (30) + 2;
					ptr = 65535;
					while (ptr) {
						txtr[ptr] = random(n);
						ptr--;
					}
					// moltissimi sassolini, se ci sono.
					rockscaling = 50 + random (75);
					rockpeaking = 30 + random (25);
					if (random(3))
						rockdensity = 31;
					else
						rockdensity = 0;
					// ma aspetta un attimo: non possono
					// esserci dei sassi galleggianti.
					liquid_water = 1;
					break;

				case PLAINS:
					// basse colline, oppure montagne
					// vere e proprie (in effetti, tutte
					// le zone ricche di vegetazione
					// rientrano in questo scenario),
					// allora diciamo cos�...
				revert:	if (random(2)) {
						// nell'un caso, le pianure.
						ptr = random(50) + 5;
						while (ptr) {
							round_hill (random(200),
								    random(200),
								    random(200) + 1,
								    random( 30) + 1, 0, 1);
							ptr--;
						}
					}
					else {
						// nell'altro caso, i monti.
						ptr = random(25) + 10;
						while (ptr) {
							round_hill (random(200),
								    random(200),
								    random(200) + 1,
								    random(100) + 1, 0, 1);
							ptr--;
						}
					}
			      addtrees: // alberi sui rilievi...
					// in certi casi, pianure steppose...
					n = random (6);
					for (ptr = 0; ptr < oc_bytes; ptr ++) {
						switch (p_surfacemap[ptr] / 25) {
							case 0:	objectschart[ptr].object1_class = VEGET;
								objectschart[ptr].object2_class = VEGET;
								break;
							case 1:	objectschart[ptr].object1_class = VEGET;
								objectschart[ptr].object2_class = TREES;
								break;
							case 2:	objectschart[ptr].object1_class = TREES;
								objectschart[ptr].object2_class = TREES;
								break;
						       default: objectschart[ptr].object0_class = TREES;
								objectschart[ptr].object1_class = TREES;
								objectschart[ptr].object2_class = TREES;
						}
					}
					// e texture con erba brada...
					T_SCALE = 128;
					n = random (15) + 2;
					ptr = 65535;
					while (ptr) {
						txtr[ptr] = random(n);
						ptr--;
					}
					n = 100 + random (500);
					while (n) {
						asterism (txtr, random(256), random(256),
							  random(16), random(16), random(25) + 6,
							  random(15) + 6, 256);
						n--;
					}
					// quasi mai si rende visibile
					// il livello dell'orizzonte
					// (� troppo piatto, indecorato)
					// ma a meno che non sia una localit�
					// di mare abbastanza scoperta...
					// in tal caso l'orizzonte � quasi
					// sempre costituito dal mare stesso:
					// in un caso su 5, si trover� uno
					// scenario strano ma interessante:
					// pozze d'acqua tra i cespugli,
					// un luogo "paludoso"...
					if (!waswet || (waswet && !random(5))) {
						for (ptr = 0; ptr < oc_bytes; ptr ++)
							p_surfacemap[ptr] += fast_random (3);
					}
					// qualche sasso tanto per dire...
					rockscaling = 100 + random (200);
					rockpeaking = 100 + random (200);
					rockdensity = 3 + 4 * random(2);
					// a ogni modo, togli tutti gli
					// oggetti dagli specchi d'acqua.
					if (waswet)
						liquid_water = 1;
					else
						gtx = 1;
					break;

				case DESERT:
					// beh, dune... e un palo
					// per andarci ovviamente a sbattere
					// (scherzavo, per il palo)
					// pi� alti i dislivelli,
					// maggiore lo smoothing,
					// cos� vengono fuori dune
					// arrotondate dal vento...
					n = random(100);
					rockyground (50 + n, 5 + (n>>4), 0);
					// texture a grana grossa, ghiozza.
					T_SCALE = 128;
					ptr = 65535;
					while (ptr) {
						txtr[ptr] = random(32);
						ptr--;
					}
					// qui � tutta sabbia:
					rockdensity = 0; // niente sassi
					gtx = 1; // suolo texturizzato
					break;

				case ICY:
					snowy = 0;
					frosty = 0;
					// mah, quattro tipi di orografia.
					switch (random(4)) {
						case 0: // praticamente piana,
							// distesa nevosa...
							rockyground (15, 5, 0);
							snowy = 1;
							break;
						case 1:	// brulla distesa di
							// ghiaccio permanente
							rockyground (10 + random(10), 1 + random(2), 0);
							frosty = 1;
							break;
						case 2: // colline di neve...
							ptr = random (50) + 50;
							while (ptr) {
								round_hill (random(200),
									    random(200),
									    random(200) + 1,
									    random( 75) + 1, 0, 1);
								ptr--;
							}
							snowy = 1;
							break;
						case 3: // e anche icebergs...
							rockyground (50 + random(50), 3 + random(3), -(random(40) + 20));
							frosty = 1;
							break;
					}
					// qualche sasso? raro e grosso.
					rockscaling = 200 + random (500);
					rockpeaking = 150 + random (250);
					rockdensity = 2 * random(2);
			       similar:	if (snowy || frosty) { // textures "nevose"
						T_SCALE = 32;
						n = random (16) + 16;
						ptr = 65535;
						while (ptr) {
							txtr[ptr] = random(n);
							ptr--;
						}
						n = 1 + random(3);
						while (n) {
							ptr = 65535 - 257;
							while (ptr) {
								cx = txtr[ptr] + txtr[ptr+1] + txtr[ptr+256] + txtr[ptr+257];
								txtr[ptr] = cx >> 2;
								ptr--;
							}
							n--;
						}
					}
					if (frosty) { // textures "ghiacciate"
						T_SCALE = 16 + random(48);
						n = random (250);
						while (n) {
							srf_darkline (txtr, 100 + random(200), -random(2), 0, 256);
							n--;
						}
					}
					break;
			}

			// I quarzi si possono trovare anche sui pianeti
			// abitabili, ma solo a volte...
			if (!random(5)) quartz = 1;

			break;

		case 4:	// descritto come "di medie dimensioni, pietroso
			// (petroso) e corrugato". � un pianeta roccioso
			// che non ha pressoch� nessun cratere d'impatto,
			// ma la cui superficie � disseminata di enormi
			// massi grandi come case. nel sistema solare
			// un possibile corrispondente � la superficie
			// di phobos, ma per certi versi lo sarebbe anche
			// la Luna, se non avesse crateri...

			// il terreno � piuttosto liscio, di per s�,
			// con dislivelli simili a colline molto schiacciate,
			// che possono essere inframezzate da ampie pianure...
			rockyground (15, 3 + random (3), -random(5));

			// e ora si aggiungono i pietroni della descrizione.
			// pu� anche non essercene nessuno, in un quadrante...
			n = random (15);
			while (n) {
				hf = random (15) + 7;
				hr = hf * (flandom() * 3.5 + 3.5);
				ht = hr * (flandom() * 0.2 + 0.3);
				if (ht > 127) ht = 127;
				round_hill (random(200),
					    random(200),
					    hf, hr, ht, 0);
				n--;
			}

			// vanno per� arrotondati un po', perch� in effetti
			// � presumibile che siano coperti di polvere...
			smoothterrain (1 + random(2));

			// qui disegna dei piccolissimi crateri sulla texture
			// di superficie... che pi� che altro rappresentano
			// macchie e avvallamenti nelle zone a albedo bassa.
			n = 64 - albedo;
			if (nightzone)
				hf = 0.50;
			else
				hf = 0.25;
			while (n) {
				cx = random (150) + 25;
				cz = random (150) + 25;
				cr = random ( 10) + 15;
				std_crater (txtr, cx, cz, -cr, 31, hf, 1, 256);
				n--;
			}

			// piccoli sassi a grappoli, frammenti dei grandi
			// massi e polveri addensate..
			rockscaling = 100 + random (200);
			rockdensity = 3 + 4 * random(2);
			rockpeaking = 100 + random (200);

			break;

		case 5: // con atmosfera sottile (marte etc...)
			// possono avere un terreno piuttosto accidentato,
			// e raramente liscio: � anche prevista la possibilit�
			// di pozze d'acqua date dalla presenza del permafrost,
			// per� sono gelate e ci si pu� viaggiare sopra...
			// scenari principali: pianure, territori accidentati.
			if (random(2)) {
				n = 5 + random(10);
				if (albedo > 48) n /= 2;
				rockyground (n, 1, 0);
			}
			else {
				n = 15 + random(32);
				if (albedo > 48) n /= 2;
				rockyground (n, 1, -random(24));
			}
			// va incluso qualche cratere eroso: � possibile...
			n = random(68) - albedo;
			if (n > 10) n = 10;
			if (n < 1)  n = 1;
			while (n) {
				hf = (float)random(5) * 0.015;
				hr = (float)(random(10) + 10) * 0.27;
				std_crater (p_surfacemap,
					    random(200), random(200),
					    random(35) + 5, 127, hf, hr, 200);
				n--;
			}
			// molte pietre e pietruzze... s�, s�...
			// per� non � detto che siano tantissime, e
			// in certi punti il terreno potrebbe essere sgombro.
			rockscaling = 50 + random (400);
			rockpeaking = 50 + random (250);
			rockdensity = 1 + 30 * random(2);
			// zone ad albedo alta sono coperte di nubi brillanti
			if (albedo > 50) {
				sky_brightness *= 2;
				if (sky_brightness > 63) sky_brightness = 63;
			}
			// zone ad albedo medio-alta: si tratta di vulcani,
			// ci sono molte rocce grandi e la superficie �
			// fatta "a padella", descrive ampie curve.
			if (albedo > 40 && albedo <= 50) {
				rockscaling *= 2;
				rockdensity = 15 + 16 * random(2);
				hf = (float)random(5) * 0.01;
				hr = (float)(random(5) + 5) * 0.5;
				std_crater (p_surfacemap,
					    90 + random(20), 90 + random(20),
					    100 + random(10), 127, hf, hr, 200);
			}
			// e per quanto riguarda i dettagli sulla superficie,
			// roba molto irregolare: crepe, sassi e buche...
			ptr = random (1500) + 500;
			n = albedo * 5;
			while (n) {
				srf_darkline (txtr, random(ptr), -1, -1, 256);
				n--;
			}
			break;

		case 7: // gelidi, solcati di strie.
			rockyground (10 - (albedo / 8), 0, 20 + random(100));
			n = albedo - random(albedo) + 10;
			while (n) { // fai qualche crepaccio nella superficie
				srf_darkline (p_surfacemap, random(500), -1, -1, 200);
				n--;
			}
			n = albedo + random(200) - random (100); if (n<0) n = 0;
			while (n) { // e aggiungi piccoli crateri "a macchia"
				cx = random (192) + 32;
				cz = random (192) + 32;
				cr = random ( 16) + 16;
				std_crater (txtr, cx, cz, -cr, 31, 0.15, 1, 256);
				n--;
			}
			n = albedo + random(100) - random (50); if (n<0) n = 0;
			n /= 2; while (n) { // ma s�, anche qualche crepetta pi� piccola, sparsa...
				srf_darkline (txtr, random(100), -random(2), -random(2), 256);
				n--;
			}
			// pietre? poche. qualcuna, di media taglia...
			rockscaling = 50 + random (400);
			rockpeaking = 50 + random (200);
			rockdensity = 3 + 4 * random(2);
			break;

		case 8: // lattiginosi (pianeti al quarzo).
			// le zone pi� scure sono coperte di strutture
			// piuttosto allungate, simili a duomi tettonici.
			if (albedo < 20) {
				ptr = 100 - albedo;
				while (ptr) {
					hr = random (300);
					round_hill (random(150) + 25,
						    random(150) + 25,
						    random(  5) + 2,
						    hr + 1, 127, 0);
					ptr--;
				}
				smoothterrain (2 + random(3));
			}
			// altrove, sono normalmente coperti di montagnole,
			// o da agglomerati informi...
			ptr = (100 - albedo) * 2;
			while (ptr) {
				round_hill (random(200),
					    random(200),
					    random( 25) + 1,
					    random( 25) + 1, 0, 1);
				ptr--;
			}
			// abbastanza roccioso, s�... direi.
			// quarziti molto irregolari, � ovvio...
			quartz = 1;
			rockscaling = 50 + random (300);
			rockpeaking = 50 + random (300);
			rockdensity = 7 + 8 * random(2);
			// le macchie chiare sono zone pi� pianeggianti...
			// quarzo fuso e successivamente risolidificato
			// da estrusioni calde dall'interno: meno sassi qui.
			if (albedo > 40) {
				rockscaling *= 0.5;
				rockpeaking = rockscaling;
				rockdensity = 3 + 4 * random(2);
				smoothterrain (1 + random(10));
			}
			// ripeti una texture "nevosa" o "ghiacciata".
			// non ci stanno male.
			snowy = 0; frosty = 0;
			if (random(2))
				snowy = 1;
			else
				frosty = 1;
			goto similar;

	}

	n = random (5);
	if (n) {
		while (n) { // crepacci nella superficie possono essere ovunque
			felisian_srf_darkline (p_surfacemap, random(500), -1, -1, 200);
			n--;
		}
		ptr1 = 200;
		while (ptr1 < 38800) {
			n = p_surfacemap[ptr1];
			n += p_surfacemap[ptr1 - 1];
			n += p_surfacemap[ptr1 + 1];
			n += p_surfacemap[ptr1 - 200];
			n += p_surfacemap[ptr1 + 200];
			p_surfacemap[ptr1] = n / 5;
			ptr1++;
		}
	}

	// gli oggetti, ovviamente, tendono a distribuirsi pi� numerosi
	// sulle zone pianeggianti, rotolando gi� dai rilievi:
	// questo vale anche per i vegetali, anche se le erbacce
	// potrebbero rimanere attaccate, ma vengono gestite
	// separatamente dal codice dello scenario "plains".
	for (ptr = 0; ptr < oc_bytes; ptr ++) {
		incl  = abs (p_surfacemap[ptr] - p_surfacemap[ptr +   1]);
		incl += abs (p_surfacemap[ptr] - p_surfacemap[ptr + 200]);
		if (incl < 20)
			objectschart[ptr].nr_of_objects = random (2);
		if (incl < 15)
			objectschart[ptr].nr_of_objects = random (3);
		if (incl < 10)
			objectschart[ptr].nr_of_objects = random (4);
	}

	// se c'� acqua liquida sulla superficie, non possono esserci
	// oggetti che vi galleggiano: � opinabile, ma per ora lasciamo
	// perdere eventuali alghe, pezzi di legno...
	if (liquid_water) {
		for (ptr = 0; ptr < oc_bytes; ptr ++) {
			if (!p_surfacemap[ptr])
				objectschart[ptr].nr_of_objects = 0;
		}
	}

	////////////////////////////////////////////////////////////////////
	// INIZIO MODIFICHE STORICHE

	// reimpostazione seeds.
	// qualche pezzo di codice precedente sembra influire
	// sulla distribuzione delle rovine. Sospetto si tratti
	// del frammento che ridistribuisce gli oggetti a seconda
	// delle zone pi� o meno pianeggianti, in quanto qualche
	// byte dell'orografia del territorio sembra cambiare
	// (a volte, errori di sconfinamento dovuti a procedimenti
	// di arrotondamento, ecc...) l'orografia, per�, se cambia,
	// di certo cambia in minuscoli particolari, perch� si direbbe
	// generalmente costante.

	fast_srand (landing_pt_lat * landing_pt_lon);
	srand (landing_pt_lat * landing_pt_lon);

	if ((long)(nearstar_identity * 1E6) == -37828) {
		// Questa stella � Balastrackonastreya
		if (ip_targetted == 3) {
			// Questo mondo � Felysia:
			// la gravit� � gi� regolata da una costante
			// in modo che Felysia abbia 1 FG (Felysian G),
			// ma la pressione va regolata; per estrazione
			// pseudo-casuale altrimenti sarebbe circa 1.2 ATM.
			base_pp_pressure = 1;
			// Le rovine qui sono imponenti, tutte storiche.
			make_ruins (0,1,1,2,2, 3);
		}
		else {
			// Costruisce rovine sugli altri pianeti abitabili
			// attorno a Balastrackonastreya. In particolare si
			// tratta delle due lune di Fal Galmatrifal.
			// Squallide palazzine, qualche rimasuglio di
			// edifici pi� grandi, e tardi edifici coloniali.
			if (nearstar_p_type[ip_targetted] == 3)
				make_ruins (3,3,3,1,4, 1);
		}
	}

	if ((long)(nearstar_identity * 1E5) == 1599551984L) {
		// Questa stella � Fenia
		// Peach, base navale storica:
		// molti grattacieli, grossi edifici rovinati (industrie).
		if (ip_targetted == 2)
			make_ruins (0,0,0,1,1, 2);
		// Pleasance, zona residenziale:
		// palazzi, piazze, colonnati decorativi, edifici coloniali.
		if (ip_targetted == 3)
			make_ruins (2,2,3,3,4, 2);
		// Wetwick, mondo coloniale periferico:
		// principalmente stili coloniali, e qualche rara palazzina.
		if (ip_targetted == 6)
			make_ruins (3,4,4,4,4, 1);
	}

	if ((long)(nearstar_identity * 1E8) == -11543634L) {
		// Questa stella � Ylastravenia, fu esplorata
		// per prima (� vicinissima a Balastrackonastreya)
		// ed il quarto pianeta, Suricrasia, venne colonizzato
		// con uno stile tutto particolare...
		if (ip_targetted == 3) {
			make_ruins (2,4,5,5,5, 2);
			// Il "Suricrasian Cube" era l'unica formazione
			// nota prima che riaggiustassi i semi per le
			// rovine. Dopo la normalizzazione di tali semi,
			// � sparito. Si tratta semplicemente di un grosso
			// cubo, e questo frammento si occupa di
			// ripristinarlo come appariva nella vecchia
			// fotografia SNAP0106, o almeno all'incirca
			// come appariva in quella foto.
			if (landing_pt_lon == 18 && landing_pt_lat == 60) {
				for (ptr = 112; ptr < 112 + 25; ptr++) {
					for (ptr1 = 103; ptr1 < 103 + 25; ptr1++) {
						p_surfacemap[m200[ptr1] + ptr] = 127;
						if (ptr1 == 103 + 1 || ptr1 == 103 + 2)
							ruinschart[m200[ptr1] + ptr] = AF1;
					}
					if (ptr == 112 + 19 || ptr == 112 + 20 || ptr == 112 + 2 || ptr == 112 + 3) {
						for (ptr1 = 103; ptr1 < 103 + 25; ptr1++)
							ruinschart[m200[ptr1] + ptr] = AF1;
					}
				}
			}
		}
	}

	// FINE MODIFICHE STORICHE
	////////////////////////////////////////////////////////////////////

	// infine si calcola la mappa di shading.
	sh_delta = 0;
	if (fabs(sun_x) > 0.33 * dsd1) {
		if (sun_x > 0)
			sh_delta = 1;
		else
			sh_delta = -1;
	}
	if (fabs(sun_z) > 0.33 * dsd1) {
		if (sun_z > 0)
			sh_delta += 200;
		else
			sh_delta -= 200;
	}
}

/* Funzione che definisce il cielo visto da un pianeta.
   Oltretutto, definisce anche: tavola colori, temperatura e pressione. */

void create_sky (char atmosphere)
{
	// filtri colorati di base.

	float br = (float)sky_red_filter / 64,
	      bg = (float)sky_grn_filter / 64,
	      bb = (float)sky_blu_filter / 64;

	float tr = (float)gnd_red_filter / 64,
	      tg = (float)gnd_grn_filter / 64,
	      tb = (float)gnd_blu_filter / 64;

	float fr[4], fg[4], fb[4];	// filtri colorati per 4 sfumature.
	float al = (albedo / 64);	// costante di albedo

	// calcola il fattore "distanza dal sole" per l'intensit� della luce
	// � infuenzato anche dal tipo di stella.

	float sb, dfs;
	int   owner = nearstar_p_owner[ip_targetted];

	if (owner == -1)
		dfs = 1 - ((float)(ip_targetted) * 0.05);
	else
		dfs = 1 - ((float)(owner) * 0.05);

	if (!atmosphere)
		sb = 1;
	else {
		sb = (float)sky_brightness / 24;
		if (nightzone) dfs *= 0.5;
	}

	if (owner > 2)
		sb *= (dfs * dfs);
	else
		dfs = 1;

	switch (nearstar_class) {
		case  0: dfs *= 1.0; break;
		case  1: dfs *= 1.5; break;
		case  2: dfs *= 0.5; break;
		case  3: dfs *= 0.8; break;
		case  4: dfs *= 1.2; break;
		case  5: dfs *= 0.1; break;
		case  6: dfs *= 0.1; break;
		case  7: dfs *= 0.4; break;
		case  8: dfs *= 0.9; break;
		case  9: dfs *= 1.3; break;
		case 10: dfs *= 0.5; break;
		case 11: dfs *= 0.2; break;
	}

	// calcola il fattore di saturazione (influenza i pianeti abitabili
	// quando piove, tende a far scivolare le sfumature verso il grigio)

	float saturation = 1 - (0.15 * rainy);
	int shade_nr;

	fast_srand (global_surface_seed);
	srand (global_surface_seed);

	switch (nearstar_p_type[ip_targetted]) {

	     // case 0: ATTUALMENTE non considerato: � un pianeta vulcanico.

		case 1: // rocciosi (stile luna)

			pp_pressure = 0;

		 like1:	// colori per le terre emerse.
			// simili a quelli della superficie vista dallo
			// spazio, che tendono a essere alquanto grigiastri.
			fr[0] = tr;// * 0.5 + 0.5 * al;
			fg[0] = tg;// * 0.5 + 0.5 * al;
			fb[0] = tb;// * 0.5 + 0.5 * al;
			// colori per il cielo.
			// non c'� il cielo. non c'� aria.
			// ma servono per le stelle, belle brillanti.
			fr[1] = 1.5;
			fg[1] = 1.5;
			fb[1] = 1.5;
			// colori per l'orizzonte.
			// come quelli delle terre emerse, ma pi� sbiaditi.
			fr[2] = 2 * fr[0];
			fg[2] = 2 * fg[0];
			fb[2] = 2 * fb[0];
			// colori per la vegetazione.
			// non c'� vegetazione, quindi per ora nulli.
			fr[3] = 0.0;
			fg[3] = 0.0;
			fb[3] = 0.0;

			break;

		case 2: // con spessa atmosfera (pianeti "venusiani")

			// colori per tutto.
			// per questo specifico tipo di pianeta,
			// quelli del cielo sono pressoch� uguali
			// a quelli delle nubi. quelli del terreno
			// sono il negativo fotografico, perch� il
			// cielo filtra interamente i colori opposti.
			fr[0] = 1.2 - tr;
			fr[1] = tr + flandom()*0.15 - flandom()*0.15 + 0.3;
			fr[2] = tr + flandom()*0.30 - flandom()*0.30 + 0.2;
			fr[3] = tr + flandom()*0.45 - flandom()*0.45 + 0.1;
			fg[0] = 1.2 - tg;
			fg[1] = tg + flandom()*0.15 - flandom()*0.15 + 0.3;
			fg[2] = tg + flandom()*0.30 - flandom()*0.30 + 0.2;
			fg[3] = tg + flandom()*0.45 - flandom()*0.45 + 0.1;
			fb[0] = 1.2 - tb;
			fb[1] = tb + flandom()*0.15 - flandom()*0.15 + 0.3;
			fb[2] = tb + flandom()*0.30 - flandom()*0.30 + 0.2;
			fb[3] = tb + flandom()*0.45 - flandom()*0.45 + 0.1;

			nebular_sky (); // cielo adatto all'uopo.

			pp_pressure = fast_flandom() * 20 + albedo + 1;

			break;

		case 3: // abitabili

			// colori per il cielo.
			fr[1] = br * 0.5 + 0.5 * flandom();
			fg[1] = bg * 0.5 + 0.5 * flandom();
			fb[1] = bb * 0.5 + 0.5 * flandom();

			switch (sctype) {
				case OCEAN:
					// albedo bassa (32-39): oceani liquidi
					// ------------------------------------
					// colori per le terre emerse.
					fr[0] = 0.65 + 0.5 * flandom();
					fg[0] = 0.45 + 0.4 * flandom();
					fb[0] = 0.25 + 0.3 * flandom();
					if (fg[0] < 0.6) fg[0] *= 2;
					// colori per il mare.
					fr[2] = 0.8 * flandom();
					fg[2] = 0.8 * flandom();
					fb[2] = fb[0] * 2 + 0.4;
					// colori per la vegetazione.
					fr[3] = 0.2 + flandom();
					fg[3] = 0.4 + flandom();
					fb[3] = flandom() * 0.6;
					// cielo (solitamente) gremito di nubi.
					cloudy_sky (50, 1);
					break;
				case PLAINS: // albedo media (40-47): prateria stepposa e zone "verdi".
					// ------------------------------------
					// colori per le terre emerse.
					fr[0] = 0.25 + 0.5 * flandom();
					fg[0] = 0.50 + 0.4 * flandom();
					fb[0] = 0.25 + 0.3 * flandom();
					if (fg[0] < 0.75) fg[0] *= 1.5;
					// colori per l'orizzonte.
					fr[2] = (flandom() * 0.4) + fr[0] * 0.3;
					fr[2] = (flandom() * 0.7) + fg[0] * 0.3;
					fr[2] = (flandom() * 0.2) + fb[0] * 0.3;
					// colori per la vegetazione.
					fr[3] = flandom();
					fg[3] = flandom();
					fb[3] = flandom();
					// cielo mediamente nuvoloso, pioggie in normali quantit�...
					cloudy_sky (33, 1);
					break;
				case DESERT: // albedo medio-alta (48-55): aree (semi)desertiche
					// ------------------------------------
					// colori per le terre emerse.
					fr[0] = tr + flandom() * 0.33;
					fg[0] = tg + flandom() * 0.25;
					fb[0] = tb + flandom() * 0.12;
					// colori per l'orizzonte.
					fr[2] = tr;
					fg[2] = tg;
					fb[2] = tb;
					// colori per la vegetazione.
					fr[3] = 0.50 * flandom();
					fg[3] = 0.90 * flandom();
					fb[3] = 0.40 * flandom();
					// cielo molto pulito, pioggie molto scarse...
					cloudy_sky (10, 1);
					break;
				case ICY: // albedo alta (56-63): nevi perenni e ghiacciai.
					// ------------------------------------
					// colori per le terre emerse.
					fr[0] = 0.25 + flandom ();
					fg[0] = 0.55 + flandom ();
					fb[0] = 1.00 + flandom ();
					// colori per l'orizzonte.
					fr[2] = fr[0] * 0.6;
					fg[2] = fg[0] * 0.8;
					fb[2] = fb[0];
					// colori per la vegetazione.
					fr[3] = 0.95 * flandom ();
					fg[3] = 0.95 * flandom ();
					fb[3] = 0.95 * flandom ();
					// cielo pulito (poca umidit� nell'aria)
					cloudy_sky (15, 1);
					break;
			}

			pp_pressure = fast_flandom() * 0.8 + 0.6;

			break;

		case 4: // pietrosi e corrugati...
			pp_pressure = fast_flandom() * 0.1;
			goto like1;

		case 5: // con atmosfera sottile (marte etc...)

			// colori per le terre emerse.
			// mah... in genere simili a quelli della superficie
			// vista dallo spazio, ma qualche variazione �
			// possibile, plausibile... dovuta ai componenti
			// del suolo locale.
			fr[0] = tr + 0.33 * flandom() * al;
			fg[0] = tg + 0.33 * flandom() * al;
			fb[0] = tb + 0.33 * flandom() * al;
			// colori per il cielo.
			// sono pressoch� ininfluenti, ma quasi costanti.
			fr[1] = 0.8 * tb + 0.2 * flandom() * al;
			fg[1] = 0.8 * tg + 0.2 * flandom() * al;
			fb[1] = 0.8 * tr + 0.2 * flandom() * al;
			// colori per l'orizzonte.
			// come quelli delle terre emerse, ma pi� sbiaditi.
			fr[2] = 0.5 + fr[0] * 0.5 * al;
			fg[2] = 0.5 + fg[0] * 0.5 * al;
			fb[2] = 0.5 + fb[0] * 0.5 * al;
			// colori per la vegetazione.
			// non c'� vegetazione, quindi per ora nulli.
			fr[3] = 0.0;
			fg[3] = 0.0;
			fb[3] = 0.0;

			// l'atmosfera lascia vedere le stelle
			// per quasi tutto il giorno, di solito...
			sky_brightness = (float)sky_brightness * 0.65;

			// l'aspetto del cielo, anche qui, pu� essere nuvoloso,
			// ma con subi sottili e poco marcate, e foschia appena percepibile.
			cloudy_sky (10, 2);

			pp_pressure = fast_flandom() * 0.05 + 0.01;

			break;

	     // case 6: non considerato: � un gigante gassoso.

		case 7:	// gelido, solcato di strie (tipo Europa)

			pp_pressure = fast_flandom() * 0.02;

		 like7:	// colori per le terre emerse. molto chiari.
			// simili a quelli della superficie vista dallo
			// spazio, che tendono a essere alquanto grigiastri.
			fr[0] = tr + flandom() * al;
			fg[0] = tg + flandom() * al;
			fb[0] = tb + flandom() * al;
			// colori per il cielo.
			// non c'� il cielo. non c'� aria.
			// ma servono per le stelle, brillanti.
			fr[1] = 1.3;
			fg[1] = 1.4;
			fb[1] = 1.5;
			// colori per l'orizzonte.
			// come quelli delle terre emerse, ma pi� sbiaditi.
			fr[2] = 0.5 + fr[0];
			fg[2] = 0.5 + fg[0];
			fb[2] = 0.5 + fb[0];
			// colori per la vegetazione.
			// non c'� vegetazione, quindi per ora nulli.
			fr[3] = 0.0;
			fg[3] = 0.0;
			fb[3] = 0.0;

			break;

		case 8:	// lattiginoso.
			pp_pressure = fast_flandom() + 0.2;
			goto like7;

	     // case 9: non considerato: � un oggetto substellare.
	     // case 10: non considerato: � una stella compagna.

	}

	// evita gradienti negativi, non hanno senso.
	for (shade_nr = 0; shade_nr < 4; shade_nr++) {
		if (fr[shade_nr] < 0) fr[shade_nr] = 0;
		if (fg[shade_nr] < 0) fg[shade_nr] = 0;
		if (fb[shade_nr] < 0) fb[shade_nr] = 0;
	}

	// correzione saturazione colori (foschia e pioggia, ove plausibili).
	if (nearstar_p_type[ip_targetted] == 3 || nearstar_p_type[ip_targetted] == 5) {
		for (shade_nr = 0; shade_nr < 4; shade_nr++) {
			fr[shade_nr] = (fr[shade_nr] - 0.5) * saturation + 0.5;
			fg[shade_nr] = (fg[shade_nr] - 0.5) * saturation + 0.5;
			fb[shade_nr] = (fb[shade_nr] - 0.5) * saturation + 0.5;
		}
	}

	// prepara le sfumature
	fr[0]*=64 * dfs;      fg[0]*=64 * dfs;      fb[0]*=64 * dfs;
	fr[1]*=64 * dfs * sb; fg[1]*=64 * dfs * sb; fb[1]*=64 * dfs * sb;
	fr[2]*=64 * dfs;      fg[2]*=64 * dfs;      fb[2]*=64 * dfs;
	fr[3]*=64 * dfs;      fg[3]*=64 * dfs;      fb[3]*=64 * dfs;

	// se non c'� atmosfera, stelle sempre ben visibili.
	// alternativamente, si rendono visibili di notte.
	if (!atmosphere)
		shade (surface_palette, 64, 64, 0, 0, 0, 100, 110, 120);
	else {
		if (nightzone) {
			shade (surface_palette, 64, 64, 0, 0, 0, 60, 62, 64);
			if (nearstar_p_type[ip_targetted] == 3) {
				shade (surface_palette, 0, 64, 0, 0, 0, 64, 62, 60);
				shade (surface_palette, 128, 64, 0, 0, 0, 64, 64, 64);
				shade (surface_palette, 192, 64, 8, 12, 16, 56, 60, 64);
				goto nightcolors;
			}
			fr[0] *= 0.33;
			fg[0] *= 0.44;
			fb[0] *= 0.55;
			fr[2] *= 0.33;
			fg[2] *= 0.44;
			fb[2] *= 0.55;
			fr[3] *= 0.33;
			fg[3] *= 0.44;
			fb[3] *= 0.55;
		}
		else
			shade (surface_palette, 64, 64, 0, 0, 0, fr[1], fg[1], fb[1]);
	}

	// sfumatura per il suolo.
	shade (surface_palette,   0, 44,     0,     0,     0, fr[0], fg[0], fb[0]);
	shade (surface_palette,  44, 20, fr[0], fg[0], fb[0], fr[1], fg[1], fb[1]);

	// sfumatura per l'orizzonte.
	shade (surface_palette, 128, 10,     0,     0,     0, fr[0], fg[0], fb[0]);
	shade (surface_palette, 138, 44, fr[0], fg[0], fb[0], fr[2], fg[2], fb[2]);
	shade (surface_palette, 182, 10, fr[2], fg[2], fb[2], fr[1], fg[1], fb[1]);

	// sfumatura per la vegetazione.
	shade (surface_palette, 192, 10,     0,     0,     0, fr[0], fg[0], fb[0]);
	shade (surface_palette, 202, 44, fr[0], fg[0], fb[0], fr[3], fg[3], fb[3]);
	shade (surface_palette, 246, 10, fr[3], fg[3], fb[3], fr[1], fg[1], fb[1]);

    nightcolors:

	// calcolo della temperatura.

	pp_temp = 90 - dsd1 * 0.33;

	if (!atmosphere) {
		pp_temp -= 44;
		pp_temp *= fabs(pp_temp * 0.44);
		if (nightzone)
			pp_temp *= 0.3;
		else
			pp_temp *= 0.3 + exposure * 0.0077;
	}
	else {
		if (nightzone)
			pp_temp *= 0.6;
		else
			pp_temp *= 0.6 + exposure * 0.0044;
	}

	pp_temp -= (0.5 + 0.5 * fast_flandom()) * abs (landing_pt_lat - 60);

	if (pp_temp < -269)
		pp_temp = -269 + 4 * fast_flandom();

	if (nearstar_p_type[ip_targetted] == 2)
		pp_temp += fast_flandom() * 150;

	if (nearstar_p_type[ip_targetted] == 3) {
		switch (sctype) {
			case OCEAN:
				while (pp_temp < +10) pp_temp += fast_flandom() * 5;
				while (pp_temp > +60) pp_temp -= fast_flandom() * 5;
				break;
			case PLAINS:
				while (pp_temp < -10) pp_temp += fast_flandom() * 5;
				while (pp_temp > +45) pp_temp -= fast_flandom() * 5;
				break;
			case DESERT:
				while (pp_temp < +20) pp_temp += fast_flandom() * 5;
				while (pp_temp > +80) pp_temp -= fast_flandom() * 5;
				break;
			case ICY:
				while (pp_temp <-120) pp_temp += fast_flandom() * 5;
				while (pp_temp >  +4) pp_temp -= fast_flandom() * 5;
		}
	}

	base_pp_temp = pp_temp;
	base_pp_pressure = pp_pressure;
}

/* Funzione che definisce le forme di vita proprie ai pianeti abitabili. */

void setup_animals ()
{
	int n, p, x;

	int bird_probability;
	int reptil_probability;
	int mammal_probability;

	// impostazione numero animali.
	// questo parametro dipende dal tipo di scenario.
	// non sono assolutamente considerate possibilit� di vita
	// su pianeti non classificati come abitabili.

	if (nearstar_p_type[ip_targetted] != 3) {
		animals = 0;
		return;
	}

	switch (sctype) {
		case OCEAN:  animals = LFS/5  + random (LFS-LFS/5); break;
		case PLAINS: animals = LFS/2  + random (LFS-LFS/2); break;
		case DESERT: animals = LFS/10 + random (LFS/5);     break;
		case ICY:    animals = LFS/10 + random (LFS/2);     break;
	}

	// impostazione seme pseudo e scala.
	// questo parametro dipende dal seme globale del pianeta.

	srand (global_surface_seed);
	fast_srand (global_surface_seed);

	for (n = 0; n < animals; n++) {
		ani_seed[n]  = fast_random (0xFFFFFFFF);
		ani_scale[n] = 10 * flandom() + 5;
	}

	// impostazione tipologia animali.
	// questo parametro dipende dal seme globale e dal tipo di scenario.

	switch (sctype) {
		case OCEAN:	bird_probability   = 1 + random(9);
				reptil_probability = 0;
				mammal_probability = 0;
				break;
		case PLAINS:	bird_probability   = 5 + random(9);
				reptil_probability = 3 + random(3);
				mammal_probability = 5 + random(4);
				break;
		case DESERT:	bird_probability   = 1 + random(2);
				reptil_probability = 9 + random(9);
				mammal_probability = 1 + random(2);
				break;
		case ICY:	bird_probability   = 2 + random(2);
				reptil_probability = 0 + random(2);
				mammal_probability = 0 + random(2);
	}

	for (n = 0; n < animals; n++) {
		while (1) {
			x = random (3);
			p = random (18);
			if (x == 0 && p <= bird_probability) {
				ani_type[n] = BIRD;
				goto anitypeselected;
			}
			if (x == 1 && p <= reptil_probability) {
				ani_type[n] = REPTIL;
				goto anitypeselected;
			}
			if (x == 2 && p <= mammal_probability) {
				ani_type[n] = MAMMAL;
				goto anitypeselected;
			}
		}
		anitypeselected:
	}

	// impostazione posizione attuale e dati accessori sugli animali.
	// questi parametri sono del tutto casuali: certo, solo insetti
	// e uccelli possono volare, e quindi c'� da tenerne conto.

	srand (secs);
	fast_srand (secs);

	for (n = 0; n < animals; n++) {
		ani_x[n] = flandom() * 3276800;
		ani_z[n] = flandom() * 3276800;
		if (ani_type[n] == BIRD)
			ani_quote[n] = flandom() * 25000;
		else
			ani_quote[n] = 0;
		ani_pitch[n] = flandom() * 360;
		ani_speed[n] = flandom() * 100;
		tgt_pitch[n] = ani_pitch[n];
		tgt_speed[n] = ani_speed[n];
		tgt_quote[n] = ani_quote[n];
		ani_lcount[n] = 0;
		ani_sqc[n] = ani_z[n] / 16384;
		ani_sqc[n] = m200[ani_sqc[n]] + ani_x[n] / 16384;
		ani_mtype[n] = random (3);
	}

	// caricamento forme di base.

	unloadallpv ();
	fast_srand (global_surface_seed);

	// mammiferi:
	loadpv (mamm_base, mammal_ncc,
		1, 0.5 + 0.5*flandom(), 0.75 + 0.5*flandom(),
		0, 0, 0, fast_random(0xC0), 1);
	modpv (mamm_base, -1, -1, 2*flandom(), 2*flandom(), 2*flandom(), 45 - 90 * flandom(), 0, 0, mamm_ears);
	loadpv (mamm_result, mammal_ncc, 1, 1, 1, 0, 0, 0, 0x80, 1);

	// uccelli:
	loadpv (bird_base, birdy_ncc,
		1, 0.3 + flandom(), 0.75 + flandom(),
		0, 0, 0, fast_random(0xC0), 1);
	loadpv (bird_result, birdy_ncc, 1, 1, 1, 0, 0, 0, 0x80, 1);
}

/* Questa funzione aggiorna sp_x;y;z in modo da tener conto della distanza
   dalla superficie (il parametro height) e dell'orientamento della normale
   alla superficie in quel punto. sp_x;y;z stanno per "stepping pos x;y;z" */

void add_height (long px, long pz, float height)
{
	long cpos;

	float x[3], y[3], z[3];
	float h1, h2, h3, h4, icx, icz;

	cpos   = m200[pz>>14] + (px>>14);

	h1     = - ((long)(p_surfacemap[cpos])     << 11);
	h2     = - ((long)(p_surfacemap[cpos+1])   << 11);
	h3     = - ((long)(p_surfacemap[cpos+201]) << 11);
	h4     = - ((long)(p_surfacemap[cpos+200]) << 11);

	icx    = px & 16383;
	icz    = pz & 16383;

	if (icx+icz<16384) {
		x[0] = (px>>14) << 14; y[0] = h1; z[0] = (pz>>14) << 14;
		x[1] = ((px>>14) + 1) << 14; y[1] = h2; z[1] = z[0];
		x[2] = x[0]; y[2] = h4; z[2] = ((pz>>14) + 1) << 14;
	}
	else {
		x[0] = ((px>>14) + 1) << 14; y[0] = h2; z[0] = (pz>>14) << 14;
		x[1] = x[0]; y[1] = h3; z[1] = ((pz>>14) + 1) << 14;
		x[2] = (px>>14) << 14; y[2] = h4; z[2] = z[1];
	}

	pnorm (x, y, z);

	sp_x += (pnx * height - sp_x) * 0.25;
	sp_y += (pny * height - sp_y) * 0.25;
	sp_z += (pnz * height - sp_z) * 0.25;
}

/* Ciclo principale nell'esplorazione delle superfici planetarie. */

float tiredness = 0;

char exitflag = 0; // flag: se settato al ritorno di planetary_main,
		   // significa che c'� stato un quit sulla superficie.
char entryflag = 0;// flag: se settato all'ingresso di planetary_main,
		   // significa che si sta recuperando la situazione
		   // di superficie.
int movie_paused = 0;
void planetary_main ()
{
	const int widesnappingangle = 71;
	
	fcs_status_delay = 0; // no more 'ghost' UNLOCKING message (neuzd)
	unsigned pqw = QUADWORDS;
	long  	 cpos;

	char     bfa	    = 0;
	char     flash      = 1;
	char     flashes    = 0;
	char     flashed    = 0;
	char     recover    = 0;
	char     atmosphere = 1;

	
	int moviedelay;
	int moviefsec = 1;
	extern int movieexists;
	extern char movie;
	extern int movienr;
	extern char movieflashoff;
	extern int moviefscap;
	extern int moviestime;
	extern int moviedeck;
	int movie_cutoff=0;
	/*global*/opencapcount= 1;
	int	 opencapdelta = 0;
	int	 openhudcount = 180;
	int	 openhuddelta = 0;
	char	 hud_closed   = 1;
	char	 widesnapping = 0;

	int 	 w, lw = 10, now = 25, waveratio = 5, waveblur = 0;
	int      flick, flicks, fshift;

	char	 secondarysun = 0;
	int  	 ts, te, ll, plwp;
	int  	 te_ll_distance, te_ll_distance_1, te_ll_distance_2;
	int  	 ts_ll_distance, ts_ll_distance_1, ts_ll_distance_2;

	float	 secondary_nearstar_x;
	float	 secondary_nearstar_z;
	float	 pri_to_sec_distance, compdist;

	// Questo pezzo controlla se c'� una stella molto vicina a parte
	// la primaria (in sistemi multipli). La pi� vicina delle compagne
	// viene mostrata anche sulla superficie. E' chiamata qui il
	// "sole secondario".

	dsd1 = dsd;
	nray1 = nearstar_ray;

	compdist = 1E9;
	w = 0; while (w < nearstar_nop) {
		if (nearstar_p_type[w] == 10) {
		if (nearstar_p_qsortdist[w] < compdist) {
			compdist = nearstar_p_qsortdist[w];
			dsd2 = nearstar_p_qsortdist[w];
			nray2 = nearstar_p_ray[w];
			planet_xyz (w);
			secondary_nearstar_x = plx;
			secondary_nearstar_z = plz;
			pri_to_sec_distance  = (nearstar_x - plx) * (nearstar_x - plx);
			pri_to_sec_distance += (nearstar_y - ply) * (nearstar_y - ply);
			pri_to_sec_distance += (nearstar_z - plz) * (nearstar_z - plz);
			pri_to_sec_distance  = sqrt (pri_to_sec_distance);
			pri_latitude = (nearstar_y - ply) / pri_to_sec_distance;
			pri_latitude = atan (pri_latitude) / deg;
			secondarysun = 1;
		}}
		w++;
	}

	// Se invece si tratta di un pianeta che gira attorno ad una
	// delle stelle compagne della primaria, il sole secondario
	// diventa il primario, dato che i fattori di esposizione
	// (crepzone e nightzone) erano gi� stati calcolati per la
	// stella compagna cui il pianeta appartiene, per via dei
	// controlli svolti dalla f. "cplx_planet_viewpoint".

	if (nearstar_p_owner[ip_targetted] != -1 &&
	    nearstar_p_type[nearstar_p_owner[ip_targetted]] == 10) {
		dsd1 = nearstar_p_qsortdist[nearstar_p_owner[ip_targetted]];
		nray1 = nearstar_p_ray[nearstar_p_owner[ip_targetted]];
		dsd2 = dsd;
		nray2 = nearstar_ray;
		secondary_nearstar_x = nearstar_x;
		secondary_nearstar_z = nearstar_z;
		planet_xyz (nearstar_p_owner[ip_targetted]);
		pri_to_sec_distance  = (nearstar_x - plx) * (nearstar_x - plx);
		pri_to_sec_distance += (nearstar_y - ply) * (nearstar_y - ply);
		pri_to_sec_distance += (nearstar_z - plz) * (nearstar_z - plz);
		pri_to_sec_distance  = sqrt (pri_to_sec_distance);
		pri_latitude = (ply - nearstar_y) / pri_to_sec_distance;
		pri_latitude = atan (pri_latitude) / deg;
		secondarysun = 1;
	}

	float    gravity, crcy, temp;
	float    backup_dzat_x = dzat_x; dzat_x = 0;
	float    backup_dzat_y = dzat_y; dzat_y = 0;
	float    backup_dzat_z = dzat_z; dzat_z = 0;
	float	 backup_cam_x, backup_cam_y, backup_cam_z;
	float	 backup__alfa, backup__beta;
	float 	 drop_x, drop_y, drop_z;
	float	 pos_x0, pos_z0, bkshift, bkstep, directional_beta;
	float	 upanddown = 0, waving_y = 0, dfc = 0, maxdfc;

	float 	 x[4] = { +2e6, +2e6, -2e6, -2e6 };
	float 	 y[4] = { +4e5, +4e5, +4e5, +4e5 };
	float 	 z[4] = { +2e6, -2e6, -2e6, +2e6 };

	float 	 xx[4], yy[4], zz[4];
	float    wx[25], wy[25], wz[25], wr[25], wh[25], wl[25], wd[25];

/* operazioni preliminari generali. */

	// backup colori di ritorno.
	memcpy (return_palette, tmppal, 768);

	// Dissolvenza al nero (blank frame).
	for (w=64; w>=0; w-=4) {
		tavola_colori (return_palette, 0, 256, w, w, w);
		ll = clock(); while (ll == clock());
	}

	// regolazione della forza di gravit�.
	gravity = nearstar_p_ray[ip_targetted];
	planet_grav = gravity * 2000;
	pp_gravity = gravity * 38.26;
	gravity *= 1E4;

	// inizializzazione coordinate di stepping.
	sp_x = 0;
	sp_y = 0;
	sp_z = 0;

	// inizializzazione dati per le onde.
	w = now;
	while (w) {
		w--;
		wy[w] = 50;
		wx[w] = 1E6 + random (30000) - random (30000);
		wz[w] = 1E6 + random (30000) - random (30000);
		wr[w] = 25E5 - (float) random (2500) * 1000;
		wd[w] = - random (500) - 500;
		wl[w] = random (5000) + 1000;
		wh[w] = random (1500);
		if (wh[w]<0) wh[w] = random (100);
	}
	w = 10; while (w<now) { wr[w] = 0; w++; }

	// preparazione di alcune variabili di controllo.
	bfa                = field_amplificator;
	field_amplificator = 0;
	flashed            = 0;
	landed    	   = 0;

	// "latitude" andr� da 0 a 90.
	// il range di "landing_pt_lat" va da -60 a +60.
	// quindi moltiplicando il valore assoluto per 1.5 si ottiene
	// un range da 0 a 90 gradi. 0 � l'equatore, 90 sono i poli.
	latitude  	   = (float)(abs (landing_pt_lat - 60)) * 1.5;

	// "exposure" segnala l'esposizione alla stella che fa da sole.
	// la variabile "crepzone" ha un range di variabilit� che,
	// nell'area diurna, abbraccia 230 gradi in longitudine, dato che
	// i due terminatori sono calcolati a partire da +35 e +165 gradi
	// dal punto che rappresenta il punto al bordo destro del disco
	// planetario visto dalla stella. Il range di 230 gradi � per�
	// normalizzato in modo da far coincidere i terminatori con zero
	// e il sole a perpendicolo con la sua met�, quindi con 115 gradi.
	// Normalmente il range diurno come quello notturno dovrebbero
	// occupare entrambi 180 gradi. Per�, per via delle funzioni che
	// tracciano i globi e delle loro estreme ottimizzazioni, il range
	// notturno � stato rimpicciolito perch� i terminatori coincidano
	// effettivamente con i bordi del disco planetario quando lo si
	// osserva "in controluce" stagliantesi contro la sua stella.
	// Il trucco in genere non si nota; si ha l'effettiva impressione
	// che l'area diurna copra un'area considerevolmente maggiore, ma
	// dato che i terminatori coincidono sembra tutto regolare...
	// d'altronde, le sfere non posso tracciarle che cos�, se voglio
	// abbastanza velocit�.
	// "exposure" viene normalizzato per l'area diurna fra 0 e 90 gradi,
	// e in pratica rappresenta la declinazione del sole locale: zero
	// significa che il sole � all'orizzonte.
	exposure	   = (float)(crepzone) * 0.7826; // 90/115

	// "sun_x_factor" prende il valore -1 quando il sole � verso il lato
	// ovest (tramonto), altrimenti prende il valore +1. Per calcolare
	// le tre coordinate del sole si parte dalle coordinate come
	// sarebbero se entrambi gli angoli (latitude ed exposure) fossero
	// pari a zero, poi si ruota il tutto. Se gli angoli fossero zero,
	// allora sarebbe x = -d; y = 0; z = 0. Dove "d" = distanza del sole.
	// Le formule di rotazione a questo punto diventano:
	//	rx = -d * cos(beta);
	//	ry = -d * sin(beta) * sin(alfa);
	//	rz = d * sin(beta) * cos(alfa);
	// dove la "x" di partenza � la distanza del sole,
	// "alfa" � la latitudine e "beta" l'angolo d'incidenza
	// orizzontale altrimenti chiamato "exposure".
	// Infine bisogna tener conto della direzione da cui viene
	// la luce del sole (verso l'alba/verso il tramonto), che non
	// viene considerata da "exposure" ("exposure" non ha segno).
	// Per questo c'� la variabile "sun_x_factor".
	// Il tutto viene implementato come:
	alfa = deg * (90 - latitude);
	beta = deg * exposure;
	sun_x = -dsd1 * cos(beta) * sun_x_factor;
	sun_y = -dsd1 * sin(beta) * sin(alfa);
	sun_z = +dsd1 * sin(beta) * cos(alfa);

//	z2 = z * cos(beta) - x * sin(beta);
//	rz = z2 * cos(alfa) + y * sin(alfa);
//	rx = x * cos(beta) + z * sin(beta);
//	ry = y * cos(alfa) - z2 * sin(alfa);

//	rz = (z * cos(beta) - x * sin(beta)) * cos(alfa) + y * sin(alfa);
//	rx = x * cos(beta) + z * sin(beta);
//	ry = y * cos(alfa) - (z * cos(beta) - x * sin(beta)) * sin(alfa);

//	rx = x * cos(beta);
//	ry = x * sin(beta) * sin(alfa);
//	rz = -x * sin(beta) * cos(alfa);

	// Ora, eventualmente, si proietta il sole primario.
	// Succede quando ci si trova attorno ad una stella secondaria
	// di un sistema multiplo (classe 8). Vanno ripetuti tutti i
	// calcoli, compresi quelli svolti dalla f. "planets", purtroppo.
	if (!secondarysun) goto nosecondarysun;
	//
	pri_nightzone = 0;
	//
	planet_xyz (ip_targetted);
	plwp = 89 - planet_viewpoint (secondary_nearstar_x, secondary_nearstar_z);
	plwp += nearstar_p_rotation[ip_targetted];
	plwp %= 360; if (plwp < 0) plwp += 360;
	//
	ts = plwp + 35; if (ts >= 360) ts -= 360;
	te = ts + 130; if (te >= 360) te -= 360;
	ll = landing_pt_lon;
	//
	if (ts > te) {
		if (ll >= ts
			|| ll < te)
				pri_nightzone = 1;
	}
	else {
		if (ll >= ts
			&& ll < te)
				pri_nightzone = 1;
	}
	//
	te_ll_distance_1 = 0;
	ll = landing_pt_lon;
	while (ll != te) {
		te_ll_distance_1++;
		ll++; if (ll >= 360) ll = 0;
	}
	te_ll_distance_2 = 0;
	ll = landing_pt_lon;
	while (ll != te) {
		te_ll_distance_2++;
		ll--; if (ll <= -1) ll = 359;
	}
	if (te_ll_distance_1 < te_ll_distance_2)
		te_ll_distance = te_ll_distance_1;
	else
		te_ll_distance = te_ll_distance_2;
	ts_ll_distance_1 = 0;
	ll = landing_pt_lon;
	while (ll != ts) {
		ts_ll_distance_1++;
		ll++; if (ll >= 360) ll = 0;
	}
	ts_ll_distance_2 = 0;
	ll = landing_pt_lon;
	while (ll != ts) {
		ts_ll_distance_2++;
		ll--; if (ll <= -1) ll = 359;
	}
	if (ts_ll_distance_1 < ts_ll_distance_2)
		ts_ll_distance = ts_ll_distance_1;
	else
		ts_ll_distance = ts_ll_distance_2;
	if (ts_ll_distance <= te_ll_distance) {
		pri_sun_x_factor = +1;
		pri_crepzone = ts_ll_distance;
	}
	else {
		pri_sun_x_factor = -1;
		pri_crepzone = te_ll_distance;
	}
	// E con questo si sono finiti i calcoli fatti normalmente dalla
	// funzione "planets". Per� resta da ripetere i calcoli di prima,
	// su altre variabili col prefisso "pri_".
	// Oh, naturalmente bisogna usare "dsd2" al posto di "dsd1".
	pri_exposure = (float)(pri_crepzone) * 0.7826; // 90/115
	alfa = deg * (90 - latitude + pri_latitude);
	beta = deg * pri_exposure;
	pri_x = -dsd2 * cos(beta) * pri_sun_x_factor;
	pri_y = -dsd2 * sin(beta) * sin(alfa);
	pri_z = +dsd2 * sin(beta) * cos(alfa);
	nosecondarysun:

	// regolazione della direzione del vento.
	wdir = flandom() * 2 * M_PI;

	// regolazione dell'intensit� del vento.
	iwp = flandom () * 0.5 - flandom() * 0.5;
	rwp = flandom () * 50 - flandom() * 50;

/* operazioni preliminari per ogni tipo di pianeta. */

	switch (nearstar_p_type[ip_targetted]) {
		case 1: sky_brightness = 0;
			atmosphere = 0;
			break;
		case 2: sky_brightness = 63 - nightzone * 31;
			break;
		case 3:	// attribuzione pseudo-casuale degli scenari
			// per le caratteristiche generali del pianeta...
			srand (landing_pt_lon * landing_pt_lat);
			if (random(100) > 5) {
				cpos   = 555 * nearstar_p_orb_orient[ip_targetted];
				sctype = (cpos % 4) + 1;
			}
			else
				sctype = random (4) + 1;
			// correzioni per le regioni oceaniche
			if (albedo < 25)
				sctype = OCEAN;
			// correzioni per le regioni polari
			if (latitude > 75) sctype = ICY;
			if (latitude > 60 && random (3)) sctype = ICY;
			// correzioni casuali guidate all'umidit� del cielo
			// (specie in caso di deserti e ghiacciai, dove
			// praticamente non pu� piovere)
			switch (sctype) {
				case DESERT:	rainy /= random(4) + 1;
						break;
				case ICY:	rainy /= random(3) + 2;
						break;
			}
			if (rainy < 0) rainy = 0;
			if (rainy > 5) rainy = 5;
			break;
		case 4: sky_brightness = 0;
			atmosphere = 0;
			break;
		case 7: sky_brightness = 0;
			atmosphere = 0;
			break;
	}

/* selezione della tabella pseudo valida per tutto il pianeta
   (cambia solo con la latitudine, ma � previsto che sia cos�, dato
   che bisogna differenziare gli ambienti caldi da quelli freddi). */

	global_surface_seed = (nearstar_p_ray[ip_targetted]
			      + nearstar_p_orb_ray[ip_targetted]
			      + nearstar_p_orb_orient[ip_targetted]) * 4112;

	if (nearstar_p_type[ip_targetted] == 3) {
		srand (global_surface_seed + landing_pt_lon);
		if (latitude > 25 + (global_surface_seed % 15) + random(5))
			global_surface_seed++;
	}

/* generazione del cielo (in sfondo). */

	_fmemset (s_background, sky_brightness, st_bytes);
	create_sky (atmosphere);

	vptr = 0;
	for (cpos=0; cpos<bk_lines_to_horizon; cpos++) {
		for (mpul=0; mpul<360; mpul++) {
			crcy = (float) s_background[vptr] * cpos;
			crcy /= bk_lines_to_horizon;
			if (nightzone)
				s_background[vptr] = crcy / 2;
			else
				s_background[vptr] = crcy;
			vptr++;
		}
	}

/* generazione superficie. */

	build_surface ();

/* generazione animali e altre forme di vita indigene, qualora presenti. */

	setup_animals ();

/* inizio del ciclo d'esplorazione. */
	char moviestat = 0;
	int	bounces = 0;
	int	resolve = 1;
	float	fixed_step = 0;

	int	ih, oh, i9997, i9998, i9999;
	char	t[54];
	long	prog;
	long	line;

	exitflag = 0;

	if (entryflag) {
		sfh = _open (surface_file, 0);
		if (sfh != -1) {
			_read (sfh, &landing_pt_lon, 2);
			_read (sfh, &landing_pt_lat, 2);
			_read (sfh, &atl_x, 4);
			_read (sfh, &atl_z, 4);
			_read (sfh, &atl_x2, 4);
			_read (sfh, &atl_z2, 4);
			_read (sfh, &pos_x, 4);
			_read (sfh, &pos_y, 4);
			_read (sfh, &pos_z, 4);
			_read (sfh, &user_alfa, 4);
			_read (sfh, &user_beta, 4);
			_read (sfh, &openhuddelta, 2);
			_read (sfh, &openhudcount, 2);
			_read (sfh, &hud_closed, 1);
			_close (sfh);
			landed = 1;
			opencapdelta = 0;
			opencapcount = 0;
		}
		else
			goto nosurfacefile;
	}
	else {
	    nosurfacefile:
		srand (clock());
		pos_x = 1638400; pos_z = 1638400;
		pos_y = hpoint (pos_x, pos_z) - 3.2E5;
		opencapdelta = -1;
	}

	shift = 0; step = 0;
	directional_beta = user_beta;

	do {
		// inizio sincronizzazione e risolvenza dal "blank frame".
		sync_start ();
		getsecs ();
		fast_srand (secs/2);
		if (resolve <= 63) {
			tavola_colori (surface_palette, 0, 256,
				       resolve, resolve, resolve);
			resolve += 4;
		}
		// calcolo delle costanti trigonometriche degli effetti vento
		wdirsin = 10 * sin (wdir) * wp;
		wdircos = 10 * cos (wdir) * wp;
		// riduzione stanchezza (semprech� si stia fermi)
		tiredness *= 0.9977;
		pp_pulse = (1 + tiredness) * 118;
		pp_pulse += fast_flandom () * 8;
		pp_pulse -= fast_flandom () * 8;
		// variazioni atmosferiche (se c'�, l'atmosfera)
		if (atmosphere) {
			pp_pressure = base_pp_pressure;
			pp_temp += pos_y * 0.000001;
			pp_pressure *= 1 + fast_flandom () * 0.005;
			pp_pressure /= 1 + fast_flandom () * 0.005;
			pp_temp = base_pp_temp;
			pp_temp += pos_y * 0.000025;
			pp_temp *= 1 + fast_flandom () * 0.005;
			pp_temp /= 1 + fast_flandom () * 0.005;
		}
		// avanzamento fisso (tasti da 0 a 9)
		step += fixed_step;
		// reazione ai movimenti del mouse.
		bkshift = shift;
		bkstep = step;
		mpul = 0;
		mouse_input ();
		int ml = (option_mouseLook>0)?2:0;
		FastBool mlook = (mpul&2) ^ (ml);
		if (mpul&2) {
			shift += mdltx;
		} else {
			dlt_beta -= (float) mdltx / 6;
		}
		
		//(1 and 0) or (0 and 1)
		if (mlook) {
			if (option_mouseLook==1) {
				dlt_alfa += (float) mdlty / 8;
			} else {
				dlt_alfa -= (float) mdlty / 8;
			}
		}
		if (pos_y == 0) {
			if (nearstar_p_type[ip_targetted] == 3) {
				if (sctype == ICY) {
					if (mpul&1) step += 150 * landed;
					if (!mlook) step -= 10 * mdlty * landed;
				}
				else {
					if (sctype == PLAINS) {
						if (mpul&1) step += 75 * landed;
						if (!mlook) step -= 5 * mdlty * landed;
					}
					else {
						if (mpul&1) step += 50 * landed;
						if (!mlook) step -= 2 * mdlty * landed;
					}
				}
			#if defined(ALL) || defined(TEMPERATURE)
			} else if (glassified) {
				if (mpul&1) step += 150 * landed;
				if (!mlook) step -= 10 * mdlty * landed;
			#endif
			} else {
				if (mpul&1) step += 125 * landed;
				if (!mlook) step -= 5 * mdlty * landed;
			}
		}
		else {
			if (mpul&1) step += 75 * landed;
			if (!mlook) step -= 5 * mdlty * landed;
		}
		tiredness += fabs(step) * 0.000001;
		// causa una scivolata su pareti ripide discese rapidamente,
		// mentre in circostanze normali aggiorna la direzione di
		// avanzamento (che pu� non coincidere con quella in cui si
		// guarda, appunto durante le scivolate o per brevi voli su
		// pianeti con scarsa gravit�).
		if (pos_y < crcy && (!option_flying_lander || landed) && !jetpack) { //added if jetpack is OFF (MEGA)
			shift = bkshift;
			step = bkstep;
		}
		else
			directional_beta = user_beta;
		// nella capsula non pu� muoversi autonomamente.
		if (opencapdelta) {
			shift = 0;
			step = 0;
		}
		// normalizzazione degli angoli visuali.
		user_alfa += dlt_alfa;
		dlt_alfa /= 1.5; if (fabs(dlt_alfa)<0.25) dlt_alfa = 0;
		if (user_alfa<-44.9) {
			user_alfa = -44.9;
			dlt_alfa = 0;
		}
		if (user_alfa>+44.9) {
			user_alfa = +44.9;
			dlt_alfa = 0;
		}
		user_beta += dlt_beta;
		dlt_beta /= 1.5; if (fabs(dlt_beta)<0.25) dlt_beta = 0;
		if (widesnapping == 1) user_beta += widesnappingangle;
		if (widesnapping == 2) user_beta -= 2 * widesnappingangle;
		if (user_beta>180) user_beta -= 360;
		if (user_beta<-180) user_beta += 360;
		// moto proprio del player.
		refx = pos_x; refz = pos_z;
		// moto trasversale.
		alfa = 0; beta = directional_beta - 90; change_angle_of_view (); p_Forward (shift);
		// moto diretto.
		alfa = 0; beta = directional_beta; change_angle_of_view (); p_Forward (step);
		// attrito.
		if (pos_y >= crcy) {
			shift /= 1.5; if (fabs(shift)<0.5) shift = 0;
			step /= 1.25; if (fabs(step)<0.5) step = 0;
		}
		// resistenza in salita.
		if (landed && !jumping) { //I added !jumping. That fixes the "jumping slowdown on steep slopes" bug. (MEGA)
			drop_x  = pp_gravity * 0.012;
			drop_x *= hpoint (refx, refz) - hpoint (pos_x, pos_z);
			drop_x -= 2.4;
		} else {
			drop_x=0;
		}
		if (drop_x > 0) {
			if (drop_x > 1)
				drop_x = 1;
			else
				drop_x *= drop_x;
			shift *= 1 - drop_x;
			step *= 1 - drop_x;
			pos_x = refx;
			pos_z = refz;
			alfa = 0; beta = directional_beta - 90; change_angle_of_view (); p_Forward (shift);
			alfa = 0; beta = directional_beta; change_angle_of_view (); p_Forward (step);
		}
		if (pos_y > crcy - 1200)
			user_alfa /= 1 + fabs(step) * 0.000064;
		// normalizzazione area di movimento.
		drop_x = pos_x - 1.6384E6;
		drop_z = pos_z - 1.6384E6;
		drop_y = sqrt(drop_x*drop_x + drop_z*drop_z);
		if (landed)
			maxdfc = 1.5000E6;
		else
			maxdfc = 0.7500E6;
		if (drop_y > maxdfc) {
			drop_y -= maxdfc;
			drop_y *= 0.000001;
			drop_y  = 1 - drop_y;
			drop_x *= drop_y;
			drop_z *= drop_y;
			pos_x   = drop_x + 1.6384E6;
			pos_z   = drop_z + 1.6384E6;
		}
		// visualizzazione fulmini (ove presenti).
		if (flashed) {
			tavola_colori (surface_palette, 0, 256, 63, 63, 63);
			flashed = 0;
		}
		if (!flash) {
			flashes++; if (flashes>30) flashes = 30;
			ptr = 40000;
			while (ptr) {
				s_background[ptr] = 63 - s_background[ptr];
				ptr--;
			}
			// (p.s. i fulmini illuminano anche il suolo, un po')
			w = random (64) + 64;
			tavola_colori (tmppal, 0, 256, w, w, w);
			flashed = 1;
		}
		// tracciamento del cielo (che ovviamente � lo sfondo).
		alfa = user_alfa; beta = user_beta;
		ptr = ((int)(5*beta)%5) - 320*((int)(4*(alfa+180))%4) - 4;
		ptr -= (int)(beta / 72) * 320;
		ptr -= 639;
		background (360*(int)(alfa+51)-(int)(beta)%360,
			    adapted, s_background, n_offsets_map,
			    om_bytes, ptr);
		// backup zone di cielo coperte dai fulmini.
		if (!flash) {
			ptr = 40000;
			while (ptr) {
				s_background[ptr] = 63 - s_background[ptr];
				ptr--;
			}
		}
		// disegna le stelle, se � il caso di farlo.
		// il punto di vista � dell'astrozattera,
		// ma gli angoli sono quelli del protagonista.
		if (sky_brightness < 32 && rainy < 2.0) {
			cam_x = backup_dzat_x;
			cam_y = backup_dzat_y;
			cam_z = backup_dzat_z;
			alfa = user_alfa;
			beta = user_beta;
			change_angle_of_view();
			sky (0x003E);
		}
		// tracciamento del "sole" locale.
		// qui il punto di vista � fittizio:
		// � l'origine degli assi perch� il
		// sole ha delle coordinate proiettate
		// rispetto a molti fattori...
		// e vengono calcolate prima di entrare
		// nel ciclo di esplorazione.
		if (!nightzone && rainy < 2.5) {
			alfa = user_alfa;
			beta = user_beta;
			change_angle_of_view();
			cam_x = 0; cam_y = 0; cam_z = 0;
			if (atmosphere)
				whitesun (adapted, sun_x, sun_y, sun_z,
					  4 * nray1, 0);
			else
				whitesun (adapted, sun_x, sun_y, sun_z,
					  3 * nray1, 0.5);
		}
		if (secondarysun) {
		if (!pri_nightzone && rainy < 2.0) {
			alfa = user_alfa;
			beta = user_beta;
			change_angle_of_view();
			cam_x = 0; cam_y = 0; cam_z = 0;
			if (atmosphere)
				whitesun (adapted, pri_x, pri_y, pri_z,
					  4 * nray2, 0);
			else
				whitesun (adapted, pri_x, pri_y, pri_z,
					  3 * nray2, 0.5);
		}}
		// il cielo ha come colore di base 64.
		mask_pixels (adapted+2880, 64);
		// inizializzazione superficie dell'orizzonte.
		QUADWORDS = 256;
		for (ptr = 0; ptr < 32; ptr++)
			pclear (&n_globes_map[ptr<<10], ptr >> 1);
		QUADWORDS = pqw;
		txtr = n_globes_map;
		// tracciamento dell'orizzonte.
		// punto di vista: all'incirca quello dell'utente,
		// ma l'angolo beta (orientamento) viene
		// sempre impostato a zero perch� la superficie
		// � tracciata con una semi-texture e comunque
		// non cambia girandosi attorno: cambia solo se
		// si guarda verso l'alto o verso il basso.
		// inoltre, le coordinate X/Z dell'utente non
		// influiscono sull'aspetto dell'orizzonte,
		// mentre la y � amplificata per far coincidere
		// l'orizzonte con il bordo del landscape (che
		// in realt� � pi� vicino perch� altrimenti sarebbe
		// problematico disegnare qualcosa di pi� lontano).
		cam_x = 0;
		cam_y = pos_y - 1E4;
		cam_z = 0;
		alfa = user_alfa + 12;
		beta = 0;
		change_angle_of_view ();
		if (nearstar_p_type[ip_targetted] == 3) {
			// orizzonte per pianeti abitabili.
			if (sctype == PLAINS || sctype == DESERT) {
				if (sctype == DESERT) {
					// sfondo per deserti.
					if (nightzone)
						polymap (x, y, z, 4, 128);
					else
						polymap (x, y, z, 4, 160);
				}
				else {
					// sfondo per prati.
					if (nightzone)
						polymap (x, y, z, 4, 0);
					else
						polymap (x, y, z, 4, 32);
				}
			}
			else {
				// mare, per liquido o ghiacciato che sia.
				culling_needed = 1;
				if (nightzone)
					poly3d (x, y, z, 4, 128);
				else
					polymap (x, y, z, 4, 144);
				// disegna il riflesso della superficie...
				if (!waves_in) {
					backup_cam_x = cam_x;
					backup_cam_y = cam_y;
					backup_cam_z = cam_z;
					backup__alfa = alfa;
					backup__beta = beta;
					from_user ();
					cam_x += sp_x;
					cam_y += sp_y + waving_y;
					cam_z += sp_z;
					mirror = 1;
					halfscan_needed = 1;
					txtr = p_background;
					ipfx = ((long)(cam_x)) >> 14;
					ipfz = ((long)(cam_z)) >> 14;
					iperficie (0);
					txtr = n_globes_map;
					halfscan_needed = 0;
					cam_x = backup_cam_x;
					cam_y = backup_cam_y;
					cam_z = backup_cam_z;
					alfa  = backup__alfa;
					beta  = backup__beta;
					change_angle_of_view ();
				}
				// luccich�i generici
				if (sctype == ICY) {
					// appaiono semisolidificati, sul ghiaccio
					fast_srand ( ((long)(pos_x)>>10) + ((long)(pos_z)>>10) );
				}
				else {
					// e molto pi� variabili sull'acqua
					fast_srand (pos_x+pos_z+clock()*3);
				}
				flicks = fabs(wp*50) + 100;
				for (flick = 0; flick < flicks; flick++)
					n_globes_map[fast_random(32767)] = 16 + fast_random(15);
				// luccich�i mirati (in riflesso diretto della luce della stella)
				if (!nightzone) {
					fshift = xsun_onscreen - x_centro_f;
					fshift = (float)fshift * -0.34;
					fshift += 90;
					if (fshift>0 && fshift<256) {
						flicks = 250 + fabs(wp*25);
						for (flick = 0; flick < flicks; flick++) {
							ptr = fshift + (fast_random(127) << 8);
							n_globes_map[ptr + fast_random(63)] = 16 + fast_random(15);
						}
					}
				}
				setfx (1); // poligoni lucidi
				polymap (x, y, z, 4, 0);
				setfx (0); // poligoni normali
				culling_needed = 0;
			}
		}
		else {
			// orizzonte planetario (standard)
			if (nightzone)
				ptr = albedo / 4;
			else
				ptr = albedo / 2;
			polymap (x, y, z, 4, ptr);
		}
		txtr = p_background;
		// variazioni alla posizione: moto ondoso.
		upanddown += 0.2;
		pos_x0 = pos_x - 1E6;
		pos_z0 = pos_z - 1E6;
		if (waves_in || waves_out) {
			if (crcy == 0) {
				if (waves_in) {
					drop_y    = sqrt (pos_x0*pos_x0 + pos_z0*pos_z0) / (1E5 - 1E3*wp);
					waving_y  = drop_y * sin(upanddown) - drop_y;
				}
				else
					waving_y  = 0;
				waving_y += 50 * sin(3*upanddown) - 100;
			}
			else
				waving_y = 0;
		}
		// variazioni alla posizione: "saltellamento".
		// nell'acqua simula il movimento oscillatorio dovuto
		// alle onde ed al nuoto. sulla terraferma, tende ad
		// orientare verticalmente lo sguardo in modo da seguire
		// l'andamento dei rilievi (salite e discese).
		if (landed && user_alfa>-40 && user_alfa<40) {
			if (waving_y && !jumping)
				dlt_alfa += sin(upanddown)/3;		//To decrease bobbing, the /3 was added.
			else {
				//This code was making the user look down every time the time-based resync was performed.
				//Now it only does it when we jump or use the jetpack.
				// (SL)
				if (jumping && pos_y > crcy - 2400) {
					drop_x = ((float)(crcy) - pos_y) * 0.001;
					if (drop_x < 0) drop_x *= 1.67;
					//dlt_alfa += drop_x;	//commenting this out gives you full control over your head again.. :) (MEGA)
				}
			}
		}
		// variazioni alla posizione:
		// attribuisce "consistenza" al suolo ed ai suoi dislivelli,
		// e controlla gli effetti del vento quando si � sospesi
		// a mezz'aria (durante un grosso salto o scendendo con la
		// capsula). inoltre controlla l'accelerazione gravitazionale.
		crcy = hpoint (pos_x, pos_z);
		pos_y += gravity;
		if (pos_y > crcy) {
			pos_y = crcy;
			if (!landed) {
				refy = hpoint (pos_x, pos_z);
				compdist = -1000000;
				temp = 0;
				while (temp < 2*M_PI) {
					drop_x = pos_x - 1024 * sin(temp);
					drop_z = pos_z + 1024 * cos(temp);
					drop_y = hpoint (drop_x, drop_z) - refy;
					if (drop_y > compdist) {
						compdist = drop_y;
						wdir = temp;
					}
					temp += 0.025;
				}
				if (!opencapdelta && gravity < 250
				&& (compdist < 512 || bounces > 10)) {
					pos_x   = ((((long)pos_x) >> 14) << 14) + 8192;
					pos_z   = ((((long)pos_z) >> 14) << 14) + 8192;
					pos_y   = hpoint (pos_x, pos_z);
					atl_x   = ((long)pos_x) >> 14;
					atl_z   = ((long)pos_z) >> 14;
					atl_x2  = 8192;
					atl_z2  = 8192;
					gravity = 0;
					landed = 1;
				}
				else {
					bounces++;
					gravity = - 0.32 * gravity;
				}
			}
		}
		else {
			if (landed) {
				if (pos_y > crcy - 300) {
					drop_y = planet_grav * (pos_y - crcy) * 0.00333; // 1/300
					gravity += drop_y;
				}
				else
					gravity += planet_grav;
			}
			else {
				gravity += planet_grav * 0.16;
				pos_x -= sin(wdir) * wp * 10;
				pos_z += cos(wdir) * wp * 10;
			}
		}
		if (pos_y > (crcy - 200)){ //disable Jumping whenever you're on the ground again. (MEGA)
			jumping = 0;
			jetpack = 0;
		}
		if (pos_y < (crcy - 200)){
			jumping = 1;
		}
		// controlla se si � tornati alla capsula.
		if (landed) {
			drop_x = pos_x - ((atl_x << 14) + atl_x2);
			drop_z = pos_z - ((atl_z << 14) + atl_z2);
			drop_y = pos_y - hpoint ((atl_x << 14) + atl_x2, (atl_z << 14) + atl_z2);
			if (sqrt(drop_x*drop_x+drop_y*drop_y+drop_z*drop_z)<1600) {
				if (recover) {
					pos_x -= 0.125 * drop_x;
					pos_y -= 0.125 * drop_y;
					pos_z -= 0.125 * drop_z;
					opencapdelta = +1;
				}
			}
			else
				recover = 1;
		}
		// tracciamento onde in arrivo (onde del mare)
		from_user ();
		cam_x += sp_x;
		cam_y += sp_y + waving_y;
		cam_z += sp_z;
		if (waves_in) {
			// calcolo della distanza del player dal centro onde.
			dfc = sqrt (pos_x0 * pos_x0 + pos_z0 * pos_z0);
			// inizializzazione texture delle onde in arrivo.
			txtr = n_globes_map;
			for (ptr = 20480+256; ptr < 32768; ptr++)
				n_globes_map[ptr] = n_globes_map[ptr-256] >> 1;
			// usa i primi 32K, dove c'� la sfumatura del mare...
			V_MATRIXS = 8; change_txm_repeating_mode();
			// ora attiva poligoni brillanti...
			setfx (2);
			// e un basso livello di dettaglio...
			culling_needed = 1;
			// ciclo di tracciamento delle onde in arrivo.
			// (normali onde del mare, prodotte dal vento)
			w = 10;
			while (w) {
				w--;
				if (wr[w] <= 0) {
					// questo produce nuove onde...
					wh[w] = 500 + wp * 100 + random(1500);
					wd[w] = (wp + 50) * -10;
					wr[w] = 15E5;
				}
				else {
					// questo invece succede quando
					// si "becca" un'onda in faccia...
					if (landed && !opencapdelta) {
						if (pos_y > -50 || waveblur) {
							if (waveblur) {
								pos_x = pos_x0 / (1 - (wd[w] / 2E6)) + 1E6;
								pos_z = pos_z0 / (1 - (wd[w] / 2E6)) + 1E6;
								user_alfa *= 1 + (float) random(5) * 0.002;
							}
							if (fabs (dfc - wr[w]) - 3*wl[w] <= fabs (2*wd[w]))
								waveblur = 1 + random (3);
						}
					}
				}
				if (fabs(pos_y)+fabs(dfc-wr[w]) > 0.5*wr[w])
					goto dontdo;
				drop_x = 0;
				yy[1] = wy[w];
				yy[0] = wy[w] - wh[w];
				yy[3] = wy[w] - wh[w];
				yy[2] = wy[w];
				drop_z = 18 * deg;
				cpos = 0;
			  redo:	while (drop_x<2*M_PI) {
					drop_y = wr[w] - wl[w];
					xx[1] = cos (drop_x) * wr[w]  + wx[w];
					zz[1] = sin (drop_x) * wr[w]  + wz[w];
					xx[0] = cos (drop_x) * drop_y + wx[w];
					zz[0] = sin (drop_x) * drop_y + wz[w];
					drop_x += drop_z;
					xx[3] = cos (drop_x) * drop_y + wx[w];
					zz[3] = sin (drop_x) * drop_y + wz[w];
					xx[2] = cos (drop_x) * wr[w]  + wx[w];
					zz[2] = sin (drop_x) * wr[w]  + wz[w];
					polymap (xx, yy, zz, 4, 128);
				}
				if (!cpos) {
					drop_x = 9*deg; cpos++;
					goto redo;
				}
			dontdo:	wr[w] += wd[w];
			}
			// reimpostazione dei normali parametri
			// per il texturing...
			setfx (0); // seleziona poligoni normali
			culling_needed = 0; // alto livello dettaglio
			V_MATRIXS = 16; change_txm_repeating_mode();
			txtr = p_background;
		}
		// qui disegna tutto il landscape.
		mirror = 0; // non � un riflesso.
		ipfx = ((long)(cam_x)) >> 14;
		ipfz = ((long)(cam_z)) >> 14;
		if (landed)
			iperficie (1);
		else
			iperficie (4);
		// questo traccia la capsuletta mentre si scende...
		if (!landed) {
			H_MATRIXS = 0;
			V_MATRIXS = 0;
			change_txm_repeating_mode();
			txtr = n_globes_map;
			cam_x = 0;
			cam_z = 0;
			cam_y = 1030;
			polycupola (-1, 1);
			setfx (0); cupola (-1, 8);
			cam_y -= 1030;
			polycupola (+1, 1);
			setfx (0); cupola (+1, 8);
			cam_y = 0;
			// reimpostazione dei normali parametri
			// per il texturing...
			setfx (0); // seleziona poligoni normali
			culling_needed = 0; // alto livello dettaglio
			V_MATRIXS = 16; change_txm_repeating_mode();
			txtr = p_background;
		}
		// tracciamento onde in partenza (acqua smossa)
		if (waves_out) {
			// inizializzazione della texture per gli spruzzi
			// prodotti dal fatto che si sta nuotando, o comunque
			// annaspando da fermi, per rimanere a galla...
			txtr = n_globes_map;
			// usa i primi 32K, dove c'� la sfumatura del mare...
			V_MATRIXS = 8; change_txm_repeating_mode();
			// ora attiva poligoni brillanti...
			setfx (2);
			// e un basso livello di dettaglio...
			culling_needed = 1;
			// e costruisci le sagome degli spruzzi,
			// quando si nuota rapidamente...
			if (fabs(step) > 200 && pos_y > -50) {
				for (ptr = 20480; ptr < 20480+256; ptr++)
					n_globes_map[ptr] = fast_random(63);
				for (ptr = 20480; ptr > 0; ptr--) {
					if (n_globes_map[ptr+256])
						n_globes_map[ptr] = n_globes_map[ptr+256] - 1;
					else
						n_globes_map[ptr] = 0;
				}
			}
			for (ptr = 20480+256; ptr < 32768; ptr++)
				n_globes_map[ptr] = n_globes_map[ptr-256] >> 1;
			// ciclo di tracciamento delle onde in partenza.
			// queste sono quelle prodotte dal protagonista,
			// e le differenze sono le seguenti:
			// - la cresta � orientata verso l'esterno.
			// - l'onda si espande, non si restringe.
			// - l'onda si appiattisce sempre pi� e,
			//   dopo un po' di tempo, svanisce.
			w = now;
			while (w >= 10) {
				w--;
				if (wr[w] > 100 * wh[w]) wr[w] = 0;
				if (wr[w] == 0) goto prox;
				drop_x = 0;
				yy[1] = wy[w];
				yy[0] = wy[w] - wh[w];
				yy[3] = wy[w] - wh[w];
				yy[2] = wy[w];
				drop_z = 18 * deg;
				while (drop_x<2*M_PI) {
					drop_y = wr[w] + wl[w];
					xx[0] = cos (drop_x) * drop_y + wx[w];
					zz[0] = sin (drop_x) * drop_y + wz[w];
					xx[1] = cos (drop_x) * wr[w]  + wx[w];
					zz[1] = sin (drop_x) * wr[w]  + wz[w];
					drop_x += drop_z;
					if (hpoint (xx[0], zz[0]) == 0) {
						xx[2] = cos (drop_x) * wr[w]  + wx[w];
						zz[2] = sin (drop_x) * wr[w]  + wz[w];
						xx[3] = cos (drop_x) * drop_y + wx[w];
						zz[3] = sin (drop_x) * drop_y + wz[w];
						polymap (xx, yy, zz, 4, 128);
					}
				}
				wr[w] += wd[w];
				wh[w] /= 1.025;
				prox:
			}
			// reimpostazione dei normali parametri
			// per il texturing...
			setfx (0); // seleziona poligoni normali
			culling_needed = 0; // alto livello dettaglio
			V_MATRIXS = 16; change_txm_repeating_mode();
			txtr = p_background;
		}
		// tracciamento dell'alone del "sole", eventualmente.
		if (!nightzone && rainy < 1.2) {
			if (nearstar_class!=5&&nearstar_class!=6&&nearstar_class!=10) {
				if (nearstar_class!=11||gl_start<90) {
					if (dsd1<1000*nray1&&dsd1>=10*nray1) {
						alfa = user_alfa;
						beta = user_beta;
						change_angle_of_view();
						lens_flares_for (0, 0, 0, sun_x, sun_y, sun_z,
								 (10 * nray1) / dsd1, 1 + (0.002 * dsd1), hud_closed, 2, 1, 1);
					}
				}
			}
		}
		// tracciamento dell'alone del "sole" primario, se presente.
		if (secondarysun) {
		if (!pri_nightzone && rainy < 2.1) {
			if (dsd2<1000*nray2&&dsd2>=10*nray2) {
				alfa = user_alfa;
				beta = user_beta;
				change_angle_of_view();
				lens_flares_for (0, 0, 0, pri_x, pri_y, pri_z,
						 (10 * nray2) / dsd2, 1 + (0.002 * dsd2), hud_closed, 2, 1, 1);
			}
		}}
		// Variazioni ambientali.
		// Include: pioggia, vento, generazione delle onde, fulmini,
		//          effetti di un'atmosfera particolarmente densa.
		if (atmosphere) {
			// effetti del vento, che si fanno sentire quandunque
			// ci sia un'atmosfera, al di l� del fatto che essa
			// sia respirabile o meno.
			iwp  += (float)(random(albedo)) * 0.020;
			iwp  -= (float)(random(albedo)) * 0.020;
			if (iwp < -1) iwp = -1;
			if (iwp > 1) iwp = 1;
			wp   += ((iwp * rwp) - wp) * 0.05;
			wdir += flandom() * iwp * 0.1;
			wdir -= flandom() * iwp * 0.1;
		}
		if (nearstar_p_type[ip_targetted] == 2) {
			mask_pixels (adapted+2880, 0);
			psmooth_grays (adapted+2880);
			goto ends;
		}
		if (nearstar_p_type[ip_targetted] == 3) {
			if (rainy >= 2 || flashes > 5) {
				// quando piove...
				wdirsin = sin (wdir) * wp;
				wdircos = cos (wdir) * wp;
				wdirsin -= (pos_z - refz) * 0.333;
				wdircos -= (pos_x - refx) * 0.333;
				srand (clock());
				fast_srand (clock() % 18);
				ptr = random(3) + 1 ; while (ptr) {
					flash = random (150 / rainy);
					ptr--;
				}
				if (rainy > 3) {
					setfx (1);
					Forward (-1000);
					ptr = random (25 * flashes) + 50;
					temp = (float)(clock()%18) * 100;
					while (ptr) {
						drop_x = pos_x + fast_random (1023) - fast_random (1023);
						drop_z = pos_z + fast_random (1023) - fast_random (1023);
						drop_y = pos_y - fast_random (2047) + temp;
						stick3d (drop_x, drop_y, drop_z,
							 drop_x + wdircos, drop_y + 100, drop_z + wdirsin);
						ptr--;
					}
					setfx (0);
				}
			}
			if (pos_y>=0) {
				// Nuota a pelo d'acqua (ove possibile).
				// Quindi fa delle piccole onde...
				if (nearstar_p_type[ip_targetted] == 3
				&& (sctype == PLAINS || sctype == DESERT))
					goto ends; // non c'� acqua su deserti e prati.
				waveratio = 10 - fabs(step) / 5;
				if (waveratio < 4) waveratio = 4;
				if (!(clock()%waveratio)) {
					from_user ();
					srand (clock());
					wr[lw] = 200;
					wh[lw] = random (100)    + fabs(3 * step);
					wy[lw] = 50 		 + fabs(3 * step);
					wd[lw] = random (5) + 12 + fabs(2 * step);
					wl[lw] = random (50)     + fabs(2 * step);
					wx[lw] = pos_x - step*opt_tsinbeta*opt_tcosalfa;
					wz[lw] = pos_z + step*opt_tcosbeta*opt_tcosalfa;
					lw++; if (lw>24) lw = 10;
				}
			}
		}
		// annebbiamento della vista (eventuale),
		// subito dopo che si � stati bagnati da un'onda...
	  ends: 
	  	// trying to have status on surface working (neuzd)
	  	if (fcs_status_delay>0) {
			int len = strlen(fcs_status_extended);
			len = len+len;
			wrouthud(160-len, 100, NULL, fcs_status_extended);
			fcs_status_delay--;
		}
		
	  	if (waveblur) {
			ptr = waveblur;
			while (ptr) {
				psmooth_64 (adapted, 160);
				ptr--;
			}
			waveblur--;
		}
		else {
			QUADWORDS = 160 + openhudcount * 80;
			psmooth_64 (adapted, 160);
			psmooth_64 (adapted, 160);
			QUADWORDS = pqw;
		}
		
		if (moviestat) {							//F3 - MOVIE STATUS
			ShowMovieSetup(moviefsec, movieflashoff, moviedeck);
		}
		CalculatemovieFPS(moviefscap, moviestime);
		
		
		// il fotogramma � finito. ora lo visualizza.
		surrounding (1, openhudcount);
		QUADWORDS = 16000;
		if (!widesnapping) pcopy (adaptor, adapted);
		QUADWORDS = pqw;
		// effetto di apertura della capsula:
		// quando � totalmente aperta, si pu� scendere.
		if (opencapdelta < 0) {
			opencapcount += opencapdelta;
			if (opencapcount <= 0) {
				opencapdelta = 0;
				opencapcount = 0;
			}
		}
		// effetto di chiusura della capsula:
		// quando � totalmente sigillata, scotty beam me up.
		if (opencapdelta > 0) {
			opencapcount += opencapdelta;
			if (opencapcount > 32) {
				pos_y -= 20 * (opencapcount - 31);
				landed = 0;
				
				// movie recording has been extended for some time during the ascent.
				// so the control has been moved elsewhere (neuzd)
				//movie = 0;
			}
			if (opencapcount > 250)
				break;
		}
		// effetto di apertura della visiera dell'HUD.
		if (openhuddelta < 0) {
			openhudcount += openhuddelta;
			if (openhudcount <= 0) {
				openhuddelta = 0;
				openhudcount = 0;
			}
		}
		// effetto di chiusura della visiera dell'HUD.
		if (openhuddelta > 0) {
			openhudcount += openhuddelta;
			if (openhudcount >= 180) {
				openhuddelta = 0;
				openhudcount = 180;
				hud_closed = 1;
			}
		}
		// consumi supplementari della navicella
		// (ovviamente continua a fare il suo lavoro anche
		// mentre si esplora la superficie, per cui consuma)
		additional_consumes ();
		// aggiornamento coordinate di stepping.
		add_height (pos_x, pos_z, 300);
		// fine sincronizzazione fotogrammi.
		sync_stop ();
		// controllo funzione widesnapping.
		if (widesnapping) {
			if (widesnapping == 1) {
				dzat_x = backup_dzat_x;
				dzat_y = backup_dzat_y;
				dzat_z = backup_dzat_z;
				snapshot (9997+widesnapping, 1);
				dzat_x = 0;
				dzat_y = 0;
				dzat_z = 0;
			}
			else
				snapshot (9997+widesnapping, 0);
			widesnapping++;
			if (widesnapping == 3) {
				char recycled;
				widesnapping = 0;
				user_beta += widesnappingangle;
				//
				ih = sa_open (header_bmp);
				if (ih==NOFILE) goto wserror;
				READFILE (ih, t, 54);
				t[18] = 0x94;
				t[19] = 0x03;
				CLOSEFILE (ih);
				prog = lastSnapshot;
				recycled=0;
				do {
					prog++; if (prog == 100000000) {	//Image limit increased to 99,999,999 (SL)
						if (recycled) {
							status("TOO MANY SNAPSHOTS", 300);
							goto wserror;
						} else {
							recycled=1;
							prog=0;
						}
					}
					sprintf (snapfilename, "..\\GALLERY\\%08d.BMP", prog);	//Image limit increased to 99,999,999 (SL)
					ih = OPENFILE (snapfilename, OPEN_RB);
					if (ih != NOFILE) CLOSEFILE (ih);
				} while (ih != NOFILE);
				lastSnapshot = prog;
				//Panoramic snapshots no longer overwrite pictures 9997-9999 if those happened to exist. (SL)
				i9997 = _open ("..\\GALLERY\\WIDE9997.BMP", 0);
				i9998 = _open ("..\\GALLERY\\WIDE9998.BMP", 0);
				i9999 = _open ("..\\GALLERY\\WIDE9999.BMP", 0);
				if (i9997 == -1 || i9998 == -1 || i9999 == -1) {
					if (i9997 != -1) _close (i9997);
					if (i9998 != -1) _close (i9998);
					if (i9999 != -1) _close (i9999);
					goto wserror;
				}
				//
				oh = _creat (snapfilename, 0);
				if (oh==-1) goto wserror;
				_write (oh, t, 54);
				//
				lseek (i9997, 54L, SEEK_SET);
				_read (i9997, adapted, 1024);
				_write (oh, adapted, 1024);
				//
				lseek (i9997, 1078L, SEEK_SET);
				for (line = 0; line < 200; line++) {
					_read (i9997, adapted, 320);
					lseek (oh, line*916L + 1078L + 309L, SEEK_SET);
					_write (oh, adapted + 10, 299);
				}
				//
				lseek (i9998, 1078L, SEEK_SET);
				for (line = 0; line < 200; line++) {
					_read (i9998, adapted, 320);
					lseek (oh, line*916L + 1078L, SEEK_SET);
					_write (oh, adapted, 309);
				}
				//
				lseek (i9999, 1078L, SEEK_SET);
				for (line = 0; line < 200; line++) {
					_read (i9999, adapted, 320);
					lseek (oh, line*916L + 1078L + 608L, SEEK_SET);
					_write (oh, adapted + 10, 308);
				}
				//
				_close (oh);
				//
			    wserror:
				//Fixed the taking-panoramic-screenshots-b0rks-stuff bug. (SL)
				//(Alex forgot to close the original screenshot files before deleting them)
				if (i9997!=-1) _close (i9997);
				if (i9998!=-1) _close (i9998);
				if (i9999!=-1) _close (i9999);
				remove ("..\\GALLERY\\WIDE9997.BMP");
				remove ("..\\GALLERY\\WIDE9998.BMP");
				remove ("..\\GALLERY\\WIDE9999.BMP");
			}
		}
		
		// movie stuff
		if (movie) {
			moviedelay = moviedelay - 1;
			if (moviedelay <= 0) {
				widesnapping = 0;
				snapshot (0, 0);
				moviedelay = moviefsec;
				
			}
			// the counter increases every gameplay frame (not recorded frames)
			// and starts only if the pod is being recalled to the SD.  
			if (!landed && recover)
				movie_cutoff++;
				
			// movie recording shuts off after 100 frames (neuzd)
			// this is because if we leave the movie going, the pod won't be
			// able to leave the planet.
			// In this way we can record a portion of the ascent.
			if (movie_cutoff > 100)
			{
				movie = 0;
				moviedeck++;
				movieexists=0;
				movie_paused = 0;
			}	
		}
		// controllo tastiera.
		while (tasto_premuto()) {
			w = attendi_pressione_tasto();
			if (!w) {
				w = attendi_pressione_tasto();
				if (w == 0x49) {
					openhuddelta = -5;
					hud_closed = 0;
				}
				if (w == 0x51)
					openhuddelta = +5;
				if (w == 83){								// Delete - RAWSNAP  ( also "b")
					snapshot (0, 0);
				}
				
				if (w==0x3D){								// f3 - moviestat
					if (moviestat == 0){
						moviestat = 1;
						
					}
					else {
						moviestat = 0;
					}
				}
				if (w == 0x48){
					//user_alfa = user_alfa - 1;
					option_mouseLook++;
					if (option_mouseLook>2) option_mouseLook=0;
					if (option_mouseLook==0) {
						status("MOUSELOOK DISABLED", 50);
					} else if (option_mouseLook==1) {
						status("MOUSELOOK ENABLED", 50);
					} else {
						status("INVERTED Y AXIS", 50);
					}
				}
				if (w==144 && moviestat && !movie) {					//add 1 to moviedeck
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
				if (w==142 && moviestat && !movie ) {					//substract 1 from moviedeck
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
				/*if (w == 80) {
					latitude++;
					alfa = deg * (90 - latitude);
					beta = deg * exposure;
					sun_x = -dsd1 * cos(beta) * sun_x_factor;
					sun_y = -dsd1 * sin(beta) * sin(alfa);
					sun_z = +dsd1 * sin(beta) * cos(alfa);
					alfa = deg * (90 - latitude + pri_latitude);
					beta = deg * pri_exposure;
					pri_x = -dsd2 * cos(beta) * pri_sun_x_factor;
					pri_y = -dsd2 * sin(beta) * sin(alfa);
					pri_z = +dsd2 * sin(beta) * cos(alfa);
				}
				if (w == 72) {
					latitude--;
					alfa = deg * (90 - latitude);
					beta = deg * exposure;
					sun_x = -dsd1 * cos(beta) * sun_x_factor;
					sun_y = -dsd1 * sin(beta) * sin(alfa);
					sun_z = +dsd1 * sin(beta) * cos(alfa);
					alfa = deg * (90 - latitude + pri_latitude);
					beta = deg * pri_exposure;
					pri_x = -dsd2 * cos(beta) * pri_sun_x_factor;
					pri_y = -dsd2 * sin(beta) * sin(alfa);
					pri_z = +dsd2 * sin(beta) * cos(alfa);
				}*/
			}
			else {
				//if (w=='+' && surlight < 63) surlight++;
				//if (w=='-' && surlight > 10) surlight--;
				if (w=='+' && surlight < 63 && !moviestat) surlight++;
				if (w=='-' && surlight > 10 && !moviestat) surlight--;

				if (w=='+' && moviestat && moviefsec < 999 && !movie) moviefsec++;
				if (w=='-' && moviestat && moviefsec > 1 && !movie ) moviefsec--;
				if (w==13) {
					if (movie == 0 && moviestat) {
						movie = 1;
						moviefscap = 0;
						moviestime = gtime;
						moviestat = 0;
						movienr = 0;
						movie_paused = 0;
					}
					else if (movie == 1) {
                                           movie = 0;
                                           //movieexists = 2;
                                           movieexists = 0;
                                           moviedeck++;
                                           movie_paused = 0;
                                          }
				}
				if (w=='f' && moviestat && !movie) {
					if (movieflashoff == 1) movieflashoff = 0;
					else movieflashoff = 1;
				}

				if ((w == '*'  || w == 'm' ) && !widesnapping) {
					dzat_x = backup_dzat_x;
					dzat_y = backup_dzat_y;
					dzat_z = backup_dzat_z;
					snapshot (0, 1);
					dzat_x = 0;
					dzat_y = 0;
					dzat_z = 0;
				}
				if ((w == '/' || w == 'n') && !widesnapping) {
					snapshot (9997, 0);
					widesnapping = 1;
				}
				if (w >= 48 && w <= 57) {
					w = w - 48;
					if (fixed_step == (w * 10))
						fixed_step = 0;
					else
						fixed_step = (w * 10);
					goto endtype;
				}
				if ((w == 'b') && widesnapping==0) {                // b - RAWSNAP (also 'delete')
					snapshot (0, 0);
				}
				if (w == 'c') {
					jetpack = 0;
				}
				if (w == 'j') {
					if (pos_y > crcy - 10) {
					gravity -= gravity + 500;
					jumping = 1;
				        }
				}
				if (w == 32) { //pressin spacebar? JETPACK THE HELL OUTTA HERE!!! :P (MEGA)
					if (landed) {	//Jetpack shouldn't work if you're in the lander (SL)
						blinkhudlights = 1;
						blinkhudlights_stay = 0;
						if (pos_y > crcy - 150 ) {
							gravity -=gravity + 500;
						} else {
							gravity = gravity - 50;
							jumping = 1;
							jetpack = 1;
						}
					}
				}
				// Added a pause feature when filming a movie (neuzd)
				if (w == 'p')
				{
					if (movie)
					{
						movie = 0;
						moviestat = 1;
						movie_paused = 1;
					}
					else
					{
						if (moviestat && movie_paused)
						{
							movie = 1;
							moviestat = 0;
							movie_paused = 0;
						}
					}
				}
				
				if (w == 'l' && pos_y < crcy + 10) { //Mommy, can I return now to the surface? (MEGA)
					gravity = gravity + 400;
				}

				if (w == 27 && landed) {
					sfh = _creat (surface_file, 0);
					if (sfh != -1) {
						_write (sfh, &landing_pt_lon, 2);
						_write (sfh, &landing_pt_lat, 2);
						_write (sfh, &atl_x, 4);
						_write (sfh, &atl_z, 4);
						_write (sfh, &atl_x2, 4);
						_write (sfh, &atl_z2, 4);
						_write (sfh, &pos_x, 4);
						_write (sfh, &pos_y, 4);
						_write (sfh, &pos_z, 4);
						_write (sfh, &user_alfa, 4);
						_write (sfh, &user_beta, 4);
						_write (sfh, &openhuddelta, 2);
						_write (sfh, &openhudcount, 2);
						_write (sfh, &hud_closed, 1);
						_close (sfh);
						exitflag = 1;
						goto nodissolve;
					}
				}
			}
		}
		endtype:
	} while (1);

	/* operazioni di recupero per ritornare al ciclo principale. */

	// Dissolvenza al nero (blank frame di ritorno).
	for (w=64; w>=0; w-=4) {
		tavola_colori (surface_palette, 0, 256, w, w, w);
		ll = clock(); while (ll == clock());
	}

    nodissolve:
	pclear (adapted, 0);
	pclear (adaptor, 0);

	dzat_x = backup_dzat_x;
	dzat_y = backup_dzat_y;
	dzat_z = backup_dzat_z;

	field_amplificator = bfa;

	pos_x = 0; pos_y = 0; pos_z = -3100;
	user_alfa = 0; user_beta = 0;
	dlt_alfa = 0; dlt_beta = 0;
	step = 100; shift = 0;

	unloadallpv ();
	loadpv (vehicle_handle, vehicle_ncc, 15, 15, 15, 0, 0, 0, 0, 1);

	load_QVRmaps ();
	load_starface ();
	load_digimap2 ();

	npcs = -12345;
}
