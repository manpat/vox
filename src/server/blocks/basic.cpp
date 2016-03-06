#include "blocks/basic.h"

void TestDynamicBlock::OnPlace(u16 playerID){
	Log("TestDynamicBlock") << "OnPlace Server " << playerID;
}
void TestDynamicBlock::OnBreak(u16 playerID){
	Log("TestDynamicBlock") << "OnBreak Server " << playerID;
}
void TestDynamicBlock::OnInteract(u16 playerID){
	Log("TestDynamicBlock") << "OnInteract Server " << playerID;
}
