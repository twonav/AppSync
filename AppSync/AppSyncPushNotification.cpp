#include "stdafx.h"
#include "AppSyncPushNotification.h"
#include "map"
#include <string.h>

using namespace Ble;
using namespace btstack;

TAppSyncPushNotificationParser::TAppSyncPushNotificationParser()
{
}

std::unique_ptr<AppSyncPushNotification> TAppSyncPushNotificationParser::Process(const TBtStackDeviceEvent::NotificationEventData* data)
{
	const int minimumNotificationLenght = 6;

	AppSyncPushNotificationEventId eventId;
	if(data->dataLength < minimumNotificationLenght) {
		return nullptr;
	}

	bool eventIdOk = GetEventId(data->data[1], eventId);
	if(!eventIdOk) {
		return nullptr;
	}

	std::unique_ptr<AppSyncPushNotification> notification(new AppSyncPushNotification());
	notification->eventId = eventId;
	memcpy(&notification->uuid, &data->data[2], 4);

	NotificationState state = NotificationState::atttibuteId;
	PushNotificationAttributeId attributeId = PushNotificationAttributeId::none;
	size_t attrLen  = 0;

	for(uint32_t i = 6; i<data->dataLength; i++) {
		switch(state) {
			case NotificationState::atttibuteId:
				attributeId = static_cast<PushNotificationAttributeId>(data->data[i]);
				state = NotificationState::attributeLength;
				break;
			case NotificationState::attributeLength:
				attrLen = static_cast<size_t>(data->data[i] + (data->data[i+1] << 8));
				i++;
				state = NotificationState::attributeData;
				break;
			case NotificationState::attributeData: {
				std::string* str = nullptr;

				switch(attributeId) {
					case PushNotificationAttributeId::appPackage:
						str = &notification->appPackage;
						break;
					case PushNotificationAttributeId::text:
						str = &notification->text;
						break;
					case PushNotificationAttributeId::title:
						str = &notification->title;
						break;
					case PushNotificationAttributeId::date:
						str = &notification->date;
						break;
					default:
						break;
				}

				if(str) {
					*str= std::string(reinterpret_cast<const char*>(&data->data[i]), attrLen);
				}

				i+=attrLen-1;
				state = NotificationState::atttibuteId;
				break;
			}
		}
	}

	return notification;
}

bool TAppSyncPushNotificationParser::GetEventId(uint8_t valueToCheck, AppSyncPushNotificationEventId& eventId) {
	const std::map<uint8_t, AppSyncPushNotificationEventId> flagValueMap = {
		{0, AppSyncPushNotificationEventId::none},
		{1, AppSyncPushNotificationEventId::added},
		{2, AppSyncPushNotificationEventId::removed}
	};

	auto itFlag = flagValueMap.find(valueToCheck);
	bool found = (itFlag != flagValueMap.end());
	if(found) {
		eventId = itFlag->second;
	}
	return found;
}
