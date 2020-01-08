#include "stdafx.h"
#include "BleFileTransfer.h"
#include "registra_gps.h"
#include <sstream>
#include <iomanip>

#define NOTIFICATION_BLE_OVERHEAD	3

using namespace Ble;
using namespace btstack;

#define NOTIFICAITON_CTRL_PACKET_FIXED_LENGTH				12

#define NOTIFICAITON_CTRL_MSG_TYPE_FIELD			0
#define NOTIFICAITON_CTRL_PACKET_NUMBER_FIELD		1
#define NOTIFICAITON_CTRL_CRC32_FIELD				5
#define NOTIFICAITON_CTRL_FILENAME_LEN_FIELD		9
#define NOTIFICAITON_CTRL_FILENAME_DATA_FIELD		11

#define NOTIFICAITON_CTRL_FILE_TRANSFER_REQUEST		0x00
#define NOTIFICAITON_CTRL_FILE_TRANSFER_CANCEL		0x01

#define NOTIFICATION_DATA_PACKET_NUMBER_FIELD	0
#define NOTIFICATION_DATA_DATA_FIELD			1


#define WRITE_CTRL_PACKET_LENGTH			6

#define WRITE_CTRL_ACK_FIELD				0
#define WRITE_CTRL_PACKET_NUMBER_FIELD		1
#define WRITE_CTRL_ACTION_FIELD				5
#define WRITE_CTRL_REASON_FIELD				6
#define WRITE_CTRL_CHCKSM_FIELD				7

#define ACTION_STOP						0x00
#define ACTION_START					0x01
#define ACTION_CONTINUE					0x02
#define ACTION_COMPLETED				0x03

#define ACK_CODE	0x06
#define NACK_CODE	0x15


#define CRC32_POLY 0xedb88320
#define CRC32_INIT 0


TBleFileTransfer::TBleFileTransfer(btstack::TBtStackAdapter& adapter,
								   uint16_t mtu,
								   const std::string& filename,
								   std::vector<uint8_t>& bytes,
								   OnEventReceivedCb onEventReceived) :
	adapter(adapter),
	onEvent(onEventReceived)
{
	this->mtu = mtu;
	adapter.registerGattServerFileCtrlCb(OnWriteCtrlRcvd, this);
	state = TBleFileTransfer::State::stopped;
	dataChunkNumber = 0;
	fileNameToSend = filename;
	fileBytesToSend.swap(bytes);
}

TBleFileTransfer::~TBleFileTransfer()
{
	adapter.registerGattServerFileCtrlCb(nullptr, nullptr);
}


bool TBleFileTransfer::Start() {
	bool fileTransferStarted {false};

	if(state == TBleFileTransfer::State::stopped) {
		fileTransferStarted = SendFileRequest();
	}

	std::string log = "PERIPHERAL: FT: File Transfer Started ";
	log += (fileTransferStarted ? "YES" : "NO");
	RegistraGPS("PERIPHERAL: FT: File Transfer Started", true);

	return fileTransferStarted;
}

void TBleFileTransfer::Update() {
	switch(state) {
		case TBleFileTransfer::State::stopped:
		case TBleFileTransfer::State::failed:
		case TBleFileTransfer::State::finished:
		case TBleFileTransfer::State::waiting_confirmation:
		case TBleFileTransfer::State::waiting_cancel_confirmation:
		case TBleFileTransfer::State::waiting_data_sent:
		case TBleFileTransfer::State::waiting_transfer_complete:
			break;
		case TBleFileTransfer::State::sending_data:
			SendDataChunk();
			break;
		case TBleFileTransfer::State::transfer_complete: {
			bool completed = true;
			bool error = false;
			onEvent(completed, error, FT_REASON_SUCCESS);
			break;
		}
	}
}

void TBleFileTransfer::Cancel() {
	Abort();
}

void TBleFileTransfer::Abort() {
	RegistraGPS("PERIPHERAL: FT: File Transfer ABORTED", true);
	state = TBleFileTransfer::State::failed;
}

void TBleFileTransfer::Continue() {
	state = TBleFileTransfer::State::sending_data;
}

void TBleFileTransfer::SendDataChunk() {
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
		frameToSend[framesize-1] = CalculateChecksum(&frameToSend[0], framesize);

		state = TBleFileTransfer::State::waiting_data_sent;
		adapter.sendGattServerFileData(OnGattServerFileDataSent,
									   this,
									   &frameToSend[0],
									   static_cast<uint16_t>(framesize));

		dataChunkNumber++;
	}
	else {
		RegistraGPS("PERIPHERAL: FT: File Transfer Completed. Waiting confirmation", true);
		state = TBleFileTransfer::State::waiting_transfer_complete;
	};
}

void TBleFileTransfer::OnGattServerFileCtrlSent(void* context, bool success) {
	TBleFileTransfer* fileTransfer = static_cast <TBleFileTransfer*>(context);
	success ? fileTransfer->Continue() : fileTransfer->Abort(); // TODO: Remove!
	fileTransfer->onEvent(false , !success, success ? FT_REASON_SUCCESS : FT_DATA_NOT_SENT);
}

void TBleFileTransfer::OnGattServerFileDataSent(void* context, bool success) {
	TBleFileTransfer* fileTransfer = static_cast <TBleFileTransfer*>(context);
	success ? fileTransfer->Continue() : fileTransfer->Abort();
	fileTransfer->onEvent(false , !success, success ? FT_REASON_SUCCESS : FT_DATA_NOT_SENT);
}

void TBleFileTransfer::OnWriteCtrlRcvd(void* context, const uint8_t* bytes, uint16_t len) {
	TBleFileTransfer* fileTransfer = static_cast <TBleFileTransfer*>(context);

	bool completed = false;
	bool error = (bytes == nullptr);
	uint8_t reason = FT_BAD_DATA_CTRL_FORMAT;
	TBleFileTransfer::State newState = TBleFileTransfer::State::sending_data;

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
							newState = TBleFileTransfer::State::finished;
							break;
						}
						case ACTION_STOP: {
							error = true;
							newState = TBleFileTransfer::State::failed;
							break;
						}
						case ACTION_START: {
							newState = TBleFileTransfer::State::sending_data;
							break;
						}
						case ACTION_CONTINUE: {
							newState = TBleFileTransfer::State::sending_data;
							if(ackField == NACK_CODE) {
								error = false;
								fileTransfer->dataChunkNumber = fileTransfer->lastPacketConfirmed;
							}
							break;
						}
						default: {
							newState = TBleFileTransfer::State::failed;
							error = true;
							reason = FT_BAD_DATA_CTRL_FORMAT;
							break;
						}
					}
					break;
				}
				default: {
					newState = TBleFileTransfer::State::failed;
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

uint8_t TBleFileTransfer::CalculateChecksum(const uint8_t* bytes, size_t len) {
	uint8_t chkcksm = 0;
	size_t i = 0;

	for(; i<len-1; i++) {
		chkcksm = bytes[i];
	}

	return chkcksm;
}

bool TBleFileTransfer::CheckWriteCtrlPacket(const uint8_t* bytes, uint16_t len) {
	bool checkOk = (len == WRITE_CTRL_PACKET_LENGTH &&
					bytes[WRITE_CTRL_CHCKSM_FIELD] == CalculateChecksum(bytes, len));

	return checkOk;
}

uint32_t TBleFileTransfer::Crc32(uint32_t crc, const void *buf, size_t buf_size) {
	if (!buf)
		return CRC32_INIT;
	const uint8_t *p = (const uint8_t *)buf;
	size_t i, j;
	crc = ~crc;
	for (i = 0; i < buf_size; i++) {
		crc ^= p[i];
		for (j = 0; j < 8; j++) {
			int b0 = crc & 1;
			crc >>= 1;
			if (b0)
				crc ^= CRC32_POLY;
		}
	}
	return ~crc;
}

bool TBleFileTransfer::SendFileRequest()
{
	size_t filenameLen = strlen(fileNameToSend.data());
	size_t packetLen = NOTIFICAITON_CTRL_PACKET_FIXED_LENGTH + filenameLen;

	if(packetLen > (mtu - NOTIFICATION_BLE_OVERHEAD)) {
		// FIXME
		RegistraGPS("PERIPHERAL: FT: Notification Ctrl over MTU. Unhandled case", true);
		return false;
	}

	state = TBleFileTransfer::State::waiting_confirmation;
	dataChunkNumber = 0;

	uint32_t nChunks = fileBytesToSend.size() / mtu;
	uint32_t crc32 = Crc32(CRC32_INIT, &fileBytesToSend[0], fileBytesToSend.size());

	std::vector<uint8_t> frame;
	frame.resize(packetLen);
	frame[NOTIFICAITON_CTRL_MSG_TYPE_FIELD] = NOTIFICAITON_CTRL_FILE_TRANSFER_REQUEST;
	memcpy(&frame[NOTIFICAITON_CTRL_PACKET_NUMBER_FIELD], &nChunks, 4);
	memcpy(&frame[NOTIFICAITON_CTRL_CRC32_FIELD], &crc32, 4);
	memcpy(&frame[NOTIFICAITON_CTRL_FILENAME_LEN_FIELD], &filenameLen, 2);
	memcpy(&frame[NOTIFICAITON_CTRL_FILENAME_DATA_FIELD], fileNameToSend.data(), filenameLen);
	frame[NOTIFICAITON_CTRL_FILENAME_DATA_FIELD + filenameLen] = CalculateChecksum(&frame[0], frame.size());

	adapter.sendGattServerFileCtrl(OnGattServerFileCtrlSent, this, &frame[0], static_cast<uint16_t>(frame.size()));

	return true;
}

void TBleFileTransfer::SendFileCancel()
{
	state = TBleFileTransfer::State::waiting_cancel_confirmation;
	dataChunkNumber = 0;

	std::vector<uint8_t> frame;
	frame.resize(2);
	frame[0] = NOTIFICAITON_CTRL_FILE_TRANSFER_CANCEL;
	frame[1] = CalculateChecksum(&frame[0], static_cast<uint16_t>(frame.size()));
	adapter.sendGattServerFileCtrl(OnGattServerFileCtrlSent, this, &frame[0], static_cast<uint16_t>(frame.size()));
}

