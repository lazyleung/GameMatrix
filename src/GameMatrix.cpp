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
#define PIECE_SIZE 4
#define BOARD_X_OFFSET 7
#define BOARD_Y_OFFSET 4


#define LINE_CLEAR_TIMER 20
#define GRAVITY_TIMER 60

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

// ========== Tetris Stuff ==========
// ---------- Struct & Fields ----------

enum BlockStatus
{
	None,
	Clearing,
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

// TODO change from buffer to linked list

static PiecePos currentPiece[PIECE_SIZE];
static PiecePos savedPiece[PIECE_SIZE];

static BlockStatus _currentPieceStatus;

// Point of rotation is [?][1]
// Based on following mapping:
// 1 2 3 4
//   5 6 7
int pieceShapes[7][4] =
{
    1,3,5,7, // I
    2,4,5,7, // Z
    3,5,4,6, // S
    3,5,4,7, // T
    2,3,5,7, // L
    3,5,7,6, // J
    2,3,4,5, // O
};

static Row * TetrisBoard;
static int _baseRow;

static Color * _defaultColor;

static bool _rotate = false;

static bool _isLineClearing = false;
static int _linesToClear = 0;
static int _clearTimer = 0;

static int _gravityTimer = 0;

// ---------- Accessors ----------

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

static void UpdateDefaultColor()
{
	if (_defaultColor->r < 255)
	{
		_defaultColor->r+=5;
	}
	else if (_defaultColor->g < 255)
	{
		_defaultColor->g+=5;
	}
	else if (_defaultColor->b < 255)
	{
		_defaultColor->b+=5;
	}
	else if (_defaultColor->b > 0)
	{
		_defaultColor->b-=5;
	}
	else if (_defaultColor->g > 0)
	{
		_defaultColor->g-=5;
	}
	else if (_defaultColor->r > 0)
	{
		_defaultColor->r-=5;
	}

	std::cout << "Default color is now: " << unsigned(_defaultColor->r) << "\t" << unsigned(_defaultColor->g) << "\t" << unsigned(_defaultColor->b) << std::endl;
}

static void ClearLines (int y)
{
	std::cout << "Lines cleared: " << y << std::endl;
	_isLineClearing = true;
	_linesToClear = y;
	for (int i = 0; i < y; i++)
	{
		Row *r = GetRow(i);
		for (int j = 0; j < TETRIS_BOARD_COLS; j++)
		{
			r->cols[j] = Clearing;
		}
	}
}

static void IncreaseBaseRow()
{
	if (!_isLineClearing)
	{
		for (int i = 0; i < _linesToClear; i++)
		{
			Row *r = GetRow(i);
			for (int j = 0; j < TETRIS_BOARD_COLS; j++)
			{
				r->cols[j] = None;
			}
		}

		_baseRow = (_baseRow + _linesToClear) % TETRIS_BOARD_ROWS;

		_linesToClear = 0;
	}
}

// ---------- Helpers ----------

// Check that all blocks of piece are in valid locations
bool checkPiecePos(PiecePos *piece)
{
	for (int block = 0; block < PIECE_SIZE; block++)
	{
		// Within board
		if (piece[block].x < 0 || piece[block].x >= TETRIS_BOARD_COLS ||
			piece[block].y < 0 || piece[block].y >= TETRIS_BOARD_ROWS)
		{
			return false;
		}
		// Not on board block
		else if (GetRow(piece[block].y)->cols[piece[block].x] != None)
		{
			return false;
		}
	}
	return true;
}

// Reset next piece to current if pos invalid
void checkCurrentPiecePos()
{
	if(!checkPiecePos(currentPiece))
	{
		for (int block = 0; block < PIECE_SIZE; block++)
		{
			currentPiece[block] = savedPiece[block];
		}
	}
}

static void addPiece()
{
	// Insert base piece
	int shape = rand() % 7;
	// TODO add random color status
	_currentPieceStatus = Default;
	for (int block = 0; block < PIECE_SIZE; block++)
	{
		currentPiece[block].y = TETRIS_BOARD_ROWS-1 - (pieceShapes[shape][block] % 2 == 0 ?  1 : 0);
		currentPiece[block].x = TETRIS_BOARD_COLS/2 - 2 + (pieceShapes[shape][block] / 2);
	}
}

// ---------- Game Functions ----------

void InitTetris()
{
	int rows = TETRIS_BOARD_ROWS;
	TetrisBoard = (Row*)calloc(sizeof(Row), rows);
	_baseRow = 0;

	std::cout << "Base Row: " << GetBaseRow() << std::endl;
	for (int y = rows - 1; y >= 0; y--)
	{
		std::cout << "Row " << y << ":\t";
		Row *r = GetRow(y);
		for (int x = 0; x < TETRIS_BOARD_ROWS; x++)
		{
			std::cout << (r->cols[x] != None ? "1 " : "0 ");
		}
		std::cout << std::endl;
	}

	_defaultColor = new Color(0, 0, 0);

	addPiece();
}

static void DrawTetris(RGBMatrix *matrix)
{
	FrameCanvas *canvas = matrix->CreateFrameCanvas();

	UpdateDefaultColor();

	for (int x = 0; x < canvas->width(); x++)
	{
		for (int y = 0; y < canvas->height(); y++)
		{
			if (interrupt_received)
				return;

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
					// Draw board background or piece block
					canvas->SetPixel(x, y, 0, 0, 0);
					for (int block = 0; block < PIECE_SIZE; block++)
					{
						if (currentPiece[block].x == col && currentPiece[block].y == row)
						{
							int bX = (x - BOARD_X_OFFSET) % BLOCK_SIZE;
							int bY = (canvas->height() -y - BOARD_Y_OFFSET) % BLOCK_SIZE;
							if (bX == 0 || bY == 0 || bX == BLOCK_SIZE - 1 || bY == BLOCK_SIZE - 1)
							{
								// Draw piece block border
								canvas->SetPixel(x, y, 255, 25, 25);
							}
							else 
							{
								// Draw piece block
								canvas->SetPixel(x, y, 100, 100, 100);
							}
							break;
						}
					}
				}
				else
				{
					// Draw board block
					int bX = (x - BOARD_X_OFFSET) % BLOCK_SIZE;
					int bY = (canvas->height() -y - BOARD_Y_OFFSET) % BLOCK_SIZE;
					if (bX == 0 || bY == 0 || bX == BLOCK_SIZE - 1 || bY == BLOCK_SIZE - 1)
					{
						// Draw block border
						Color *c = new Color(255, 255, 255);
						switch(bStatus)
						{
							case Clearing:
								break;
							case Default:
								// TODO not working
								c = _defaultColor;
								break;
							case Blue:
								c->r = 38;
								c->g = 48;
								c->b = 195;
								break;
							case Pink:
								c->r = 210;
								c->g = 42;
								c->b = 171;
								break;
							default:
							case None:
								break;
						}
						canvas->SetPixel(x, y, c->r, c->g, c->b);
					}
					else
					{
						// Draw block
						if (bStatus == Clearing)
						{
							// TODO set fade
							canvas->SetPixel(x, y, 255, 255, 255);
						}
						else 
						{
							canvas->SetPixel(x, y, 20, 20, 20);
						}
					}
				}
			}
		}
	}
	matrix->SwapOnVSync(canvas, 2U);
}

void PlayTetris()
{	
	// Check if in clearing stage
	if(!_isLineClearing)
	{
		// Handle move
		for (int block = 0; block < PIECE_SIZE; block++)
		{
			// Save current piece
			savedPiece[block] = currentPiece[block];

			// TODO handle move
			// currentPiece[block].x += xMovement
			// currentPiece[block].y += yMovement
		}
		checkCurrentPiecePos();

		// Handle rotate
		if (_rotate)
		{
			PiecePos rotationPos = currentPiece[1];
			for (int block = 0; block < PIECE_SIZE; block++)
			{
				// Save current piece
				savedPiece[block] = currentPiece[block];

				// Transpose y distance from rotationPos to x axis
				int x = savedPiece[block].y - rotationPos.y;
				currentPiece[block].x = rotationPos.x - x;

				// Transpose x distance from rotationPos to y axis
				int y = savedPiece[block].x - rotationPos.x;
				currentPiece[block].y = rotationPos.y + y;
			}
			checkCurrentPiecePos();
			_rotate = false;
		}

		if (_linesToClear != 0)
		{
			// Finished clearing stage
			IncreaseBaseRow();
		}
		else 
		{
			if (_gravityTimer >= GRAVITY_TIMER)
			{	
				// Handle piece gravity
				for (int block = 0; block < PIECE_SIZE; block++)
				{
					// Save current piece
					savedPiece[block] = currentPiece[block];

					currentPiece[block].y -= 1;
				}
				if(!checkPiecePos(currentPiece))
				{	
					// Piece is at bottom
					for (int block = 0; block < PIECE_SIZE; block++)
					{
						GetRow(savedPiece[block].y)->cols[savedPiece[block].x] = _currentPieceStatus;
					}

					addPiece();
				}

				_gravityTimer = 0;
			}
			else
			{
				_gravityTimer++;
			}
			

			// Check if lines need to be cleared
			int linesToClear = 0;
			for (bool isClear = true; linesToClear < TETRIS_BOARD_ROWS && isClear;)
			{
				Row *r = GetRow(linesToClear);
				for (int c = 0; c < TETRIS_BOARD_COLS; c++)
				{
					if (r->cols[c] == None)
					{
						isClear = false;
						break;
					}
				}
				r++;
			}

			if (linesToClear > 0)
			{
				// Enter clearing stage
				ClearLines(linesToClear);
			}
		}
	}
	else if (_clearTimer >= LINE_CLEAR_TIMER)
	{
		// Mark the end of the line clearing stage
		_isLineClearing = 0;
	}
	else
	{
		// Line clearing stage
		_clearTimer++;
	}
}

void CleanupTetris()
{
	free(TetrisBoard);
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

	rtOptions.daemon = 0; 			// Set to 1 for production
	rtOptions.do_gpio_init = true; 	// Set to false for debugging on pc

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
	while (!interrupt_received)
	{
		PlayTetris();
		DrawTetris(matrix);
	}

	CleanupTetris();
	delete matrix;

	return 0;
}
