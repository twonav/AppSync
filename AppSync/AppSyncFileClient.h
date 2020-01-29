#ifndef AppSyncFileClientH
#define AppSyncFileClientH

#include "btstack/BtStackAdapter.h"
#include <vector>
#include <functional>
#include <mutex>


namespace Ble {
	class AppSyncFileClient {
		public:
			enum class State {
				stopped,
				incoming_file_request,
				recieving_file,
				rollback,
				packet_error,
				file_complete
			};

			AppSyncFileClient();
			~AppSyncFileClient();

			bool ProcessCtrlPacket(const uint8_t* bytes, uint16_t bytesLen);
			bool ProcessDataPacket(const uint8_t* bytes, uint16_t bytesLen);
			void Update();

		private:
			State state;

			std::string incomingFileName;
			uint32_t incomingFileCrc32;
			uint32_t incomingChunks;
			std::vector<uint8_t> incomingFileBuffer;
			uint32_t lastPacketRecieved;
			std::vector<uint8_t> frameToSend;

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
