#ifndef _inputs
#define _inputs

#define GPIO_OFFSET 64

#define JOYSTICK_INPUTS 4
#define BUTTON_INPUTS 3
#define TOTAL_INPUTS JOYSTICK_INPUTS + BUTTON_INPUTS

enum inputsMap
{
	LeftStick,
	RightStick,
	UpStick,
	DownStick,
	AButton,
	BButton,
	MenuButton
};

#endif
