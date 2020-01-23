#include "stdafx.h"
#include "AppSyncFileClient.h"
#include <string.h>
#include <string>
#include "registra_gps.h"

#define LOG_TAG "PERIPHERAL: FR: "

using namespace btstack;
using namespace Ble;


AppSyncFileClient::AppSyncFileClient()
{

}

AppSyncFileClient::~AppSyncFileClient()
{

}

void AppSyncFileClient::OnWriteCtrlSent(void* context, bool success) {
	AppSyncFileClient* reciever = static_cast <AppSyncFileClient*>(context);

	if(!success) {
		// TODO: Error? but.. what to do in this case?
	}
}


bool AppSyncFileClient::ProcessCtrlPacket(const uint8_t* bytes, uint16_t bytesLen) {
	std::string error;

	bool isValid = IsPacketValid(bytes, bytesLen, error);
	if(!isValid) {
		// TODO: Handle error anyway?
		std::string log = LOG_TAG + error;
		RegistraGPS(log.c_str(), true);
		return false;
	}

	bool processed = false;
	uint8_t msgType = bytes[NOTIFICAITON_CTRL_MSG_TYPE_FIELD];
	if(msgType == NOTIFICAITON_CTRL_FILE_TRANSFER_REQUEST &&
			bytesLen >= NOTIFICAITON_CTRL_INCOMING_FILE_FIXED_LENGTH)
	{
		size_t incomingFileNameLen;
		memcpy(&incomingChunks, &bytes[NOTIFICAITON_CTRL_PACKET_NUMBER_FIELD], 4);
		memcpy(&incomingFileCrc32, &bytes[NOTIFICAITON_CTRL_CRC32_FIELD], 4);
		memcpy(&incomingFileNameLen, &bytes[NOTIFICAITON_CTRL_FILENAME_LEN_FIELD], 2);
		incomingFileName = std::string(reinterpret_cast<const char*>(&bytes[NOTIFICAITON_CTRL_FILENAME_DATA_FIELD]),
									   incomingFileNameLen);
		state = AppSyncFileClient::State::incoming_file_request;
		processed = true;
	}
	else if(msgType == NOTIFICAITON_CTRL_FILE_TRANSFER_CANCEL)
	{
		// TODO: Abort file transfer
		Reset();
		processed = true;
	}
	else {
		// TODO: Handle incorrect message type?
		std::string log = LOG_TAG + std::string("Incorrect Message Type");
		RegistraGPS(log.c_str(), true);
	}

	return processed;
}

bool AppSyncFileClient::ProcessDataPacket(const uint8_t* bytes, uint16_t bytesLen) {
	std::string error;
	bool isValid = IsPacketValid(bytes, bytesLen, error);
	if(!isValid) {
		// TODO: Handle error anyway?
		std::string log = LOG_TAG + error;
		RegistraGPS(log.c_str(), true);
		return false;
	}

	uint32_t expectedChunkNum = (lastChunkRecieved + 1);
	uint8_t chunkNum = bytes[NOTIFICATION_DATA_PACKET_NUMBER_FIELD];
	if(chunkNum == (expectedChunkNum & 0xFF)) {
		if(chunkNum % ACK_CTRL_PACKET_PERIOD == 0) {
			// TODO: Send ACK Ctrl
		}
		incomingFileBuffer.insert(incomingFileBuffer.end(), &bytes[NOTIFICATION_DATA_DATA_FIELD], &bytes[bytesLen - 1]);
		lastChunkRecieved = expectedChunkNum;
	}
	else {
		// TODO: Send NACK + (recievedChunks + 1)
	};

	return true;
}

void AppSyncFileClient::Update()
{
	switch(state) {
		case AppSyncFileClient::State::stopped:
		case AppSyncFileClient::State::incoming_file_request:
			AuthorizeTransfer();
			break;
		case AppSyncFileClient::State::recieving_file:
		case AppSyncFileClient::State::packet_error:
		case AppSyncFileClient::State::file_complete:
			break;
	}

}

void AppSyncFileClient::Reset() {
	incomingFileBuffer.clear();
	incomingFileName.clear();
	incomingFileCrc32 = 0;
	state = AppSyncFileClient::State::stopped;
}

bool AppSyncFileClient::IsPacketValid(const uint8_t* bytes, uint16_t bytesLen, std::string& error) {
	const uint16_t minPacketLen = 2; // packet num + checksum

	bool valid = (bytesLen >= minPacketLen);
	if(!valid) {
		// TODO: invalid packet recieved. Abort?
		error = "Bad len";
		return false;
	}

	valid = (TBleFileUtils::CalculateChecksum(bytes, bytesLen) == bytes[bytesLen-1]);
	if(!valid) {
		// TODO: invalid chcksm recieved
		error = "Bad Chcksm";
		return false;
	}

	return valid;
}

std::vector<uint8_t> AppSyncFileClient::FormatCtrlPacket(bool ack, uint32_t chunk, uint8_t action, uint8_t reason) {
	std::vector<uint8_t> frame;

	frame.resize(WRITE_CTRL_PACKET_LENGTH);
	frame[WRITE_CTRL_ACK_FIELD] = ack? ACK_CODE : NACK_CODE;
	memcpy(&frame[WRITE_CTRL_PACKET_NUMBER_FIELD], &chunk, 4);
	frame[WRITE_CTRL_ACTION_FIELD] = action;
	frame[WRITE_CTRL_REASON_FIELD] = reason;
	frame[WRITE_CTRL_CHCKSM_FIELD] = TBleFileUtils::CalculateChecksum(&frame[0], frame.size());

	return frame;
}

void AppSyncFileClient::AuthorizeTransfer() {
	auto frame = FormatCtrlPacket(true, 0, ACTION_START, FT_REASON_SUCCESS);

	// TODO:
}

void AppSyncFileClient::ContinueTransmission(bool ack, uint32_t chunk) {
	auto frame = FormatCtrlPacket(ack, chunk, ACTION_CONTINUE, ack ? FT_REASON_SUCCESS : FT_DATA_NOT_SENT);
	// TODO:
}

void AppSyncFileClient::StopTransfer(uint8_t reason) {
	auto frame = FormatCtrlPacket(true, 0, ACTION_STOP, reason);
	// TODO:
}

void AppSyncFileClient::CompleteTransfer(uint32_t totalChunksRecieved) {
	auto frame = FormatCtrlPacket(true, totalChunksRecieved, ACTION_COMPLETED, FT_REASON_SUCCESS);
	// TODO:
}



