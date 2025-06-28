#define NOMINMAX
#include "max.h"
#include <array>
#include <thunder2d.h>
#include <vector>

std::vector<std::array<max::vec2<float>, 4>> parse_sheet(max::vec2<float> texture_size,max::vec2<float> cell_size,bool mirrorX){
    std::vector<std::array<max::vec2<float>, 4>> rtrn{};
    int cols = texture_size.x/cell_size.x;
    int rows = texture_size.y/cell_size.y;

    int cellWidth = cell_size.x;
    int cellHeight = cell_size.y;
    int sheetWidth = texture_size.x;
    int sheetHeight = texture_size.y;

    int tile_count = cols*rows; 


    for (int i = 0; i < tile_count; i++)
    {
    int col = i % cols;
    int row = i / cols;
    
    float x0 = (col * cellWidth) / (float)sheetWidth;
    float y0 = 1.0f - ((row + 1) * cellHeight) / (float)sheetHeight;
    
    float x1 = ((col + 1) * cellWidth) / (float)sheetWidth;
    float y1 = 1.0f - (row * cellHeight) / (float)sheetHeight;

    if(mirrorX){
    std::array<max::vec2<float>, 4> arr = {
    max::vec2<float>{x1,y0},
    max::vec2<float>{x0,y0},
    max::vec2<float>{x0,y1},
    max::vec2<float>{x1,y1}
    };
    rtrn.push_back(arr);
    }else{
    std::array<max::vec2<float>, 4> arr = {
    max::vec2<float>{x0,y0},
    max::vec2<float>{x1,y0},
    max::vec2<float>{x1,y1},
    max::vec2<float>{x0,y1}
    };
    rtrn.push_back(arr);
    }
    
    }
    return rtrn;
}
