// Key handler.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

// inherits from
#include "KeyHandler.h"

KeyHandler::KeyHandler(IFrame& frame, CompositeCanvas& split)
  : frame(frame)
  , split(split) {}

KeyHandler::~KeyHandler() {}

void KeyHandler::Handle(KeyboardEventArg arg) {
    if(arg.type == EVENT_PRESS) {
        if (arg.sym == 'f') frame.ToggleOption(FRAME_FULLSCREEN);
       
        if (arg.sym >= KEY_1 && arg.sym <= KEY_9) split.ToggleMaximize(arg.sym - KEY_1);
        
        if (arg.sym == 'a') split.SetColumns(split.GetColumns()+1);
        if (arg.sym == 'z') {
            unsigned int i = split.GetColumns()-1;
            if (i == 0) i = 1;
            split.SetColumns(i);
        }
    }
}

