// Cube Othello -- entry point (DXLib requires a WinMain on Windows).
#include <cstdint>
#include <optional>
#include <string>

#include <windows.h>

#include "game_engine.hpp"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
    // "--ai" / "--ai=white" selects Human(Black) vs. AI(White) (default when
    // "--ai" is given with no color); "--ai=black" plays AI as Black instead.
    std::optional<int8_t> ai_color;
    std::string args = lpCmdLine ? lpCmdLine : "";
    if (args.find("--ai=black") != std::string::npos) {
        ai_color = cubo_othello::BLACK;
    } else if (args.find("--ai") != std::string::npos) {
        ai_color = cubo_othello::WHITE;
    }

    cubo_othello::GameEngine engine(ai_color, /*ai_depth=*/3);
    return engine.run();
}
