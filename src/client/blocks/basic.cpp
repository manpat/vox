#include "blocks/basic.h"

void TestDynamicBlock::OnPlace(){
	Log("TestDynamicBlock") << "OnPlace Client";
}
void TestDynamicBlock::OnBreak(){
	Log("TestDynamicBlock") << "OnBreak Client";
}
void TestDynamicBlock::OnInteract(){
	Log("TestDynamicBlock") << "OnInteract Client";
}
