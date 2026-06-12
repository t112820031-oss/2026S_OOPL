#pragma once
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>

class EffectLine : public Util::GameObject {
public:
    EffectLine();
    
    // 設定線條 (中心點 / 長度 / 是否縱向 / 粗度像素 / 顏色貼圖路徑)
    void Setup(glm::vec2 center, float length, bool vertical,
               float thicknessPx, const std::string& imagePath);

    // 動畫推進。回傳 true 表示特效結束，可以移除
    bool Tick(float dt);

    void SetLifetime(float seconds) { m_Lifetime = seconds; m_Elapsed = 0.0f; }

private:
    float m_Lifetime = 0.3f;
    float m_Elapsed  = 0.0f;
};