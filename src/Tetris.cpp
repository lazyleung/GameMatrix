#include "Tetris.h"

#include "led-matrix.h"
#include "graphics.h"

#include <iostream>

using namespace rgb_matrix;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

// ---------- Helpers ----------

uint8_t Tetris::scale_col(int val, int lo, int hi) {
    if (val <= lo) 
    {
        return 0;
        //val += hi;
    }
    if (val >= hi)
    {
        return 255;
        //val += lo - hi;
    }
    return 255 * (val - lo) / (hi - lo);
}

Color * Tetris::getDefaultColor(int x, int y, Canvas *c)
{
    int shift = _defaultColorShift % c->width();
    return new Color(
        scale_col(x + shift, 0, c->width()),
        255 - scale_col(y - shift, 0, c->height()),
        scale_col(y - shift, 0, c->height())
    );
}

void Tetris::UpdateDefaultColorShift()
{
    _defaultColorShift++;
}

// Copy board line from src row to dest row
void Tetris::copyLine(int src, int dest)
{
    for (int i = 0; i < TETRIS_BOARD_COLS; i++)
    {
        tetrisBoard[dest].cols[i] = tetrisBoard[src].cols[i];
    }
    tetrisBoard[dest].toClear = false;
}

// Clear lines that are marked
void Tetris::clearLines ()
{
    int lowestValidRow = 0;
    for (int i = 0; i < TETRIS_BOARD_ROWS; i++)
    {
        if (!tetrisBoard[i].toClear)
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
            tetrisBoard[lowestValidRow].cols[i] = None;
            tetrisBoard[lowestValidRow].toClear = false;
        }
    }
}

// Check that all blocks of piece are in valid locations
bool Tetris::checkPiecePos(PiecePos *piece)
{
    for (int block = 0; block < PIECE_SIZE; block++)
    {
        // Within board
        if (piece[block].x < 0 || piece[block].x >= TETRIS_BOARD_COLS ||
            piece[block].y < 0 || piece[block].y >= TETRIS_BOARD_ROWS + TETRIS_BOARD_ROWS_HIDDEN)
        {
            return false;
        }
        // Not on board block
        else if (tetrisBoard[piece[block].y].cols[piece[block].x] != None)
        {
            return false;
        }
    }
    return true;
}

// Reset next piece to current if pos invalid
void Tetris::checkCurrentPiecePos()
{
    if(!checkPiecePos(currentPiece))
    {
        for (int block = 0; block < PIECE_SIZE; block++)
        {
            currentPiece[block] = savedPiece[block];
        }
    }
}

// Add next piece to board
void Tetris::addPiece()
{
    // Insert base piece

    if (pieceBag == 0xFF)
    {
        pieceBag = 0x80;
    }

    int shape = 7;
    bool isPieceSelected = false;
    while (!isPieceSelected)
    {
        std::cout << "Shape " << shape;
        if (shape >= 7 && (pieceBag & (1 << shape)))
        {
            // Piece already in bag
            shape = rand() % 7;
        }
        else
        {
            // Piece valid!
            isPieceSelected = true;
            pieceBag = pieceBag | (1 << shape);
        }
    }

    std::cout << "PieceBag: " << std::hex << (0xFF & pieceBag) << std::endl;

    // TODO add random color status
    _currentPieceStatus = Default;
    for (int block = 0; block < PIECE_SIZE; block++)
    {
        currentPiece[block].y = TETRIS_BOARD_ROWS - (pieceShapes[shape][block] % 2 == 0 ?  1 : 0);
        currentPiece[block].x = TETRIS_BOARD_COLS/2 - 2 + (pieceShapes[shape][block] / 2);
    }
}

void Tetris::clearPieceBag()
{
    pieceBag = 0x80;
}

// ---------- Constructors and Destructors ----------

void Tetris::InitTetris()
{
	tetrisBoard = (Row*)calloc(sizeof(Row), TETRIS_BOARD_ROWS);

	int rows = TETRIS_BOARD_ROWS;
	for (int y = rows - 1; y >= 0; y--)
	{
		std::cout << "Row " << y << ":\t";
		for (int x = 0; x < TETRIS_BOARD_ROWS; x++)
		{
			std::cout << (tetrisBoard[y].cols[x] != None ? "1 " : "0 ");
		}
		std::cout << std::endl;
	}

	_tState = Normal;
	_defaultColorShift = 0;
	_gravityCount = 0;
	_bottomCountTarget = 0;
	_clearCount = 0;

    clearPieceBag();
	addPiece();
}

void Tetris::CleanupTetris()
{
    free(tetrisBoard);
    tetrisBoard = NULL;
}

Tetris::Tetris(/* args */)
{
    InitTetris();
}

Tetris::~Tetris()
{
    CleanupTetris();
}

// ---------- Game Functions ----------

void Tetris::DrawTetris(RGBMatrix *matrix)
{
	FrameCanvas *canvas = matrix->CreateFrameCanvas();

    UpdateDefaultColorShift();

	for (int x = 0; x < canvas->width(); x++)
	{
		for (int y = 0; y < canvas->height(); y++)
		{
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

				if (tetrisBoard[row].cols[col] == None)
				{
					// Draw board background or piece block
					canvas->SetPixel(x, y, 0, 0, 0);
					for (int block = 0; block < PIECE_SIZE; block++)
					{
						if (_tState != ClearAnimation && currentPiece[block].x == col && currentPiece[block].y == row)
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
				else if (tetrisBoard[row].toClear)
				{
					// Draw clear line animation
					uint shift = _clearCount * 3;
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
						switch(tetrisBoard[row].cols[col])
						{
							case Default:
							{
								c = getDefaultColor(x, y, canvas);
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

void Tetris::PlayTetris(volatile char inputC)
{	
	switch (_tState)
	{
		case Normal:
		{
			int xShift = 0;
			int yshift = 0;
			_rotateState = NoRotate;

			// Capture Input
			if (inputC != 0x00)
			{
				switch (inputC)
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
					//case 0x00:           // Other issue from getch()
					{
						// TODO Sent signal to end program
                        // _running = false;
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

					// TODO Rotate not blocked by walls and height
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

			if (_gravityCount++ % GRAVITY_UPDATE_TARGET == 0)
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
					if(_gravityCount >= _bottomCountTarget)
					{
						// Piece is at bottom
						for (int block = 0; block < PIECE_SIZE; block++)
						{
							tetrisBoard[savedPiece[block].y].cols[savedPiece[block].x] = _currentPieceStatus;
						}

						addPiece();
					}
					else
					{
						_bottomCountTarget = _gravityCount + _bottomCountTarget;

						// Reset piece
						for (int block = 0; block < PIECE_SIZE; block++)
						{
							currentPiece[block] = savedPiece[block];
						}
					}
				}

				// Check if lines need to be cleared
				for (int r = 0; r < TETRIS_BOARD_ROWS; r++)
				{
					for (int c = 0; c < TETRIS_BOARD_COLS; c++)
					{
						if (tetrisBoard[r].cols[c] == None)
						{
							// Gap found in line
							break;
						}
						if (c == TETRIS_BOARD_COLS - 1)
						{
							// Line marked to be cleared
							_tState = ClearAnimation;
							tetrisBoard[r].toClear = true;
						}
					}
				}
			}
			break;
		}
		case ClearAnimation:
		{
			if (_clearCount++ >= LINE_CLEAR_TARGET)
			{
				// Mark the end of the line clearing stage
				_clearCount = 0;
				_tState = Clearing;
			}
			break;
		}
		case Clearing:
		{
			clearLines();
			_tState = Normal;
			break;
		}
	}
}