#include "Tetris.h"

#include <iostream>
#include<thread>

using namespace rgb_matrix;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

// ---------- Helpers ----------

uint8_t Tetris::scale_col(int val, int lo, int hi) {
    if (val <= lo) 
    {
        return 0;
        // val += hi;
    }
    if (val >= hi)
    {
         return 255;
        // val += lo - hi;
    }
    return 255 * (val - lo) / (hi - lo);
}

Color * Tetris::getDefaultColor(int x, int y, Canvas *c)
{
    int shift = defaultColorShift % c->width();
    return new Color(
        scale_col(x + shift, 0, c->width()),
        255 - scale_col(y - shift, 0, c->height()),
        scale_col(y - shift, 0, c->height())
    );
}

void Tetris::UpdateDefaultColorShift()
{
    if (isShiftInc)
    {
        if (defaultColorShift < 255)
        {
            defaultColorShift++;
        }
        else
        {
            isShiftInc = false;
        }
    }
    else
    {
        if (defaultColorShift > 0)
        {
            defaultColorShift--;
        }
        else
        {
            isShiftInc = true;
        }
    }
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
        // Check if within board
        if (piece[block].x < 0 || piece[block].x >= TETRIS_BOARD_COLS ||
            piece[block].y < 0 || piece[block].y >= TETRIS_BOARD_ROWS + TETRIS_BOARD_ROWS_HIDDEN)
        {
            return false;
        }
        // Not on board block
        else if (piece[block].y < TETRIS_BOARD_ROWS && tetrisBoard[piece[block].y].cols[piece[block].x] != None)
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

void Tetris::getNextShape()
{
    // Insert base piece
    if (pieceBag == 0xFF)
    {
        std::cout << "PieceBag Full!" << std::endl;
        pieceBag = 0x80;
    }
    int shape = 7;
    bool isPieceSelected = false;
    while (!isPieceSelected)
    {
        if (shape >= 7 || (pieceBag & (0x01 << shape)))
        {
            // Piece already in bag
            shape = rand() % 7;
        }
        else
        {
            // Piece valid!
            isPieceSelected = true;
            pieceBag = pieceBag | (0x01 << shape);
            std::cout << "Shape Added: " << shape << std::endl;
        }
    }
    nextShape = shape;
    return;
}

// Add next piece to board
void Tetris::addPiece()
{
    // TODO add random color status
    currentPieceStatus = Default;
    for (int block = 0; block < PIECE_SIZE; block++)
    {
        currentPiece[block].y = TETRIS_BOARD_ROWS - (pieceShapes[nextShape][block] % 2 == 0 ?  1 : 0);
        currentPiece[block].x = TETRIS_BOARD_COLS/2 - 2 + (pieceShapes[nextShape][block] / 2);
    }
    std::cout << "Current piece added!" << std::endl;

    std::thread thread_object(&Tetris::getNextShape, this);
    thread_object.detach();
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

    for (int i = 0; i < TOTAL_INPUTS; i++)
    {
        inputDelayCounts[i] = 0;
        prevInputs[i] = false;
    }

    tState = Normal;
    defaultColorShift = 0;
    gravityCount = 0;
    bottomCountTarget = 0;
    clearCount = 0;

    srand(time(NULL));
    clearPieceBag();
    getNextShape();
    addPiece();
}

void Tetris::CleanupTetris()
{
    if (tetrisBoard != NULL)
    {
        free(tetrisBoard);
        tetrisBoard = NULL;
    }
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
            if ((x < BOARD_X_OFFSET || x > BOARD_X_OFFSET - 1 + (BLOCK_SIZE * TETRIS_BOARD_COLS)) ||
                (y > canvas->height() - BOARD_Y_OFFSET - 1 || y < canvas->height() - BOARD_Y_OFFSET - 1 - (BLOCK_SIZE * TETRIS_BOARD_ROWS)))
            {
                // Draw border background
                canvas->SetPixel(x, y, 108, 64, 173);

                // TODO Draw preview piece in border
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
                        // Don't draw piece if clearing
                        if (tState != ClearAnimation && currentPiece[block].x == col && currentPiece[block].y == row)
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
                    uint shift = clearCount * 3;
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

void Tetris::PlayTetris(volatile bool *inputs)
{    
    switch (tState)
    {
        case Normal:
        {
            int xShift = 0;
            int yshift = 0;
            rotateState = NoRotate;

            // For block movement
            // Immediately react on button down
            // Set a delay before input is recognized again
            // This delay is shorter if button is being held
            if (inputDelayCounts[Left] == 0 && inputs[Left] && !inputs[Up] && !inputs[Down])
            {
                if (!prevInputs[Left])
                {
                    inputDelayCounts[Left] = INPUT_DELAY_TARGET;
                }
                else
                {
                    inputDelayCounts[Left] = INPUT_DELAY_TARGET/2;
                }
                
                xShift--;
            }

            if (inputDelayCounts[Right] == 0 && inputs[Right] && !inputs[Up] && !inputs[Down])
            {
                if (!prevInputs[Right])
                {
                    inputDelayCounts[Right] = INPUT_DELAY_TARGET;
                }
                else
                {
                    inputDelayCounts[Right] = INPUT_DELAY_TARGET/2;
                }
                xShift++;
            }

            if (inputDelayCounts[Down] == 0 && inputs[Down] && !inputs[Left] && !inputs[Right])
            {
                if (!prevInputs[Down])
                {
                    inputDelayCounts[Down] = INPUT_DELAY_TARGET;
                }
                else
                {
                    inputDelayCounts[Down] = INPUT_DELAY_TARGET/2;
                }
                yshift--;
            }

            // Only on button down
            if (!prevInputs[A] && inputs[A])
            {
                rotateState = CounterClockwise;
            }

            for (int i = 0; i < TOTAL_INPUTS; i++)
            {
                prevInputs[i] = inputs[i];
                inputs[i] = false;
                if (inputDelayCounts[i] > 0)
                {
                    inputDelayCounts[i]--;
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
            if (rotateState != NoRotate)
            {
                PiecePos rotationPos = currentPiece[1];
                for (int block = 0; block < PIECE_SIZE; block++)
                {
                    // Save current piece
                    savedPiece[block] = currentPiece[block];

                    int x = savedPiece[block].y - rotationPos.y;
                    int y = savedPiece[block].x - rotationPos.x;

                    // TODO Rotate not blocked by walls and height
                    switch (rotateState)
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
                rotateState = NoRotate;
            }

            // Update gravity and check the board at set intervals
            if (gravityCount++ % GRAVITY_UPDATE_TARGET == 0)
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
                    // Piece has hit a block
                    if(bottomCountTarget >= GRAVITY_BOTTOM_TARGET)
                    {
                        // Save piece location
                        for (int block = 0; block < PIECE_SIZE; block++)
                        {
                            tetrisBoard[savedPiece[block].y].cols[savedPiece[block].x] = currentPieceStatus;
                        }

                        addPiece();

                        // Add input delay
                        for (int i = 0; i < TOTAL_INPUTS; i++)
                        {
                            inputDelayCounts[i] = INPUT_DELAY_TARGET * 2;
                        } 
                    }
                    else
                    {
                        // Delay before piece get's saved to block
                        bottomCountTarget++;

                        // Reset piece
                        for (int block = 0; block < PIECE_SIZE; block++)
                        {
                            currentPiece[block] = savedPiece[block];
                        }
                    }
                }

                // Check if board is full
                if (tetrisBoard[TETRIS_BOARD_ROWS - 1].cols[TETRIS_BOARD_COLS/2] != None)
                {
                    // Clear all lines!
                    for (int r = 0; r < TETRIS_BOARD_ROWS; r++)
                    {
                        tetrisBoard[r].toClear = true;
                    }
                    
                    tState = ClearAnimation;
                    break;
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
                            tState = ClearAnimation;
                            tetrisBoard[r].toClear = true;
                        }
                    }
                }
            }
            break;
        }
        case ClearAnimation:
        {
            if (clearCount++ >= LINE_CLEAR_TARGET)
            {
                // Mark the end of the line clearing stage
                clearCount = 0;
                tState = Clearing;
            }
            break;
        }
        case Clearing:
        {
            clearLines();
            tState = Normal;
            break;
        }
    }
}