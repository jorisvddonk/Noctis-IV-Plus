extern float moviefps;
extern char outhudbuffer[81];
extern clock_t gtime;
extern void CalculatemovieFPS(int moviefscap, int moviestime);
extern void ShowMovieSetup(int moviefsec, char movieflash, int moviedeck);
extern void wrouthud (unsigned x, unsigned y, unsigned l, char *text);
extern void FitOutHudBuffer(int min, int max);