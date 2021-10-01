#include "Menu.h"
#include "Inputs.h"

#include "led-matrix.h"
#include "graphics.h"

#include <ctime>

using namespace rgb_matrix;

const int x_orig = 11;
const int y_orig = 10;
const int y_scale = 12;
const int x_marker_shift = 6;
const int y_marker_shift = 3;
const int marker_radius = 2;
const int letter_spacing = 0;

void Menu::upOption()
{
    selectedOption = (selectedOption == TetrisMenuOption) ? RotateMenuOption : static_cast<MenuOptions>(static_cast<int>(selectedOption)-1);
}

void Menu::downOption()
{
    selectedOption = (selectedOption == RotateMenuOption) ? TetrisMenuOption : static_cast<MenuOptions>(static_cast<int>(selectedOption)+1);
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
        for (int i = 0; i < TOTAL_INPUTS; i++)
        {
            prevInputs[i] = inputs[i];
            inputs[i] = false;
        }
        
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
    if (!font.LoadFont(FONT_FILE_8BIT)) {
        fprintf(stderr, "Couldn't load font '%s'\n", FONT_FILE_8BIT);
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
            case AnimationMenuOption:
                text = "Animate";
                break;
            case ClockMenuOption:
                text = "Clock";
                break;
            case RotateMenuOption:
                text = "Rotate";
                break;
            default:
                break;
        }
        rgb_matrix::DrawText(matrix, font, x_orig, y_orig + i * y_scale, color, &bg_color, text, letter_spacing);
    }

    rgb_matrix::DrawCircle(matrix, x_orig - x_marker_shift, y_orig - y_marker_shift + selectedOption*y_scale, marker_radius, color);

    return -1;
}

int Menu::ClockLoop(RGBMatrix *matrix, volatile bool *inputs)
{
    // Proccess inputs on button down
    if (inputs[UpStick] && !prevInputs[UpStick])
    {
        // Change clock styles
    }
    
    if (inputs[AButton] && !prevInputs[AButton])
    {
        for (int i = 0; i < TOTAL_INPUTS; i++)
        {
            prevInputs[i] = inputs[i];
            inputs[i] = false;
        }
        
        return -1;
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
    if (!font.LoadFont(FONT_FILE_CLOCK)) {
        fprintf(stderr, "Couldn't load font '%s'\n", FONT_FILE_CLOCK);
        return -1;
    }

    // Clean background
    matrix->Fill(flood_color.r, flood_color.g, flood_color.b);

    time_t now = time(0);
    //char* dt = ctime(&now);
    tm *ltm = localtime(&now);

    // Draw Text
    char buf[10];
    strftime(buf, 10, "%I:%M:%S", ltm);

    //const char* text = "Time: " + (5 + ltm->tm_hour) + ":" + (30 + ltm->tm_min) + ":" + ltm->tm_sec;
    rgb_matrix::DrawText(matrix, font, 10, 32, color, &bg_color, buf, letter_spacing);

    return 0;
}

int Menu::TestLoop(RGBMatrix *matrix, volatile bool *inputs, const char* text)
{
     // Proccess inputs on button down
    if (inputs[MenuButton] && !prevInputs[MenuButton])
    {
        return -1;
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
    rgb_matrix::DrawText(matrix, font, x_orig, y_orig , color, &bg_color, text, letter_spacing);

    return 0;
}