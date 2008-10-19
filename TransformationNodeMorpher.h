#ifndef _TRANSFORMATION_NODE_MORPHER_
#define _TRANSFORMATION_NODE_MORPHER_

#include <Math/Quaternion.h>
#include <Math/Vector.h>
#include <Scene/TransformationNode.h>

class TransformationNodeMorpher : public IMorpher<Scene::TransformationNode> {
 private:
  Scene::TransformationNode* current;

 public:
  TransformationNodeMorpher() { 
    current = new Scene::TransformationNode(); 
  }

  void Morph(Scene::TransformationNode* left,
	     Scene::TransformationNode* right,
	     float scaling) {
    Math::Vector<3,float> vFrom = left->GetPosition();
    Math::Vector<3,float> vTo = right->GetPosition();

    Math::Quaternion<float> qFrom = left->GetRotation();
    Math::Quaternion<float> qTo = right->GetRotation();

    Math::Vector<3,float> position = vFrom + (vTo-vFrom)*scaling;
    current->SetPosition(position);
    current->SetRotation(Math::Quaternion<float>(qFrom, qTo, scaling));
  }

  Scene::TransformationNode* GetObject() {
    return current;
  }
};

#endif // _TRANSFORMATION_NODE_MORPHER_


