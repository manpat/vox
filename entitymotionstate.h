#ifndef ENTITYMOTIONSTATE_H
#define ENTITYMOTIONSTATE_H

#include <LinearMath/btMotionState.h>

struct Entity;

class EntityMotionState : public btMotionState {
public:
	// Ogre::SceneNode* sceneNode;

	EntityMotionState(Entity*);

	void getWorldTransform(btTransform& worldTrans) const override;
	void setWorldTransform(const btTransform& newTrans) override;
};


#endif