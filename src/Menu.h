#ifndef _menu
#define _menu

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
        static int selectedOption;
        static bool isOptionsDrawn;
        static void upOption();
        static void downOption();
    public:
        static void Loop(RGBMatrix *canvas, volatile bool *inputs);
};
