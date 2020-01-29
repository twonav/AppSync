#include "stdafx.h"
#include "AppSyncFileClient.h"
#include <string.h>
#include <string>
#include "registra_gps.h"

#include "AppSyncFTNDPacket.h"
#include "AppSyncFTNCPacket.h"
#include "AppSyncFTWCPacket.h"

#define LOG_TAG "PERIPHERAL: FR: "

#define ACK_CTRL_PACKET_PERIOD				64

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

	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;

	bool decoded = packet.Decode(data, bytes, bytesLen);
	if(decoded) {
		incomingChunks = data.numPackets;
		incomingFileCrc32 = data.crc32;
		incomingFileName = data.fileName;
		state = AppSyncFileClient::State::incoming_file_request;
	}
	else {
		std::string log = LOG_TAG + std::string("Incorrect Message Type");
		RegistraGPS(log.c_str(), true);
		state = AppSyncFileClient::State::packet_error;
	}

	// TODO: OnEvent!!!

	return decoded;
}

bool AppSyncFileClient::ProcessDataPacket(const uint8_t* bytes, uint16_t bytesLen) {
	std::string error;

	TAppSyncFTNDPacket packet;
	TAppSyncFTNDPacket::FTNDData data;

	bool decoded = packet.Decode(data, bytes, bytesLen);
	if(decoded) {
		uint32_t expectedPacketNum = (lastPacketRecieved + 1);
		if(data.packetNumber == (expectedPacketNum & 0xFF)) {
			incomingFileBuffer.insert(incomingFileBuffer.end(),
									  &data.fileBytes[0],
									  &data.fileBytes[data.fileBytesLen - 1]);
			lastPacketRecieved = expectedPacketNum;

			if(incomingChunks - 1 == lastPacketRecieved) {
				state = AppSyncFileClient::State::file_complete;
			}
		}
	}
	else {
		std::string log = LOG_TAG + error;
		RegistraGPS(log.c_str(), true);
		state = AppSyncFileClient::State::rollback;
	}

	// TODO: OnEvent!!!

	return decoded;
}

void AppSyncFileClient::Update()
{
	switch(state) {
		case AppSyncFileClient::State::stopped:
			break;
		case AppSyncFileClient::State::incoming_file_request:
			state = AppSyncFileClient::State::recieving_file;
			AuthorizeTransfer();
			break;
		case AppSyncFileClient::State::recieving_file: {
			if(lastPacketRecieved % ACK_CTRL_PACKET_PERIOD == 0) {
				ContinueTransfer(lastPacketRecieved);
			}
			break;
		}
		case AppSyncFileClient::State::rollback:
			RollbackTransfer(lastPacketRecieved +1);
			break;
		case AppSyncFileClient::State::packet_error:
			StopTransfer();
			Reset();
			break;
		case AppSyncFileClient::State::file_complete:
			CompleteTransfer(lastPacketRecieved + 1);
			Reset();
			break;
	}

}

void AppSyncFileClient::Reset() {
	incomingFileBuffer.clear();
	incomingFileName.clear();
	incomingFileCrc32 = 0;
	state = AppSyncFileClient::State::stopped;
}

void AppSyncFileClient::AuthorizeTransfer() {
	TAppSyncFTWCPacket packet;
	packet.EncodeAuthorizeTransfer(frameToSend);
	// TODO: Send Packet
}

void AppSyncFileClient::ContinueTransfer(uint32_t confirmedPacket) {
	TAppSyncFTWCPacket packet;
	packet.EncodeContinueTransfer(frameToSend, confirmedPacket);
	// TODO: Send Packet
}

void AppSyncFileClient::RollbackTransfer(uint32_t packetNumber) {
	TAppSyncFTWCPacket packet;
	packet.EncodeRollback(frameToSend, packetNumber);
	// TODO: Send Packet
}

void AppSyncFileClient::StopTransfer() {
	TAppSyncFTWCPacket packet;
	packet.EncodeStopTransfer(frameToSend, TAppSyncFTWCPacket::Reason::success);
	// TODO: Send Packet
}

void AppSyncFileClient::CompleteTransfer(uint32_t totalChunksRecieved) {
	TAppSyncFTWCPacket packet;
	packet.EncodeTransferComplete(frameToSend, totalChunksRecieved);
	// TODO: Send Packet
}



