#include <vector>
#include <SDL/SDL_keysym.h>

std::vector<std::vector<int> > getKeyboard()
{
	// en_US

	// row 0
	int row0[] = { SDLK_ESCAPE, SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12, SDLK_F13, SDLK_F14, SDLK_F15, SDLK_SYSREQ, SDLK_PAUSE, SDLK_INSERT, SDLK_DELETE, SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY };

	// row 1
	int row1[]= { SDLK_BACKQUOTE, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_MINUS, SDLK_EQUALS, SDLK_BACKSPACE, SDLK_KP_PERIOD, SDLK_KP_PLUS, SDLK_KP_MINUS };

	// row 2
	int row2[] = { SDLK_TAB, SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i, SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET, SDLK_BACKSLASH, SDLK_KP7, SDLK_KP8, SDLK_KP9 };

	// row 3
	int row3[] = { SDLK_CAPSLOCK, SDLK_a, SDLK_s, SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE, SDLK_RETURN, SDLK_KP4, SDLK_KP5, SDLK_KP6 };

	// row 4
	int row4[] = { SDLK_LSHIFT, SDLK_z, SDLK_x, SDLK_c, SDLK_v, SDLK_b, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RSHIFT, SDLK_UP, SDLK_KP1, SDLK_KP2, SDLK_KP3 };

	// row 5
	int row5[] = { SDLK_LCTRL, SDLK_LALT, SDLK_SPACE, SDLK_RALT, SDLK_RCTRL, SDLK_LEFT, SDLK_DOWN, SDLK_RIGHT, SDLK_KP0, SDLK_KP_ENTER };

	std::vector<int> row0v(row0, row0 + sizeof(row0) / sizeof(int));
	std::vector<int> row1v(row1, row1 + sizeof(row1) / sizeof(int));
	std::vector<int> row2v(row2, row2 + sizeof(row2) / sizeof(int));
	std::vector<int> row3v(row3, row3 + sizeof(row3) / sizeof(int));
	std::vector<int> row4v(row4, row4 + sizeof(row4) / sizeof(int));
	std::vector<int> row5v(row5, row5 + sizeof(row5) / sizeof(int));

	std::vector<std::vector<int> > kb;
	kb.push_back(row0v);
	kb.push_back(row1v);
	kb.push_back(row2v);
	kb.push_back(row3v);
	kb.push_back(row4v);
	kb.push_back(row5v);

	return kb;
}
