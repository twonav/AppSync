#ifndef AppSyncFileClientH
#define AppSyncFileClientH

#include "btstack/BtStackAdapter.h"
#include "AppSyncFileUtils.h"

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
			uint32_t lastChunkRecieved;


			static void OnWriteCtrlSent(void* context, bool success);

			void Reset();

			bool IsPacketValid(const uint8_t* bytes, uint16_t bytesLen, std::string& error);

			std::vector<uint8_t> FormatCtrlPacket(bool ack, uint32_t chunk, uint8_t action, uint8_t reason);
			void AuthorizeTransfer();
			void ContinueTransmission(bool ack, uint32_t chunk);
			void StopTransfer(uint8_t reason);
			void CompleteTransfer(uint32_t totalChunksRecieved);

	};
}
#endif
