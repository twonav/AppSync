#include "stdafx.h"
#include "AppSyncFileServer.h"
#include "registra_gps.h"
#include <sstream>
#include <iomanip>

#include "AppSyncFTNDPacket.h"
#include "AppSyncFTNCPacket.h"
#include "AppSyncFTWCPacket.h"
#include "AppSyncCrc32.h"

#define NOTIFICATION_BLE_OVERHEAD	3

using namespace Ble;
using namespace btstack;

AppSyncFileServer::AppSyncFileServer(TBtStackAdapter& adapter,
								   uint16_t mtu,
								   const std::string& filename,
								   std::vector<uint8_t>& bytes,
								   OnEventReceivedCb onEventReceived) :
	adapter(adapter),
	onEvent(onEventReceived)
{
	this->mtu = mtu;
	adapter.registerGattServerFileCtrlCb(OnWriteCtrlRcvd, this);
	state = AppSyncFileServer::State::stopped;
	dataChunkNumber = 0;
	fileNameToSend = filename;
	fileBytesToSend.swap(bytes);
}

AppSyncFileServer::~AppSyncFileServer()
{
	adapter.registerGattServerFileCtrlCb(nullptr, nullptr);
}


bool AppSyncFileServer::Start() {
	bool fileTransferStarted {false};

	if(state == AppSyncFileServer::State::stopped) {
		fileTransferStarted = SendFileRequest();
	}

	std::string log = "PERIPHERAL: FT: File Transfer Started ";
	log += (fileTransferStarted ? "YES" : "NO");
	RegistraGPS("PERIPHERAL: FT: File Transfer Started", true);

	return fileTransferStarted;
}

void AppSyncFileServer::Update() {
	switch(state) {
		case AppSyncFileServer::State::stopped:
		case AppSyncFileServer::State::failed:
		case AppSyncFileServer::State::finished:
		case AppSyncFileServer::State::waiting_confirmation:
		case AppSyncFileServer::State::waiting_cancel_confirmation:
		case AppSyncFileServer::State::waiting_data_sent:
		case AppSyncFileServer::State::waiting_transfer_complete:
			break;
		case AppSyncFileServer::State::sending_data:
			SendDataChunk();
			break;
		case AppSyncFileServer::State::transfer_complete: {
			bool completed = true;
			bool error = false;
			onEvent(completed, error);
			break;
		}
	}
}

void AppSyncFileServer::Cancel() {
	Abort();
}

void AppSyncFileServer::Abort() {
	RegistraGPS("PERIPHERAL: FT: File Transfer ABORTED", true);
	state = AppSyncFileServer::State::failed;
}

void AppSyncFileServer::Continue() {
	state = AppSyncFileServer::State::sending_data;
}

void AppSyncFileServer::SendDataChunk() {
	size_t maxframeSize = mtu - NOTIFICATION_BLE_OVERHEAD;
	size_t maxBytesToSend = maxframeSize -2;  // 2 for packet header and checksum
	size_t bytesSent =  std::min(dataChunkNumber * maxBytesToSend, fileBytesToSend.size());
	size_t remainingBytes = static_cast<uint16_t>(fileBytesToSend.size() - bytesSent);
	size_t bytesToSend = std::min(remainingBytes, maxBytesToSend);

	if(bytesToSend > 0) {
		std::string log = "PERIPHERAL: FT: Sending Data: ";
		log += std::to_string(bytesSent) + "/" + std::to_string(fileBytesToSend.size());
		RegistraGPS(log.c_str(), true);
		
		TAppSyncFTNDPacket packetToSend;
		TAppSyncFTNDPacket::FTNDData data;
		data.packetNumber = dataChunkNumber & 0xFF;
		data.fileBytes = &fileBytesToSend[bytesSent];
		data.fileBytesLen = bytesSent;
		packetToSend.Encode(frameToSend, data);

		state = AppSyncFileServer::State::waiting_data_sent;
		adapter.sendGattServerFileData(OnGattServerFileDataSent,
									   this,
									   &frameToSend[0],
									   static_cast<uint16_t>(frameToSend.size()));

		dataChunkNumber++;
	}
	else {
		RegistraGPS("PERIPHERAL: FT: File Transfer Completed. Waiting confirmation", true);
		state = AppSyncFileServer::State::waiting_transfer_complete;
	};
}

void AppSyncFileServer::OnGattServerFileCtrlSent(void* context, bool success) {
	AppSyncFileServer* fileTransfer = static_cast <AppSyncFileServer*>(context);
	//success ? fileTransfer->Continue() : fileTransfer->Abort(); // TODO: Remove!
	fileTransfer->onEvent(false , !success);
}

void AppSyncFileServer::OnGattServerFileDataSent(void* context, bool success) {
	AppSyncFileServer* fileTransfer = static_cast <AppSyncFileServer*>(context);
	success ? fileTransfer->Continue() : fileTransfer->Abort();
	fileTransfer->onEvent(false , !success);
}

void AppSyncFileServer::OnWriteCtrlRcvd(void* context, const uint8_t* bytes, uint16_t len) {
	AppSyncFileServer* fileTransfer = static_cast <AppSyncFileServer*>(context);

	AppSyncFileServer::State newState = AppSyncFileServer::State::sending_data;

	bool completed {false};
	TAppSyncFTWCPacket packet;
	TAppSyncFTWCPacket::FTWCData data;
	bool error = !packet.Decode(data, bytes, len);
	if(!error) {
		std::stringstream log;
		log << "PERIPHERAL: GattServer: Len [" << std::to_string(len) << "] firstByte [";
		log << std::hex << std::setfill('0') << std::uppercase << std::setw(2)  << static_cast<int>(bytes[0]) << "]";
		RegistraGPS(log.str().c_str());

		if(data.ack) {
			switch(data.action) {
				case TAppSyncFTWCPacket::Action::completed:
					completed = true;
					newState = AppSyncFileServer::State::finished;
					break;
				case TAppSyncFTWCPacket::Action::stop:
					error = true; // INFO: To Force error. Maybe not clear...
					newState = AppSyncFileServer::State::failed;
					break;
				case TAppSyncFTWCPacket::Action::start:
					newState = AppSyncFileServer::State::sending_data;
					break;
				case TAppSyncFTWCPacket::Action::continue_transfer:
					newState = AppSyncFileServer::State::sending_data;
					break;

			default:
				newState = AppSyncFileServer::State::failed;
				error = true;
				break;
			}
		}
		else {
			if(data.action == TAppSyncFTWCPacket::Action::continue_transfer) {
				error = true;
				fileTransfer->dataChunkNumber = fileTransfer->lastPacketConfirmed;
			}
		};
	}

	fileTransfer->state = newState;
	fileTransfer->onEvent(completed, error);
}

bool AppSyncFileServer::SendFileRequest() {
	state = AppSyncFileServer::State::waiting_confirmation;
	dataChunkNumber = 0;

	TAppSyncCrc32 crc32;
	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;
	data.action = TAppSyncFTNCPacket::Action::request;
	data.fileName = fileNameToSend;
	data.numPackets = fileBytesToSend.size() / mtu;
	data.crc32 = crc32.Calculate(crc32.CRC32_INIT,
								 &fileBytesToSend[0],
								 fileBytesToSend.size());

	packet.EncodeFileRequest(frameToSend, data);
	adapter.sendGattServerFileCtrl(OnGattServerFileCtrlSent,
								   this,
								   &frameToSend[0],
								   static_cast<uint16_t>(frameToSend.size()));

	return true;
}

void AppSyncFileServer::SendFileCancel() {
	state = AppSyncFileServer::State::waiting_cancel_confirmation;
	dataChunkNumber = 0;

	TAppSyncFTNCPacket packet;
	packet.EncodeFileCancel(frameToSend);
	adapter.sendGattServerFileCtrl(OnGattServerFileCtrlSent,
								   this,
								   &frameToSend[0],
								   static_cast<uint16_t>(frameToSend.size()));
}
