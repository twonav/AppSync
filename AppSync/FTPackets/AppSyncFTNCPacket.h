#ifndef AppSyncFTNCPacketH
#define AppSyncFTNCPacketH

#include <vector>
#include <stdint.h>
#include <stddef.h>
#include<string>

namespace Ble {
	class TAppSyncFTNCPacket {
		public:
			enum class Action {
				request = 0x00,
				cancel = 0x01,
				unknown
			};

			struct FTNCData {
				Action action;
				std::string fileName;
				uint32_t numPackets;
				uint32_t crc32;
			};

			TAppSyncFTNCPacket();
			void EncodeFileRequest(std::vector<uint8_t>& encodedData,
								   const FTNCData& data);
			void EncodeFileCancel(std::vector<uint8_t>& encodedData);

			bool Decode(FTNCData &data,
						const uint8_t *packetBytes,
						const size_t bytesLen);
	};
};

#endif
