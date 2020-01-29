#include "stdafx.h"
#include "AppSyncFTNCPacket.h"
#include "AppSyncPacketChecksum.h"
#include <string.h>

#define NOTIFICAITON_CTRL_INCOMING_FILE_FIXED_LENGTH	12

#define NOTIFICAITON_CTRL_MSG_TYPE_FIELD			0
#define NOTIFICAITON_CTRL_PACKET_NUMBER_FIELD		1
#define NOTIFICAITON_CTRL_CRC32_FIELD				5
#define NOTIFICAITON_CTRL_FILENAME_LEN_FIELD		9
#define NOTIFICAITON_CTRL_FILENAME_DATA_FIELD		11

using namespace Ble;

TAppSyncFTNCPacket::TAppSyncFTNCPacket()
{
}

void TAppSyncFTNCPacket::EncodeFileRequest(std::vector<uint8_t> &encodedData,
										   const FTNCData& data)
{
	size_t filenameLen = data.fileName.length();
	size_t packetLen = NOTIFICAITON_CTRL_INCOMING_FILE_FIXED_LENGTH + filenameLen;

	encodedData.clear();
	encodedData.resize(packetLen);
	encodedData[NOTIFICAITON_CTRL_MSG_TYPE_FIELD] = static_cast<uint8_t>(Action::request);
	memcpy(&encodedData[NOTIFICAITON_CTRL_PACKET_NUMBER_FIELD], &data.numPackets, 4);
	memcpy(&encodedData[NOTIFICAITON_CTRL_CRC32_FIELD], &data.crc32, 4);
	memcpy(&encodedData[NOTIFICAITON_CTRL_FILENAME_LEN_FIELD], &filenameLen, 2);
	if(filenameLen > 0) {
		memcpy(&encodedData[NOTIFICAITON_CTRL_FILENAME_DATA_FIELD], data.fileName.data(), filenameLen);
	}
	encodedData[NOTIFICAITON_CTRL_FILENAME_DATA_FIELD + filenameLen] = TAppSyncPacketChecksum().Calculate(&encodedData[0], encodedData.size());

}

void TAppSyncFTNCPacket::EncodeFileCancel(std::vector<uint8_t> &encodedData)
{
	encodedData.clear();
	encodedData.resize(NOTIFICAITON_CTRL_INCOMING_FILE_FIXED_LENGTH);
	encodedData[0] = static_cast<uint8_t>(Action::cancel);
	memset(&encodedData[1], 0, 10);
	encodedData[NOTIFICAITON_CTRL_INCOMING_FILE_FIXED_LENGTH - 1] = TAppSyncPacketChecksum().Calculate(&encodedData[0], static_cast<uint16_t>(encodedData.size()));
}

bool TAppSyncFTNCPacket::Decode(TAppSyncFTNCPacket::FTNCData &data,
								const uint8_t *packetBytes,
								const size_t bytesLen)
{
	auto getAction = [](uint8_t byte){
		switch(byte) {
			case 0x00: return Action::request;
			case 0x01: return Action::cancel;
			default:
				return Action::unknown;
		}
	};

	bool decoded = (packetBytes != nullptr) &&
				   (bytesLen >= NOTIFICAITON_CTRL_INCOMING_FILE_FIXED_LENGTH) &&
				   (TAppSyncPacketChecksum().Calculate(&packetBytes[0], bytesLen) == packetBytes[bytesLen-1]);
	if(decoded) {
		uint16_t fileNameLen;
		data.action = getAction(packetBytes[NOTIFICAITON_CTRL_MSG_TYPE_FIELD]);
		memcpy(&data.numPackets, &packetBytes[NOTIFICAITON_CTRL_PACKET_NUMBER_FIELD], 4);
		memcpy(&data.crc32, &packetBytes[NOTIFICAITON_CTRL_CRC32_FIELD], 4);
		memcpy(&fileNameLen, &packetBytes[NOTIFICAITON_CTRL_FILENAME_LEN_FIELD], 2);
		if(fileNameLen > 0) {
			data.fileName = std::string(reinterpret_cast<char const*>(&packetBytes[NOTIFICAITON_CTRL_FILENAME_DATA_FIELD]), fileNameLen);
		}
		else {
			data.fileName = "NoNamedFile";
		}
	}

	return decoded;
}
