#include "stdafx.h"
#include "AppSyncFTWCPacket.h"
#include <string.h>
#include "AppSyncPacketChecksum.h"

using namespace Ble;

#define ACK_CODE	0x06
#define NACK_CODE	0x15

#define WRITE_CTRL_PACKET_LENGTH			8

#define WRITE_CTRL_ACK_FIELD				0
#define WRITE_CTRL_PACKET_NUMBER_FIELD		1
#define WRITE_CTRL_ACTION_FIELD				5
#define WRITE_CTRL_REASON_FIELD				6
#define WRITE_CTRL_CHCKSM_FIELD				7


TAppSyncFTWCPacket::TAppSyncFTWCPacket()
{

}

void TAppSyncFTWCPacket::EncodeAuthorizeTransfer(std::vector<uint8_t>& encodedPacket)
{
	FormatCtrlPacket(encodedPacket, true, 0, Action::start, Reason::success);
}

void TAppSyncFTWCPacket::EncodeContinueTransfer(std::vector<uint8_t>& encodedPacket,
													uint32_t confirmedPacket)
{
	FormatCtrlPacket(encodedPacket,
					 true,
					 confirmedPacket,
					 Action::continue_transfer,
					 Reason::success);
}

void TAppSyncFTWCPacket::EncodeRollback(std::vector<uint8_t> &encodedPacket, uint32_t packetNumber)
{
	FormatCtrlPacket(encodedPacket, false,
					 packetNumber,
					 Action::continue_transfer,
					 Reason::bad_format);
}

void TAppSyncFTWCPacket::EncodeStopTransfer(std::vector<uint8_t>& encodedPacket,
											Reason reason)
{
	FormatCtrlPacket(encodedPacket,
					 true,
					 0,
					 Action::stop,
					 reason);
}

void TAppSyncFTWCPacket::EncodeTransferComplete(std::vector<uint8_t>& encodedPacket,
												uint32_t totalPacketsRecieved)
{
	FormatCtrlPacket(encodedPacket,
					 true,
					 totalPacketsRecieved,
					 Action::completed,
					 Reason::success);
}

bool TAppSyncFTWCPacket::Decode(FTWCData &data,
								const uint8_t *packetBytes,
								const size_t bytesLen)
{
	auto getAction = [](uint8_t byte){
		switch(byte) {
			case 0x00: return Action::stop;
			case 0x01: return Action::start;
			case 0x02: return Action::continue_transfer;
			case 0x03: return Action::completed;
			default:
				return Action::unknown;
		}
	};

	auto getReason = [](uint8_t byte){
		switch(byte) {
			case 0x00: return Reason::success;
			case 0x01: return Reason::not_sent;
			case 0x02: return Reason::bad_format;
			default:
				return Reason::unknown;
		}
	};

	bool decoded = (packetBytes != nullptr &&
					bytesLen == WRITE_CTRL_PACKET_LENGTH &&
					(packetBytes[WRITE_CTRL_CHCKSM_FIELD] == TAppSyncPacketChecksum().Calculate(packetBytes, bytesLen)));

	if(decoded) {
		data.ack = packetBytes[WRITE_CTRL_ACK_FIELD];
		data.action = getAction(packetBytes[WRITE_CTRL_ACTION_FIELD]);
		data.reason = getReason(packetBytes[WRITE_CTRL_REASON_FIELD]);
		memcpy(&data.packetNumber, &packetBytes[WRITE_CTRL_PACKET_NUMBER_FIELD], 4);
		decoded = true;
	}

	return decoded;
}

void TAppSyncFTWCPacket::FormatCtrlPacket(std::vector<uint8_t>& encodedPacket,
										  bool ack,
										  uint32_t packetNumber,
										  Action action,
										  Reason reason)
{
	encodedPacket.clear();
	encodedPacket.resize(WRITE_CTRL_PACKET_LENGTH);
	encodedPacket[WRITE_CTRL_ACK_FIELD] = ack? ACK_CODE : NACK_CODE;
	memcpy(&encodedPacket[WRITE_CTRL_PACKET_NUMBER_FIELD], &packetNumber, 4);
	encodedPacket[WRITE_CTRL_ACTION_FIELD] = static_cast<uint8_t>(action);
	encodedPacket[WRITE_CTRL_REASON_FIELD] = static_cast<uint8_t>(reason);
	encodedPacket[WRITE_CTRL_CHCKSM_FIELD] = TAppSyncPacketChecksum().Calculate(&encodedPacket[0], encodedPacket.size());
}



