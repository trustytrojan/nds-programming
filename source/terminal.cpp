#include "everything.h"

void createTerminal(void)
{
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);
	static PrintConsole console;
	consoleInit(&console, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	keyboardDemoInit()->OnKeyPressed = [](const auto key)
	{ if (key > 0) std::cout << (char)key; };
	keyboardShow();
	swiWaitForVBlank();
}
