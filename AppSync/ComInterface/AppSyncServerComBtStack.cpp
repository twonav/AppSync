#include "stdafx.h"
#include "AppSyncServerComBtStack.h"

using namespace Ble;
using namespace btstack;

TAppSyncServerComBtStack::TAppSyncServerComBtStack(TBtStackAdapter &adapter) :
	adapter(adapter)
{
}

TAppSyncServerComBtStack::~TAppSyncServerComBtStack()
{
}

void TAppSyncServerComBtStack::RegisterOnCtrlPacketRcvdCb(OnCtrlPacketRcvdCb onCtrlPacketRcvd, void *context)
{
	adapter.registerGattServerFileCtrlCb(onCtrlPacketRcvd, context);
}

void TAppSyncServerComBtStack::SendDataPacket(OnPacketSent onDataPacketSent, void *context, uint8_t *bytes, uint16_t len)
{
	adapter.sendGattServerFileData(onDataPacketSent, context, bytes, len);
}

void TAppSyncServerComBtStack::SendCtrlPacket(OnPacketSent onCtrlPacketSent, void *context, uint8_t *bytes, uint16_t len)
{
	adapter.sendGattServerFileData(onCtrlPacketSent, context, bytes, len);
}

