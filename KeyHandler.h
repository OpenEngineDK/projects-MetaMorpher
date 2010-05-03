// Key handler.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _KEY_HANDLER_H_
#define _KEY_HANDLER_H_


// inherits from
#include <Core/IListener.h>
#include <Display/IFrame.h>
#include <Devices/IKeyboard.h>
//#include <Renderers/OpenGL/CompositeCanvas.h>


using namespace OpenEngine::Display;
using namespace OpenEngine::Core;
using namespace OpenEngine::Devices;
//using namespace OpenEngine::Renderers::OpenGL;

class KeyHandler : public IListener<KeyboardEventArg> {
private:
    IFrame& frame;
    //CompositeCanvas& split;
public:
    KeyHandler(IFrame& frame/*, CompositeCanvas& split*/);
    virtual ~KeyHandler();

    void Handle(KeyboardEventArg arg);
};

#endif // _KEY_HANDLER_H_
