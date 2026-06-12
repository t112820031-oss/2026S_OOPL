#include "EffectLine.hpp"

EffectLine::EffectLine() {
    SetZIndex(-1);    
}

void EffectLine::Setup(glm::vec2 center, float length, bool vertical,
                       float thicknessPx, const std::string& imagePath) {
    SetDrawable(std::make_shared<Util::Image>(imagePath));
    m_Transform.translation = center;
    
    // 圖檔 8x8 → scale = 目標 / 8
    if (vertical) {
        m_Transform.scale = glm::vec2(thicknessPx / 8.0f, length / 8.0f);
    } else {
        m_Transform.scale = glm::vec2(length / 8.0f, thicknessPx / 8.0f);
    }
    
    m_Lifetime = 0.015f;
    m_Elapsed = 0.0f;
}

bool EffectLine::Tick(float dt) {
    m_Elapsed += dt;
    return m_Elapsed >= m_Lifetime;
}