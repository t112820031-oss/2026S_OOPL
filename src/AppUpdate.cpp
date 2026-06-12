#include <cmath>
#include "Core/Context.hpp"
#include "App.hpp"
#include "Prop.hpp"
#include "Ball.hpp"
#include "LevelManager.hpp"
#include "Util/Input.hpp" 
#include "Util/Logger.hpp"
#include "Physics.hpp" 

bool CheckAABBCollision(glm::vec2 posA, glm::vec2 sizeA, glm::vec2 posB, glm::vec2 sizeB) {
    bool collisionX = posA.x + sizeA.x / 2.0f >= posB.x - sizeB.x / 2.0f &&
                      posB.x + sizeB.x / 2.0f >= posA.x - sizeA.x / 2.0f;
    bool collisionY = posA.y + sizeA.y / 2.0f >= posB.y - sizeB.y / 2.0f &&
                      posB.y + sizeB.y / 2.0f >= posA.y - sizeA.y / 2.0f;
    return collisionX && collisionY;
}

glm::vec2 GetNormalByOrientation(int orientation) {
    float n = 0.7071f; 
    switch (orientation) {
        case 0: return glm::vec2(n, n);   // 斜邊朝右上
        case 1: return glm::vec2(n, -n);  // 斜邊朝右下
        case 2: return glm::vec2(-n, -n); // 斜邊朝左下
        case 3: return glm::vec2(-n, n);  // 斜邊朝左上
        default: return glm::vec2(0, 0);
    }
}

bool ResolveCollision(glm::vec2& ballPos, glm::vec2 ballSize, glm::vec2 brickPos, glm::vec2 brickSize, glm::vec2& ballVel, Brick::BrickType type, int orientation) {
    
    float dx = ballPos.x - brickPos.x;
    float dy = ballPos.y - brickPos.y;

    float overlapX = (ballSize.x + brickSize.x) / 2.0f - std::abs(dx);
    float overlapY = (ballSize.y + brickSize.y) / 2.0f - std::abs(dy);

    // 只要重疊大於 0，代表進入了磚塊所在的「正方形區域」
    if (overlapX > 0 && overlapY > 0) {
        
        // 撞到三角形
        if (type == Brick::BrickType::TRIANGLE) {
            float R = ballSize.x * 0.5f * 0.6f; 
            
            float dist = 0.0f;
            bool isSlash = false; // true = '/', false = '\'

            // 計算球中心到「斜線」的垂直距離
            switch (orientation) {
                case 0: // ◣ (0度, 直角在左下)
                    dist = (dx + dy) / 1.4142f;  isSlash = false; break;
                
                case 1: // ◢ (90度逆時針, 直角在右下) 
                    dist = (-dx + dy) / 1.4142f; isSlash = true;  break; 
                
                case 2: // ◥ (180度, 直角在右上)
                    dist = -(dx + dy) / 1.4142f; isSlash = false; break;
                
                case 3: // ◤ (270度逆時針, 直角在左上) 
                    dist = (dx - dy) / 1.4142f;  isSlash = true;  break; 
            }

            if (dist > R) return false;

            // 找出穿透最淺的面（斜邊、垂直邊X、垂直邊Y）
            float penSlant = R - dist;  // 對斜邊的穿透深度
            float penX = overlapX;      // 對垂直邊的穿透深度
            float penY = overlapY;      // 對水平邊的穿透深度

            // 如果斜邊的穿透深度是最小的，代表球確實是打在斜邊上！
            if (penSlant < penX && penSlant < penY) {
                // 執行 45 度無損反彈 (直接交換 X/Y 速度)
                float old_vx = ballVel.x;
                float old_vy = ballVel.y;
                if (isSlash) {
                    ballVel.x = old_vy;
                    ballVel.y = old_vx;
                } else {
                    ballVel.x = -old_vy;
                    ballVel.y = -old_vx;
                }
            } 
            else if (penX < penY) {
                ballVel.x = dx > 0 ? std::abs(ballVel.x) : -std::abs(ballVel.x);
                ballPos.x += dx > 0 ? penX : -penX; // 防穿透外推
            } 
            else {
                ballVel.y = dy > 0 ? std::abs(ballVel.y) : -std::abs(ballVel.y);
                ballPos.y += dy > 0 ? penY : -penY; // 防穿透外推
            }
            return true;
        }
        // 撞到普通方形磚塊，執行 AABB 的反彈邏輯
        else {
            if (overlapX < overlapY) {
                // 側面撞擊：反轉 X 速度，並把球往外推 overlapX 的距離
                ballVel.x = dx > 0 ? std::abs(ballVel.x) : -std::abs(ballVel.x);
                ballPos.x += dx > 0 ? overlapX : -overlapX; 
            } else {
                // 上下撞擊：反轉 Y 速度，並把球往外推 overlapY 的距離
                ballVel.y = dy > 0 ? std::abs(ballVel.y) : -std::abs(ballVel.y);
                ballPos.y += dy > 0 ? overlapY : -overlapY; 
            }
        }
        return true;
    }
    return false;
}

bool IsButtonClicked(std::shared_ptr<Util::GameObject> button, glm::vec2 mousePos, float width, float height) {
    // 如果按鈕不存在，就當作沒點擊
    if (button == nullptr) {
        return false;
    }

    auto btnPos = button->m_Transform.translation;
    // 檢查滑鼠游標是否在按鈕的矩形範圍內
    bool inX = mousePos.x >= (btnPos.x - width / 2) && mousePos.x <= (btnPos.x + width / 2);
    bool inY = mousePos.y >= (btnPos.y - height / 2) && mousePos.y <= (btnPos.y + height / 2);
    
    // 如果游標在範圍內，且按下了滑鼠左鍵
    return inX && inY && Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB);
}

void App::UpdateBallsAndCollisions(float dt) {
    // 蒐集本幀被尖刺消滅的球，loop 結束後統一移除
    std::unordered_set<Ball*> ballsToKill;

    // ==========================================
    // 上排 UI 生成（金幣 / 回合 / 最高紀錄）
    // 螢幕 540x960，中心原點 Y 朝上
    // (30,40)→(-240,440)  (270,40)→(0,440)  (480,40)→(210,440)
    // ==========================================
    if (m_CoinTextObj == nullptr) {
        LOG_INFO("✅ 啟動上排 UI 強制加載！");

        // 1. 金幣（左上角，黃色，size 24）
        m_CoinTextObj = std::make_shared<Util::GameObject>();
        m_CoinTextDrawable = std::make_shared<Util::Text>(
            RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 24,
            std::to_string(m_Coins),
            Util::Color(255, 255, 0, 255)        // 純黃色
        );
        m_CoinTextObj->SetDrawable(m_CoinTextDrawable);
        m_CoinTextObj->SetZIndex(80);
        m_CoinTextObj->m_Transform.translation = glm::vec2(-240.0f, 440.0f);
        m_Root.AddChild(m_CoinTextObj);

        // 2. 回合（正上方，白色，size 24）
        m_RoundTextObj = std::make_shared<Util::GameObject>();
        m_RoundTextDrawable = std::make_shared<Util::Text>(
            RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 24,
            "Round " + std::to_string(m_CurrentRound),
            Util::Color(255, 255, 255, 255)
        );
        m_RoundTextObj->SetDrawable(m_RoundTextDrawable);
        m_RoundTextObj->SetZIndex(80);
        m_RoundTextObj->m_Transform.translation = glm::vec2(0.0f, 440.0f);
        m_Root.AddChild(m_RoundTextObj);

        // 3. 最高紀錄（右上角，白色，size 24）
        m_TopTextObj = std::make_shared<Util::GameObject>();
        m_TopTextDrawable = std::make_shared<Util::Text>(
            RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 24,
            "TOP " + std::to_string(m_HighScore),
            Util::Color(255, 255, 255, 255)
        );
        m_TopTextObj->SetDrawable(m_TopTextDrawable);
        m_TopTextObj->SetZIndex(80);
        m_TopTextObj->m_Transform.translation = glm::vec2(210.0f, 440.0f);
        m_Root.AddChild(m_TopTextObj);
    }

    // 收回模式：瞬間吸回所有球到 target
    if (m_IsRecycling) {
        for (auto& ball : m_Balls) {
            if (!ball || !ball->IsActive()) continue;
            ball->SetPosition(m_RecycleTarget);
            ball->SetVelocity(glm::vec2(0.0f, 0.0f));
        }
        m_IsRecycling = false;
        LOG_INFO("回收完成");
        return;
    }

    // 每一幀都同步一次數值
    RefreshTopUI();
        
    glm::vec2 ballSize(16.0f, 16.0f); 
    glm::vec2 brickSize(64.0f, 64.0f); 

    // 道具的較小碰撞箱 (例如 24x24)
    glm::vec2 propSize(24.0f, 24.0f);

    if (!m_LevelManager) return; 

    auto& bricks = m_LevelManager->GetBricks();
    
    // 倒數所有 SPLIT 新生磚塊的穿透計時
    for (auto& b : m_LevelManager->GetBricks()) {
        if (b && b->m_GraceTimer > 0.0f) {
            b->m_GraceTimer -= dt;
        }
    }

    for (auto& ball : m_Balls) {
        if (ball == nullptr) continue;

        if (!ball->IsActive()) continue;          
        if (glm::length(ball->GetVelocity()) < 0.01f) continue;
        
        // 1. 取得現在的位置與速度
        auto pos = ball->GetPosition();
        auto vel = ball->GetVelocity();
        
        // 2. 物理移動：位置 = 原位置 + (速度 * 時間差)
        pos += vel * dt;
        
        // 3. 牆壁反彈檢查
        if (pos.x - 10.0f <= -245.0f || pos.x + 10.0f >= 245.0f) {
            vel.x = -vel.x; // 左右撞牆反彈
            pos.x = (pos.x > 0) ? 235.0f : -235.0f; // 防卡牆
        }
        if (pos.y + 10.0f >= 395.0f) {
            vel.y = -vel.y; 
            pos.y = 385.0f; // 防卡牆也要跟著調高
        }

        // 底部碰撞邏輯 (回收球機制)
        float floorY = -270.0f; 
        
        if (pos.y <= floorY && vel.y <= 0.0f) {
            // 彩虹球落地：原地停下，不影響發射點，不算第一顆回來
            if (ball->IsRainbow()) {
                vel = glm::vec2(0.0f, 0.0f);
                pos.y = floorY;
            }
            else if (!m_IsFirstBallReturned) {
                m_IsFirstBallReturned = true;
                m_NextLaunchPos = glm::vec2(pos.x, floorY); 
                vel = glm::vec2(0.0f, 0.0f); 
                pos = m_NextLaunchPos;
            } else {
                float dx = m_NextLaunchPos.x - pos.x;
                if (std::abs(dx) > 20.0f) { 
                    float slideSpeed = 800.0f; 
                    vel = glm::vec2((dx > 0 ? slideSpeed : -slideSpeed), 0.0f);
                    pos.y = floorY; 
                } else {
                    vel = glm::vec2(0.0f, 0.0f);
                    pos = m_NextLaunchPos;
                }
            }
        }
        
        // 4. 磚塊碰撞檢查 (內部迴圈開始)
        for (auto it = bricks.begin(); it != bricks.end(); ) {
            auto& brick = *it;
            if (brick == nullptr) {
                it++; continue;
            }
            
            auto brickPos = brick->GetPosition();

            glm::vec2 currentBrickSize = brick->GetSize();

            if (!brick->IsProp() && brick->GetType() != Brick::BrickType::GIANT) {
                currentBrickSize = glm::vec2(70.0f, 70.0f);
            }
            
            // 如果是任何「穿透型道具」 (不反彈)
            if (brick->IsProp()) { 
                if (CheckAABBCollision(pos, ballSize, brickPos, propSize)) {
                    
                    auto type = brick->GetType();

                    // 金幣、加球 (碰到立刻發動並消失)
                    if (type == Brick::BrickType::ADD_BALL || type == Brick::BrickType::COIN) {
                        if (type == Brick::BrickType::ADD_BALL) {
                            LOG_INFO("吃到加球道具！");
                            m_PendingExtraBalls++;
                        } else {
                            LOG_INFO("吃到金幣！");
                            m_Coins++; 
                            auto testTextObj = std::make_shared<Util::GameObject>();
                            auto testTextDrawable = std::make_shared<Util::Text>(
                                RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 
                                40, 
                                "TEST: " + std::to_string(m_Coins),
                                Util::Color(255, 255, 255, 255) 
                            );
                            testTextObj->SetDrawable(testTextDrawable);
                            testTextObj->SetZIndex(100); 
                            testTextObj->m_Transform.translation = glm::vec2(0.0f, 0.0f); // 螢幕正中央
                            
                            // 加入畫面
                            m_Root.AddChild(testTextObj);
                        }
                        
                        m_Root.RemoveChild(brick); 
                        it = bricks.erase(it);     
                        continue; 
                    }
                    // 延後消失類：雷射、散射 (碰到先標記，等迴圈結束再統一處理)
                    else { 
                        // 只要碰到，就標記這回合必須被刪除
                        brick->SetActivated(true); 
                        
                        if (brick->GetLastHitBall() == ball.get()) {
                            it++;
                            continue;
                        }

                        // 如果是新的球，更新記錄並發動效果
                        brick->SetLastHitBall(ball.get());

                        // 執行雷射與散射效果
                        if (type == Brick::BrickType::HLASER) { 
                            LOG_INFO("觸發橫向雷射");
                            for (auto& otherBrick : bricks) {
                                if (!otherBrick->IsProp() && std::abs(otherBrick->GetPosition().y - brickPos.y) < 10.0f) {
                                    otherBrick->SetHp(otherBrick->GetHp() - 1);
                                }
                            }
                            // 特效：藍色橫線
                            SpawnLaserEffectHorizontal(brick->GetRow(), 20.0f, "fx_laser_blue.png");
                        }
                        else if (type == Brick::BrickType::VLASER) { 
                            LOG_INFO("觸發直向雷射");
                            for (auto& otherBrick : bricks) {
                                if (!otherBrick->IsProp() && std::abs(otherBrick->GetPosition().x - brickPos.x) < 10.0f) {
                                    otherBrick->SetHp(otherBrick->GetHp() - 1);
                                }
                            }
                            // 特效：藍色直線
                            SpawnLaserEffectVertical(brick->GetCol(), 20.0f, "fx_laser_blue.png");
                        }
                        else if (type == Brick::BrickType::SCATTER) { 
                            LOG_INFO("觸發散射");
                            
                            // 1. 設定角度範圍：15度 到 165度
                            int minAngle = 15;
                            int maxAngle = 165;
                            
                            // 2. 取隨機角度並轉為弧度 (Radian)
                            int randomDeg = minAngle + (rand() % (maxAngle - minAngle + 1));
                            float randomRad = randomDeg * 3.14159f / 180.0f;
                            
                            // 3. 取得球原本的速度大小 (速率)
                            float speed = glm::length(vel);
                            
                            // 4. 給予全新的 X 加上 Y 速度！
                            vel = glm::vec2(cos(randomRad) * speed, sin(randomRad) * speed);
                        }
                    }
                }
                it++; 
                continue; 
            }
            
            // 實體磚塊
            if (brick->IsInvisibleHidden()) {
                it++;
                continue;
            }

            bool inGrace = (brick->m_GraceTimer > 0.0f);
            bool hit = false;

            if (ball->IsRainbow()) {
                // 彩虹球：只偵測碰撞，不反彈
                hit = CheckAABBCollision(pos, ballSize, brickPos, currentBrickSize);
            } else {
                // 普通球：碰撞 + 反彈
                hit = ResolveCollision(pos, ballSize, brickPos, currentBrickSize, vel, 
                                    brick->GetType(), brick->GetOrientation());
            }

            if (hit) {
                if (ball->IsRainbow()) {
                    // 這顆磚塊已被這顆彩虹球扣過血 → 跳過
                    if (ball->m_HitBricks.count(brick.get())) {
                        it++;
                        continue;
                    }
                    ball->m_HitBricks.insert(brick.get());

                    // 關卡模式：彩虹球扣當下血量一半；無限模式：直接秒殺
                    int dmg = (m_LevelManager && m_LevelManager->IsLevelMode())
                            ? (brick->GetHp() + 1) / 2
                            : 99999;
                    bool dead = brick->OnHit(dmg, true);
                    
                    if (dead && brick->GetType() == Brick::BrickType::SPIKE) {
                        m_LevelManager->OnSpikeKilled();
                    }

                    auto bossType = brick->GetType();
                    int threshold = 0;
                    if (bossType == Brick::BrickType::LEVEL10_BOSS) threshold = 10;
                    else if (bossType == Brick::BrickType::LEVEL15_BOSS) threshold = 15;
                    else if (bossType == Brick::BrickType::LEVEL20_BOSS) threshold = 10;

                    if (threshold > 0 && !dead) {
                        while (brick->m_DmgSinceTeleport >= threshold) {
                            brick->m_DmgSinceTeleport -= threshold;
                            if (bossType == Brick::BrickType::LEVEL10_BOSS) {
                                m_LevelManager->Level10TeleportBoss();
                            } else if (bossType == Brick::BrickType::LEVEL15_BOSS) {
                                m_LevelManager->Level15OnBossDamaged();
                            } else if (bossType == Brick::BrickType::LEVEL20_BOSS) {
                                m_LevelManager->Level20OnBossDamaged();
                            }
                        }
                    }

                    if (dead) {
                        if (brick->GetType() == Brick::BrickType::TNT) {
                            int tntRow = brick->GetRow();
                            int tntCol = brick->GetCol();
                            m_Root.RemoveChild(brick);
                            bricks.erase(it);
                            m_LevelManager->TriggerTNTExplosion(tntRow, tntCol, m_Root);
                            break;
                        }
                        m_Root.RemoveChild(brick);
                        it = bricks.erase(it);
                    } else {
                        it++;
                    }
                } else {
                    // 普通球：扣血

                    // 厚底：球從底部碰過來不扣血
                    if (brick->GetType() == Brick::BrickType::HARDENED_BOTTOM
                        && pos.y < brickPos.y) {
                        it++;
                        continue;
                    }

                    // LINKED
                    if (brick->GetType() == Brick::BrickType::LINKED) {
                        m_LevelManager->DamageLinked(1);
                        it++;
                        continue;
                    }

                    bool dead = brick->OnHit(1, false);

                    if (dead && brick->GetType() == Brick::BrickType::SPIKE) {
                        m_LevelManager->OnSpikeKilled();
                    }

                    auto bossType = brick->GetType();
                    int threshold = 0;
                    if (bossType == Brick::BrickType::LEVEL10_BOSS) threshold = 10;
                    else if (bossType == Brick::BrickType::LEVEL15_BOSS) threshold = 15;
                    else if (bossType == Brick::BrickType::LEVEL20_BOSS) threshold = 10;

                    if (threshold > 0 && !dead) {
                        while (brick->m_DmgSinceTeleport >= threshold) {
                            brick->m_DmgSinceTeleport -= threshold;
                            if (bossType == Brick::BrickType::LEVEL10_BOSS) {
                                m_LevelManager->Level10TeleportBoss();
                            } else if (bossType == Brick::BrickType::LEVEL15_BOSS) {
                                m_LevelManager->Level15OnBossDamaged();
                            } else if (bossType == Brick::BrickType::LEVEL20_BOSS) {
                                m_LevelManager->Level20OnBossDamaged();
                            }
                        }
                    }

                    // 尖刺：球消失
                    if (brick->GetType() == Brick::BrickType::SPIKE) {
                        ball->SetActive(false);
                        vel = glm::vec2(0.0f, 0.0f);
                        pos = glm::vec2(-9999.0f, -9999.0f);
                    }

                    if (dead) {
                        if (brick->GetType() == Brick::BrickType::SPLIT) {
                            // 分裂：新磚塊 HP 為原本最大血量的一半
                            int newHp = std::max(1, brick->GetMaxHp() / 2);
                            int row = brick->GetRow();
                            int col = brick->GetCol();
                            m_Root.RemoveChild(brick);
                            bricks.erase(it);
                            m_LevelManager->HandleSplit(newHp);
                            break;   
                        }

                        if (brick->GetType() == Brick::BrickType::TNT) {
                            int tntRow = brick->GetRow();
                            int tntCol = brick->GetCol();
                            m_Root.RemoveChild(brick);
                            bricks.erase(it);
                            m_LevelManager->TriggerTNTExplosion(tntRow, tntCol, m_Root);
                            break;
                        }
                        m_Root.RemoveChild(brick);
                        it = bricks.erase(it);
                    } else {
                        it++;
                    }
                }
            } else {
                it++;
            }
        } 

        // 5. 把更新後的位置與速度還給球
        ball->SetPosition(pos);
        ball->SetVelocity(vel);
        
    }

    // 清理所有被雷射殺死的磚塊 (必須在所有迴圈之外)
    for (auto it = bricks.begin(); it != bricks.end(); ) {
        if ((*it) && !(*it)->IsProp() && (*it)->GetHp() <= 0) {
            m_Root.RemoveChild(*it);
            it = bricks.erase(it);
        } else {
            it++;
        }
    }

    // LINKED：池歸零 → 場上所有 LINKED 一起消滅
    if (m_LevelManager->GetLinkedSharedHp() <= 0) {
        auto& allBricks = m_LevelManager->GetBricks();
        for (auto bit = allBricks.begin(); bit != allBricks.end(); ) {
            if ((*bit) && (*bit)->GetType() == Brick::BrickType::LINKED) {
                m_Root.RemoveChild(*bit);
                bit = allBricks.erase(bit);
            } else {
                ++bit;
            }
        }
    }
}

void App::ResetGame() {
    // 1. 清除磚塊
    if (m_LevelManager) m_LevelManager->Clear();

    // 2. 重置數據
    m_CurrentRound = 1;
    m_PendingExtraBalls = 0;
    m_GameOverTimer = 0.0f;

    // 3. 重置球群 (清空舊的球，重新建立一顆初始球)
    for (auto& ball : m_Balls) {
        m_Root.RemoveChild(ball);
    }
    m_Balls.clear();

    auto firstBall = std::make_shared<Ball>(RESOURCE_DIR "/Image/NormalBall.png");
    firstBall->SetPosition(glm::vec2(0.0f, -250.0f)); // 回到起始發射點
    firstBall->SetZIndex(10);
    m_Root.AddChild(firstBall);
    m_Balls.push_back(firstBall);

    // 4. 生成第一波敵人
    m_LevelManager->SpawnRowEndless(m_CurrentRound);

    // 5. 回歸遊戲狀態
    m_IsFiringSequence = false;
    m_State = GameState::MAIN_MENU; // 或者直接切換到主畫面
}

void App::Update() {
    // 推進所有特效
    for (auto it = m_Effects.begin(); it != m_Effects.end(); ) {
        if (*it) {
            (*it)->Update();
            if ((*it)->IsDead()) {
                m_Root.RemoveChild(*it);
                it = m_Effects.erase(it);
                continue;
            }
        }
        ++it;
    }

    if (m_LevelManager && m_LevelManager->m_LevelFailed) {
        m_State = GameState::BALL_SELECTION;
        m_LevelManager->m_LevelFailed = false;
        m_IsFiringSequence = false;
        m_IsRoundActive = false;
        m_IsRecycling = false;
        m_IsSpeedUp = false;
        m_IsDraggingTnt = false;
        if (m_DragTntIcon) m_DragTntIcon->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
    }

    // 退出機制
    if (Util::Input::IsKeyDown(Util::Keycode::ESCAPE)) {
        switch (m_State) {
            case GameState::MAIN_MENU:
                // 主畫面按 ESC 還是離開遊戲
                Core::Context::GetInstance()->SetExit(true);
                break;
            
            case GameState::BALL_SELECTION:
                // 選關畫面 → 回主畫面
                m_State = GameState::MAIN_MENU;
                break;
            
            case GameState::GAMEPLAY_ENDLESS:
                // 無限模式 → 回主畫面
                m_State = GameState::MAIN_MENU;
                // 同時清掉場上戰鬥狀態
                m_IsFiringSequence = false;
                m_IsRoundActive = false;
                m_IsRecycling = false;
                m_IsSpeedUp = false;
                m_IsDraggingTnt = false;
                if (m_DragTntIcon) m_DragTntIcon->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
                break;
            
            case GameState::GAMEPLAY_LEVEL:
                // 關卡模式 → 回選關畫面
                m_State = GameState::BALL_SELECTION;
                m_IsFiringSequence = false;
                m_IsRoundActive = false;
                m_IsRecycling = false;
                m_IsSpeedUp = false;
                m_IsDraggingTnt = false;
                if (m_DragTntIcon) m_DragTntIcon->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
                break;
            
            case GameState::LEVEL_INTRO:
                // 介紹畫面 → 回選關
                m_State = GameState::BALL_SELECTION;
                m_LevelIntroObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
                break;

            case GameState::PROPS_INTRO:
                // 道具介紹 → 回主畫面
                m_State = GameState::MAIN_MENU;
                if (m_PropsIntroObj) {
                    m_PropsIntroObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
                }
                break;
                
            default:
                Core::Context::GetInstance()->SetExit(true);
                break;
        }
    }

    // 推進所有特效
    const float dt = 0.016f;
    for (auto it = m_LaserEffects.begin(); it != m_LaserEffects.end(); ) {
        if (*it && (*it)->Tick(dt)) {
            m_Root.RemoveChild(*it);
            it = m_LaserEffects.erase(it);
        } else {
            ++it;
        }
    }

    // 取得滑鼠座標
    glm::vec2 mousePos = Util::Input::GetCursorPosition(); 

    // 狀態機處理
    switch (m_State) {
        case GameState::MAIN_MENU: {
            // 只在 MAIN_MENU 下繪製主畫面的東西
            if (m_MainMenuBG) m_MainMenuBG->Draw();
            
            //  主畫面的 TOP 文字
            if (!m_MenuTopTextObj) {
                m_MenuTopTextDrawable = std::make_shared<Util::Text>(
                    RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 56,
                    "TOP " + std::to_string(m_HighScore),
                    Util::Color(255, 255, 255, 255)
                );
                m_MenuTopTextObj = std::make_shared<Util::GameObject>();
                m_MenuTopTextObj->SetDrawable(m_MenuTopTextDrawable);
                m_MenuTopTextObj->SetZIndex(5);
                m_MenuTopTextObj->m_Transform.translation = glm::vec2(0.0f, 100.0f);
            }
            
            m_MenuTopTextDrawable->SetText("TOP " + std::to_string(m_HighScore));
            m_MenuTopTextObj->Draw();
            if (m_BtnEndless) m_BtnEndless->Draw();
            if (m_BtnLevel) m_BtnLevel->Draw();

            // 點擊無限模式按鈕 → 進入無限模式
            if (IsButtonClicked(m_BtnEndless, mousePos, 714.0f, 216.0f)) {
                LOG_INFO("進入遊戲模式！");

                m_CurrentRound = 1;
                m_PendingExtraBalls = 0;
                m_GameOverTimer = 0.0f;
                m_IsFiringSequence = false;
                m_IsRoundActive = false;
                m_IsRecycling = false;
                m_IsSpeedUp = false;
                m_IsDraggingTnt = false;
                m_SelectedLevel = 0;                  // 標記為非關卡模式
                m_NextLaunchPos = glm::vec2(0.0f, -250.0f);
                m_RoundStartPos = glm::vec2(0.0f, -250.0f);

                // 清掉場上所有舊磚塊 + 重置 LevelManager 內部狀態
                if (m_LevelManager) {
                    m_LevelManager->Clear();
                    m_LevelManager->SetCurrentLevel(0);   // 0 = 無限模式
                }

                // 重置球
                for (auto& ball : m_Balls) m_Root.RemoveChild(ball);
                m_Balls.clear();
                auto ball = std::make_shared<Ball>(RESOURCE_DIR "/Image/NormalBall.png");
                ball->SetPosition(glm::vec2(0.0f, -250.0f));
                ball->SetVelocity(glm::vec2(0.0f, 0.0f));
                ball->SetZIndex(10);
                m_Root.AddChild(ball);
                m_Balls.push_back(ball);

                // 生成第一波磚塊
                if (m_LevelManager) m_LevelManager->SpawnRowEndless(m_CurrentRound);

                if (!m_PropsIntroShownEndless) {
                    m_PropsIntroShownEndless = true;
                    ShowPropsIntro(false);
                    m_State = GameState::PROPS_INTRO;
                } else {
                    m_State = GameState::GAMEPLAY_ENDLESS;
                }
            }

            // 點擊 Blue_Button → 進入選關畫面
            if (IsButtonClicked(m_BtnLevel, mousePos, 512.0f * 0.5f, 128.0f * 0.5f)) {
                m_State = GameState::BALL_SELECTION;   // 用 BALL_SELECTION 當選關
            }
            break;
        }

        case GameState::GAMEPLAY_ENDLESS: {
            // 1. 物理系統
            UpdateBallsAndCollisions(0.016f);

            // 2. 畫出背景與角色
            if (m_InGameBG) m_InGameBG->Draw();
            if (m_Character) {
                m_Character->m_Transform.translation = glm::vec2(45.0f, -240.0f);
                m_Character->Draw(); 
            }
            if (m_LevelManager) m_LevelManager->Draw();

            // 雷射特效（在磚塊上、球下）
            for (auto& fx : m_LaserEffects) {
                if (fx) fx->Draw();
            }
            // 死亡特效
            for (auto& fx : m_Effects) {
                if (fx) fx->Draw();
            }

            for (auto& ball : m_Balls) {
                if (ball != nullptr) ball->Draw();
            }

            // 上次加的三行 UI 文字
            if (m_CoinTextObj)  m_CoinTextObj->Draw();
            if (m_RoundTextObj) m_RoundTextObj->Draw();
            if (m_TopTextObj)   m_TopTextObj->Draw();

            // 繪製
            if (m_BtnRecycle)     m_BtnRecycle->Draw();
            if (m_BtnSpeedUpIn)   m_BtnSpeedUpIn->Draw();
            if (m_BtnSpeedUpOut)  m_BtnSpeedUpOut->Draw();

            // 加速按鈕外層旋轉（慢=60 deg/s、快=180 deg/s）
            float rotSpeed = m_IsSpeedUp ? 180.0f : 60.0f;
            m_SpeedUpRotation += rotSpeed * 0.016f;
            if (m_BtnSpeedUpOut) {
                m_BtnSpeedUpOut->m_Transform.rotation = glm::radians(m_SpeedUpRotation);
            }

            // 道具按鈕：先畫，再偵測點擊
            if (m_BtnDrillProp)            m_BtnDrillProp->Draw();
            if (m_BtnHorizontalLaserProp)  m_BtnHorizontalLaserProp->Draw();
            if (m_BtnRainbowProp)          m_BtnRainbowProp->Draw();
            if (m_BtnTntProp)              m_BtnTntProp->Draw();

            bool propClicked = false;

            // 拖曳 TNT 中時，所有按鈕都不能再按，直到放下為止
            if (!m_IsDraggingTnt) {
                // Drill：點擊立刻使用
                if (IsButtonClicked(m_BtnDrillProp, mousePos, 80.0f, 80.0f)) {
                    propClicked = true;
                    if (m_Coins < 5) {
                        ShowWarning("Not enough coins");
                    } else {
                        // 直接呼叫 LevelManager 的版本；回傳 false 代表場上沒磚塊
                        bool success = m_LevelManager->UseDrill(m_Root);
                        if (success) {
                            m_Coins -= 5;
                            // 特效：淺紫色直線
                            int col = m_LevelManager->GetLastDrillTargetCol();
                            if (col >= 0) SpawnLaserEffectVertical(col, 32.0f, "fx_laser_purple.png");
                        } else {
                            ShowWarning("No more bricks to break");
                        }
                    }
                }
                // HorizontalLaser：點擊立刻使用
                else if (IsButtonClicked(m_BtnHorizontalLaserProp, mousePos, 80.0f, 80.0f)) {
                    propClicked = true;
                    if (m_Coins < 4) {
                        ShowWarning("Not enough coins");
                    } else {
                        bool success = m_LevelManager->UseHorizontalLaser(m_Root);
                        if (success) {
                            m_Coins -= 4;
                            // 特效：淺紫色橫線
                            int row = m_LevelManager->GetLastLaserTargetRow();
                            if (row >= 0) SpawnLaserEffectHorizontal(row, 32.0f, "fx_laser_purple.png");
                        } else {
                            ShowWarning("No more bricks to break");
                        }
                    }
                }
                // Rainbow：點擊購買
                else if (IsButtonClicked(m_BtnRainbowProp, mousePos, 80.0f, 80.0f)) {
                    propClicked = true;
                    if (m_HasBoughtRainbow) ShowWarning("You already bought it");
                    else if (m_Coins < 8)   ShowWarning("Not enough coins");
                    else { m_Coins -= 8;    m_HasBoughtRainbow = true; }
                }
                // TNT：左鍵點擊開始拖曳
                else if (IsButtonClicked(m_BtnTntProp, mousePos, 80.0f, 80.0f)) {
                    propClicked = true;
                    if (m_Coins < 5) ShowWarning("Not enough coins");
                    else             m_IsDraggingTnt = true;
                }
                else if (IsButtonClicked(m_BtnRecycle, mousePos, 216.0f, 60.0f)) {
                    propClicked = true;
                    RecycleBalls();
                }
                else if (IsButtonClicked(m_BtnSpeedUpOut, mousePos, 60.0f, 60.0f)) {
                    propClicked = true;
                    float scale = m_IsSpeedUp ? (1.0f / 3.0f) : 3.0f;
                    for (auto& ball : m_Balls) {
                        if (ball && ball->IsActive() && glm::length(ball->GetVelocity()) > 0.01f) {
                            ball->SetVelocity(ball->GetVelocity() * scale);
                        }
                    }
                    m_IsSpeedUp = !m_IsSpeedUp;
                }
            }

            if (m_WarningTimer > 0.0f) {
                m_WarningTimer -= 0.016f;
                if (m_WarningTimer <= 0.0f && m_WarningObj) {
                    m_WarningObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
                }
            }
            if (m_WarningObj) m_WarningObj->Draw();

            RefreshTopUI();

            // TNT 拖曳中：圖示跟著滑鼠跑，放開後嘗試放置
            if (m_IsDraggingTnt) {
                m_DragTntIcon->m_Transform.translation = mousePos;
                m_DragTntIcon->Draw();

                // 放開滑鼠 = 嘗試放置
                if (Util::Input::IsKeyUp(Util::Keycode::MOUSE_LB)) {
                    m_IsDraggingTnt = false;
                    m_DragTntIcon->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);

                    int row = -1, col = -1;
                    bool ok = m_LevelManager->TryPlaceBomb(mousePos, row, col);

                    if (ok) {
                        m_Coins -= 5;
                    } else {
                        ShowWarning("Choose empty slot");
                    }
                }
            }
            // 連發與瞄準邏輯
            
            // 判定是否正在連發中
            if (m_IsFiringSequence) {
                m_FireTimer += 0.016f;
                float fireInterval = m_IsSpeedUp ? 0.016f : 0.08f;
                float ballSpeed    = m_IsSpeedUp ? 2400.0f : 800.0f;

                if (m_FireTimer >= fireInterval) {
                    m_FireTimer = 0.0f;
                    if (m_BallsFiredCount < (int)m_Balls.size()) {
                        auto& ball = m_Balls[m_BallsFiredCount];
                        if (ball->IsActive()) {
                            ball->SetPosition(m_RoundStartPos);
                            ball->SetVelocity(m_FireDirection * ballSpeed);
                        }
                        m_BallsFiredCount++;
                    } else {
                        m_IsFiringSequence = false;
                    }
                }
            }
            else {
                // 清理已停下的彩虹球
                for (auto it = m_Balls.begin(); it != m_Balls.end(); ) {
                    if (*it && (*it)->IsRainbow()
                        && glm::length((*it)->GetVelocity()) < 0.01f) {
                        m_Root.RemoveChild(*it);
                        it = m_Balls.erase(it);
                    } else {
                        ++it;
                    }
                }

                bool allBallsStopped = true;
                for (auto& ball : m_Balls) {
                    if (!ball->IsActive()) continue;       
                    if (glm::length(ball->GetVelocity()) > 0.01f) {
                        allBallsStopped = false;
                        break;
                    }
                }
                if (allBallsStopped && !m_Balls.empty()) {
                    // 回合結算系統
                    if (m_IsRoundActive) {
                        m_IsRoundActive = false; // 關閉戰鬥狀態
                        m_IsSpeedUp = false;        // 每回合重置加速狀態
                        m_CurrentRound++;        // 回合數 +1 
                        
                        {
                            auto& allBricks = m_LevelManager->GetBricks();
                            for (auto& b : allBricks) {
                                if (b) b->OnRoundEnd();
                            }
                        }

                        // 尖刺殺掉的球下回合復活
                        for (auto& b : m_Balls) {
                            if (b && !b->IsActive()) {
                                b->SetActive(true);
                                b->SetPosition(m_NextLaunchPos);
                                b->SetVelocity(glm::vec2(0.0f, 0.0f));
                            }
                        }

                        // 清理階段：移除這回合已經「發動過」的機關 (雷射、散射)
                        auto& allObjects = m_LevelManager->GetBricks();
                        for (auto it = allObjects.begin(); it != allObjects.end(); ) {
                            if ((*it) && (*it)->IsProp() && (*it)->IsActivated()) {
                                m_Root.RemoveChild(*it);
                                it = allObjects.erase(it); // 從陣列中移除
                            } else {
                                it++;
                            }
                        }

                        // 將吃到的球補進陣列中
                        for (int i = 0; i < m_PendingExtraBalls; ++i) {
                            auto newBall = std::make_shared<Ball>(RESOURCE_DIR "/Image/NormalBall.png");
                            
                            newBall->SetPosition(m_NextLaunchPos); 
                            newBall->SetVelocity(glm::vec2(0.0f, 0.0f));
                            newBall->SetZIndex(10);
                            
                            m_Root.AddChild(newBall);
                            m_Balls.push_back(newBall); 
                        }
                        m_PendingExtraBalls = 0; 

                        // 移動與生成階段：先讓所有磚塊下移，再生成新的一排
                        if (m_LevelManager) {
                            bool isDead = false;
                            float deathLineY = -230.0f; 
                            
                            for (auto& obj : allObjects) {
                                if (obj) {
                                    auto pos = obj->GetPosition();
                                    float newY = pos.y - 70.0f;
                                    obj->SetPosition(glm::vec2(pos.x, newY));
                                    obj->m_RowIndex += 1;
                                    obj->RefreshInvisibleVisual();

                                    if (!obj->IsProp() && newY <= deathLineY) {
                                        isDead = true;
                                    }
                                }
                            }

                            // 所有磚塊一般下移完畢後，再讓 DOUBLE_DROP 處理一次額外下落
                            m_LevelManager->ProcessDoubleDrop();
                            
                            // DOUBLE_DROP 額外下落後，再檢查一次死亡線
                            for (auto& obj : allObjects) {
                                if (obj && !obj->IsProp()
                                    && obj->GetType() != Brick::BrickType::LEVEL10_BOSS
                                    && obj->GetType() != Brick::BrickType::LEVEL15_BOSS
                                    && obj->GetType() != Brick::BrickType::LEVEL20_BOSS
                                    && !obj->m_IsBossSpike) {
                                    if (obj->GetPosition().y <= deathLineY) {
                                        isDead = true;
                                        break;
                                    }
                                }
                            }

                            if (isDead) {
                                LOG_INFO("磚塊觸底 Game Over ");
                                m_State = GameState::GAME_OVER;
                                m_GameOverTimer = 0.0f; 
                            } else {
                                // 如果沒死，生成新的一排
                                m_LevelManager->SpawnRowEndless(m_CurrentRound);
                            }
                        }
                    }
                    // 玩家瞄準與發射系統
                    else {
                        glm::vec2 firstBallPos = m_Balls[0]->GetPosition();

                        // 動作 A：按下
                        if (!propClicked && Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB)) {
                            m_IsAiming = true;
                            m_DragStart = mousePos; 
                        }

                        // 動作 B：拖曳 (畫虛線)
                        if (m_IsAiming && Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
                            glm::vec2 dragDir = m_DragStart - mousePos; 
                            if (glm::length(dragDir) > 10.0f) {
                                glm::vec2 normDir = glm::normalize(dragDir);
                                if (normDir.y > 0.05f) {
                                    float totalLength = 600.0f; 
                                    int dotCount = 30;          
                                    float spacing = totalLength / dotCount; 
                                    for (int i = 1; i <= dotCount; ++i) {
                                        glm::vec2 dotPos = m_NextLaunchPos + normDir * (i * spacing);
                                        if (m_AimDot) { 
                                            m_AimDot->m_Transform.translation = dotPos;
                                            m_AimDot->Draw();
                                        }
                                    }
                                }
                            }
                        }

                        // 動作 C：放開
                        if (m_IsAiming && Util::Input::IsKeyUp(Util::Keycode::MOUSE_LB)) {
                            m_IsAiming = false;
                            glm::vec2 dragDir = m_DragStart - mousePos;
                            if (glm::length(dragDir) > 10.0f && glm::normalize(dragDir).y > 0.05f) {
                                m_FireDirection = glm::normalize(dragDir);

                                // 如果有買彩虹球：只發射一顆彩虹球，且這一發不算回合結束
                                if (m_HasBoughtRainbow) {
                                    auto rainbow = std::make_shared<Ball>(RESOURCE_DIR "/Image/Rainbow_Ball.png");
                                    rainbow->SetPosition(m_NextLaunchPos);
                                    rainbow->SetVelocity(m_FireDirection * 800.0f);
                                    rainbow->SetZIndex(10);
                                    rainbow->SetRainbow(true);     // 標記為彩虹球，碰到磚塊直接消滅、不反彈
                                    m_Root.AddChild(rainbow);
                                    m_Balls.push_back(rainbow);    // 加進球清單，physics 才會處理它
                                    rainbow->m_HitBricks.clear();

                                    m_HasBoughtRainbow = false;
                                }
                                // 沒買彩虹球：照舊全彈匣連射
                                else {
                                    m_IsFiringSequence = true;
                                    m_BallsFiredCount = 0;
                                    m_FireTimer = 0.1f;
                                    m_IsRoundActive = true;
                                    m_IsFirstBallReturned = false;
                                    m_RoundStartPos = m_NextLaunchPos;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        case GameState::GAME_OVER: {
            // 依然畫出原本的遊戲畫面
            if (m_InGameBG) m_InGameBG->Draw();
            if (m_Character) m_Character->Draw();
            if (m_LevelManager) m_LevelManager->Draw();

            // 雷射特效（在磚塊上、球下）
            for (auto& fx : m_LaserEffects) {
                if (fx) fx->Draw();
            }
            // 死亡特效
            for (auto& fx : m_Effects) {
                if (fx) fx->Draw();
            }

            for (auto& ball : m_Balls) {
                if (ball != nullptr) ball->Draw();
            }
            
            // 在遮罩之前畫 UI，否則會被半透明黑蓋住
            if (m_CoinTextObj)  m_CoinTextObj->Draw();
            if (m_RoundTextObj) m_RoundTextObj->Draw();
            if (m_TopTextObj)   m_TopTextObj->Draw();

            // 畫出 GAMEOVER 文字
            if (m_GameOverBG) m_GameOverBG->Draw();
            if (m_GameOverText) m_GameOverText->Draw();

            // 3 秒倒數計時
            m_GameOverTimer += 0.016f; // 每幀約 16 毫秒
            
            if (m_GameOverTimer >= 3.0f) {
                LOG_INFO("3 秒結束，返回主畫面...");
                
                // 切換回主畫面
                m_State = GameState::MAIN_MENU; 

                LOG_INFO("重新開始新局...");
                ResetGame(); // 執行大掃除並重新開始
            }
            break;
        }
        case GameState::GAMEPLAY_LEVEL: {
            // 通關倒數
            if (m_LevelAccomplishedTimer > 0.0f) {
                m_LevelAccomplishedTimer -= 0.016f;
                if (m_LevelAccomplishedTimer <= 0.0f) {
                    m_LevelAccomplishedObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
                    m_State = GameState::BALL_SELECTION;   // 跳回選關
                }
            }
            if (m_LevelAccomplishedObj) m_LevelAccomplishedObj->Draw();
            
           // 物理系統
            UpdateBallsAndCollisions(0.016f);

            // 畫出背景與角色
            if (m_InGameBG) m_InGameBG->Draw();
            if (m_Character) {
                m_Character->m_Transform.translation = glm::vec2(45.0f, -240.0f);
                m_Character->Draw(); 
            }
            if (m_LevelManager) m_LevelManager->Draw();

            // 雷射特效（在磚塊上、球下）
            for (auto& fx : m_LaserEffects) {
                if (fx) fx->Draw();
            }
            // 死亡特效
            for (auto& fx : m_Effects) {
                if (fx) fx->Draw();
            }

            for (auto& ball : m_Balls) {
                if (ball != nullptr) ball->Draw();
            }
            if (m_CoinTextObj)  m_CoinTextObj->Draw();
            if (m_RoundTextObj) m_RoundTextObj->Draw();
            if (m_TopTextObj)   m_TopTextObj->Draw();

            // 繪製
            if (m_BtnRecycle)     m_BtnRecycle->Draw();
            if (m_BtnSpeedUpIn)   m_BtnSpeedUpIn->Draw();
            if (m_BtnSpeedUpOut)  m_BtnSpeedUpOut->Draw();

            // 加速按鈕外層旋轉（慢=60 deg/s、快=180 deg/s）
            float rotSpeed = m_IsSpeedUp ? 180.0f : 60.0f;
            m_SpeedUpRotation += rotSpeed * 0.016f;
            if (m_BtnSpeedUpOut) {
                m_BtnSpeedUpOut->m_Transform.rotation = glm::radians(m_SpeedUpRotation);
            }

            // 道具按鈕：先畫，再偵測點擊
            if (m_BtnDrillProp)            m_BtnDrillProp->Draw();
            if (m_BtnHorizontalLaserProp)  m_BtnHorizontalLaserProp->Draw();
            if (m_BtnRainbowProp)          m_BtnRainbowProp->Draw();
            if (m_BtnTntProp)              m_BtnTntProp->Draw();

            bool propClicked = false;

            if (!m_IsDraggingTnt) {
                if (IsButtonClicked(m_BtnDrillProp, mousePos, 80.0f, 80.0f)) {
                    propClicked = true;
                    if (m_Coins < 5) {
                        ShowWarning("Not enough coins");
                    } else {
                        bool success = m_LevelManager->UseDrill(m_Root);
                        if (success) {
                            m_Coins -= 5;
                            int col = m_LevelManager->GetLastDrillTargetCol();
                            if (col >= 0) SpawnLaserEffectVertical(col, 32.0f, "fx_laser_purple.png");
                        } else {
                            ShowWarning("No more bricks to break");
                        }
                    }
                }
                else if (IsButtonClicked(m_BtnHorizontalLaserProp, mousePos, 80.0f, 80.0f)) {
                    propClicked = true;
                    if (m_Coins < 4) {
                        ShowWarning("Not enough coins");
                    } else {
                        bool success = m_LevelManager->UseHorizontalLaser(m_Root);
                        if (success) {
                            m_Coins -= 4;
                            int row = m_LevelManager->GetLastLaserTargetRow();
                            if (row >= 0) SpawnLaserEffectHorizontal(row, 32.0f, "fx_laser_purple.png");
                        } else {
                            ShowWarning("No more bricks to break");
                        }
                    }
                }
                else if (IsButtonClicked(m_BtnRainbowProp, mousePos, 80.0f, 80.0f)) {
                    propClicked = true;
                    if (m_HasBoughtRainbow) ShowWarning("You already bought it");
                    else if (m_Coins < 8)   ShowWarning("Not enough coins");
                    else { m_Coins -= 8;    m_HasBoughtRainbow = true; }
                }
                else if (IsButtonClicked(m_BtnTntProp, mousePos, 80.0f, 80.0f)) {
                    propClicked = true;
                    if (m_Coins < 5) ShowWarning("Not enough coins");
                    else             m_IsDraggingTnt = true;
                }
                else if (IsButtonClicked(m_BtnRecycle, mousePos, 216.0f, 60.0f)) {
                    propClicked = true;
                    RecycleBalls();
                }
                else if (IsButtonClicked(m_BtnSpeedUpOut, mousePos, 60.0f, 60.0f)) {
                    propClicked = true;
                    float scale = m_IsSpeedUp ? (1.0f / 3.0f) : 3.0f;
                    for (auto& ball : m_Balls) {
                        if (ball && ball->IsActive() && glm::length(ball->GetVelocity()) > 0.01f) {
                            ball->SetVelocity(ball->GetVelocity() * scale);
                        }
                    }
                    m_IsSpeedUp = !m_IsSpeedUp;
                }
            }

            if (m_WarningTimer > 0.0f) {
                m_WarningTimer -= 0.016f;
                if (m_WarningTimer <= 0.0f && m_WarningObj) {
                    m_WarningObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
                }
            }
            if (m_WarningObj) m_WarningObj->Draw();

            RefreshTopUI();

            if (m_IsDraggingTnt) {
                m_DragTntIcon->m_Transform.translation = mousePos;
                m_DragTntIcon->Draw();

                if (Util::Input::IsKeyUp(Util::Keycode::MOUSE_LB)) {
                    m_IsDraggingTnt = false;
                    m_DragTntIcon->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);

                    int row = -1, col = -1;
                    bool ok = m_LevelManager->TryPlaceBomb(mousePos, row, col);

                    if (ok) {
                        m_Coins -= 5;
                    } else {
                        ShowWarning("Choose empty slot");
                    }
                }
            }
            
            // 判定是否正在連發中？
            if (m_IsFiringSequence) {
                m_FireTimer += 0.016f;
                float fireInterval = m_IsSpeedUp ? 0.016f : 0.08f;
                float ballSpeed    = m_IsSpeedUp ? 2400.0f : 800.0f;

                if (m_FireTimer >= fireInterval) {
                    m_FireTimer = 0.0f;
                    if (m_BallsFiredCount < (int)m_Balls.size()) {
                        auto& ball = m_Balls[m_BallsFiredCount];
                        if (ball->IsActive()) {
                            ball->SetPosition(m_RoundStartPos);
                            ball->SetVelocity(m_FireDirection * ballSpeed);
                        }
                        m_BallsFiredCount++;
                    } else {
                        m_IsFiringSequence = false;
                    }
                }
            }
            else {
                for (auto it = m_Balls.begin(); it != m_Balls.end(); ) {
                    if (*it && (*it)->IsRainbow()
                        && glm::length((*it)->GetVelocity()) < 0.01f) {
                        m_Root.RemoveChild(*it);
                        it = m_Balls.erase(it);
                    } else {
                        ++it;
                    }
                }

                bool allBallsStopped = true;
                for (auto& ball : m_Balls) {
                    if (!ball->IsActive()) continue;       
                    if (glm::length(ball->GetVelocity()) > 0.01f) {
                        allBallsStopped = false;
                        break;
                    }
                }
                if (allBallsStopped && !m_Balls.empty()) {
                    if (m_IsRoundActive) {

                        m_IsRoundActive = false; // 關閉戰鬥狀態
                        m_IsSpeedUp = false;        // 每回合重置加速狀態
                        m_CurrentRound++;        // 回合數 +1 
                        
                        if (m_State == GameState::GAMEPLAY_LEVEL
                            && m_LevelManager->IsLevelCleared(m_CurrentRound)) {
                            m_LevelAccomplishedObj->m_Transform.translation = glm::vec2(0.0f, 0.0f);
                            m_LevelAccomplishedTimer = 2.0f;
                        }

                        {
                            auto& allBricks = m_LevelManager->GetBricks();
                            for (auto& b : allBricks) {
                                if (b) b->OnRoundEnd();
                            }
                            m_LevelManager->OnLevelRoundEnd(m_CurrentRound);
                        }

                        // 尖刺殺掉的球下回合復活
                        for (auto& b : m_Balls) {
                            if (b && !b->IsActive()) {
                                b->SetActive(true);
                                b->SetPosition(m_NextLaunchPos);
                                b->SetVelocity(glm::vec2(0.0f, 0.0f));
                            }
                        }

                        auto& allObjects = m_LevelManager->GetBricks();
                        for (auto it = allObjects.begin(); it != allObjects.end(); ) {
                            if ((*it) && (*it)->IsProp() && (*it)->IsActivated()) {
                                m_Root.RemoveChild(*it);
                                it = allObjects.erase(it); // 從陣列中移除
                            } else {
                                it++;
                            }
                        }

                        for (int i = 0; i < m_PendingExtraBalls; ++i) {
                            auto newBall = std::make_shared<Ball>(RESOURCE_DIR "/Image/NormalBall.png");
                            
                            newBall->SetPosition(m_NextLaunchPos); 
                            newBall->SetVelocity(glm::vec2(0.0f, 0.0f));
                            newBall->SetZIndex(10);
                            
                            m_Root.AddChild(newBall);
                            m_Balls.push_back(newBall); 
                        }
                        m_PendingExtraBalls = 0; 

                        if (m_LevelManager) {
                            bool isDead = false;
                            float deathLineY = -230.0f; 
                            
                            for (auto& obj : allObjects) {
                                if (obj) {
                                    if (obj->m_IsBossSpike) {
                                        continue;
                                    }

                                    auto pos = obj->GetPosition();
                                    float newY = pos.y - 70.0f;
                                    obj->SetPosition(glm::vec2(pos.x, newY));
                                    obj->m_RowIndex += 1;
                                    obj->RefreshInvisibleVisual();

                                    if (!obj->IsProp() && newY <= deathLineY) {
                                        isDead = true;
                                    }
                                }
                            }

                            m_LevelManager->ProcessDoubleDrop();
                            
                            for (auto& obj : allObjects) {
                                if (obj && !obj->IsProp()
                                    && obj->GetType() != Brick::BrickType::LEVEL10_BOSS
                                    && obj->GetType() != Brick::BrickType::LEVEL15_BOSS
                                    && obj->GetType() != Brick::BrickType::LEVEL20_BOSS
                                    && !obj->m_IsBossSpike) {
                                    if (obj->GetPosition().y <= deathLineY) {
                                        isDead = true;
                                        break;
                                    }
                                }
                            }
                            
                            if (isDead) {
                                m_State = GameState::GAME_OVER;
                                m_GameOverTimer = 0.0f; 
                            } else {
                                if (m_State == GameState::GAMEPLAY_LEVEL) {
                                    m_LevelManager->SpawnRowLevel(m_CurrentRound);
                                } else {
                                    m_LevelManager->SpawnRowEndless(m_CurrentRound);
                                }
                            }
                        }
                    }

                    else {
                        glm::vec2 firstBallPos = m_Balls[0]->GetPosition();

                        // 動作 A：按下
                        if (!propClicked && Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB)) {
                            m_IsAiming = true;
                            m_DragStart = mousePos; 
                        }

                        // 動作 B：拖曳 (畫虛線)
                        if (m_IsAiming && Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
                            glm::vec2 dragDir = m_DragStart - mousePos; 
                            if (glm::length(dragDir) > 10.0f) {
                                glm::vec2 normDir = glm::normalize(dragDir);
                                if (normDir.y > 0.05f) {
                                    float totalLength = 600.0f; 
                                    int dotCount = 30;          
                                    float spacing = totalLength / dotCount; 
                                    for (int i = 1; i <= dotCount; ++i) {
                                        glm::vec2 dotPos = m_NextLaunchPos + normDir * (i * spacing);
                                        if (m_AimDot) { 
                                            m_AimDot->m_Transform.translation = dotPos;
                                            m_AimDot->Draw();
                                        }
                                    }
                                }
                            }
                        }

                        // 動作 C：放開
                        if (m_IsAiming && Util::Input::IsKeyUp(Util::Keycode::MOUSE_LB)) {
                            m_IsAiming = false;
                            glm::vec2 dragDir = m_DragStart - mousePos;
                            if (glm::length(dragDir) > 10.0f && glm::normalize(dragDir).y > 0.05f) {
                                m_FireDirection = glm::normalize(dragDir);

                                if (m_HasBoughtRainbow) {
                                    auto rainbow = std::make_shared<Ball>(RESOURCE_DIR "/Image/Rainbow_Ball.png");
                                    rainbow->SetPosition(m_NextLaunchPos);
                                    rainbow->SetVelocity(m_FireDirection * 800.0f);
                                    rainbow->SetZIndex(10);
                                    rainbow->SetRainbow(true);     
                                    m_Root.AddChild(rainbow);
                                    m_Balls.push_back(rainbow);    

                                    m_HasBoughtRainbow = false;
                                }

                                else {
                                    m_IsFiringSequence = true;
                                    m_BallsFiredCount = 0;
                                    m_FireTimer = 0.1f;
                                    m_IsRoundActive = true;
                                    m_IsFirstBallReturned = false;
                                    m_RoundStartPos = m_NextLaunchPos;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case GameState::BALL_SELECTION: {
            if (m_MainMenuBG) m_MainMenuBG->Draw();
            
            for (auto& b : m_LevelButtons)     if (b) b->Draw();
            for (auto& t : m_LevelButtonTexts) if (t) t->Draw();
            
            // 點擊偵測
            for (int i = 0; i < (int)m_LevelButtons.size(); ++i) {
                if (IsButtonClicked(m_LevelButtons[i], mousePos, 60.0f, 60.0f)) {
                    m_SelectedLevel = i + 1;

                    if (HasLevelIntro(m_SelectedLevel)) {
                        ShowLevelIntro(m_SelectedLevel);
                        m_State = GameState::LEVEL_INTRO;
                    } else {
                        EnterLevel(m_SelectedLevel);
                    }
                    break;
                }
            }
            break;
        }

        case GameState::LEVEL_INTRO: {
            if (m_LevelIntroObj) m_LevelIntroObj->Draw();

            // 「開始挑戰」按鈕區域（右下角）
            if (Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB)) {
                glm::vec2 btnCenter(145.0f, -350.0f);
                float bw = 220.0f, bh = 60.0f;
                if (mousePos.x >= btnCenter.x - bw/2 && mousePos.x <= btnCenter.x + bw/2 &&
                    mousePos.y >= btnCenter.y - bh/2 && mousePos.y <= btnCenter.y + bh/2) {
                    EnterLevel(m_SelectedLevel);
                }
            }
            break;
        }

        case GameState::PROPS_INTRO: {
            if (m_PropsIntroObj) m_PropsIntroObj->Draw();

            // 「開始挑戰」按鈕區域（右下角）
            if (Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB)) {
                glm::vec2 btnCenter(145.0f, -350.0f);
                float bw = 220.0f, bh = 60.0f;
                if (mousePos.x >= btnCenter.x - bw/2 && mousePos.x <= btnCenter.x + bw/2 &&
                    mousePos.y >= btnCenter.y - bh/2 && mousePos.y <= btnCenter.y + bh/2) {
                    // 隱藏介紹
                    if (m_PropsIntroObj) {
                        m_PropsIntroObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
                    }
                    // 切換到對應的遊戲狀態
                    m_State = (m_SelectedLevel == 0)
                            ? GameState::GAMEPLAY_ENDLESS
                            : GameState::GAMEPLAY_LEVEL;
                }
            }
            break;
        }
    }
}