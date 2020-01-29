#ifndef AppSyncFTWCPacketH
#define AppSyncFTWCPacketH

#include <vector>
#include <stdint.h>
#include <stddef.h>

namespace Ble {
	class TAppSyncFTWCPacket {
		public:
			enum class Action {
				stop = 0x00,
				start = 0x01,
				continue_transfer = 0x02,
				completed = 0x03,
				unknown
			};

			enum class Reason {
				success = 0x00,
				not_sent = 0x01,
				bad_format = 0x02,
				unknown
			};

			struct FTWCData {
				bool ack;
				uint32_t packetNumber;
				Action action;
				Reason reason;
			};

			TAppSyncFTWCPacket();

			void EncodeAuthorizeTransfer(std::vector<uint8_t>& encodedPacket);
			void EncodeContinueTransfer(std::vector<uint8_t>& encodedPacket,
											uint32_t confirmedPacket);
			void EncodeRollback(std::vector<uint8_t>& encodedPacket,
											uint32_t packetNumber);
			void EncodeStopTransfer(std::vector<uint8_t>& encodedPacket,
									Reason reason);
			void EncodeTransferComplete(std::vector<uint8_t>& encodedPacket,
										uint32_t totalPacketsRecieved);

			bool Decode(FTWCData& data,
						const uint8_t* packetBytes,
						const size_t bytesLen);

		private:
			void FormatCtrlPacket(std::vector<uint8_t>& encodedPacket,
								  bool ack,
								  uint32_t packetNumber,
								  Action action,
								  Reason reason);
	};
};


#endif
