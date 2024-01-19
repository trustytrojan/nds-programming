#include "everything.hpp"
#include "Cursor.hpp"

#define FLASH_INTERVAL 20

void StartShell(std::string prompt, const char cursorCharacter, const ShellLineProcessor lineProcessor)
{
	if (!lineProcessor)
	{
		std::cerr << "lineProcessor is null\n";
		return;
	}

	// Create and print the empty-line shell prompt
	std::cout << (prompt += (cursorCharacter + Cursor::moveLeftOne));

	// Variables
	signed char c;
	std::string line;
	unsigned cursorPos = 0;
	bool flashState = false;	  // Flag to track the flashing state
	unsigned char flashTimer = 0; // Timer to control the flashing interval

	while (1)
	{
		switch (c = keyboardUpdate())
		{
		// no keys pressed, or ignored keys
		case 0:
		case -1:
		case DVK_CTRL:
		case DVK_ALT:
		case DVK_CAPS:
		case DVK_DOWN:
		case DVK_UP:
		case DVK_FOLD:
		case DVK_MENU:
		case DVK_SHIFT:
			// if cursorPos isn't the end of line, flash the current character between '_' and itself every (1 / FLASH_INTERVAL) seconds
			if (cursorPos == line.size())
			{
				flashTimer = 0;
				flashState = false;
				break;
			}
			if (++flashTimer >= FLASH_INTERVAL)
			{
				flashTimer = 0;
				((flashState = !flashState) ? (std::cout << cursorCharacter) : (std::cout << line[cursorPos])) << Cursor::moveLeftOne;
			}
			break;

		// enter, newline, '\n', 10
		case DVK_ENTER:
			if (cursorPos == line.size())
				std::cout << ' ';
			else
				std::cout << line[cursorPos] << '\r';
			std::cout << '\n';
			lineProcessor(line);
			line.clear();
			cursorPos = 0;
			std::cout << prompt;
			break;

		// backspace, '\b', 8
		case DVK_BACKSPACE:
			if (!line.empty())
				line.erase(--cursorPos, 1);
			// space overwrites the cursor character, then backspace the space and the desired character
			std::cout << " \b\b" << cursorCharacter << Cursor::moveLeftOne;
			break;

		// left arrow
		case DVK_LEFT:
			if (cursorPos == 0)
				break;
			if (cursorPos == line.size())
				std::cout << " \b";
			if (flashState)
				std::cout << line[cursorPos] << Cursor::moveLeftOne;
			std::cout << Cursor::moveLeftOne;
			--cursorPos;
			break;

		// right arrow
		case DVK_RIGHT:
			if (cursorPos == line.size())
				break;
			std::cout << line[cursorPos];
			if (++cursorPos == line.size())
				std::cout << cursorCharacter << Cursor::moveLeftOne;
			break;

		// any other valid character
		default:
			if (cursorPos == line.size())
				line += c;
			else
				line[cursorPos] = c;
			std::cout << c;
			if (++cursorPos == line.size())
				std::cout << cursorCharacter << Cursor::moveLeftOne;
		}

		// Stay synchronized with the screen's refresh rate (60Hz)
		// This is necessary for cursor flashing
		swiWaitForVBlank();

		// For testing
		assert(cursorPos >= 0 && cursorPos <= line.size());
	}
}
