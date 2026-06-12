#pragma once
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <glm/vec2.hpp>
#include <memory>

class Launcher : public Util::GameObject {
public:
    Launcher();
    
    // 將座標同步給底層 GameObject
    void SetPosition(const glm::vec2& pos);
    glm::vec2 GetPosition() const { return m_Position; }

    // 處理瞄準邏輯，並畫出虛線。回傳最終限制後的發射方向向量
    glm::vec2 UpdateAndDrawAimLine(glm::vec2 mousePos);

private:
    glm::vec2 m_Position;
    std::shared_ptr<Util::Image> m_AimDotImage; // 淺灰色虛線點
};