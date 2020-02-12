#include "stdafx.h"
#include "AppSyncFTNDPacket.h"
#include <string.h>
#include "AppSyncPacketChecksum.h"

using namespace Ble;

TAppSyncFTNDPacket::TAppSyncFTNDPacket()
{

}

void TAppSyncFTNDPacket::Encode(std::vector<uint8_t>& encodedData,
								const FTNDData& data)
{
	size_t packetSize = (data.fileBytes ? data.fileBytesLen : 0) + NON_DATA_BYTES_NUM;
	encodedData.clear();
	encodedData.resize(packetSize);

	encodedData[0] = data.packetNumber;
	if(data.fileBytes && data.fileBytesLen > 0) {
		memcpy(&encodedData[1], data.fileBytes, data.fileBytesLen);
	}
	encodedData[packetSize-1] = TAppSyncPacketChecksum().Calculate(&encodedData[0], packetSize);
}

bool TAppSyncFTNDPacket::Decode(FTNDData& data,
								const uint8_t* bytes,
								const size_t bytesLen)
{
	bool decoded {false};

	if(bytes != nullptr && bytesLen >= NON_DATA_BYTES_NUM) {
		bool chkOk = (TAppSyncPacketChecksum().Calculate(&bytes[0], bytesLen) == bytes[bytesLen-1]);
		if(chkOk) {
			data.packetNumber = bytes[0];
			data.fileBytesLen = bytesLen - NON_DATA_BYTES_NUM;
			data.fileBytes = data.fileBytesLen ? const_cast<uint8_t*>(&bytes[1]) : nullptr;
			decoded = true;
		}
	}

	return decoded;
}

