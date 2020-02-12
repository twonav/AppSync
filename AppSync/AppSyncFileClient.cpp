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

using namespace Ble;

TAppSyncFileClient::TAppSyncFileClient(std::unique_ptr<TAppSyncClientComInterface> comIface) :
	comIface(std::move(comIface))
{
	this->comIface->RegisterOnCtrlPacketRcvdCb(OnCtrlPacketRecieved, this);
	this->comIface->RegisterOnDataPacketRcvdCb(OnDataPacketRecieved,this);
}

TAppSyncFileClient::~TAppSyncFileClient()
{
}

void TAppSyncFileClient::OnWriteCtrlSent(void* context, bool success) {
	TAppSyncFileClient* reciever = static_cast <TAppSyncFileClient*>(context);

	if(!success) {
		// TODO: Error? but.. what to do in this case?
	}
}


bool TAppSyncFileClient::ProcessCtrlPacket(const uint8_t* bytes, uint16_t bytesLen) {
	std::string error;

	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;

	bool decoded = packet.Decode(data, bytes, bytesLen);
	if(decoded) {
		incomingChunks = data.numPackets;
		incomingFileCrc32 = data.crc32;
		incomingFileName = data.fileName;
		state = TAppSyncFileClient::State::incoming_file_request;
	}
	else {
		std::string log = LOG_TAG + std::string("Incorrect Message Type");
		RegistraGPS(log.c_str(), true);
		state = TAppSyncFileClient::State::packet_error;
	}

	// TODO: OnEvent!!!

	return decoded;
}

bool TAppSyncFileClient::ProcessDataPacket(const uint8_t* bytes, uint16_t bytesLen) {
	std::string error;

	TAppSyncFTNDPacket packet;
	TAppSyncFTNDPacket::FTNDData data;

	bool decoded = packet.Decode(data, bytes, bytesLen);
	if(decoded) {
		uint32_t expectedPacketNum = (lastPacketRecieved + 1);
		if(data.packetNumber == (expectedPacketNum & 0xFF)) {
			incomingFileBuffer.insert(incomingFileBuffer.end(),
									  &data.fileBytes[0],
									  &data.fileBytes[data.fileBytesLen]);
			lastPacketRecieved = expectedPacketNum;

			if(incomingChunks - 1 == lastPacketRecieved) {
				state = TAppSyncFileClient::State::file_complete;
			}
		}
	}
	else {
		std::string log = LOG_TAG + error;
		RegistraGPS(log.c_str(), true);
		state = TAppSyncFileClient::State::rollback;
	}

	// TODO: OnEvent!!!

	return decoded;
}

void TAppSyncFileClient::Update()
{
	switch(state) {
		case TAppSyncFileClient::State::stopped:
			break;
		case TAppSyncFileClient::State::incoming_file_request:
			state = TAppSyncFileClient::State::recieving_file;
			AuthorizeTransfer();
			break;
		case TAppSyncFileClient::State::recieving_file: {
			if(lastPacketRecieved % ACK_CTRL_PACKET_PERIOD == 0) {
				ContinueTransfer(lastPacketRecieved);
			}
			break;
		}
		case TAppSyncFileClient::State::rollback:
			RollbackTransfer(lastPacketRecieved +1);
			break;
		case TAppSyncFileClient::State::packet_error:
			StopTransfer();
			Reset();
			break;
		case TAppSyncFileClient::State::file_complete:
			CompleteTransfer(lastPacketRecieved + 1);
			Reset();
			break;
	}

}

void TAppSyncFileClient::OnCtrlPacketRecieved(void *context, const uint8_t *bytes, uint16_t len) {
	TAppSyncFileClient* fileClient = static_cast<TAppSyncFileClient*>(context);
	fileClient->ProcessCtrlPacket(bytes, len);
}

void TAppSyncFileClient::OnDataPacketRecieved(void *context, const uint8_t *bytes, uint16_t len) {
	TAppSyncFileClient* fileClient = static_cast<TAppSyncFileClient*>(context);
	fileClient->ProcessDataPacket(bytes, len);
}

void TAppSyncFileClient::Reset() {
	incomingFileBuffer.clear();
	incomingFileName.clear();
	incomingFileCrc32 = 0;
	state = TAppSyncFileClient::State::stopped;
}

void TAppSyncFileClient::AuthorizeTransfer() {
	TAppSyncFTWCPacket packet;
	packet.EncodeAuthorizeTransfer(frameToSend);
	// TODO: Send Packet
}

void TAppSyncFileClient::ContinueTransfer(uint32_t confirmedPacket) {
	TAppSyncFTWCPacket packet;
	packet.EncodeContinueTransfer(frameToSend, confirmedPacket);
	// TODO: Send Packet
}

void TAppSyncFileClient::RollbackTransfer(uint32_t packetNumber) {
	TAppSyncFTWCPacket packet;
	packet.EncodeRollback(frameToSend, packetNumber);
	// TODO: Send Packet
}

void TAppSyncFileClient::StopTransfer() {
	TAppSyncFTWCPacket packet;
	packet.EncodeStopTransfer(frameToSend, TAppSyncFTWCPacket::Reason::success);
	// TODO: Send Packet
}

void TAppSyncFileClient::CompleteTransfer(uint32_t totalChunksRecieved) {
	TAppSyncFTWCPacket packet;
	packet.EncodeTransferComplete(frameToSend, totalChunksRecieved);
	// TODO: Send Packet
}


