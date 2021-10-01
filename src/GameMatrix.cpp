#include "led-matrix.h"
#include "Tetris.h"
#include "Inputs.h"
#include "Menu.h"

// #include "Audio/AlsaInput.h"
// #include "Audio/WaveletBpmDetector.h"
// #include "Audio/SlidingMedian.h"
// #include "Audio/FFTData.h"

#include "pixel-mapper.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <termios.h>
#include <string>

#include <wiringPi.h>
#include <mcp23017.h>

#define PI 3.14159265
#define PLASMA_BASE_COUNT 30

using namespace rgb_matrix;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

enum MatrixMode {
	MenuMode,
	TetrisMode,
	AnimationMode,
	ClockMode
};
static MatrixMode matrixMode;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static bool _running;
static bool isKB;

volatile bool inputs[TOTAL_INPUTS];

// Check if there is any input on the unbuffered terminal
bool inputAvailable()
{
	std::cin.clear();
	
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return (FD_ISSET(0, &fds));
}

// Get update inputs
static void getch() 
{
	char buf = 0;
	if (read(STDIN_FILENO, &buf, 1) < 0)
	{
		perror ("read()");
	}
	
	switch (buf)
	{
		case 'w': // Up
		case 'k':
		{
			inputs[UpStick] = true;
			break;
		}
		case 's': // Down
		case 'j':
		{
			inputs[DownStick] = true;
			break;
		}
		case 'a': // Left
		case 'h':
		{
			inputs[LeftStick] = true;
			break;
		}
		case 'd': // Right
		case 'l':
		{
			inputs[RightStick] = true;
			break;
		}
		case 'q': // Rotate
		{
			inputs[AButton] = true;
			break;
		}
		case 'e': // Rotate
		{
			inputs[BButton] = true;
			break;
		}
		// Exit program
		case 0x1B:           // Escape
		case 0x04:           // End of file
		case 0x00:           // Other issue from getch()
		{
			inputs[MenuButton] = true;
			break;
		}
	}
}

struct termios old;
void enableTerminalInput()
{
	// Set terminal to unbuffered mode
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	struct termios no_echo = old;
	no_echo.c_lflag &= ~ICANON;
	no_echo.c_lflag &= ~ECHO;
	no_echo.c_cc[VMIN] = 1;
	no_echo.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &no_echo) < 0)
		perror("tcsetattr ICANON");
}

void disableTerminalInput()
{
	// Restore terminal attributes
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
	perror ("tcsetattr ~ICANON");
}

static void getArcadeInput()
{
	int i, input;
	for (i = 0; i < TOTAL_INPUTS; i++)
	{
		input = digitalRead(GPIO_OFFSET + i);

		if (input == 0)
		{
			inputs[i] = true;
		}
	}
}

const int mapSize = 64;
int heightMap1[mapSize * mapSize];
int heightMap2[mapSize * mapSize];
int dx1, dy1, dx2, dy2;
int plasmaCount;
int plasmaCountTarget;
bool prevInputs[TOTAL_INPUTS];
Color palette[256];
Color palette1[256];
Color palette2[256];
bool prevPaletteDirection;

double distance(double x, double y)
{
	return sqrt(x * x + y * y);
}

void interpolate(Color* c, Color* c1, Color* c2, double f)
{
	c->r = floor(c1->r + (c2->r - c1->r) * f);
	c->g = floor(c1->g + (c2->g - c1->g) * f);
	c->b = floor(c1->b + (c2->b - c1->b) * f);
}

Color* randomColor()
{
	Color *c = new Color();
	c->r = floor(rand() * 255);
	c->g = floor(rand() * 255);
	c->b = floor(rand() * 255);
	return c;
}

void makeRandomPalette(Color *palette)
{
	Color *c1 = randomColor();
	Color *c2 = randomColor();
	Color *c3 = randomColor();
	Color *c4 = randomColor();
	Color *c5 = randomColor();

	 for (int i = 0; i < 64; i++) {
        double f = i / 64;
        interpolate(&palette[i], c1, c2, f);
      }

      for (int i = 64; i < 128; i++) {
        double f = (i - 64) / 64;
        interpolate(&palette[i], c2, c3, f);
      }

      for (int i = 128; i < 192; i++) {
        double f = (i - 128) / 64;
        interpolate(&palette[i], c3, c4, f);
      }

      for (int i = 192; i < 256; i++) {
        double f = (i - 192) / 64;
        interpolate(&palette[i], c4, c5, f);
      }
}

void InitPlasma()
{
	dx1 = 0;
	dy1 = 0;
	dx2 = 0;
	dy2 = 0;

	uint64_t plasmaCount = 0;
	plasmaCountTarget = PLASMA_BASE_COUNT;

	makeRandomPalette(palette1);
	makeRandomPalette(palette2);

	prevPaletteDirection = true;

	for (int u = 0; u < mapSize; u++)
	{
		for (int v = 0; v < mapSize; v++)
		{
			int i = u * mapSize + v;

			double cx = u - mapSize / 2;
			double cy = v - mapSize / 2;

			double d = distance(cx, cy);

			double stretch = (3 * PI) / (mapSize / 2);

			double ripple = sin(d * stretch);

			double normalized = (ripple + 1) / 2;

			heightMap1[i] = floor(normalized * 128);
		}
	}

	for (int u = 0; u < mapSize; u++)
	{
		for (int v = 0; v < mapSize; v++)
		{
			int i = u * mapSize + v;

			double cx = u - mapSize / 2;
			double cy = v - mapSize / 2;

			double d1 = distance(0.8 * cx, 1.3 * cy) * 0.022;
        	double d2 = distance(1.35 * cx, 0.45 * cy) * 0.022;

			double s = sin(d1);
			double c = sin(d2);
			double h = s + c;

			double normalized = (h + 2) / 4;

			heightMap2[i] = floor(normalized * 127);
		}
	}
}

int PlasmaLoop(RGBMatrix* matrix, volatile bool *inputs)
{
	// Proccess inputs on button down
    if (inputs[UpStick] && !prevInputs[UpStick])
    {
        // Change speed
    }

	if (inputs[DownStick] && !prevInputs[DownStick])
    {
        // Change speed
    }
    
    if (inputs[MenuButton] && !prevInputs[MenuButton])
    {
        for (int i = 0; i < TOTAL_INPUTS; i++)
        {
            prevInputs[i] = inputs[i];
            inputs[i] = false;
        }
        
        return -1;
    }

    for (int i = 0; i < TOTAL_INPUTS; i++)
    {
        prevInputs[i] = inputs[i];
        inputs[i] = false;
    }

	// Move height map
	if (plasmaCount++ % plasmaCountTarget == 0)
	{
		dx1 = floor((((cos(0.0002 + 0.4 + PI) + 1)/2) * mapSize) / 2);
		dy1 = floor((((cos(0.0003 - 0.1) + 1) / 2) * mapSize) / 2);
		dx2 = floor((((cos(-0.0002 + 1.2) + 1) / 2) * mapSize) / 2);
		dy2 = floor((((cos( -0.0003 - 0.8 + PI) + 1) / 2) * mapSize) / 2);
	}

	// Update palette
	double x = plasmaCount * 0.0005;
	double inter = (cos(x) + 1) / 2;

	bool direction = -sin(x) >= 0;
	if (prevPaletteDirection != direction) {
	prevPaletteDirection = direction;
		if (direction == false) {
			makeRandomPalette(palette1);
		} else {
			makeRandomPalette(palette2);
		}
	}

	// create interpolated palette for current frame
	for (int i = 0; i < 256; i++) {
		interpolate(&palette[i], &palette1[i], &palette2[i], inter);
	}

	FrameCanvas *canvas = matrix->CreateFrameCanvas();

	for (int u = 0; u < mapSize; u++)
	{
		for (int v = 0; v < mapSize; v++)
		{
			int i = (u + dy1) * mapSize + (v + dx1);
			int k = (u + dy2) * mapSize + (v + dx2);

			int h = heightMap1[i] + heightMap2[k];

			Color c = palette[h];
			canvas->SetPixel(u, v, c.r, c.g, c.b);
		}
	}

	matrix->SwapOnVSync(canvas, 2U);
	return 0;
}

int main(int argc, char *argv[]) 
{
	matrixMode = ClockMode;

	RGBMatrix::Options defaults;
	defaults.hardware_mapping = "adafruit-hat-pwm";
	defaults.rows = 32;
	defaults.cols = 32;

	defaults.parallel = 1;
	defaults.chain_length = 4;
	defaults.pixel_mapper_config = "U-Mapper;Rotate:180";

	defaults.limit_refresh_rate_hz = 120;
	defaults.show_refresh_rate = false;

	rgb_matrix::RuntimeOptions rtOptions;
	rtOptions.gpio_slowdown = 4;
	rtOptions.drop_privileges = 0;

	rtOptions.daemon = 0;
	rtOptions.do_gpio_init = true;

	RGBMatrix *matrix = RGBMatrix::CreateFromOptions(defaults, rtOptions);
	if (matrix == NULL)
	{
		return 1;
	}

	// It is always good to set up a signal handler to cleanly exit when we
	// receive a CTRL-C for instance.
	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	wiringPiSetup();
	mcp23017Setup(GPIO_OFFSET, 0x20);
	for (int i = 0; i < TOTAL_INPUTS; i++)
	{
			pinMode(GPIO_OFFSET + i, INPUT);
			pullUpDnControl(GPIO_OFFSET + i, PUD_UP);
	}

	// Init Engine Resources
	// std::shared_ptr<ThreadSync> sync = std::make_shared<ThreadSync>();
	// std::shared_ptr<AlsaInput> audio = std::make_shared<AlsaInput>(interrupt_received, sync);
	// int64_t last_read = 0;
    // unsigned int last_written = 0;
	// int64_t windowSize = 131072;
	// std::vector<Sample> values = std::vector<Sample>(windowSize);
	// std::vector<float> data = std::vector<float>(windowSize);
	// std::shared_ptr<FreqData> freq =  std::make_shared<FreqData>(2048, audio->get_rate());
	// WaveletBPMDetector detector = WaveletBPMDetector(12, windowSize, freq);
	// CircularBuffer<float> amps = CircularBuffer<float>(windowSize * 2);

	// using Timestamp = std::chrono::steady_clock::time_point;
    // using Duration = std::chrono::steady_clock::duration;
	// SlidingMedian<float, Timestamp, Duration> slide = SlidingMedian<float, Timestamp, Duration>(std::chrono::seconds(5));

	Menu *m = new Menu();
	Tetris *t  = new Tetris();

	// Enabel KB mode if specified  by cmdline arg
	isKB = false;
	if (argc > 1 && isatty(STDIN_FILENO))
	{
		std::string kb ("kb");
		if (kb.compare(argv[1]) == 0)
		{
			std::cout << "KB mode enabled!" << std::endl;
			isKB = true;
			enableTerminalInput();
		}
	}

	_running = true;

	// Game Engine
	while (!interrupt_received && _running)
	{
		if (isKB)
		{
			if (inputAvailable())
			{
				getch();
			}
		}
		else
		{
			getArcadeInput();
		}

		switch (matrixMode)
		{
			case MenuMode:
				switch (m->Loop(matrix, inputs))
				{
					case TetrisMenuOption:
						matrixMode = TetrisMode;
						break;
					case AnimationMenuOption:
						matrixMode = AnimationMode;
						break;
					case ClockMenuOption:
						matrixMode = ClockMode;
						break;
					case RotateMenuOption:
						matrix->ApplyPixelMapper(FindPixelMapper("Rotate", 4, 1, "90"));
						break;
					default:
						break;
				}
				break;
			case TetrisMode:
				if (t->PlayTetris(inputs) == -1)
				{
					matrixMode = MenuMode;
				}
				t->DrawTetris(matrix);
				break;
			case AnimationMode:
				if (PlasmaLoop(matrix, inputs) == -1)
				{
					matrixMode = MenuMode;
				}
				break;
			case ClockMode:
				if(m->ClockLoop(matrix, inputs) == -1)
				{
					matrixMode = MenuMode;
				}
				break;
			default:
				break;
		}
	}

	interrupt_received = true;

	if (isKB)
	{
		disableTerminalInput();
	}

	delete matrix;

	return 0;
}