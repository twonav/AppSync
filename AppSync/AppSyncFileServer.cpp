#include "stdafx.h"
#include "AppSyncFileServer.h"
#include "registra_gps.h"
#include <sstream>
#include <iomanip>

#include "AppSyncServerComInterface.h"
#include "AppSyncFTNDPacket.h"
#include "AppSyncFTNCPacket.h"
#include "AppSyncFTWCPacket.h"
#include "AppSyncCrc32.h"
#include <cmath>

using namespace Ble;

TAppSyncFileServer::TAppSyncFileServer(std::unique_ptr<TAppSyncServerComInterface> comInterface,
									   OnEventReceivedCb onEventReceived) :
	comInterface(std::move(comInterface)),
	onEvent(onEventReceived)
{
	this->mtu = 23; // default MTU
	this->fileNameToSend = "";
	this->state = TAppSyncFileServer::State::stopped;
	this->numPacketsSent = 0;
	this->comInterface->RegisterOnCtrlPacketRcvdCb(OnWriteCtrlRcvd, this);
}

TAppSyncFileServer::~TAppSyncFileServer()
{
	comInterface->RegisterOnCtrlPacketRcvdCb(nullptr, nullptr);
}

bool TAppSyncFileServer::Start(const std::string& filename,
							   std::vector<uint8_t>& fileBytes,
							   uint16_t mtu)
{
	bool fileTransferStarted {false};

	if(state == TAppSyncFileServer::State::stopped) {
		this->mtu = mtu;
		this->fileNameToSend = filename;
		this->fileBytesToSend.swap(fileBytes);
		this->numPacketsSent = 0;

		SetState(TAppSyncFileServer::State::waiting_start_confirmation);
		SendFileRequest();
		fileTransferStarted = true;
	}

	std::string log = "PERIPHERAL: FT: File Transfer Started ";
	log += (fileTransferStarted ? "YES" : "NO");
	RegistraGPS(log.c_str(), true);

	return fileTransferStarted;
}

void TAppSyncFileServer::Update() {
	switch(state) {
		case TAppSyncFileServer::State::stopped:
		case TAppSyncFileServer::State::waiting_start_confirmation:
		case TAppSyncFileServer::State::waiting_continue_confirmation:
		case TAppSyncFileServer::State::waiting_cancel_confirmation:
		case TAppSyncFileServer::State::failed:
		case TAppSyncFileServer::State::finished:
		case TAppSyncFileServer::State::waiting_data_sent:
		case TAppSyncFileServer::State::waiting_transfer_complete:
			break;
		case TAppSyncFileServer::State::sending_data:
			SendDataChunk();
			break;
		case TAppSyncFileServer::State::transfer_complete: {
			bool completed = true;
			bool error = false;
			onEvent(completed, error);
			SetState(TAppSyncFileServer::State::finished);
			break;
		}
	}
}

void TAppSyncFileServer::Cancel() {
	RegistraGPS("PERIPHERAL: FT: File Transfer Cancelled", true);
	Abort();
}

void TAppSyncFileServer::Abort() {
	RegistraGPS("PERIPHERAL: FT: File Transfer ABORTED", true);
	SetState(TAppSyncFileServer::State::failed);
}

void TAppSyncFileServer::Continue() {
	TAppSyncFileServer::State newState;
	numPacketsSent++;

	bool transferCompleted = (GetDataBytesPerPacket() * numPacketsSent >= fileBytesToSend.size());
	if(transferCompleted) {
		newState = TAppSyncFileServer::State::waiting_transfer_complete;
	}
	else {
		bool needsConfirmation = ((numPacketsSent % 256) == 0);
		newState = needsConfirmation ? TAppSyncFileServer::State::waiting_continue_confirmation :
									   TAppSyncFileServer::State::sending_data;
	};

	SetState(newState);
}

void TAppSyncFileServer::SendDataChunk() {
	auto logPacket = [](uint32_t packetToSend, size_t progressBytes, size_t totalBytes) {
		std::string log = "PERIPHERAL: FT: Sending Data: ";
		log += "Packet: " + std::to_string(packetToSend) + " -> Bytes: ";
		log += std::to_string(progressBytes) + "/" + std::to_string(totalBytes);
		RegistraGPS(log.c_str(), true);
	};

	size_t maxBytesToSend = GetDataBytesPerPacket();
	size_t fileSize = fileBytesToSend.size();
	size_t bytesSent =  std::min(numPacketsSent * maxBytesToSend, fileSize);
	size_t remainingBytes = fileSize - bytesSent;
	size_t bytesToSend = std::min(remainingBytes, maxBytesToSend);

	logPacket(numPacketsSent, bytesSent, fileSize);

	TAppSyncFTNDPacket packetToSend;
	TAppSyncFTNDPacket::FTNDData data;
	data.packetNumber = numPacketsSent & 0xFF;
	data.fileBytes = &fileBytesToSend[bytesSent];
	data.fileBytesLen = bytesToSend;
	packetToSend.Encode(packetBytesToSend, data);

	SetState(TAppSyncFileServer::State::waiting_data_sent);
	comInterface->SendDataPacket(OnGattServerFileDataSent,
								 this,
								 &packetBytesToSend[0],
			static_cast<uint16_t>(packetBytesToSend.size()));
}

void TAppSyncFileServer::OnCtrlPacketSent(void* context, bool success) {
	TAppSyncFileServer* fileTransfer = static_cast <TAppSyncFileServer*>(context);
	fileTransfer->onEvent(false , !success);
}

uint32_t TAppSyncFileServer::GetDataBytesPerPacket()
{
	return (mtu - TAppSyncFTNDPacket::NON_DATA_BYTES_NUM); // 2 for packet header and checksum
}

void TAppSyncFileServer::OnGattServerFileDataSent(void* context, bool success) {
	TAppSyncFileServer* fileTransfer = static_cast <TAppSyncFileServer*>(context);
	success ? fileTransfer->Continue() : fileTransfer->Abort();
	fileTransfer->onEvent(false , !success);
}

void TAppSyncFileServer::OnWriteCtrlRcvd(void* context, const uint8_t* bytes, uint16_t len) {
	TAppSyncFileServer* fileTransfer = static_cast <TAppSyncFileServer*>(context);

	TAppSyncFileServer::State newState = TAppSyncFileServer::State::failed;

	bool completed {false};
	TAppSyncFTWCPacket packet;
	TAppSyncFTWCPacket::FTWCData data;
	bool error = !packet.Decode(data, bytes, len);
	if(!error) {
		std::stringstream log;
		log << "PERIPHERAL: FT: Len [" << std::to_string(len) << "] firstByte [";
		log << std::hex << std::setfill('0') << std::uppercase << std::setw(2)  << static_cast<int>(bytes[0]) << "]";
		RegistraGPS(log.str().c_str());

		if(data.ack) {
			switch(data.action) {
				case TAppSyncFTWCPacket::Action::completed:
					RegistraGPS("PERIPHERAL: FT: TAppSyncFTWCPacket::Action::completed");
					completed = true;
					newState = TAppSyncFileServer::State::transfer_complete;
					break;
				case TAppSyncFTWCPacket::Action::stop:
					RegistraGPS("PERIPHERAL: FT: TAppSyncFTWCPacket::Action::stop");
					error = true; // INFO: To Force error. Maybe not clear...
					newState = TAppSyncFileServer::State::failed;
					break;
				case TAppSyncFTWCPacket::Action::start:
					RegistraGPS("PERIPHERAL: FT: TAppSyncFTWCPacket::Action::start");
					newState = TAppSyncFileServer::State::sending_data;
					break;
				case TAppSyncFTWCPacket::Action::continue_transfer:
					RegistraGPS("PERIPHERAL: FT: TAppSyncFTWCPacket::Action::continue_transfer");
					if(fileTransfer->state ==  TAppSyncFileServer::State::waiting_continue_confirmation) {
						newState = TAppSyncFileServer::State::sending_data;
					}
					else {
						newState = fileTransfer->state;
					}
					break;
			default:
				RegistraGPS("PERIPHERAL: FT: TAppSyncFTWCPacket::Action::unknown ACK ????");
				newState = TAppSyncFileServer::State::failed;
				error = true;
				break;
			}
		}
		else {
			if(data.action == TAppSyncFTWCPacket::Action::continue_transfer) {
				RegistraGPS("PERIPHERAL: FT: TAppSyncFTWCPacket::Action::Rollback");
				fileTransfer->numPacketsSent = data.packetNumber;
				newState = TAppSyncFileServer::State::sending_data;
			}
			else {
				RegistraGPS("PERIPHERAL: FT: TAppSyncFTWCPacket::Action::unknown NACK ????");
			}
		};
	}

	fileTransfer->SetState(newState);
	fileTransfer->onEvent(completed, error);
}

void TAppSyncFileServer::SendFileRequest() {
	TAppSyncCrc32 crc32;
	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;
	data.action = TAppSyncFTNCPacket::Action::request;
	data.fileName = fileNameToSend;
	data.numPackets = static_cast<uint32_t>(std::ceil(static_cast<float>(fileBytesToSend.size()) / mtu));
	data.crc32 = crc32.Calculate(crc32.CRC32_INIT,
								 &fileBytesToSend[0],
								 fileBytesToSend.size());

	packet.EncodeFileRequest(packetBytesToSend, data);
	comInterface->SendCtrlPacket(OnCtrlPacketSent,
								 this,
								 &packetBytesToSend[0],
			static_cast<uint16_t>(packetBytesToSend.size()));
}

void TAppSyncFileServer::SendFileCancel() {
	fileNameToSend = "";
	SetState(TAppSyncFileServer::State::stopped);
	numPacketsSent = 0;

	TAppSyncFTNCPacket packet;
	packet.EncodeFileCancel(packetBytesToSend);
	comInterface->SendCtrlPacket(OnCtrlPacketSent,
								 this,
								 &packetBytesToSend[0],
			static_cast<uint16_t>(packetBytesToSend.size()));
}

void TAppSyncFileServer::SetState(TAppSyncFileServer::State newState) {
	std::stringstream log;
	if(newState != state) {
		log << "PERIPHERAL: FT: State: " << StateToString(state) << " >> " << StateToString(newState);
		RegistraGPS(log.str().c_str());
		state = newState;
	}
}

const char* TAppSyncFileServer::StateToString(TAppSyncFileServer::State state) {
	switch(state) {
		case TAppSyncFileServer::State::stopped:						return "stopped";
		case TAppSyncFileServer::State::waiting_start_confirmation:		return "waiting_start_confirmation";
		case TAppSyncFileServer::State::waiting_continue_confirmation:	return "waiting_continue_confirmation";
		case TAppSyncFileServer::State::waiting_cancel_confirmation:	return "waiting_cancel_confirmation";
		case TAppSyncFileServer::State::failed:							return "failed";
		case TAppSyncFileServer::State::finished:						return "finished";
		case TAppSyncFileServer::State::waiting_data_sent:				return "waiting_data_sent";
		case TAppSyncFileServer::State::waiting_transfer_complete:		return "waiting_transfer_complete";
		case TAppSyncFileServer::State::sending_data:					return "sending_data";
		case TAppSyncFileServer::State::transfer_complete: 				return "transfer_complete";
	};
}
