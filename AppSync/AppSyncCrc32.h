#ifndef AppSyncCrc32H
#define AppSyncCrc32H

#include <stdint.h>
#include <stddef.h>

namespace Ble {
	class TAppSyncCrc32 {
		public:
			uint32_t Calculate(uint32_t crc, const void *buf, size_t buf_size)
			{
				if (!buf)
					return CRC32_INIT;
				const uint8_t *p = static_cast<const uint8_t*>(buf);
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
			const uint32_t CRC32_POLY = 0xedb88320;
			const uint32_t CRC32_INIT = 0;
	};
}


#endif
