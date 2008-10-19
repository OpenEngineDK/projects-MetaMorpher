#ifndef _META_MORPHER_
#define _META_MORPHER_

#include <Core/EngineEvents.h>
#include <Core/IListener.h>
#include <Utils/Timer.h>
#include <list>

#include "IMorpher.h"

using namespace OpenEngine;

template <class T>
class MetaMorpher : public Core::IListener<Core::ProcessEventArg> {
private:
  class KeyFrame {
  public:
    KeyFrame(T* obj, Utils::Time* time) {
      this->obj = obj;
      this->time = time;
    }
    Utils::Time* time; // offset for the keyframe
    T* obj;
  };

  std::list<KeyFrame*> keyFrames;
  
  KeyFrame* current;
  bool backwards;
  IMorpher<T>* morpher;
  Utils::Timer timer;
  
 public:
  MetaMorpher(IMorpher<T>* morpher) {
    current = NULL;
    backwards = false;
    timer.Start();
    this->morpher = morpher;
  }

  void Handle(Core::ProcessEventArg arg) {
    Utils::Time now = timer.GetElapsedTime();
    KeyFrame* keyFrame = NULL;

    //@todo: binary search in the sorted list 
    typename std::list<KeyFrame*>::iterator itr = keyFrames.begin();
    for (; itr!=keyFrames.end(); itr++) {
      KeyFrame* currentKeyFrame = *itr;
      if(!backwards && *currentKeyFrame->time > *current->time) {
	if (keyFrame == NULL)
	  keyFrame = currentKeyFrame;
	else if (*keyFrame->time < *currentKeyFrame->time)
	  keyFrame = currentKeyFrame;
      }
      logger.info << "cKF: " << *currentKeyFrame->time
		  << " current: "<< *current->time << logger.end;
    }

    if (keyFrame == NULL) {
      logger.info << "keyFrame is NULL" << logger.end;
      return;
    }

    float scaling = ((float)(now - *current->time).AsInt32())
      /((float)(*keyFrame->time - *current->time).AsInt32());


    logger.info << "scaling: " << scaling << logger.end;
    morpher->Morph(current->obj, keyFrame->obj, scaling);
    current = new KeyFrame(morpher->GetObject(), new Utils::Time(now));
  }

  void Add(T* obj, Utils::Time* time) {
    KeyFrame* keyFrame = new KeyFrame(obj,time);
    if (current == NULL)
      current = keyFrame;
    keyFrames.push_back(keyFrame);
    //sort(keyFrames.begin(),keyFrames.end(),SortByTime());
  } 

  T* GetObject() {
    return morpher->GetObject();
  }
};
/*
struct SortByTime : public binary_function<KeyFrame*, KeyFrame*, bool> {
  bool operator()(KeyFrame* left, KeyFrame* right) {
    return (left->time < right->time);
  }
  };*/

#endif // _META_MORPHER_
