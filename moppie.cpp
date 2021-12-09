#include <sndfile.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <SDL/SDL.h>
#ifdef __MINGW32__
#include <SDL_image.h>
#include <windows.h>
#else
#include <SDL/SDL_image.h>
#endif
#include <SDL/SDL_gfxPrimitives.h>

#include "error.h"
#include "keyboard.h"

const int nBoxes = 24;
const int draw_size = 8; // 8 per w/h
const int slp = 10000; // ms

#define min(x, y) ((x) < (y) ? (x) : (y))

typedef struct {
	int x, y;
	int r, g, b, a;
	bool is_circle;
} box_t;

#ifdef __MINGW32__
// http://stackoverflow.com/questions/17419562/how-to-remap-keyboard-key-in-c-with-lowlevelkeyboardproc
HHOOK hHook = 0;

// http://www.gamedev.net/topic/123398-disable-alt-tab--exclusive-fullscreen/
LRESULT CALLBACK LowLevelKeyboardProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	// By returning a non-zero value from the hook procedure, the

	// message does not get passed to the target window

	KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *) lParam;
	BOOL bControlKeyDown = false;

	switch (nCode)
	{
		case HC_ACTION:
			{
				// Check to see if the CTRL key is pressed

				bControlKeyDown = GetAsyncKeyState (VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1);

				// Disable CTRL+ESC

				if (pkbhs->vkCode == VK_ESCAPE && bControlKeyDown)
					return 1;            // Disable ALT+TAB

				if (pkbhs->vkCode == VK_TAB && pkbhs->flags & LLKHF_ALTDOWN)
					return 1;            // Disable ALT+ESC

				if (pkbhs->vkCode == VK_ESCAPE && pkbhs->flags & LLKHF_ALTDOWN)
					return 1;

				// http://msdn.microsoft.com/en-us/library/ee416808(v=vs.85).aspx
				if (pkbhs->vkCode == VK_LWIN || pkbhs->vkCode == VK_RWIN) // windows keys
					return 1;

				// http://stackoverflow.com/questions/4718069/what-is-win32-virtual-key-code-0xff-used-for-and-is-it-documented-somewhere
				if (pkbhs->vkCode == 0xff) // Fn
					return 1;

				// http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
				if (pkbhs->vkCode == 0x5f) // sleep
					return 1;

				if (pkbhs->vkCode == 0xad) // mute
					return 1;
			}
			break;

		default:
			break;
	}

	return CallNextHookEx (hHook, nCode, wParam, lParam);
} 
#endif

SDL_Surface* loadSurface(const char *const file, const SDL_Surface *const optimize_for)
{
	SDL_Surface *loadedSurface = IMG_Load(file);
	if (!loadedSurface)
		error_exit(false, "Failed loading %s: %s\n", file, IMG_GetError());

	return loadedSurface;
}

double get_ts()
{
	struct timeval tv = { 0, 0 };

	gettimeofday(&tv, NULL);

	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

double hue_to_rgb(double m1, double m2, double h)
{
	if (h < 0.0) h += 1.0;
	if (h > 1.0) h -= 1.0;
	if (6.0 * h < 1.0)
		return (m1 + (m2 - m1) * h * 6.0);
	if (2.0 * h < 1.0)
		return m2;
	if (3.0 * h < 2.0)
		return (m1 + (m2 - m1) * ((2.0 / 3.0) - h) * 6.0);

	return m1;
}

void hls_to_rgb(double H, double L, double S, double *r, double *g, double *b)
{
	if (S == 0)
		*r = *g = *b = L;
	else
	{
		double m2;

		if (L <=0.5)
			m2 = L*(1.0+S);
		else
			m2 = L+S-L*S;

		double m1 = 2.0 * L - m2;

		*r = hue_to_rgb(m1, m2, H + 1.0/3.0);
		*g = hue_to_rgb(m1, m2, H);
		*b = hue_to_rgb(m1, m2, H - 1.0/3.0);
	}
}

static Uint8 *audio_chunk = NULL;
static int audio_len = 0;
static int audio_pos = 0;

void fill_audio(void *udata, Uint8 *stream, int len)
{
	if (!audio_chunk)
	{
		memset(stream, 0x00, len);
		return;
	}

	int cur = min(len, audio_len - audio_pos);

	SDL_MixAudio(stream, &audio_chunk[audio_pos], cur, SDL_MIX_MAXVOLUME);
	audio_pos += cur;

	if (audio_pos >= audio_len)
		audio_chunk = NULL;
}

typedef struct
{
	uint8_t *data;
	int len;
} sample_t;

void play_audio(const sample_t *const s)
{
	audio_len = s -> len;
	audio_pos = 0;
	audio_chunk = s -> data;

	SDL_PauseAudio(0);
}

void init_audio()
{
	SDL_AudioSpec wanted;

	wanted.freq = 22050;
	wanted.format = AUDIO_S16;
	wanted.channels = 1;    /* 1 = mono, 2 = stereo */
	wanted.samples = 1024;  /* Good low-latency value for callback */
	wanted.callback = fill_audio;
	wanted.userdata = NULL;

	/* Open the audio device, forcing the desired format */
	if (SDL_OpenAudio(&wanted, NULL) == -1)
		error_exit(false, "Can't open audio device: %s", SDL_GetError());
}

void load_sample(const char *const filename, sample_t *const s)
{
	SF_INFO in_format;
	SNDFILE *fh = sf_open(filename, SFM_READ, &in_format);
	if (!fh)
		error_exit(true, "Cannot open %s", filename);

	if (in_format.channels != 1)
		error_exit(false, "Input file must be a mono recording");

	sf_count_t n_samples = sf_seek(fh, 0, SEEK_END);
	(void)sf_seek(fh, 0, SEEK_SET);

	s -> len = n_samples;
	s -> data = (uint8_t *)malloc(s -> len * sizeof(short));

	int rc = -1;
	if ((rc = sf_readf_short(fh, (short *)s -> data, n_samples)) != n_samples)
		error_exit(true, "Failed reading from %s: %s - got only %d of %d samples", filename, sf_strerror(fh), rc, n_samples);

	sf_close(fh);
}

void findKey(const std::vector<std::vector<int> > & kb, const int sc, const int xres, const int yres, int *const x, int *const y)
{
	const int hLines = kb.size();
	bool found = false;
	int xf = rand() % 100, yf = rand() % 100, scalew = 100, scaleh = 100;

	for(int yi=0; yi<hLines && !found; yi++)
	{
		const int wKeys = kb.at(yi).size();

		for(int xi=0; xi<wKeys; xi++)
		{
			if (kb.at(yi).at(xi) == sc)
			{
				xf = xi;
				yf = yi;
				scalew = wKeys;
				scaleh = hLines;
				found = true;
				break;
			}
		}
	}

	*x = (xf * xres) / scalew;
	*y = (yf * yres) / scaleh;
}

void help()
{
	printf("moppie v" VERSION ", (C) 2014 by folkert@vanheusden.com\n\n");
	printf("While running, enter \"STOP!\" to terminate the program.\n\n");
	printf("-s   be silent, do not play a sound when a key is pressed\n");
	printf("-h   this help\n");
}

int main(int argc, char *argv[])
{
	int c = -1;
	bool silent = false;

	while((c = getopt(argc, argv, "sh")) != -1)
	{
		if (c == 's')
			silent = true;
		else if (c == 'h')
		{
			help();
			return 0;
		}
	}

	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

	sample_t samples[3];

	load_sample("MetalBell.wav", &samples[0]);
	load_sample("explosionsfx.wav", &samples[1]);
	load_sample("brass.wav", &samples[2]);

	const SDL_VideoInfo *svi = SDL_GetVideoInfo();
	SDL_Surface *screen = SDL_SetVideoMode(0, 0, 32, (svi -> hw_available ? SDL_HWSURFACE : SDL_SWSURFACE) | SDL_ASYNCBLIT | SDL_FULLSCREEN);
	if (!screen)
		error_exit(false, "Failed initializing SDL: %s\n", SDL_GetError());

	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
		error_exit(false, "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());

	SDL_Surface *const snake = loadSurface("Rattlesnake.png", screen);

	SDL_WM_GrabInput(SDL_GRAB_ON);

#ifdef __MINGW32__
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, NULL, NULL);
#endif

	const int w = screen -> w;
	const int h = screen -> h;

	const int dimw = w / draw_size;
	const int dimh = w / draw_size;

	box_t box[nBoxes];

	memset(&box, 0x00, sizeof box);
	for(int i=0; i<nBoxes; i++)
		box[i].x = -1;

	int bOffset = 0;

	if (!silent)
		init_audio();

	srand(time(NULL));

	SDL_EnableUNICODE(1);
	char last_chars[5] = { 0 };

	std::vector<std::vector<int> > kb = getKeyboard();

	double downsince = get_ts();
	bool keydown = false, justdown = false;

	int bxd_speed = w / (1000000 / slp);
	int byd_speed = h / (1000000 / slp);
	int bouncex = w / 2, bxd = bxd_speed;
	int bouncey = h / 2, byd = byd_speed;

	if (rand() & 1)
		bxd = -bxd;
	if (rand() & 1)
		byd = -byd;

	for(;;)
	{
		SDL_Event event;

		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				justdown = keydown = true;
				downsince = get_ts();

				memmove(&last_chars[0], &last_chars[1], 4);
				last_chars[4] = event.key.keysym.unicode & 0x7f;

				if (memcmp(last_chars, "STOP!", 5) == 0)
					break;
			}
			else if (event.type == SDL_KEYUP)
			{
				keydown = false;
			}
		}

		if (!silent && justdown)
			play_audio(&samples[event.key.keysym.sym % 3]);

		if (justdown)
		{
			int x = -1, y = -1;
			findKey(kb, event.key.keysym.sym, w, h, &x, &y);

			box[bOffset].x = x;
			box[bOffset].y = y;

			double H = (double)(rand() % 360) / 360.0;
			double L = 0.5;
			double S = 1.0;
			double rd, gd, bd;
			hls_to_rgb(H, L, S, &rd, &gd, &bd);
			int r = rd * 255.0, g = gd * 255, b = bd * 255;
			int a = (rand() % 127) + 128;

			box[bOffset].r = r;
			box[bOffset].g = g;
			box[bOffset].b = b;
			box[bOffset].a = a;
			box[bOffset].is_circle = rand() & 1;

			bOffset++;
			if (bOffset == nBoxes)
				bOffset = 0;
		}

		justdown = false;

		boxRGBA(screen, 0, 0, w - 1, h - 1, 0, 0, 0, 255);

		for(int i=0; i<nBoxes; i++)
		{
			if (box[i].x < 0)
				continue;

			if (box[i].is_circle)
			{
				boxRGBA(screen, box[i].x, box[i].y, box[i].x + dimw, box[i].y + dimh, box[i].r, box[i].g, box[i].b, box[i].a);

				rectangleRGBA(screen, box[i].x, box[i].y, box[i].x + dimw, box[i].y + dimh, box[i].r + 5, box[i].g + 5, box[i].b + 5, 255);
			}
			else
			{
				filledCircleRGBA(screen, box[i].x, box[i].y, (dimw * 2) / 3, box[i].r, box[i].g, box[i].b, box[i].a);

				circleRGBA(screen, box[i].x, box[i].y, (dimw * 2) / 3, box[i].r + 5, box[i].g + 5, box[i].b + 5, 255);
			}
		}

		if (keydown && get_ts() - downsince > 1.0)
		{
			if (bouncex < 0)
			{
				bouncex = 0;
				bxd = bxd_speed / 4 + (rand() % ((bxd_speed * 3) / 4));
			}
			else if (bouncex >= w)
			{
				bouncex = w - 1;
				bxd = -(bxd_speed / 4 + (rand() % ((bxd_speed * 3) / 4)));
			}
			if (bouncey < 0)
			{
				bouncey = 0;
				byd = byd_speed / 4 + (rand() % ((byd_speed * 3) / 4));
			}
			else if (bouncey >= h)
			{
				bouncey = h - 1;
				byd = -(byd_speed / 4 + (rand() % ((byd_speed * 3) / 4)));
			}

			SDL_Rect d;
			d.x = bouncex;
			d.y = bouncey;
			d.w = snake -> w;
			d.h = snake -> h;

			SDL_BlitSurface(snake, NULL, screen, &d);

			bouncex += bxd;
			bouncey += byd;
		}

		SDL_UpdateRect(screen, 0, 0, w, h);

		usleep(slp);
	}

	return 0;
}
