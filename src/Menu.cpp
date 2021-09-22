#include "Menu.h"
#include "Inputs.h"

#include "led-matrix.h"
#include "graphics.h"

using namespace rgb_matrix;

const int x_orig = 11;
const int y_orig = 10;
const int y_scale = 12;
const int x_marker_shift = 5;
const int y_marker_shift = 3;
const int marker_radius = 1;
const int letter_spacing = 0;

void Menu::upOption()
{
    selectedOption = (selectedOption == TetrisMenuOption) ? RestartMenuOption : static_cast<MenuOptions>(static_cast<int>(selectedOption)-1);
}

void Menu::downOption()
{
    selectedOption = (selectedOption == RestartMenuOption) ? TetrisMenuOption : static_cast<MenuOptions>(static_cast<int>(selectedOption)+1);
}

Menu::Menu()
{
    Reset();
}

Menu::~Menu()
{

}

void Menu::Reset()
{
    selectedOption = TetrisMenuOption;

    // Prevent an option getting immediately returned
    for (int i = 0; i < TOTAL_INPUTS; i++)
    {
        prevInputs[i] = true;
    }
}

int Menu::Loop(RGBMatrix *matrix, volatile bool *inputs)
{
    // Proccess inputs on button down
    if (inputs[UpStick] && !prevInputs[UpStick])
    {
        upOption();
    }
    
    if (inputs[DownStick] && !prevInputs[DownStick])
    {
        downOption();
    }

    if (inputs[AButton] && !prevInputs[AButton])
    {
        return selectedOption;
    }

    for (int i = 0; i < TOTAL_INPUTS; i++)
    {
        prevInputs[i] = inputs[i];
        inputs[i] = false;
    }

    // Draw Menu
    Color color(255, 255, 0);
    Color bg_color(0, 0, 0);
    Color flood_color(0, 0, 0);
    
    // Load font
    rgb_matrix::Font font;
    if (!font.LoadFont(FONT_FILE)) {
        fprintf(stderr, "Couldn't load font '%s'\n", FONT_FILE);
        return -1;
    }

    // Clean background
    matrix->Fill(flood_color.r, flood_color.g, flood_color.b);

    // Draw Text
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

    rgb_matrix::DrawCircle(matrix, x_orig - x_marker_shift, y_orig - y_marker_shift + selectedOption*y_scale, marker_radius, color);

    return -1;
}
