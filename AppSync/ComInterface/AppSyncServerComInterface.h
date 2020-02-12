#ifndef AppSyncServerComInterfaceH
#define AppSyncServerComInterfaceH

#include <stdint.h>

namespace Ble
{
	class TAppSyncServerComInterface
	{
		public:
			typedef void(*OnCtrlPacketRcvdCb)(void* context,
											  const uint8_t* bytes,
											  uint16_t len);

			typedef void(*OnPacketSent)(void* context, bool success);

			virtual ~TAppSyncServerComInterface(){}
			virtual void RegisterOnCtrlPacketRcvdCb(OnCtrlPacketRcvdCb onCtrlPacketRcvd,
													void* context) = 0;
			virtual void SendDataPacket(OnPacketSent onDataPacketSent,
										void* context,
										uint8_t* bytes,
										uint16_t len) = 0;
			virtual void SendCtrlPacket(OnPacketSent onCtrlPacketSent,
										void* context,
										uint8_t* bytes,
										uint16_t len) = 0;


	};
};


#endif
