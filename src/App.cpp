#include "App.hpp"
#include "Util/Logger.hpp"

// 建構子 (Constructor)
App::App() : m_State(GameState::MAIN_MENU) {
    LOG_INFO("BBTAN Game Initialized.");
}

// 解構子 (Destructor)
App::~App() {
    // 釋放資源的預設行為
    LOG_INFO("BBTAN Game Closed.");
}