#if defined(NULLCORE_GAMES)
#include "GamesMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "games.h"
#include <globals.h>

void GamesMenu::optionsMenu() {
    options = {
        {"Jumper", game_jumper},
        {"Snake",  game_snake },
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Games");
}

// Simple gamepad icon: rounded body, a D-pad on the left and two buttons on the right.
void GamesMenu::drawIcon(float scale) {
    clearIconArea();
    int r = scale * 7;
    int w = 9 * r, h = 5 * r;

    tft.drawRoundRect(iconCenterX - w / 2, iconCenterY - h / 2, w, h, r, bruceConfig.priColor);

    // D-pad (plus) on the left
    int dx = iconCenterX - 2 * r, dy = iconCenterY;
    tft.fillRect(dx - r, dy - r / 3, 2 * r, 2 * r / 3 + 1, bruceConfig.priColor);
    tft.fillRect(dx - r / 3, dy - r, 2 * r / 3 + 1, 2 * r, bruceConfig.priColor);

    // Two action buttons on the right
    tft.fillCircle(iconCenterX + r + 2, iconCenterY - r / 2, r / 2 + 1, bruceConfig.priColor);
    tft.fillCircle(iconCenterX + 2 * r + 4, iconCenterY + r / 2, r / 2 + 1, bruceConfig.priColor);
}
#endif
