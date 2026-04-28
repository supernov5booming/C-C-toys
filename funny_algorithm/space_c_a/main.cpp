#include "raylib.h"
#include "raymath.h"
#include <bits/stdc++.h>

#define WIDTH 1500
#define HEIGHT 1200

#define leaf_radius 12
#define attr_radius 1000
#define leaf_amout 8
#define kill_dis 15
#define attr_coff 1
#define bone_radius 15



typedef struct{
    Vector2 pos;
    Color color;
    int index;
    
}leaf;

typedef struct{
    Vector2 pos;
    std::vector<leaf> neibor_leaf;
    Color color;
}bone,*B;


std::vector<leaf> leaves = {
    {{30 , 590} , RED ,0},
    {{200 , 340} , RED , 1},
    {{126 , 120} , RED , 2},
    {{543 , 520} , RED , 3},
    {{230 , 910} , RED , 4},
    {{782 , 790} , RED , 5},
    {{901 , 290} , RED , 6},
    {{614 , 670} , RED , 7},
};


std::vector<bone> bones;


void draw_Growpoint(Vector2 pos)
{
    DrawCircle(pos.x , pos.y , bone_radius , BLUE);
}

void grow()
{
    // 第一步：为每个叶子找到最近的骨骼
    std::vector<bone*> leaf_owner(leaves.size(), nullptr);
    std::vector<float> leaf_min_dis(leaves.size(), attr_radius + 1);
    
    for(int i = 0; i < leaves.size(); i++)
    {
        for(auto& bone : bones)
        {
            float dis = Vector2Distance(bone.pos, leaves[i].pos);
            if(dis <= leaf_min_dis[i] && dis <= attr_radius)
            {
                leaf_min_dis[i] = dis;
                leaf_owner[i] = &bone;
            }
        }
    }
    
    // 第二步：清空neibor_leaf并为每个骨骼添加它拥有的叶子
    for(auto& bone : bones)
        bone.neibor_leaf.clear();
        
    for(int i = 0; i < leaves.size(); i++)
    {
        if(leaf_owner[i])
        {
            leaf_owner[i]->neibor_leaf.push_back(leaves[i]);
        }
    }
    
    // 第三步：收集需要处理的骨骼（去重）
    std::vector<bone*> up_bone;
    for(int i = 0; i < leaves.size(); i++)
    {
        if(leaf_owner[i])
            up_bone.push_back(leaf_owner[i]);
    }
    std::sort(up_bone.begin(), up_bone.end());
    up_bone.erase(std::unique(up_bone.begin(), up_bone.end()), up_bone.end());
    
    // 第四步：骨骼生长和叶子删除
    std::vector<bool> leaf_should_remove(leaves.size(), false);
    std::vector<bone> new_bones;  // 存储新骨骼，避免在遍历时修改bones
    
    for(auto& b : up_bone)
    {
        bone& bone_ref = *b;
        Vector2 sum = {0, 0};
        
        if(!bone_ref.neibor_leaf.empty())
        {
            // 计算生长方向
            for(const auto& leaf : bone_ref.neibor_leaf)
            {
                Vector2 dir = Vector2Subtract(leaf.pos, bone_ref.pos);
                float distance = Vector2Length(dir);
                
                if(distance > 0)
                {
                    dir = Vector2Normalize(dir);
                    sum.x += dir.x * attr_coff;
                    sum.y += dir.y * attr_coff;
                }
            }
            
            // 生长 - 但不要移动原始骨骼！
            Vector2 grow_pos = bone_ref.pos;  // 从原始位置开始
            if(sum.x != 0 || sum.y != 0)
            {
                sum = Vector2Normalize(sum);
                grow_pos.x += sum.x * 2 * bone_radius;
                grow_pos.y += sum.y * 2 * bone_radius;
            }
            
            // 创建新骨骼 - 使用计算出的生长位置
            bone new_bone;
            new_bone.pos = grow_pos;
            new_bone.color = BLUE;
            new_bones.push_back(new_bone);
            draw_Growpoint(new_bone.pos);
            
            // 检查是否需要删除叶子
            for(int i = 0; i < leaves.size(); i++)
            {
                if(leaf_owner[i] == b)
                {
                    float dis = Vector2Distance(grow_pos, leaves[i].pos);
                    if(dis <= kill_dis)
                    {
                        leaf_should_remove[i] = true;
                    }
                }
            }
        }
    }
    
    // 第五步：添加所有新骨骼
    for(const auto& new_bone : new_bones)
    {
        bones.push_back(new_bone);
    }
    
    // 第六步：删除标记的叶子（从后往前）
    for(int i = leaves.size() - 1; i >= 0; i--)
    {
        if(leaf_should_remove[i])
        {
            leaves.erase(leaves.begin() + i);
        }
    }
}


void perish()
{

}

int main()
{


    bone newBone;
    newBone.pos = {WIDTH / 2, HEIGHT / 2};
    newBone.color = BLUE;
    bones.push_back(newBone);

    InitWindow(WIDTH, HEIGHT, "TREE");
    
    SetTargetFPS(10);
    while(!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(WHITE);
        for(auto leaf : leaves)
        {
            Vector2 pos = leaf.pos;
            DrawCircle(pos.x ,pos.y ,leaf_radius , leaf.color);
        }

        for(auto bone : bones)
        {
            DrawCircle(bone.pos.x, bone.pos.y, bone_radius, GRAY);
        }

        if(IsKeyPressed(KEY_G))
            grow();

        // if(IsKeyPressed(KEY_P))
        //     perish();
        
        EndDrawing();
    }
    
    CloseWindow();  
    
    return 0;
}