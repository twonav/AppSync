#ifndef AppSyncFileServerH
#define AppSyncFileServerH

#include "btstack/BtStackAdapter.h"
#include <vector>
#include <functional>
#include <mutex>

namespace Ble {

	class AppSyncFileServer {
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

			using OnEventReceivedCb = std::function<void(bool completed, bool error)>;

			AppSyncFileServer(btstack::TBtStackAdapter& adapter,
							 uint16_t mtu,
							 const std::string& filename,
							 std::vector<uint8_t>& bytes,
							 OnEventReceivedCb onEvent);
			~AppSyncFileServer();

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
			bool SendFileRequest();
			void SendFileCancel();
	};

}
#endif
