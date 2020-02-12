#ifndef AppSyncServerComBtStackH
#define AppSyncServerComBtStackH

#include "btstack/BtStackAdapter.h"
#include "AppSyncServerComInterface.h"

namespace Ble
{
	class TAppSyncServerComBtStack : public TAppSyncServerComInterface
	{
		public:
			TAppSyncServerComBtStack(btstack::TBtStackAdapter& adapter);
			virtual ~TAppSyncServerComBtStack();

			virtual void RegisterOnCtrlPacketRcvdCb(OnCtrlPacketRcvdCb onCtrlPacketRcvd, void *context);
			virtual void SendDataPacket(OnPacketSent onDataPacketSent, void *context, uint8_t *bytes, uint16_t len);
			virtual void SendCtrlPacket(OnPacketSent onCtrlPacketSent, void *context, uint8_t *bytes, uint16_t len);

		private:
			btstack::TBtStackAdapter& adapter;
	};
}

#endif
