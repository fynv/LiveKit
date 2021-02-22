#pragma once
#include <string>

extern std::string MAPPING_NAME;
extern int VIDEO_WIDTH;
extern int VIDEO_HEIGHT;
#define DATA_SIZE (VIDEO_WIDTH*VIDEO_HEIGHT * 3)

void LoadRegistry();


