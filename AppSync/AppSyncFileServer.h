#ifndef AppSyncFileServerH
#define AppSyncFileServerH

#include "AppSyncServerComInterface.h"
#include <vector>
#include <functional>
#include <mutex>
#include <memory>

namespace Ble {

	class TAppSyncFileServer {
		public:
			enum class State {
				stopped,
				waiting_start_confirmation,
				waiting_continue_confirmation,
				waiting_cancel_confirmation,
				sending_data,
				waiting_data_sent,
				waiting_transfer_complete,
				transfer_complete,
				failed,
				finished,
			};

			using OnEventReceivedCb = std::function<void(bool completed, bool error)>;

			TAppSyncFileServer(std::unique_ptr<TAppSyncServerComInterface> comInterface,
							   OnEventReceivedCb onEvent);

			~TAppSyncFileServer();

			bool Start(const std::string& filename, std::vector<uint8_t>& fileBytes, uint16_t mtu);
			void Update();
			void Cancel();

		private:
			std::unique_ptr<TAppSyncServerComInterface> comInterface;
			OnEventReceivedCb onEvent;

			std::vector<uint8_t> fileBytesToSend;
			std::vector<uint8_t> packetBytesToSend;
			std::string fileNameToSend;
			uint32_t numPacketsSent;
			State state;
			uint16_t mtu;

			static void OnWriteCtrlRcvd(void* context, const uint8_t* bytes, uint16_t len);
			static void OnGattServerFileDataSent(void* context, bool success);
			static void OnCtrlPacketSent(void* context, bool success);

			uint32_t GetDataBytesPerPacket();
			void Abort();
			void Continue();
			void SendDataChunk();
			void SendFileRequest();
			void SendFileCancel();
			void SetState(State newState);
			const char* StateToString(TAppSyncFileServer::State state);
	};

}
#endif
