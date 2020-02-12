#ifndef AppSyncClientComBtStackH
#define AppSyncClientComBtStackH

#include <stdint.h>
#include "AppSyncClientComInterface.h"
#include "btstack/BtStackDevice.h"

namespace Ble
{
	class TAppSyncClientComBtStack : public TAppSyncClientComInterface
	{
		public:
			TAppSyncClientComBtStack(btstack::TBtStackDevice& device);
			virtual ~TAppSyncClientComBtStack() override;
			virtual void RegisterOnCtrlPacketRcvdCb(OnPacketRecieved onCtrlPacketRcvd,
													void* context) override;
			virtual void RegisterOnDataPacketRcvdCb(OnPacketRecieved onDataPacketRcvd,
													void* context) override;

			virtual void SendCtrlPacket(OnPacketSent onCtrlPacketSent,
										void* context,
										uint8_t* bytes,
										uint16_t len) override;
		private:
			btstack::TBtStackDevice& device;

	};
};


#endif
