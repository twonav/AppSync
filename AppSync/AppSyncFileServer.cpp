#include "stdafx.h"
#include "AppSyncFileServer.h"
#include "registra_gps.h"
#include <sstream>
#include <iomanip>

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
			onEvent(completed, error, FT_REASON_SUCCESS);
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

		size_t framesize = bytesToSend + 2;
		frameToSend.resize(framesize); // 2 for ctrl bytes
		frameToSend[NOTIFICATION_DATA_PACKET_NUMBER_FIELD] = dataChunkNumber & 0xFF;
		memcpy(&frameToSend[NOTIFICATION_DATA_DATA_FIELD], &fileBytesToSend[bytesSent], bytesToSend);
		frameToSend[framesize-1] = TBleFileUtils::CalculateChecksum(&frameToSend[0], framesize);

		state = AppSyncFileServer::State::waiting_data_sent;
		adapter.sendGattServerFileData(OnGattServerFileDataSent,
									   this,
									   &frameToSend[0],
									   static_cast<uint16_t>(framesize));

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
	fileTransfer->onEvent(false , !success, success ? FT_REASON_SUCCESS : FT_DATA_NOT_SENT);
}

void AppSyncFileServer::OnGattServerFileDataSent(void* context, bool success) {
	AppSyncFileServer* fileTransfer = static_cast <AppSyncFileServer*>(context);
	success ? fileTransfer->Continue() : fileTransfer->Abort();
	fileTransfer->onEvent(false , !success, success ? FT_REASON_SUCCESS : FT_DATA_NOT_SENT);
}

void AppSyncFileServer::OnWriteCtrlRcvd(void* context, const uint8_t* bytes, uint16_t len) {
	AppSyncFileServer* fileTransfer = static_cast <AppSyncFileServer*>(context);

	bool completed = false;
	bool error = (bytes == nullptr);
	uint8_t reason = FT_BAD_DATA_CTRL_FORMAT;
	AppSyncFileServer::State newState = AppSyncFileServer::State::sending_data;

	if(!error) {
		std::stringstream log;
		log << "PERIPHERAL: GattServer: Len [" << std::to_string(len) << "] firstByte [";
		log << std::hex << std::setfill('0') << std::uppercase << std::setw(2)  << static_cast<int>(bytes[0]) << "]";
		RegistraGPS(log.str().c_str());

		bool check = fileTransfer->CheckWriteCtrlPacket(bytes, len);
		if(check) {
			uint8_t ackField = bytes[WRITE_CTRL_ACK_FIELD];
			uint8_t action = bytes[WRITE_CTRL_ACTION_FIELD];
			reason =  bytes[WRITE_CTRL_REASON_FIELD];
			memcpy(&fileTransfer->lastPacketConfirmed, &bytes[WRITE_CTRL_PACKET_NUMBER_FIELD], 4);

			switch(ackField) {
				case ACK_CODE:
				case NACK_CODE: {
					switch(action) {
						case ACTION_COMPLETED: {
							completed = true;
							newState = AppSyncFileServer::State::finished;
							break;
						}
						case ACTION_STOP: {
							error = true;
							newState = AppSyncFileServer::State::failed;
							break;
						}
						case ACTION_START: {
							newState = AppSyncFileServer::State::sending_data;
							break;
						}
						case ACTION_CONTINUE: {
							newState = AppSyncFileServer::State::sending_data;
							if(ackField == NACK_CODE) {
								error = false;
								fileTransfer->dataChunkNumber = fileTransfer->lastPacketConfirmed;
							}
							break;
						}
						default: {
							newState = AppSyncFileServer::State::failed;
							error = true;
							reason = FT_BAD_DATA_CTRL_FORMAT;
							break;
						}
					}
					break;
				}
				default: {
					newState = AppSyncFileServer::State::failed;
					error = true;
					reason = FT_BAD_DATA_CTRL_FORMAT;
					break;
				}
			}
		}
	}

	fileTransfer->state = newState;
	fileTransfer->onEvent(completed, error, reason);
}

bool AppSyncFileServer::CheckWriteCtrlPacket(const uint8_t* bytes, uint16_t len) {
	bool checkOk = (len == WRITE_CTRL_PACKET_LENGTH &&
					bytes[WRITE_CTRL_CHCKSM_FIELD] == TBleFileUtils::CalculateChecksum(bytes, len));

	return checkOk;
}

bool AppSyncFileServer::SendFileRequest()
{
	size_t filenameLen = strlen(fileNameToSend.data());
	size_t packetLen = NOTIFICAITON_CTRL_INCOMING_FILE_FIXED_LENGTH + filenameLen;

	if(packetLen > (mtu - NOTIFICATION_BLE_OVERHEAD)) {
		// FIXME
		RegistraGPS("PERIPHERAL: FT: Notification Ctrl over MTU. Unhandled case", true);
		return false;
	}

	state = AppSyncFileServer::State::waiting_confirmation;
	dataChunkNumber = 0;

	uint32_t nChunks = fileBytesToSend.size() / mtu;
	uint32_t crc32 = TBleFileUtils::Crc32(CRC32_INIT, &fileBytesToSend[0], fileBytesToSend.size());

	std::vector<uint8_t> frame;
	frame.resize(packetLen);
	frame[NOTIFICAITON_CTRL_MSG_TYPE_FIELD] = NOTIFICAITON_CTRL_FILE_TRANSFER_REQUEST;
	memcpy(&frame[NOTIFICAITON_CTRL_PACKET_NUMBER_FIELD], &nChunks, 4);
	memcpy(&frame[NOTIFICAITON_CTRL_CRC32_FIELD], &crc32, 4);
	memcpy(&frame[NOTIFICAITON_CTRL_FILENAME_LEN_FIELD], &filenameLen, 2);
	memcpy(&frame[NOTIFICAITON_CTRL_FILENAME_DATA_FIELD], fileNameToSend.data(), filenameLen);
	frame[NOTIFICAITON_CTRL_FILENAME_DATA_FIELD + filenameLen] = TBleFileUtils::CalculateChecksum(&frame[0], frame.size());

	adapter.sendGattServerFileCtrl(OnGattServerFileCtrlSent, this, &frame[0], static_cast<uint16_t>(frame.size()));

	return true;
}

void AppSyncFileServer::SendFileCancel()
{
	state = AppSyncFileServer::State::waiting_cancel_confirmation;
	dataChunkNumber = 0;

	std::vector<uint8_t> frame;
	frame.resize(2);
	frame[0] = NOTIFICAITON_CTRL_FILE_TRANSFER_CANCEL;
	frame[1] = TBleFileUtils::CalculateChecksum(&frame[0], static_cast<uint16_t>(frame.size()));
	adapter.sendGattServerFileCtrl(OnGattServerFileCtrlSent, this, &frame[0], static_cast<uint16_t>(frame.size()));
}
