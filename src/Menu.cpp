#include "Menu.h"
#include "Inputs.h"

#include "led-matrix.h"
#include "graphics.h"

using namespace rgb_matrix;

int Menu::selectedOption = TetrisMenuOption;

bool Menu::prevInputs[TOTAL_INPUTS];

void Menu::upOption()
{
    selectedOption = (selectedOption == TetrisMenuOption) ? RestartMenuOption : static_cast<MenuOptions>(static_cast<int>(selectedOption)-1);
}

void Menu::downOption()
{
    selectedOption = (selectedOption == RestartMenuOption) ? TetrisMenuOption : static_cast<MenuOptions>(static_cast<int>(selectedOption)+1);
}

void Menu::Reset()
{
    selectedOption = TetrisMenuOption;
}

int Menu::Loop(RGBMatrix *matrix, volatile bool *inputs)
{
    if (inputs[UpStick] && !prevInputs[UpStick])
    {
        upOption();
        inputs[UpStick] = false;
    }
    
    if (inputs[DownStick] && !prevInputs[DownStick])
    {
        downOption();
        inputs[DownStick] = false;
    }

    if (inputs[AButton] && !prevInputs[AButton])
    {
        return selectedOption;
    }

    for (int i = 0; i < TOTAL_INPUTS; i++)
    {
        prevInputs[i] = inputs[i];
    }
    
    Color color(255, 255, 0);
    Color bg_color(0, 0, 0);
    Color flood_color(0, 0, 0);
    Color outline_color(0,0,0);

    const int x_orig = 10;
    const int y_orig = 10;
    const int y_scale = 10;
    const int letter_spacing = 0;
    
    // Load font
    rgb_matrix::Font font;
    if (!font.LoadFont(FONT_FILE)) {
        fprintf(stderr, "Couldn't load font '%s'\n", FONT_FILE);
        return -1;
    }

    matrix->Fill(flood_color.r, flood_color.g, flood_color.b);
    for (int i = 0; i < MENU_OPTIONS_COUNT; i++)
    {
        const char* text;
        switch (i)
        {
            case TetrisMenuOption:
                text = "Tetris";
                break;
            case PokemonMenuOption:
                text = "Pokemon";
                break;
            case RestartMenuOption:
                text = "Restart";
                break;
            
            default:
                break;
        }
        rgb_matrix::DrawText(matrix, font, x_orig, y_orig + i * y_scale, color, &bg_color, text, letter_spacing);
    }

    rgb_matrix::DrawCircle(matrix, x_orig - x_orig/2, y_orig - y_scale/4 + selectedOption*y_scale, 2, color);

    return -1;
}
