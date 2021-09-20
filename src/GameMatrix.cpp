#include "led-matrix.h"
#include "Tetris.h"

#include "pixel-mapper.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <termios.h>

#include <wiringPi.h>
#include <mcp23017.h>

#define GPIO_OFFSET 64

using namespace rgb_matrix;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static bool _running;

volatile char inputC = 0x00;

static void DrawOnCanvas(RGBMatrix *matrix) {
	/*
	* Let's create a simple animation. We use the canvas to draw
	* pixels. We wait between each step to have a slower animation.
	*/

	FrameCanvas *canvas = matrix->CreateFrameCanvas();
	matrix->Fill(0, 0, 255);

	int center_x = canvas->width() / 2;
	int center_y = canvas->height() / 2;
	float radius_max = canvas->width() / 2;
	float angle_step = 1.0 / 360;
	for (float a = 0, r = 0; r < radius_max; a += angle_step, r += angle_step) {
		if (interrupt_received)
		return;
		float dot_x = cos(a * 2 * M_PI) * r;
		float dot_y = sin(a * 2 * M_PI) * r;
		matrix->SetPixel(center_x + dot_x, center_y + dot_y,
						255, 0, 0);
		usleep(1 * 30);  // wait a little to slow down things.
	}
}

// Check if there is any input on the unbuffered terminal
// bool inputAvailable()
// {
// 	std::cin.clear();
	
// 	struct timeval tv;
// 	fd_set fds;
// 	tv.tv_sec = 0;
// 	tv.tv_usec = 0;
// 	FD_ZERO(&fds);
// 	FD_SET(STDIN_FILENO, &fds);
// 	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
// 	return (FD_ISSET(0, &fds));
// }

// Get single char
// static char getch() 
// {
// 	char buf = 0;
// 	if (read(STDIN_FILENO, &buf, 1) < 0)
// 		perror ("read()");

// 	return buf;
// }

static char getArcadeInput()
{
	int i, input;
	for (i = 0; i < 4; i++)
	{
			input = digitalRead(GPIO_OFFSET + i);
			if (input == 0)
			{
					std::cout << "Joystick (Pin " << GPIO_OFFSET + i << ")" << std::endl;
					switch (i)
					{
					case 0:
						return 'a';
						break;
					case 1:
						return 'd';
						break;
					case 2:
						return 'w';
						break;
					case 3:
						return 's';
						break;
					default:
						break;
					}
			}
	}
	input = digitalRead(GPIO_OFFSET + i);
	if (input == 0)
	{
			std::cout << "Button (Pin " << GPIO_OFFSET + i << ")" <<  std::endl;
			return 'q';
	}
	return 0x00;
}

static void inputHandler() 
{
	std::cout << "Button Press!" << std::endl;
	inputC = 'q';
}

static void inputHandlerJoystick() 
{
	std::cout << "Joystick Press!" << std::endl;
	inputC = 'a';
}

int main(int argc, char *argv[]) {
	RGBMatrix::Options defaults;
	defaults.hardware_mapping = "adafruit-hat-pwm";
	defaults.rows = 32;
	defaults.cols = 32;

	defaults.parallel = 1;
	defaults.chain_length = 4;
	defaults.pixel_mapper_config = "U-Mapper";

	defaults.limit_refresh_rate_hz = 120;
	defaults.show_refresh_rate = false;

	rgb_matrix::RuntimeOptions rtOptions;
	rtOptions.gpio_slowdown = 3;
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
	// Stick
	int i;
	for (i = 0; i < 4; i++)
	{
			wiringPiISR(GPIO_OFFSET + i, INT_EDGE_FALLING, &inputHandlerJoystick) ;
			// pinMode(GPIO_OFFSET + i, INPUT);
			// pullUpDnControl(GPIO_OFFSET + i, PUD_UP);
	}

	// Buttons
	for (; i < 3; i++)
	{
			wiringPiISR(GPIO_OFFSET + i, INT_EDGE_FALLING, &inputHandler) ;
			//pinMode(GPIO_OFFSET + i, INPUT);
			//pullUpDnControl(GPIO_OFFSET + i, PUD_UP);
	}

	_running = true;

	Tetris *t = new Tetris();

	// StartUp Animation
	DrawOnCanvas(matrix);

	// Set terminal to unbuffered mode
	// struct termios old;
	// bool is_terminal = isatty(STDIN_FILENO);
	// if (is_terminal) {
	// 	if (tcgetattr(0, &old) < 0)
	// 		perror("tcsetattr()");

	// 	struct termios no_echo = old;
	// 	no_echo.c_lflag &= ~ICANON;
	// 	no_echo.c_lflag &= ~ECHO;
	// 	no_echo.c_cc[VMIN] = 1;
	// 	no_echo.c_cc[VTIME] = 0;
	// 	if (tcsetattr(0, TCSANOW, &no_echo) < 0)
	// 		perror("tcsetattr ICANON");
	// }

	// Tetris Engine
	while (!interrupt_received && _running)
	{
		// volatile char c = 0x00;
		// if (inputAvailable())
		// {
		// 	//c = tolower(getch());
		// }

		//volatile char c = getArcadeInput();

		t->PlayTetris(&inputC);
		t->DrawTetris(matrix);
	}

	// Restore terminal attributes
	// if (is_terminal) {
	// 	if (tcsetattr(0, TCSADRAIN, &old) < 0)
	// 	perror ("tcsetattr ~ICANON");
	// }

	delete matrix;

	return 0;
}
