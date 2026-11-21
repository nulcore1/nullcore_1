// nullcore: Games as a top-level main-menu section (icon tile).
#pragma once
#if defined(NULLCORE_GAMES)
#include <MenuItemInterface.h>

class GamesMenu : public MenuItemInterface {
public:
    GamesMenu() : MenuItemInterface("Games") {}

    void optionsMenu(void) override;
    void drawIcon(float scale = 1) override;

    bool hasTheme() override { return false; }
    const String &themePath() override {
        static const String empty;
        return empty;
    }
};
#endif
