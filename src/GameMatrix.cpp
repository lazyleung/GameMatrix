#include "led-matrix.h"

#include "pixel-mapper.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>

using namespace rgb_matrix;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

#define TETRIS_BOARD_ROWS 12
#define TETRIS_BOARD_COLS 10
#define BLOCK_SIZE 5
#define BOARD_X_OFFSET 7
#define BOARD_Y_OFFSET 4

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static void DrawOnCanvas(RGBMatrix *matrix) {
	/*
	* Let's create a simple animation. We use the canvas to draw
	* pixels. We wait between each step to have a slower animation.
	*/

	FrameCanvas *canvas = matrix->CreateFrameCanvas();
	canvas->Fill(0, 0, 255);

	int center_x = canvas->width() / 2;
	int center_y = canvas->height() / 2;
	float radius_max = canvas->width() / 2;
	float angle_step = 1.0 / 360;
	for (float a = 0, r = 0; r < radius_max; a += angle_step, r += angle_step) {
		if (interrupt_received)
		return;
		float dot_x = cos(a * 2 * M_PI) * r;
		float dot_y = sin(a * 2 * M_PI) * r;
		canvas->SetPixel(center_x + dot_x, center_y + dot_y,
						255, 0, 0);
		matrix->SwapOnVSync(canvas);
		usleep(1 * 500);  // wait a little to slow down things.
	}
}

enum BlockStatus
{
	None,
	Default,
	Blue,
	Pink
};

// Tetis Block size = 5
// 5 x 10 wide, 5 x 12 Tall
// Tetris width always 10 wide
struct Row {
	BlockStatus cols[TETRIS_BOARD_ROWS];
};

struct PiecePos
{
	int x, y;
};

static PiecePos currentPiecePos[4];
static PiecePos nextPiecePos[4];

static Row *TetrisBoard;
static int _baseRow;

static int GetBaseRow()
{
	return _baseRow;
}

static Row* GetRow(int rowNum)
{
	int row = _baseRow + rowNum;

	if (row >= TETRIS_BOARD_ROWS)
	{
		row -= TETRIS_BOARD_ROWS;
	}

	return &TetrisBoard[row];
}

static Color* _defaultColor;
static Color* GetDefaultColor()
{
	return new Color(_defaultColor->r, _defaultColor->g, _defaultColor->b);
}

static void UpdateDefaultColor()
{
	if (_defaultColor->r < 255)
	{
		_defaultColor->r++;
	}
	else if (_defaultColor->g < 255)
	{
		_defaultColor->g++;
	}
	else if (_defaultColor->b < 255)
	{
		_defaultColor->b++;
	}
	else if (_defaultColor->b > 0)
	{
		_defaultColor->b--;
	}
	else if (_defaultColor->g > 0)
	{
		_defaultColor->g--;
	}
	else if (_defaultColor->r > 0)
	{
		_defaultColor->r--;
	}
}

static void IncreaseBaseRow()
{
	if (++_baseRow >= TETRIS_BOARD_ROWS)
	{
		_baseRow = 0;
	}
}

void InitTetris()
{
	int rows = TETRIS_BOARD_ROWS;
	TetrisBoard = (Row*)calloc(sizeof(Row), rows);
	_baseRow = 0;

	for (int y = 0; y < rows; y++)
	{
		std::cout << "Base Row: " << GetBaseRow();
		std::cout << "Row " << y << ":\t";
		for (int x = 0; x < TETRIS_BOARD_ROWS; x++)
		{
			Row *r = GetRow(y);
			if (x == 3 && y == 7)
			{
				r->cols[x] = Blue;
			}
			else if (x == 7 && y == 3)
			{
				r->cols[x] = Pink;
			}
			else if (x == 5 && y == 5)
			{
				r->cols[x] = Default;
			}
			else
			{
				r->cols[x] = None;
			}
			std::cout << (r->cols[x] != None ? "1 " : "0 ");
		}
		std::cout << std::endl;
	}

	_defaultColor = new Color(0, 0, 0);

	// TODO Need to zero currentPiecePos or nextPiecePos?
}

static void DrawTetris(RGBMatrix *matrix)
{
	FrameCanvas *canvas = matrix->CreateFrameCanvas();

	for (int x = 0; x < canvas->width(); x++)
	{
		for (int y = 0; y < canvas->height(); y++)
		{
			if ((x < BOARD_X_OFFSET || x > BOARD_X_OFFSET + (BLOCK_SIZE * TETRIS_BOARD_COLS)) ||
				(y > canvas->height() - BOARD_Y_OFFSET || y < canvas->height() - BOARD_Y_OFFSET - (BLOCK_SIZE * TETRIS_BOARD_ROWS)))
			{
				// Draw border background
				canvas->SetPixel(x, y, 108, 64, 173);
			}
			else
			{
				// Draw tetris board
				int col = (x - BOARD_X_OFFSET) / BLOCK_SIZE;
				int row = (canvas->height() - y - BOARD_Y_OFFSET) / BLOCK_SIZE;

				Row *tRow = GetRow(row);
				BlockStatus bStatus = tRow->cols[col];

				if (bStatus == None)
				{
					// Draw board background or piece
					canvas->SetPixel(x, y, 184, 184, 184);
					for (int piece = 0; piece < 4; piece++)
					{
						if (currentPiecePos[piece].x == x && currentPiecePos[piece].y == y)
						{
							// Draw piece color
							canvas->SetPixel(x, y, 255, 25, 25);
							break;
						}
					}
				}
				else
				{
					if ((x - BOARD_X_OFFSET) % BLOCK_SIZE > 0  )
					{
						// Draw Border
						canvas->SetPixel(x, y, 0, 0, 0);
					}
					else
					{
						// Draw Color
						Color *c = new Color(255, 255, 255);
						switch(bStatus)
						{
							case Default:
								c = GetDefaultColor();
								break;
							case Blue:
								c->r = 201;
								c->g = 255;
								c->b = 229;
								break;
							case Pink:
								c->r = 255;
								c->g = 51;
								c->b = 255;
								break;
							default:
							case None:
								break;
						}
						canvas->SetPixel(x, y, c->r, c->g, c->b);
					}
				}
			}
		}
	}
	matrix->SwapOnVSync(canvas, 2U);
}

static int count = 0;
void PlayTetris()
{
	if (count++ < 100)
	{
		IncreaseBaseRow();
	}
	else
	{
		count = 0;
	}

	if (count % 10 == 0)
	{
		UpdateDefaultColor();
	}
}

int main(int argc, char *argv[]) {
	InitTetris();

	RGBMatrix::Options defaults;
	defaults.hardware_mapping = "adafruit-hat-pwm";
	defaults.rows = 32;
	defaults.cols = 32;

	defaults.parallel = 1;
	defaults.chain_length = 4;
	defaults.pixel_mapper_config = "U-Mapper";

	defaults.limit_refresh_rate_hz = 120;
	defaults.show_refresh_rate = true;

	rgb_matrix::RuntimeOptions rtOptions;
	rtOptions.gpio_slowdown = 3;
	rtOptions.drop_privileges = 0;

	rtOptions.daemon = 0; // Set to 1 for production
	rtOptions.do_gpio_init = false; // Set to true for debugging on pc

	RGBMatrix *matrix = RGBMatrix::CreateFromOptions(defaults, rtOptions);
	if (matrix == NULL)
	{
		return 1;
	}

	// It is always good to set up a signal handler to cleanly exit when we
	// receive a CTRL-C for instance.
	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	// StartUp Animation
	DrawOnCanvas(matrix);

	// Tetris Engine
	while (true)
	{
		DrawTetris(matrix);
	}

	// Finished. Shut down the RGB matrix.
	delete matrix;

	return 0;
}