#include "blocks/basic.h"

void TestDynamicBlock::OnPlace(){
	Log("TestDynamicBlock") << "OnPlace Server";
}
void TestDynamicBlock::OnBreak(){
	Log("TestDynamicBlock") << "OnBreak Server";
}
void TestDynamicBlock::OnInteract(){
	Log("TestDynamicBlock") << "OnInteract Server";
}
