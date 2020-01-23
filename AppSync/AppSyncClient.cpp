#include "stdafx.h"
#include "AppSyncClient.h"
#include "registra_gps.h"
#include <algorithm>

#define SERVICE_UUID								"669A0C20-0008-37A8-E511-FCC84249EFA3" //"A3EF4942-C8FC-11E5-A837-0800200C9A66"
#define NS_NOTIFICATION_CHARACTERISTIC_UUID			"dc7aa73d-4cb6-3299-ae47-27c8d22adf46" //"46df2ad2-c827-47ae-9932-b64c3da77adc"
#define NS_WRITE_CHARACTERISTIC_UUID				"eeb28673-e95e-9286-ad41-8fd33d6f222a" //"2a226f3d-d38f-41ad-8692-5ee97386b2ee"

#define FT_NOTIFICATION_DATA_CHARACTERISTIC_UUID	"0cac8612-5b5b-6c84-e34f-ae3ff3d6f43c" //"3cf4d6f3-3fae-4fe3-846c-5b5b1286ac0c"
#define FT_NOTIFICATION_CTRL_CHARACTERISTIC_UUID	"b619c675-f80d-daa8-0a41-3889c803e8c8" //"c8e803c8-8938-410a-a8da-0df875c619b6"
#define FT_WRITE_CTRL_CHARACTERISTIC_UUID			"4395915d-1770-acaf-b442-4e3f0f3ce661" //"61e63c0f-3f4e-42b4-afac-70175d919543



using namespace Ble;
using namespace btstack;


TAppSyncClient::TAppSyncClient() {

}

TAppSyncClient::~TAppSyncClient()
{
}

void TAppSyncClient:: SetCallbacks(TAppSyncClient::PushNotificationCb onPushNotification,
								   TAppSyncClient::FileRecievedCb onFileReceived)
{
	this->onPushNotification = onPushNotification;
	this->onFileRecieved = onFileReceived;
}

bool TAppSyncClient::Configure(TBtStackDevice* device, std::vector<TBleService>& discoveredSvcs) {
	if(device->getDeviceState() != TBtStackDevice::BLE_DEVICE_STATUS_PAIRED) {
		return false;
	}

	bool nsConfigured {false};
	bool ftDataConfigured {false};
	bool ftDataCtrlConfigured {false};

	nsNotificationCharacteristic.release();
	nsWriteCharacteristic.release();

	TBtStackUuid TwoNavSyncSvc(SERVICE_UUID);
	auto scompare =[&](const TBleService& service){	return service.getUuid()->equal(&TwoNavSyncSvc);};
	auto its = std::find_if(discoveredSvcs.begin(), discoveredSvcs.end(), scompare);

	if(its!=discoveredSvcs.end()) {
		TBtStackUuid nsData(NS_NOTIFICATION_CHARACTERISTIC_UUID);
		TBtStackUuid nsCtrl(NS_WRITE_CHARACTERISTIC_UUID);
		TBtStackUuid ftData(FT_NOTIFICATION_DATA_CHARACTERISTIC_UUID);
		TBtStackUuid ftDataCtrl(FT_NOTIFICATION_CTRL_CHARACTERISTIC_UUID);
		TBtStackUuid ftCtrl(FT_WRITE_CTRL_CHARACTERISTIC_UUID);

		std::vector<TBleService> servicesToDiscover;
		servicesToDiscover.emplace_back(*its);
		auto characteristics = device->discoverCharacteristics(servicesToDiscover);
		for(const TBleCharacteristic& characteristic : characteristics)	{
			if(characteristic.getUuid()->equal(&nsData)){
				nsNotificationCharacteristic = std::unique_ptr<TBleCharacteristic>
						(new TBleCharacteristic(characteristic));
			}
			else if(characteristic.getUuid()->equal(&nsCtrl)){
				nsWriteCharacteristic = std::unique_ptr<TBleCharacteristic>
						(new TBleCharacteristic(characteristic));
			}
			else if(characteristic.getUuid()->equal(&ftData)){
				ftDataCharacteristic = std::unique_ptr<TBleCharacteristic>
						(new TBleCharacteristic(characteristic));
			}
			else if(characteristic.getUuid()->equal(&ftDataCtrl)){
				ftDataCtrlCharacteristic = std::unique_ptr<TBleCharacteristic>
						(new TBleCharacteristic(characteristic));
			}
			else if(characteristic.getUuid()->equal(&ftCtrl)){
				ftCtrlCharacteristic = std::unique_ptr<TBleCharacteristic>
						(new TBleCharacteristic(characteristic));
			}
		}

		if(nsNotificationCharacteristic != nullptr && nsWriteCharacteristic != nullptr) {
			RegistraGPS("PERIPHERAL: Appsync: Found notification capability", true);
			nsConfigured = device->registerNotification(*nsNotificationCharacteristic);
		}

		if(ftDataCharacteristic != nullptr && ftDataCtrlCharacteristic != nullptr && ftCtrlCharacteristic != nullptr) {
			RegistraGPS("PERIPHERAL: Appsync: Found file transfer capability", true);
			ftDataConfigured = device->registerNotification(*ftDataCharacteristic);
			ftDataCtrlConfigured = device->registerNotification(*ftDataCtrlCharacteristic);
		}
	}

	return (ftDataConfigured && ftDataCtrlConfigured) || nsConfigured; // nsConfigured should or not be ( Iphones don't have it )
}

bool TAppSyncClient::ProcessNotificationEvent(const TBtStackDeviceEvent::NotificationEventData* data)
{
	bool processed {false};
	auto found = [&](TBleCharacteristic* c){
		return (c != nullptr) && data->characteristic->getUuid()->equal(c->getUuid());
	};

	if(data && data->dataLength !=0) {
		if(found(nsNotificationCharacteristic.get())) {
			NotificationType type = static_cast<NotificationType>(data->data[0]);

			switch(type) {
				case NotificationType::pushNotfication:
					auto notification = TAppSyncPushNotificationParser().Process(data);
					if(notification != nullptr) {
						onPushNotification(*notification);
					}
					break;
			}
			processed = true;
		}
		else if(found(ftDataCharacteristic.get())) {
			RegistraGPS("FileClient Data Recieved", true);
			//TODO;
			fileClient.ProcessDataPacket(data->data, data->dataLength);
			processed = true;
		}
		else if(found(ftDataCtrlCharacteristic.get())) {
			RegistraGPS("FileClient Data Ctrl Recieved", true);
			//TODO;
			fileClient.ProcessCtrlPacket(data->data, data->dataLength);
			processed = true;
		}

	}

	return processed;
}

void TAppSyncClient::Update(TBtStackDevice *device)
{
	// TODO: Update AppSyncFileClient
}
