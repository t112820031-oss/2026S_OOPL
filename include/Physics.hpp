#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include <glm/glm.hpp>
#include <algorithm>

namespace Physics {

    /**
     * @brief 偵測圓形與矩形是否碰撞，並計算出反彈所需的法向量。
     */
    inline bool CheckCircleAABBCollision(glm::vec2 circleCenter, float radius, 
                                         glm::vec2 boxCenter, float boxWidth, float boxHeight, 
                                         glm::vec2& outNormal) {
        
        float halfW = boxWidth / 2.0f;
        float halfH = boxHeight / 2.0f;

        float closestX = std::max(boxCenter.x - halfW, std::min(circleCenter.x, boxCenter.x + halfW));
        float closestY = std::max(boxCenter.y - halfH, std::min(circleCenter.y, boxCenter.y + halfH));

        glm::vec2 distanceVec = glm::vec2(circleCenter.x - closestX, circleCenter.y - closestY);
        float distanceSq = distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y;

        if (distanceSq <= radius * radius) {
            if (distanceSq == 0.0f) {
                outNormal = glm::vec2(0.0f, -1.0f);
            } else {
                outNormal = glm::normalize(distanceVec);
            }

            if (closestX > boxCenter.x - halfW && closestX < boxCenter.x + halfW) {
                outNormal = glm::vec2(0.0f, closestY > boxCenter.y ? 1.0f : -1.0f);
            } else if (closestY > boxCenter.y - halfH && closestY < boxCenter.y + halfH) {
                outNormal = glm::vec2(closestX > boxCenter.x ? 1.0f : -1.0f, 0.0f);
            }
            return true;
        }
        return false;
    } // 補上了 AABB 碰撞函式的結束括號

    // 找出一點到線段的最短距離點
    inline glm::vec2 ClosestPointOnLineSegment(glm::vec2 p, glm::vec2 a, glm::vec2 b) {
        glm::vec2 ab = b - a;
        float t = glm::dot(p - a, ab) / glm::dot(ab, ab);
        t = std::max(0.0f, std::min(1.0f, t));
        return a + t * ab;
    }

    // 判斷點是否在三角形內部
    inline bool IsPointInTriangle(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c) {
        auto sign = [](glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
            return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
        };
        bool b1 = sign(p, a, b) < 0.0f;
        bool b2 = sign(p, b, c) < 0.0f;
        bool b3 = sign(p, c, a) < 0.0f;
        return ((b1 == b2) && (b2 == b3));
    }

    // 圓形與直角三角形碰撞偵測
    inline bool CheckCircleTriangleCollision(glm::vec2 circleCenter, float radius, 
                                             glm::vec2 boxCenter, float boxSize, int orientation, 
                                             glm::vec2& outNormal) {
        float half = boxSize / 2.0f;
        
        glm::vec2 TL(boxCenter.x - half, boxCenter.y + half); 
        glm::vec2 TR(boxCenter.x + half, boxCenter.y + half); 
        glm::vec2 BL(boxCenter.x - half, boxCenter.y - half); 
        glm::vec2 BR(boxCenter.x + half, boxCenter.y - half); 

        glm::vec2 p1, p2, p3;
        
        switch (orientation) {
            case 0: p1 = TR; p2 = BL; p3 = BR; break; 
            case 1: p1 = TL; p2 = BR; p3 = BL; break; 
            case 2: p1 = TL; p2 = BR; p3 = TR; break; 
            case 3: p1 = TR; p2 = BL; p3 = TL; break; 
            default: return false;
        }

        if (IsPointInTriangle(circleCenter, p1, p2, p3)) {
            glm::vec2 hypotenuseDir = p2 - p1;
            outNormal = glm::normalize(glm::vec2(-hypotenuseDir.y, hypotenuseDir.x)); 
            if (glm::dot(outNormal, circleCenter - boxCenter) < 0) {
                outNormal = -outNormal;
            }
            return true;
        }

        glm::vec2 closest1 = ClosestPointOnLineSegment(circleCenter, p1, p2); 
        glm::vec2 closest2 = ClosestPointOnLineSegment(circleCenter, p2, p3); 
        glm::vec2 closest3 = ClosestPointOnLineSegment(circleCenter, p3, p1); 

        float d1 = glm::length(circleCenter - closest1);
        float d2 = glm::length(circleCenter - closest2);
        float d3 = glm::length(circleCenter - closest3);

        float minDist = d1;
        glm::vec2 bestClosest = closest1;

        if (d2 < minDist) { minDist = d2; bestClosest = closest2; }
        if (d3 < minDist) { minDist = d3; bestClosest = closest3; }

        if (minDist <= radius) {
            if (minDist == 0.0f) { 
                outNormal = glm::vec2(0.0f, 1.0f); 
            } else {
                outNormal = glm::normalize(circleCenter - bestClosest);
            }
            return true;
        }
        return false;
    } 

    /**
     * @brief 根據法向量計算反彈後的新速度
     */
    inline glm::vec2 GetReflectionVelocity(glm::vec2 currentVelocity, glm::vec2 normal) {
        float dotProduct = glm::dot(currentVelocity, normal);
        return currentVelocity - 2.0f * dotProduct * normal;
    }

} 

#endif