#ifndef AppsyncPacketChecksumH
#define AppsyncPacketChecksumH

#include <stdint.h>
#include <stddef.h>

namespace Ble {
	class TAppSyncPacketChecksum {
		public:
			uint8_t Calculate(const uint8_t* bytes, size_t len)
			{
				uint8_t chkcksm = 0;
				size_t i = 0;

				for(; i<len-1; i++) {
					chkcksm += bytes[i];
				}

				return chkcksm;
			}
	};
};

#endif
