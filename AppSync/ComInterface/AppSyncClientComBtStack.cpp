#include "stdafx.h"
#include "AppSyncClientComBtStack.h"

using namespace Ble;
using namespace btstack;

TAppSyncClientComBtStack::TAppSyncClientComBtStack(btstack::TBtStackDevice& device) :
	device(device)
{

}

TAppSyncClientComBtStack::~TAppSyncClientComBtStack()
{

}

void TAppSyncClientComBtStack::RegisterOnCtrlPacketRcvdCb(TAppSyncClientComInterface::OnPacketRecieved onCtrlPacketRcvd, void *context)
{

}

void TAppSyncClientComBtStack::RegisterOnDataPacketRcvdCb(TAppSyncClientComInterface::OnPacketRecieved onDataPacketRcvd, void *context)
{

}

void TAppSyncClientComBtStack::SendCtrlPacket(TAppSyncClientComInterface::OnPacketSent onCtrlPacketSent, void *context, uint8_t *bytes, uint16_t len)
{

}
