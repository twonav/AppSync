#ifndef BleFileTransferH
#define BleFileTransferH

#include "BtStackAdapter.h"
#include <vector>
#include <functional>
#include <mutex>

#define DEBUG_APPSYNC_FILE_CHANNEL

#define FT_REASON_SUCCESS		0x00
#define FT_DATA_NOT_SENT		0x01
#define FT_BAD_DATA_CTRL_FORMAT	0x02

namespace Ble {

	class TBleFileTransfer {
		public:
			enum class State {
				stopped,
				waiting_confirmation,
				waiting_cancel_confirmation,
				sending_data,
				waiting_data_sent,
				waiting_transfer_complete,
				transfer_complete,
				failed,
				finished,
			};

			using OnEventReceivedCb = std::function<void(bool completed, bool error, uint8_t reason)>;

			TBleFileTransfer(btstack::TBtStackAdapter& adapter,
							 uint16_t mtu,
							 const std::string& filename,
							 std::vector<uint8_t>& bytes,
							 OnEventReceivedCb onEvent);
			~TBleFileTransfer();

			bool Start();
			void Update();
			void Cancel();

		private:
			std::vector<uint8_t> fileBytesToSend;
			std::vector<uint8_t> frameToSend;
			std::string fileNameToSend;
			uint32_t dataChunkNumber;
			uint32_t lastPacketConfirmed;
			btstack::TBtStackAdapter& adapter;
			State state;
			uint16_t mtu;

			//std::recursive_mutex mutex;

			OnEventReceivedCb onEvent;

			static void OnWriteCtrlRcvd(void* context, const uint8_t* bytes, uint16_t len);
			static void OnGattServerFileDataSent(void* context, bool success);
			static void OnGattServerFileCtrlSent(void* context, bool success);

			void Abort();
			void Continue();
			void SendDataChunk();
			uint8_t CalculateChecksum(const uint8_t* bytes, size_t len);
			bool CheckWriteCtrlPacket(const uint8_t* bytes, uint16_t len);

			uint32_t Crc32(uint32_t crc, const void *buf, size_t buf_size);
			bool SendFileRequest();
			void SendFileCancel();
	};

}
#endif

