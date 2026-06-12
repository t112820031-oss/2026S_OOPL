#include "App.hpp"
#include "Core/Context.hpp"

int main(int, char**) {
    // 取得 Context 單例，這通常會初始化 SDL 視窗和 OpenGL 內容
    auto context = Core::Context::GetInstance(); 

    App app;
    app.Start(); 
    app.Init();  

    while (!context->GetExit()) {
        
        app.Update(); 
        
        // 這個 Update 負責交換緩衝區 (Swap Buffers)，將畫好的東西顯示到螢幕上
        context->Update(); 
    }

    return 0;
}