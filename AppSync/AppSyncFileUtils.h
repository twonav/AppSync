#ifndef BleFileUtilsH
#define BleFileUtilsH

#include <stdint.h>
#include <stddef.h>

#define FT_REASON_SUCCESS			0x00
#define FT_DATA_NOT_SENT			0x01
#define FT_BAD_DATA_CTRL_FORMAT		0x02

#define NOTIFICATION_BLE_OVERHEAD	3

#define NOTIFICAITON_CTRL_INCOMING_FILE_FIXED_LENGTH	12

#define NOTIFICAITON_CTRL_MSG_TYPE_FIELD			0
#define NOTIFICAITON_CTRL_PACKET_NUMBER_FIELD		1
#define NOTIFICAITON_CTRL_CRC32_FIELD				5
#define NOTIFICAITON_CTRL_FILENAME_LEN_FIELD		9
#define NOTIFICAITON_CTRL_FILENAME_DATA_FIELD		11

#define NOTIFICAITON_CTRL_FILE_TRANSFER_REQUEST		0x00
#define NOTIFICAITON_CTRL_FILE_TRANSFER_CANCEL		0x01

#define NOTIFICATION_DATA_PACKET_NUMBER_FIELD	0
#define NOTIFICATION_DATA_DATA_FIELD			1

#define ACK_CTRL_PACKET_PERIOD				64

#define WRITE_CTRL_PACKET_LENGTH			8

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

namespace Ble {
	class TBleFileUtils {
		public:
			static uint32_t Crc32(uint32_t crc, const void *buf, size_t buf_size)
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

			static uint8_t CalculateChecksum(const uint8_t* bytes, size_t len) {
				uint8_t chkcksm = 0;
				size_t i = 0;

				for(; i<len-1; i++) {
					chkcksm = bytes[i];
				}

				return chkcksm;
			}

		private:

	};
}

#endif
