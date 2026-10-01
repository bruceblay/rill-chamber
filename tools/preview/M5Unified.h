// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
// Host adapter for the playing-screen artwork. Uses M5GFX's actual bitmap fonts.
#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#define PROGMEM
struct GFXglyph { uint16_t bitmapOffset; uint8_t width,height,xAdvance; int8_t xOffset,yOffset; };
struct GFXfont { uint8_t* bitmap; GFXglyph* glyph; uint16_t first,last; uint8_t yAdvance; };
namespace fonts {
#include <GFXFF/FreeMono9pt7b.h>
#include <GFXFF/FreeMonoBold9pt7b.h>
#include <GFXFF/FreeSansBold12pt7b.h>
#include <GFXFF/FreeSansBold24pt7b.h>
inline const GFXfont Font0{};
// Menu-only Font2 is not supported by this playing-screen adapter.
inline const GFXfont Font2{};
}
namespace glcd {
#include <glcdfont.h>
}
enum { top_left, top_center, top_right };
struct Display {
  uint16_t color565(int r,int g,int b) { return ((r&248)<<8)|((g&252)<<3)|(b>>3); }
};
inline struct { Display Display; } M5;
class M5Canvas {
 public:
  void setFont(const GFXfont* f) { font_=f; }
  void setTextColor(uint16_t c,uint16_t =0) { ink_=c; }
  void setTextDatum(int d) { datum_=d; }
  void fillScreen(uint16_t c) { pixels_.fill(c); }
  void fillRect(int x,int y,int w,int h,uint16_t c) {
    for(int py=std::max(0,y);py<std::min(240,y+h);++py)
      for(int px=std::max(0,x);px<std::min(135,x+w);++px) pixels_[py*135+px]=c;
  }
  void drawFastHLine(int x,int y,int w,uint16_t c) { fillRect(x,y,w,1,c); }
  int fontHeight() const { return font_->bitmap?font_->yAdvance:8; }
  int textWidth(const char* text) const {
    int w=0;
    for(;*text;++text) {
      unsigned c=static_cast<unsigned char>(*text);
      if(!font_->bitmap) w+=6;
      else if(c>=font_->first&&c<=font_->last) w+=font_->glyph[c-font_->first].xAdvance;
    }
    return w;
  }
  void drawString(const char* text,int x,int y) {
    if(datum_==top_center)x-=textWidth(text)/2;
    if(datum_==top_right)x-=textWidth(text);
    int baseline=0;
    if(font_->bitmap)
      for(unsigned i=0;i<font_->last-font_->first;++i)baseline=std::max(baseline,-int(font_->glyph[i].yOffset));
    for(;*text;++text) {
      unsigned c=static_cast<unsigned char>(*text);
      if(!font_->bitmap) {
        for(int col=0;col<5;++col)for(int row=0;row<8;++row)
          if(glcd::font[c*5+col]&(1<<row))fillRect(x+col,y+row,1,1,ink_);
        x+=6;continue;
      }
      if(c<font_->first||c>font_->last)continue;
      const auto& g=font_->glyph[c-font_->first];
      for(unsigned row=0;row<g.height;++row)for(unsigned col=0;col<g.width;++col) {
        unsigned bit=row*g.width+col;
        if(font_->bitmap[g.bitmapOffset+bit/8]&(0x80>>(bit%8)))
          fillRect(x+g.xOffset+int(col),y+baseline+g.yOffset+int(row),1,1,ink_);
      }
      x+=g.xAdvance;
    }
  }
  void save(const char* path) const {
    FILE* f=std::fopen(path,"wb");if(!f)std::exit(1);
    std::fprintf(f,"P6\n135 240\n255\n");
    for(auto c:pixels_) {
      unsigned char rgb[]={static_cast<unsigned char>((c>>11)*255/31),static_cast<unsigned char>(((c>>5)&63)*255/63),static_cast<unsigned char>((c&31)*255/31)};
      std::fwrite(rgb,1,3,f);
    }
    std::fclose(f);
  }
 private:
  std::array<uint16_t,135*240> pixels_{};
  const GFXfont* font_=&fonts::Font0;
  uint16_t ink_=0xffff;
  int datum_=top_left;
};
