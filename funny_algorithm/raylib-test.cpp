#include "raylib.h"

#define Width 1600
#define Height 900
#define radius 50
#define velocity 1
int dir_X = 1;
int dir_Y = 1;

void move(float& circle_X, float& circle_Y)
{
    float new_X = circle_X + velocity * dir_X;
    float new_Y = circle_Y + velocity * dir_Y;

    float l = new_X - radius;
    float r = new_X + radius;
    float u = new_Y + radius;
    float d = new_Y - radius;


    if(r > Width) {dir_X = -1;}
    if(l < 0){dir_X = 1;}
    if(u > Height) {dir_Y = -1;}
    if(d < 0) {dir_Y = 1;}

    circle_X += dir_X * velocity;
    circle_Y += dir_Y * velocity;
        // 绘制圆
    DrawCircle(circle_X, circle_Y, radius, YELLOW);
}


int main()
{
    InitWindow(Width, Height, "first");
    
    // // 设置目标FPS（可选，但推荐）
    SetTargetFPS(120);
    
    float circle_X = Width / 2;
    float circle_Y = Height / 2;
    
    while(!WindowShouldClose())
    {
        // 开始绘制
        BeginDrawing();
        // 清除背景（如果没有这个，会保留上一帧的内容）
        ClearBackground(RAYWHITE);

        DrawFPS(10, Height - 20);
        move(circle_X, circle_Y);        
        // 或者使用自定义颜色：DrawCircle(Width / 2, Height / 2, radius, Color{255, 45, 23, 255});
        
        // 结束绘制
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}