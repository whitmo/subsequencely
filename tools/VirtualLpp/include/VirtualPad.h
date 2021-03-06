
#pragma once

#include <string>
#include <map>

#include "cinder/app/App.h"
#include "cinder/Cinder.h"
#include "cinder/Font.h"
#include "cinder/GeomIo.h"
#include "cinder/gl/gl.h"
#include "glm/gtx/component_wise.hpp"

#include "seq.h"

using namespace std;
using namespace ci;
using namespace glm;

class VirtualPad
{
public:
    static Color defaultColor;
    static Font font;
    static gl::TextureFontRef textureFont;
    static gl::GlslProgRef prog;
    static gl::BatchRef rectBatch;
    static gl::BatchRef circleBatch;
    
    VirtualPad(const char* l=nullptr);
    
    void draw(float x, float y, float w, float h);
    
    Color getColor() { return color; }
    void setColor(Color c) { color = c; }
    
    void press(u8 v, bool aftertouch);
    
    u8 getIndex() { return index; }
    void setIndex(u8 i) { index = i; }
    
    bool isHeld() { return held; }
    
    int getVelocity() { return velocity; }
    
private:
    u8 index;
    
    const string label;
    bool isButton;
    Color color;
    float brightness;
    
    bool held;
    u8 velocity;
};
