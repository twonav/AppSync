#ifndef AppSyncClientH
#define AppSyncClientH

#include "btstack/BtStackDevice.h"
#include "btstack/BtStackDeviceEvent.h"
#include "AppSyncPushNotification.h"
#include "AppSyncFileClient.h"
#include <memory>

namespace Ble
{
	class TAppSyncClient
	{
		public:
			using PushNotificationCb = std::function<void(const AppSyncPushNotification& notification)>;
			using FileRecievedCb = std::function<void(const std::string& filename, const std::vector<uint8_t>& fileBytes)>;

			TAppSyncClient();
			virtual ~TAppSyncClient();


			void SetCallbacks(TAppSyncClient::PushNotificationCb onPushNotification, TAppSyncClient::FileRecievedCb onFileReceived);
			bool Configure(btstack::TBtStackDevice* device,std::vector<btstack::TBleService>& discoveredSvcs);
			bool ProcessNotificationEvent(const btstack::TBtStackDeviceEvent::NotificationEventData* data);

			void Update(btstack::TBtStackDevice *device);

		private:
			enum class NotificationType {
				pushNotfication = 0x00,
			};

			PushNotificationCb onPushNotification;
			FileRecievedCb onFileRecieved;

			std::unique_ptr<TAppSyncFileClient> fileClient;

			std::unique_ptr<btstack::TBleCharacteristic> nsNotificationCharacteristic;
			std::unique_ptr<btstack::TBleCharacteristic> nsWriteCharacteristic;
			std::unique_ptr<btstack::TBleCharacteristic> ftDataCharacteristic;
			std::unique_ptr<btstack::TBleCharacteristic> ftDataCtrlCharacteristic;
			std::unique_ptr<btstack::TBleCharacteristic> ftCtrlCharacteristic;

	};
}

#endif
