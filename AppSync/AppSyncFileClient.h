#ifndef AppSyncFileClientH
#define AppSyncFileClientH

#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include "AppSyncClientComInterface.h"

namespace Ble {
	class TAppSyncFileClient {
		public:
			enum class State {
				stopped,
				incoming_file_request,
				recieving_file,
				rollback,
				packet_error,
				file_complete
			};

			TAppSyncFileClient(std::unique_ptr<TAppSyncClientComInterface> comIface);
			~TAppSyncFileClient();

			bool ProcessCtrlPacket(const uint8_t* bytes, uint16_t bytesLen);
			bool ProcessDataPacket(const uint8_t* bytes, uint16_t bytesLen);
			
			void Update();

		private:
			std::unique_ptr<TAppSyncClientComInterface> comIface;
			State state;
			std::string incomingFileName;
			uint32_t incomingFileCrc32;
			uint32_t incomingChunks;
			std::vector<uint8_t> incomingFileBuffer;
			uint32_t lastPacketRecieved;
			std::vector<uint8_t> frameToSend;

			static void OnCtrlPacketRecieved(void* context, const uint8_t* bytes, uint16_t len);
			static void OnDataPacketRecieved(void* context, const uint8_t* bytes, uint16_t len);
			static void OnWriteCtrlSent(void* context, bool success);

			void Reset();
			void AuthorizeTransfer();
			void ContinueTransfer(uint32_t confirmedPacket);
			void RollbackTransfer(uint32_t packetNumber);
			void StopTransfer();
			void CompleteTransfer(uint32_t totalChunksRecieved);

	};
}
#endif
