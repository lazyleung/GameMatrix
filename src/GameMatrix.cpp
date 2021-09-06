#include "led-matrix.h"

#include "pixel-mapper.h"
#include "graphics.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <termios.h>

using namespace rgb_matrix;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

// Tetris width always 10 wide
#define TETRIS_BOARD_COLS 10
#define TETRIS_BOARD_ROWS 12
// How many pixels per Tetris block
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

static bool _running;

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
	Default,
	Blue,
	Pink
};

struct Row {
	BlockStatus cols[TETRIS_BOARD_ROWS];
	bool 		toClear;
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
static Color * _defaultColor;

enum tetrisState 
{
	Normal,
	ClearAnimation,
	Clearing
};
static tetrisState _tState;

enum rotateState
{
	NoRotate,
	Clockwise,
	CounterClockwise
};
static rotateState _rotateState;

static int _clearTimer;
static int _gravityTimer;

// ---------- Helpers ----------

static void UpdateDefaultColor()
{
	_defaultColor->g++;
}

static void copyLine(int src, int dest)
{
	for (int i = 0; i < TETRIS_BOARD_COLS; i++)
	{
		TetrisBoard[dest].cols[i] = TetrisBoard[src].cols[i];
	}
	TetrisBoard[dest].toClear = false;
}

static void ClearLines ()
{
	int lowestValidRow = 0;
	for (int i = 0; i < TETRIS_BOARD_ROWS; i++)
	{
		if (!TetrisBoard[i].toClear)
		{
			// Move valid row to lowest row
			if (lowestValidRow < i)
			{
				copyLine(i, lowestValidRow);
			}
			lowestValidRow++;
		}
	}

	// Zero out rest of rows
	for (;lowestValidRow < TETRIS_BOARD_ROWS; lowestValidRow++)
	{
		for (int i = 0; i < TETRIS_BOARD_COLS; i++)
		{
			TetrisBoard[lowestValidRow].cols[i] = None;
			TetrisBoard[lowestValidRow].toClear = false;
		}
	}
}

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
		else if (TetrisBoard[piece[block].y].cols[piece[block].x] != None)
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

// Check if there is any input on the unbuffered terminal
bool inputAvailable()
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return (FD_ISSET(0, &fds));
}

// Get single char
static char getch() 
{
	char buf = 0;
	if (read(STDIN_FILENO, &buf, 1) < 0)
		perror ("read()");

	return buf;
}

// ---------- Game Functions ----------

void InitTetris()
{
	TetrisBoard = (Row*)calloc(sizeof(Row), TETRIS_BOARD_ROWS);

	int rows = TETRIS_BOARD_ROWS;
	for (int y = rows - 1; y >= 0; y--)
	{
		std::cout << "Row " << y << ":\t";
		for (int x = 0; x < TETRIS_BOARD_ROWS; x++)
		{
			std::cout << (TetrisBoard[y].cols[x] != None ? "1 " : "0 ");
		}
		std::cout << std::endl;
	}

	_tState = Normal;
	_defaultColor = new Color(0, 0, 0);
	_gravityTimer = 0;
	_clearTimer = 0;

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
				(y > canvas->height() - BOARD_Y_OFFSET - 1 || y < canvas->height() - BOARD_Y_OFFSET - 1 - (BLOCK_SIZE * TETRIS_BOARD_ROWS)))
			{
				// Draw border background
				canvas->SetPixel(x, y, 108, 64, 173);
			}
			else
			{
				// Draw tetris board
				int col = (x - BOARD_X_OFFSET) / BLOCK_SIZE;
				int row = (canvas->height() - y - BOARD_Y_OFFSET - 1) / BLOCK_SIZE;

				if (TetrisBoard[row].cols[col] == None)
				{
					// Draw board background or piece block
					canvas->SetPixel(x, y, 0, 0, 0);
					for (int block = 0; block < PIECE_SIZE; block++)
					{
						if (currentPiece[block].x == col && currentPiece[block].y == row)
						{
							int bX = (x - BOARD_X_OFFSET) % BLOCK_SIZE;
							int bY = (canvas->height() -y - BOARD_Y_OFFSET - 1) % BLOCK_SIZE;
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
				else if (TetrisBoard[row].toClear)
				{
					// Draw clear line animation
					uint shift = _clearTimer * 4;
					if (shift > 255)
					{
						shift = 0;
					}
					canvas->SetPixel(x, y, 255 - shift, 255 - shift, 255 - shift);
				}
				else
				{
					// Draw board block
					int bX = (x - BOARD_X_OFFSET) % BLOCK_SIZE;
					int bY = (canvas->height() -y - BOARD_Y_OFFSET - 1) % BLOCK_SIZE;
					
					if (bX == 0 || bY == 0 || bX == BLOCK_SIZE - 1 || bY == BLOCK_SIZE - 1)
					{
						// Draw block border
						Color *c = new Color(255, 255, 255);
						switch(TetrisBoard[row].cols[col])
						{
							case Default:
							{
								c = _defaultColor;
								break;
							}
							case Blue:
							{
								c->r = 38;
								c->g = 48;
								c->b = 195;
								break;
							}
							case Pink:
							{
								c->r = 210;
								c->g = 42;
								c->b = 171;
								break;
							}
							default:
							case None:
							{
								break;
							}
						}
						canvas->SetPixel(x, y, c->r, c->g, c->b);
					}
					else
					{
						canvas->SetPixel(x, y, 20, 20, 20);
					}
				}
			}
		}
	}
	matrix->SwapOnVSync(canvas, 2U);
}

void PlayTetris()
{	
	switch (_tState)
	{
		case Normal:
		{
			int xShift = 0;
			int yshift = 0;
			_rotateState = NoRotate;

			// Capture Input
			if (inputAvailable())
			{
				const char c = tolower(getch());
				switch (c)
				{
					case 'w': // Up
					case 'k':
					{
						// TODO drop
						break;
					}
					case 's': // Down
					case 'j':
					{
						yshift--;
						break;
					}
					case 'a': // Left
					case 'h':
					{
						xShift--;
						break;
					}
					case 'd': // Right
					case 'l':
					{
						xShift++;
						break;
					}
					case 'q': // Rotate
					{
						_rotateState = CounterClockwise;
						break;
					}
					case 'e': // Rotate
					{
						_rotateState = Clockwise;
						break;
					}
					// Exit program
					case 0x1B:           // Escape
					case 0x04:           // End of file
					case 0x00:           // Other issue from getch()
					{
						_running = false;
						break;
					}
				}
			}

			// Handle move
			for (int block = 0; block < PIECE_SIZE; block++)
			{
				// Save current piece
				savedPiece[block] = currentPiece[block];

				currentPiece[block].x += xShift;
				currentPiece[block].y += yshift;
			}
			checkCurrentPiecePos();

			// Handle rotate
			if (_rotateState != NoRotate)
			{
				PiecePos rotationPos = currentPiece[1];
				for (int block = 0; block < PIECE_SIZE; block++)
				{
					// Save current piece
					savedPiece[block] = currentPiece[block];

					int x = savedPiece[block].y - rotationPos.y;
					int y = savedPiece[block].x - rotationPos.x;

					switch (_rotateState)
					{
						case Clockwise:
						{
							// Transpose y distance from rotationPos to x axis
							currentPiece[block].x = rotationPos.x + x;

							// Transpose x distance from rotationPos to -y axis
							currentPiece[block].y = rotationPos.y - y;
							break;
						}
						case CounterClockwise:
						{
							// Transpose y distance from rotationPos to -x axis
							currentPiece[block].x = rotationPos.x - x;

							// Transpose x distance from rotationPos to y axis
							currentPiece[block].y = rotationPos.y + y;
							break;
						}
						case NoRotate:
						{
							break;
						}
					}
				}
				checkCurrentPiecePos();
				_rotateState = NoRotate;
			}

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
						TetrisBoard[savedPiece[block].y].cols[savedPiece[block].x] = _currentPieceStatus;
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
			for (int r = 0; r < TETRIS_BOARD_ROWS; r++)
			{
				for (int c = 0; c < TETRIS_BOARD_COLS; c++)
				{
					if (TetrisBoard[r].cols[c] == None)
					{
						// Gap found in line
						break;
					}
					if (c == TETRIS_BOARD_COLS - 1)
					{
						// Line marked to be cleared
						_tState = ClearAnimation;
						TetrisBoard[r].toClear = true;
					}
				}
			}
			break;
		}
		case ClearAnimation:
		{
			if (_clearTimer >= LINE_CLEAR_TIMER)
			{
				// Mark the end of the line clearing stage
				_clearTimer = 0;
				_tState = Clearing;
			}
			else
			{
				// Line clearing stage
				_clearTimer++;
			}
			break;
		}
		case Clearing:
		{
			ClearLines();
			_tState = Normal;
			break;
		}
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
	defaults.show_refresh_rate = false;

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

	_running = true;

	// StartUp Animation
	DrawOnCanvas(matrix);

	// Set terminal to unbuffered mode
	struct termios old;
	bool is_terminal = isatty(STDIN_FILENO);
	if (is_terminal) {
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

	// Tetris Engine
	while (!interrupt_received && _running)
	{
		PlayTetris();
		DrawTetris(matrix);
	}

	// Restore terminal attributes
	if (is_terminal) {
		if (tcsetattr(0, TCSADRAIN, &old) < 0)
		perror ("tcsetattr ~ICANON");
	}

	CleanupTetris();
	delete matrix;

	return 0;
}
