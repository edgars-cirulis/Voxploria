#include "engine/core/AppHost.hpp"
#include "game/Game.hpp"

#include <iostream>

int main() {
  AppHost app;
  try {
    app.initWindow(1280, 720, "Voxploria");

    int fbw, fbh;
    glfwGetFramebufferSize(app.getWindow(), &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0) {
      fbw = 1280;
      fbh = 720;
    }

    Game game;
    game.init(app.getWindow(), fbw, fbh);

    app.run([&](float dt) { return game.tick(dt); });

    game.shutdown();
    app.shutdown();
  } catch (const std::exception& e) {
    std::cerr << "Fatal: " << e.what() << "\n";
    app.shutdown();
    return -1;
  }
  return 0;
}
