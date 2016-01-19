#include "entitymotionstate.h"
#include "bullethelpers.h"
#include "entity.h"

EntityMotionState::EntityMotionState(Entity* e){
	if(!e) throw "Tried to create EntityMotionState with null entity";

	// sceneNode = e->ogreSceneNode;
}

void EntityMotionState::getWorldTransform(btTransform& worldTrans) const {
	// This gets called ONCE for non-kinematic bodies
	// It gets called every frame for kinematic bodies
	// auto pos = sceneNode->_getDerivedPosition();
	// auto ori = sceneNode->_getDerivedOrientation();
	worldTrans.setIdentity();
	// worldTrans.setOrigin(o2bt(pos));
	// worldTrans.setRotation(o2bt(ori));
}

void EntityMotionState::setWorldTransform(const btTransform& newTrans) {
	// auto ori = newTrans.getRotation();
	// auto pos = newTrans.getOrigin();
	// sceneNode->_setDerivedOrientation(bt2o(ori));
	// sceneNode->_setDerivedPosition(bt2o(pos));
}