#include "Launcher.hpp"
#include <cmath>

Launcher::Launcher() {
    // 1. 設定主角圖片 (原本在 AppUtil.cpp 的那段)
    SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/Character.png"));
    SetZIndex(5);
    
    // 2. 載入虛線點素材
    m_AimDotImage = std::make_shared<Util::Image>(RESOURCE_DIR "/Image/Dot.png");
}

void Launcher::SetPosition(const glm::vec2& pos) {
    m_Position = pos;
    m_Transform.translation = pos;
}

glm::vec2 Launcher::UpdateAndDrawAimLine(glm::vec2 mousePos) {
    // 1. 計算方向向量：滑鼠位置 - 發射器的目前位置
    glm::vec2 dir = mousePos - m_Transform.translation;

    // 2. 將向量單位化 (長度變成 1)，並防止滑鼠與發射器重疊時的除以零錯誤
    if (glm::length(dir) > 0.0f) {
        dir = glm::normalize(dir);
    } else {
        dir = glm::vec2(0.0f, 1.0f); // 預設朝上
    }

    return dir;
}