#ifndef AppSyncTypesH
#define AppSyncTypesH

#include <string>

namespace Ble {
	enum class AppSyncPushNotificationEventId {
		none,
		added,
		removed
	};

	struct AppSyncPushNotification {
		std::string title = "";
		std::string text = "";
		std::string appPackage = "";
		std::string date = "";
		AppSyncPushNotificationEventId eventId = AppSyncPushNotificationEventId::none;
		uint32_t uuid;
	};
}

#endif
