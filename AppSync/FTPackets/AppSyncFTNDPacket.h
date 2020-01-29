#ifndef AppSyncFTNDPacketH
#define AppSyncFTNDPacketH

#include <vector>
#include <stdint.h>
#include <stddef.h>

namespace Ble {
	class TAppSyncFTNDPacket {
		public:
			struct FTNDData {
				uint8_t packetNumber;
				const uint8_t* fileBytes;
				size_t fileBytesLen;
			};

			TAppSyncFTNDPacket();
			void Encode(std::vector<uint8_t>& encodedData,
						const FTNDData& data);

			bool Decode(FTNDData& data,
						const uint8_t* bytes,
						const size_t bytesLen);

	};
};


#endif
