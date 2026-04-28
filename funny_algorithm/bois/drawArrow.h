#include "raylib.h"
#include "raymath.h"
#include <cmath>
class ArrowRenderer {
    private:
        Vector2 start, end;
        Color color;
        float lineThickness;
        float headSize;
        float headAngle;  // 箭头张角
        
    public:
        ArrowRenderer() 
            : start({0, 0}), end({0, 0}), color(RED),
              lineThickness(2.0f), headSize(10.0f), headAngle(PI/6) {}
        
        ArrowRenderer(Vector2 startPos, Vector2 endPos, Color col = RED)
            : start(startPos), end(endPos), color(col),
              lineThickness(2.0f), headSize(10.0f), headAngle(PI/6) {}
        
        // 设置属性
        void SetPositions(Vector2 startPos, Vector2 endPos) {
            start = startPos;
            end = endPos;
        }
        
        void SetColor(Color col) { color = col; }
        void SetThickness(float thick) { lineThickness = thick; }
        void SetHeadSize(float size) { headSize = size; }
        void SetHeadAngle(float angle) { headAngle = angle; }
        
        // 绘制箭头
        void Draw() const {
            // 绘制线条
            DrawLineEx(start, end, lineThickness, color);
            
            // 计算箭头方向
            Vector2 dir = Vector2Subtract(end, start);
            float length = Vector2Length(dir);
            
            if (length > 0) {
                // 归一化方向向量
                dir = Vector2Scale(dir, 1.0f / length);
                
                // 计算角度
                float angle = atan2f(dir.y, dir.x);
                
                // 箭头头部的两个点
                float leftAngle = angle + headAngle;
                float rightAngle = angle - headAngle;
                
                Vector2 left = {
                    end.x - headSize * cosf(leftAngle),
                    end.y - headSize * sinf(leftAngle)
                };
                
                Vector2 right = {
                    end.x - headSize * cosf(rightAngle),
                    end.y - headSize * sinf(rightAngle)
                };
                
                // 绘制箭头头部
                DrawTriangle(end, left, right, color);
            }
        }
        
        // 绘制带有渐变效果的箭头
        void DrawGradient(Color startColor, Color endColor) const {
            // 线条渐变
            
            // 箭头头部使用endColor
            Vector2 dir = Vector2Subtract(end, start);
            float length = Vector2Length(dir);
            
            if (length > 0) {
                dir = Vector2Scale(dir, 1.0f / length);
                float angle = atan2f(dir.y, dir.x);
                
                Vector2 left = {
                    end.x - headSize * cosf(angle - headAngle),
                    end.y - headSize * sinf(angle - headAngle)
                };
                
                Vector2 right = {
                    end.x - headSize * cosf(angle + headAngle),
                    end.y - headSize * sinf(angle + headAngle)
                };
                
                DrawTriangle(end, left, right, endColor);
            }
        }
        
        // 绘制虚线箭头
        void DrawDashed(float dashLength = 5.0f, float gapLength = 3.0f) const {
            Vector2 dir = Vector2Subtract(end, start);
            float totalLength = Vector2Length(dir);
            
            if (totalLength > 0) {
                dir = Vector2Scale(dir, 1.0f / totalLength);
                
                float traveled = 0;
                while (traveled < totalLength - headSize) {
                    float dashStart = traveled;
                    float dashEnd = fminf(dashStart + dashLength, totalLength - headSize);
                    
                    Vector2 dashStartPos = Vector2Add(start, Vector2Scale(dir, dashStart));
                    Vector2 dashEndPos = Vector2Add(start, Vector2Scale(dir, dashEnd));
                    
                    DrawLineEx(dashStartPos, dashEndPos, lineThickness, color);
                    
                    traveled = dashEnd + gapLength;
                }
                
                // 绘制箭头头部
                float angle = atan2f(dir.y, dir.x);
                Vector2 arrowBase = Vector2Add(start, Vector2Scale(dir, totalLength - headSize));
                
                Vector2 left = {
                    end.x - headSize * cosf(angle - headAngle),
                    end.y - headSize * sinf(angle - headAngle)
                };
                
                Vector2 right = {
                    end.x - headSize * cosf(angle + headAngle),
                    end.y - headSize * sinf(angle + headAngle)
                };
                
                DrawTriangle(end, left, right, color);
            }
        }
    };