#ifndef _menu
#define _menu

#include "Inputs.h"

#include "led-matrix.h"

#define FONT_FILE_8BIT "/usr/font/8bit.bdf"
#define FONT_FILE_CLOCK "/usr/font/9x18.bdf"
#define MENU_OPTIONS_COUNT 3

enum MenuOptions
{
    TetrisMenuOption,
    //AnimationMenuOption,
    ClockMenuOption,
    RotateMenuOption
};

#endif

using namespace rgb_matrix;

class Menu
{
    private:
        int selectedOption;
        bool prevInputs[TOTAL_INPUTS];
        void upOption();
        void downOption();
        bool isShowSeconds;
        int clockXShift;
        int clockYShift;
    public:
        Menu();
        ~Menu();

        void Reset();
        int Loop(RGBMatrix *matrix, volatile bool *inputs);
        int ClockLoop(RGBMatrix *matrix, volatile bool *inputs);
        int TestLoop(RGBMatrix *matrix, volatile bool *inputs, const char* text);
};
