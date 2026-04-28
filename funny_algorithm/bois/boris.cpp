#include "raylib.h"
#include "drawArrow.h"
#include <iostream>
#include <bits/stdc++.h>
#include <cstdlib>  
#include <ctime>    
using namespace std;

#define N 200
#define sep_dis 20
#define group_dis 100 
#define bird_radius 5
#define sep_coff 5
#define co_coff 50

#define Width 1080
#define Height 720


struct bird
{
    Vector2 v;
    Vector2 pos;
    Vector2 align_v;
    Vector2 sep_v;
    Vector2 co_v;
    int dir_X ;
    int dir_Y ;

    bird()
    {
        v = {(rand() % 10 - 5) * 0.5f , (rand() % 10 - 5) * 0.5f };
        if(v.x < 0) dir_X = -1;
        else dir_X = 1;
        
        if(v.y < 0)dir_Y = -1;
        else dir_Y = 1;
        pos = {float(rand() % (Width - 1)) , float(rand() % (Height - 1))};

    }

    bird(float pos_x , float pos_y)
    {
        v = {(rand() % 8 - 4) * 0.25f , (rand() % 8 - 4) * 0.25f };
        if(v.x < 0) dir_X = -1;
        else dir_X = 1;
        
        if(v.y < 0)dir_Y = -1;
        else dir_Y = 1;
        pos = {pos_x , pos_y};
    }
};

vector<bird> birds;


float norm_distance(bird b0 , bird b1)
{
    return sqrt(pow(b0.pos.x - b1.pos.x , 2) + pow(b0.pos.y - b1.pos.y , 2));
}

// Vector2 diffadd(Vector2 b , bird b0 , bird b1)
// {
//     return {b.x + b0.pos.x - b1.pos.x , b.y + b0.pos.y - b1.pos.y};
// }

// Vector2 operator+ (const Vector2& b0 , const Vector2& b1)
// {
//     return {b0.x + b1.x , b0.y + b1.y};
// }

// Vector2 operator- (const Vector2& b0 ,const Vector2& b1)
// {
//     return {b0.x - b1.x , b0.y - b1.y};
// }


Vector2 Align(bird s_bird)  
{
    int group_n = 0;
    Vector2 align_v = {0 , 0};
    for(auto bird : birds)
    {
        if(norm_distance(s_bird , bird) < group_dis)
        {
            align_v = align_v + bird.v - s_bird.v;
            group_n++;
        }
    }
    
    if(group_n)
    {
        align_v = {align_v.x / group_n , align_v.y / group_n};
        return align_v;
    }

    return align_v; 
}



Vector2 Separate(bird s_bird) 
{
    int sep_count = 0;
    Vector2 sep_v = {0 , 0};
    for(auto bird : birds)
    {
        if(norm_distance(s_bird , bird) < sep_dis)
        {
            sep_v = sep_v + s_bird.v - bird.v;
            sep_count++;
        }
        if(sep_count)
            return {sep_v.x / sep_count / sep_coff  , sep_v.y / sep_count / sep_coff}; 
    }
    return {0, 0};
}

Vector2 Cohension(bird s_bird)  
{

    int group_n = 0;
    Vector2 co_p = {0 , 0};
    for(auto bird : birds)
    {
        if(norm_distance(bird , s_bird) < group_dis)
        {
            co_p = co_p + bird.pos;
            group_n++; 
        }
    }

    if(group_n)
    {
        co_p = {co_p.x / group_n , co_p.y / group_n};
    }

    co_p = co_p - s_bird.pos;

    return {co_p.x / co_coff , co_p.y / co_coff};
}

void move(bird& s_bird , Vector2 sep_v, Vector2 align_v , Vector2 co_v)  
{
    float theta = (rand() % 500 + 500) / 1000.0f;
    
    float v_x = s_bird.v.x + sep_v.x + align_v.x + co_v.x;
    float v_y = s_bird.v.y + sep_v.y + align_v.y + co_v.y;

    float  new_X = s_bird.pos.x + v_x;
    float new_Y = s_bird.pos.y + v_y;

    float l = new_X - bird_radius;
    float r = new_X + bird_radius;
    float u = new_Y + bird_radius;
    float d = new_Y - bird_radius;


    if(r > Width || l < 0) {s_bird.v.x *= -1; s_bird.pos.x += (s_bird.v.x + sep_v.x + align_v.x + co_v.x) * theta;}
    else{s_bird.pos.x += s_bird.v.x + sep_v.x + align_v.x + co_v.x;}
    if(u > Height || d < 0) { s_bird.v.y *= -1; s_bird.pos.y += (s_bird.v.y + sep_v.y + align_v.y + co_v.y) * (1 - theta);}
    else{s_bird.pos.y += s_bird.v.y+ sep_v.y + align_v.y + co_v.y;}

    v_x = s_bird.v.x + sep_v.x + align_v.x + co_v.x;
    v_y = s_bird.v.y + sep_v.y + align_v.y + co_v.y;

    auto norm_x = v_x * 20 / sqrt(v_x * v_x + v_y * v_y);
    auto norm_y = v_y * 20 / sqrt(v_x * v_x + v_y * v_y);
    Vector2 t = {norm_x , norm_y};
    ArrowRenderer* arrow = new ArrowRenderer(s_bird.pos , s_bird.pos + t , BLUE);
    arrow->Draw();

        // 绘制圆
    DrawCircle(s_bird.pos.x, s_bird.pos.y, bird_radius, RED);
}


void draw(vector<bird>& birds)
{

    
    while(!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawFPS(10, Height - 20);
        
        for(int i = 0; i < N; i++)
        {
            Vector2 align_v = Align(birds[i]);
            Vector2 sep_v = Separate(birds[i]);
            Vector2 co_v = Cohension(birds[i]);
            co_v = {co_v.x/ 50 , co_v.y / 50};
            move(birds[i] ,sep_v,align_v,co_v);
            
        }
        
        EndDrawing();
    }
}


int main()
{
    srand(time(nullptr));
    for(int i = 0; i < N; i++)
    {
        birds.emplace_back();
    }
    
 
    InitWindow(1600, 1200, "bois_algorithm");
    SetTargetFPS(120);
    
    draw(birds); 
    
    CloseWindow();
    return 0;
}