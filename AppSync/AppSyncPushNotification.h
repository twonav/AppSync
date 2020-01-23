#ifndef AppSyncPushNotificationH
#define AppSyncPushNotificationH

#include "btstack/BtStackDeviceEvent.h"
#include "AppSyncTypes.h"
#include <string>
#include <memory>

namespace Ble {
	class TAppSyncPushNotificationParser {
		public:		
			TAppSyncPushNotificationParser();

			std::unique_ptr<AppSyncPushNotification> Process(const btstack::TBtStackDeviceEvent::NotificationEventData* data);

		private:
			enum class NotificationState {
				atttibuteId,
				attributeLength,
				attributeData,
				};

			enum class PushNotificationAttributeId {
				appPackage = 0x00,
				title = 0x01,
				text = 0x02,
				date = 0x03,
				none = 0xFF
			};

			bool GetEventId(uint8_t valueToCheck, AppSyncPushNotificationEventId& eventId);
	};
}

#endif
