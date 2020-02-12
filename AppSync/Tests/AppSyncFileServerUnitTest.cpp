#include "stdafx.h"
#include "gtest/gtest.h"
#include <iostream>
#include <cmath>
#include <ctime>
#include <random>
#include "AppSyncFileServer.h"
#include "AppSyncServerComInterface.h"

#include "AppSyncFTNCPacket.h"
#include "AppSyncFTNDPacket.h"
#include "AppSyncFTWCPacket.h"

using namespace Ble;

class TAppSyncFileServerUnitTest : public testing::Test {

protected:

	virtual void SetUp() {}
	virtual void TearDown() {}

};

class TAppSyncServerComMock : public TAppSyncServerComInterface {
	public:
		TAppSyncServerComMock(uint8_t packetLossChancePercentage) {
			this->packetLossChancePercentage = packetLossChancePercentage;
		}
		virtual ~TAppSyncServerComMock() override {}

		double GenerateRandomPercent() {
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_real_distribution<double> dist(0, 99);
			return dist(mt);
		}

		void ClientDataProcess(void *context, uint8_t *bytes, uint16_t len) {
			bool packetOk {false};
			bool packetLost = GenerateRandomPercent() < packetLossChancePercentage;

			if(packetLost) {
				std::cout << "Packet Lost" << std::endl;
			}
			else {
				TAppSyncFTNDPacket nd;
				TAppSyncFTNDPacket::FTNDData ndData;

				bool decoded = nd.Decode(ndData, bytes, len);
				if(decoded) {
					uint32_t packetRecieved = expectedPacketToRecieve&0xFFFFFF00 + ndData.packetNumber;
					bool isExpectedPacket = (packetRecieved == expectedPacketToRecieve);
					std::cout << "Client: Packet Recieved: " << std::to_string(packetRecieved) << std::endl;
					if(isExpectedPacket) {
						fileRecieved.insert(fileRecieved.end(),
											&ndData.fileBytes[0],
											&ndData.fileBytes[ndData.fileBytesLen]);

						if(ndData.packetNumber == 0xFF) {
							TAppSyncFTWCPacket wc;
							std::vector<uint8_t> wcEncoded;
							wc.EncodeContinueTransfer(wcEncoded, packetRecieved);
							ctrlPacketRcvd(context, &wcEncoded[0], static_cast<uint16_t>(wcEncoded.size()));
							std::cout << "Client: Confirmation Sent " << std::to_string(packetRecieved) << std::endl;
						}

						expectedPacketToRecieve++;

						/*if(expectedPacketToRecieve == totalPacketsToRecieve) {
							TAppSyncFTWCPacket wc;
							std::vector<uint8_t> wcEncoded;
							wc.EncodeTransferComplete(wcEncoded, expectedPacketToRecieve);
							ctrlPacketRcvd(context, &wcEncoded[0], static_cast<uint16_t>(wcEncoded.size()));

						}*/

						packetOk = true;
					}
					else {
						std::cout << "Client: Unexpected packet " << std::to_string(packetRecieved) << std::endl;
					}
				}
			}

			if(packetLost || !packetOk) {
				TAppSyncFTWCPacket wc;
				std::vector<uint8_t> wcEncoded;
				wc.EncodeRollback(wcEncoded, expectedPacketToRecieve);
				ctrlPacketRcvd(context, &wcEncoded[0], static_cast<uint16_t>(wcEncoded.size()));
				std::cout << "Client: Rollback To " << std::to_string(expectedPacketToRecieve) << std::endl;
			}
		}

		virtual void RegisterOnCtrlPacketRcvdCb(OnCtrlPacketRcvdCb onCtrlPacketRcvd, void *context) override {
			ctrlPacketRcvd = onCtrlPacketRcvd;
		}

		virtual void SendDataPacket(OnPacketSent onDataPacketSent, void *context, uint8_t *bytes, uint16_t len) override {
			onDataPacketSent(context, true);
			sendDataPacketCalled = true;

			ClientDataProcess(context, bytes, len);
		}

		virtual void SendCtrlPacket(OnPacketSent onCtrlPacketSent, void *context, uint8_t *bytes, uint16_t len) override {
			onCtrlPacketSent(context, true);
			sendCtrlPacketCalled = true;
			TAppSyncFTNCPacket packet;
			TAppSyncFTNCPacket::FTNCData data;

			bool decoded = packet.Decode(data, bytes, len);
			if(decoded) {
				lastCtrlPacketSent = data;
				if(data.action == TAppSyncFTNCPacket::Action::request) {
					totalPacketsToRecieve = data.numPackets;
				}
			}
		}

		TAppSyncFTNCPacket::FTNCData lastCtrlPacketSent;
		OnCtrlPacketRcvdCb ctrlPacketRcvd;
		std::vector<uint8_t> fileRecieved;
		uint32_t expectedPacketToRecieve = 0;
		uint32_t totalPacketsToRecieve = 0;
		uint8_t packetLossChancePercentage = 0;
		bool sendCtrlPacketCalled {false};
		bool sendDataPacketCalled {false};
};

void GenerateRandomFile(std::vector<uint8_t>& fileBytes, size_t fileSize) {
	fileBytes.resize(fileSize);
	for(size_t i = 0; i<fileSize; i++) {
		fileBytes[i] = std::rand() & 0xFF;
	}
}

void CompleteTransferTest(uint8_t packetLossPercentage) {
	bool onEventCalled {false};
	bool onEventCompleted {false};
	bool onEventError {false};

	// Event Listener
	auto onEvent = [&](bool completed, bool error){
		onEventCalled = true;
		onEventCompleted = completed;
		onEventError = error;
	};

	// File Server creation
	auto comIface {std::unique_ptr<TAppSyncServerComMock>(new TAppSyncServerComMock(packetLossPercentage))};
	auto comPtr = comIface.get();
	TAppSyncFileServer fileServer(std::move(comIface), onEvent);

	// Prepare File Test
	std::string filename = "TestFile.trk";
	uint16_t mtu = 509;
	size_t fileSize = 500000;
	uint32_t numberOfPackets = 983; // ceil(filezize/mtu)
	std::vector<uint8_t> fileBytes;
	GenerateRandomFile(fileBytes, fileSize);
	std::vector<uint8_t> fileBytesCopy = fileBytes;

	// Request File transfer to Client
	bool started = fileServer.Start(filename, fileBytes, mtu);

	EXPECT_TRUE(started);
	EXPECT_TRUE(comPtr->sendCtrlPacketCalled);
	EXPECT_EQ(comPtr->lastCtrlPacketSent.numPackets, numberOfPackets);
	comPtr->sendCtrlPacketCalled = false;

	// Client simulation: Authorize File Transfer
	TAppSyncFTWCPacket packet;
	std::vector<uint8_t> encodedPacket;
	packet.EncodeAuthorizeTransfer(encodedPacket);
	comPtr->ctrlPacketRcvd(&fileServer, &encodedPacket[0], static_cast<uint16_t>(encodedPacket.size()));


	EXPECT_TRUE(onEventCalled);
	EXPECT_FALSE(onEventCompleted);
	EXPECT_FALSE(onEventError);

	onEventCalled = false;

	// Server Transfer phase
	uint32_t count = 0;
	while(!onEventCompleted) {
		fileServer.Update();
		if(onEventCalled) {
			onEventCalled = false;
		}
		else {
			break;
		}
		count++;
	}

	EXPECT_EQ(comPtr->fileRecieved.size(), fileBytesCopy.size());
	bool memcpyOk = (memcmp(&comPtr->fileRecieved[0], &fileBytesCopy[0], fileBytesCopy.size()) == 0);
	EXPECT_TRUE(memcpyOk);

	// Client Simulation
	if(memcpyOk) {
		TAppSyncFTWCPacket packet;
		std::vector<uint8_t> encodedPacket;
		packet.EncodeTransferComplete(encodedPacket, numberOfPackets);
		comPtr->ctrlPacketRcvd(&fileServer, &encodedPacket[0], static_cast<uint16_t>(encodedPacket.size()));
	}

	EXPECT_TRUE(onEventCompleted);
	EXPECT_FALSE(onEventError);
}

TEST_F(TAppSyncFileServerUnitTest,test_Start_Accept_expectTrue) {
	uint8_t packetLossPercentage = 0;
	CompleteTransferTest(packetLossPercentage);
}

TEST_F(TAppSyncFileServerUnitTest,test_Start_Accept_enter_PacketLoss10Percent_expectTrue) {
	uint8_t packetLossPercentage = 10;
	CompleteTransferTest(packetLossPercentage);
}

TEST_F(TAppSyncFileServerUnitTest,test_Start_Accept_enter_PacketLoss80Percent_expectTrue) {
	uint8_t packetLossPercentage = 80;
	CompleteTransferTest(packetLossPercentage);
}
