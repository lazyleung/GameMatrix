#include "Menu.h"
#include "Inputs.h"

#include "led-matrix.h"
#include "graphics.h"

using namespace rgb_matrix;

int Menu::selectedOption;

void Menu::upOption()
{
    selectedOption = (selectedOption == TetrisMenuOption) ? RestartMenuOption : static_cast<MenuOptions>(static_cast<int>(selectedOption)-1);
}

void Menu::downOption()
{
    selectedOption = (selectedOption == TetrisMenuOption) ? RestartMenuOption : static_cast<MenuOptions>(static_cast<int>(selectedOption)+1);
}

void Menu::Loop(RGBMatrix *matrix, volatile bool *inputs)
{
    

    if (inputs[UpStick])
    {
        upOption();
        inputs[UpStick] = false;
    }
    
    if (inputs[DownStick])
    {
        downOption();
        inputs[DownStick] = false;
    }
    
    Color color(255, 255, 0);
    Color bg_color(0, 0, 0);
    Color flood_color(0, 0, 0);
    Color outline_color(0,0,0);

    int x_orig = 10;
    int y_orig = 10;
    int letter_spacing = 0;
    
    // Load font
    rgb_matrix::Font font;
    if (!font.LoadFont(FONT_FILE)) {
        fprintf(stderr, "Couldn't load font '%s'\n", FONT_FILE);
        return;
    }

    //int x_scale = 20;
    int y_scale = 15;

    matrix->Clear();
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

        if (selectedOption == i)
        {
            rgb_matrix::DrawCircle(matrix, x_orig - 3, y_orig + i * y_scale + 3 , 5, color);
        }
    }

    return;
}
