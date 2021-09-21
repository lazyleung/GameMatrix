#ifndef _tetris
#define _tetris

#include "Inputs.h"

#include "led-matrix.h"
#include "graphics.h"

// Tetris width always 10 wide
#define TETRIS_BOARD_COLS 10
#define TETRIS_BOARD_ROWS 12
#define TETRIS_BOARD_ROWS_HIDDEN 8
// How many pixels per Tetris block
#define BLOCK_SIZE 5
#define PIECE_SIZE 4
#define BOARD_X_OFFSET 7
#define BOARD_Y_OFFSET 4

#define INPUT_DELAY_TARGET 5
#define LINE_CLEAR_TARGET 50
#define GRAVITY_UPDATE_TARGET 60
#define GRAVITY_BOTTOM_TARGET 3

using namespace rgb_matrix;

#endif

// ========== Tetris Stuff ==========
// ---------- Struct & Fields ----------

class Tetris
{
    private:
        enum BlockStatus
        {
            None,
            Default,
            Blue,
            Pink
        };
        BlockStatus currentPieceStatus;

        enum tetrisState 
        {
            Normal,
            ClearAnimation,
            Clearing
        };
        tetrisState tState;

        enum rotateState
        {
            NoRotate,
            Clockwise,
            CounterClockwise
        };
        rotateState rotateState;

        struct Row {
            BlockStatus cols[TETRIS_BOARD_ROWS];
            bool 		toClear;
        };

        struct PiecePos
        {
            int x, y;
        };
        PiecePos currentPiece[PIECE_SIZE];
        PiecePos savedPiece[PIECE_SIZE];

        // Point of rotation is [?][1]
        // Based on following mapping:
        // 1 2 3 4
        //   5 6 7
        const int pieceShapes[7][4] =
        {
            1,3,5,7, // I
            2,4,5,7, // Z
            3,5,4,6, // S
            3,5,4,7, // T
            2,3,5,7, // L
            3,5,7,6, // J
            2,3,4,5, // O
        };

        uint8_t pieceBag;
        int nextShape;
        bool isGettingNextShape;

        int inputDelayCounts[TOTAL_INPUTS];
        bool prevInputs[TOTAL_INPUTS];

        Row * tetrisBoard;
        int defaultColorShift;
        bool isShiftInc;

        int gravityCount;
        int bottomCountTarget;
        int clearCount;

        uint8_t scale_col(int val, int lo, int hi);
        Color * getDefaultColor(int x, int y, Canvas *c);

        void copyLine(int src, int dest);
        void clearLines ();

        bool checkPiecePos(PiecePos *piece);
        void checkCurrentPiecePos();
        void addPiece();
        void clearPieceBag();

    public:
        Tetris(/* args */);
        ~Tetris();

        void InitTetris();
        void CleanupTetris();

        void UpdateDefaultColorShift();

        void DrawTetris(RGBMatrix *matrix);
        void PlayTetris(volatile bool *inputs);
};