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

int main(int argc, char *argv[]) 
{
	matrixMode = MenuMode;

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
						break;
					case RotateMenuOption:
						matrix->ApplyPixelMapper(FindPixelMapper("Rotate", 4, 1, "Rotate:90"));
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

				break;
			case ClockMode:
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