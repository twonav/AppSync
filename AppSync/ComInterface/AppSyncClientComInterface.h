#ifndef AppSyncClientComInterfaceH
#define AppSyncClientComInterfaceH

#include <stdint.h>

namespace Ble
{
	class TAppSyncClientComInterface
	{
		public:
			typedef void(*OnPacketRecieved)(void* context,
											  const uint8_t* bytes,
											  uint16_t len);

			typedef void(*OnPacketSent)(void* context, bool success);

			virtual ~TAppSyncClientComInterface(){}
			virtual void RegisterOnCtrlPacketRcvdCb(OnPacketRecieved onCtrlPacketRcvd,
													void* context) = 0;
			virtual void RegisterOnDataPacketRcvdCb(OnPacketRecieved onDataPacketRcvd,
													void* context) = 0;

			virtual void SendCtrlPacket(OnPacketSent onCtrlPacketSent,
										void* context,
										uint8_t* bytes,
										uint16_t len) = 0;


	};
};


#endif
