#ifndef _META_MORPHER_
#define _META_MORPHER_

#include <Core/EngineEvents.h>
#include <Core/IListener.h>
#include <Utils/Timer.h>
#include <list>

#include "IMorpher.h"

using namespace OpenEngine;

//@todo: cannot be inside class, because the class is templeted
enum EndType { NORMAL, LOOP, SCANNING };

template <class T>
class MetaMorpher : public Core::IListener<Core::ProcessEventArg> {
 protected:
  class KeyFrame {
  public:
    KeyFrame(T* obj, Utils::Time* time) {
      this->obj = obj;
      this->time = time;
    }
    Utils::Time* time; // offset for the keyframe
    T* obj;
  };

 public:

 MetaMorpher(IMorpher<T>* morpher, EndType endType) : endType(endType) {
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
      if(backwards) {
	if(*currentKeyFrame->time < *current->time) {
	  if (keyFrame == NULL)
	    keyFrame = currentKeyFrame;
	  else if (*keyFrame->time < *currentKeyFrame->time)
	    keyFrame = currentKeyFrame;
	} 
      } else
	if(*currentKeyFrame->time > *current->time) {
	  if (keyFrame == NULL)
	    keyFrame = currentKeyFrame;
	  else if (*keyFrame->time > *currentKeyFrame->time)
	    keyFrame = currentKeyFrame;
	}
    }

    float scaling = 0.0; // use from
    //logger.info << "going for: " << keyFrame->time->ToString() << logger.end;
    if (keyFrame == NULL) {
      if (endType == LOOP) {
	timer.Reset();
	scaling = 1.0; // use to
	current = keyFrame = *keyFrames.begin(); // to
	morpher->Morph(current->obj, keyFrame->obj, scaling);
      } else if (endType == SCANNING)
	backwards = !backwards;
      else
	return;
    } else {
      scaling = ((float)(now - *current->time).AsInt32())
	/((float)(*keyFrame->time - *current->time).AsInt32());

      morpher->Morph(current->obj, keyFrame->obj, scaling);
      current = new KeyFrame(morpher->GetObject(), new Utils::Time(now));
    }
  }

  void Add(T* obj, Utils::Time* time) {
    KeyFrame* keyFrame = new KeyFrame(obj,time);
    if (current == NULL)
      current = keyFrame;
    keyFrames.push_back(keyFrame);
    if (length < *time)
      length = *time;
    logger.info << "length: " << length << logger.end;
    //sort(keyFrames.begin(),keyFrames.end(),SortByTime());
  } 

  T* GetObject() {
    return morpher->GetObject();
  }

 private:
  std::list<KeyFrame*> keyFrames;
  
  KeyFrame* current;
  bool backwards;
  IMorpher<T>* morpher;
  Utils::Timer timer;
  EndType endType;
  Utils::Time length;
};
/*
struct SortByTime : public binary_function<KeyFrame*, KeyFrame*, bool> {
  bool operator()(KeyFrame* left, KeyFrame* right) {
    return (left->time < right->time);
  }
  };*/

#endif // _META_MORPHER_
