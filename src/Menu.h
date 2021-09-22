#ifndef _menu
#define _menu

#include "Inputs.h"

#include "led-matrix.h"

#define FONT_FILE "./8bit.bdf"
#define MENU_OPTIONS_COUNT 3

enum MenuOptions
{
    TetrisMenuOption,
    PokemonMenuOption,
    RestartMenuOption
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
    public:
        Menu();
        ~Menu();

        void Reset();
        int Loop(RGBMatrix *canvas, volatile bool *inputs);
};
