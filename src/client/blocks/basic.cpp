#include "blocks/basic.h"

void TestDynamicBlock::OnPlace(u16 playerID){
	Log("TestDynamicBlock") << "OnPlace Client " << playerID;
}
void TestDynamicBlock::OnBreak(u16 playerID){
	Log("TestDynamicBlock") << "OnBreak Client " << playerID;
}
void TestDynamicBlock::OnInteract(u16 playerID){
	Log("TestDynamicBlock") << "OnInteract Client " << playerID;
}
