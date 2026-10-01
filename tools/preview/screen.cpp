// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
// Render the real firmware's playing screen for the shared 3D model.
#include <cstdlib>
#include "Screen.h"
int main(int argc,char** argv) {
  if(argc!=2)return 2;
  unsigned index=0;
  for(;index<score::pieceCount;++index)
    if(std::strcmp(score::piece(index).title,"Contrapunctus I")==0)break;
  if(index==score::pieceCount)return 3;
  const auto& piece=score::piece(index);
  const auto& part=score::part(piece,0);
  constexpr double pulse=68;
  player::View view;
  view.active=true;view.part=0;view.flash=.25f;
  for(unsigned i=0;i<part.noteCount;++i)
    if(score::noteEvents[part.note+i].start<=pulse)view.step=i;
  M5Canvas canvas;
  screen::playing(canvas,piece,&view,1,true,pulse,0,67,false);
  canvas.save(argv[1]);
}
