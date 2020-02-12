#include "stdafx.h"
#include "gtest/gtest.h"
#include "AppSyncFTWCPacket.h"

using namespace Ble;

class TAppSyncFTWCPacketUnitTest : public testing::Test {

protected:

	virtual void SetUp() {}
	virtual void TearDown() {}

};

TEST_F(TAppSyncFTWCPacketUnitTest,test_EncodeAuthorizeTransfer_expect_memcmp_ok) {
	TAppSyncFTWCPacket packet;
	std::vector<uint8_t> encodedData;

	std::vector<uint8_t> expectedEncodedData {0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07};

	packet.EncodeAuthorizeTransfer(encodedData);

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTWCPacketUnitTest,test_EncodeStopTransfer_expect_memcmp_ok) {
	TAppSyncFTWCPacket packet;
	std::vector<uint8_t> encodedData;

	std::vector<uint8_t> expectedEncodedData {0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08};

	packet.EncodeStopTransfer(encodedData, TAppSyncFTWCPacket::Reason::bad_format);

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTWCPacketUnitTest,test_EncodeContinueTransfer_expect_memcmp_ok) {
	TAppSyncFTWCPacket packet;
	std::vector<uint8_t> encodedData;

	std::vector<uint8_t> expectedEncodedData {0x06, 0x80, 0x01, 0x00, 0x00, 0x02, 0x00, 0x89};

	packet.EncodeContinueTransfer(encodedData, 0x0180);

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTWCPacketUnitTest,test_EncodeRollback_expect_memcmp_ok) {
	TAppSyncFTWCPacket packet;
	std::vector<uint8_t> encodedData;

	std::vector<uint8_t> expectedEncodedData {0x15, 0x80, 0x01, 0x00, 0x00, 0x02, 0x03, 0x9b};

	packet.EncodeRollback(encodedData, 0x0180);

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTWCPacketUnitTest,test_EncodeTransferComplete_expect_memcmp_ok) {
	TAppSyncFTWCPacket packet;
	std::vector<uint8_t> encodedData;

	std::vector<uint8_t> expectedEncodedData {0x06, 0x80, 0x01, 0x00, 0x00, 0x03, 0x00, 0x8a};

	packet.EncodeTransferComplete(encodedData, 0x0180);

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTWCPacketUnitTest,test_Decode_expectData) {
	std::vector<uint8_t> packetBytes {0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07};

	TAppSyncFTWCPacket packet;
	TAppSyncFTWCPacket::FTWCData data;

	TAppSyncFTWCPacket::FTWCData expectedData;
	expectedData.ack = true;
	expectedData.action = TAppSyncFTWCPacket::Action::start;
	expectedData.reason = TAppSyncFTWCPacket::Reason::success;
	expectedData.packetNumber = 0x00;

	bool decoded = packet.Decode(data, &packetBytes[0], packetBytes.size());

	EXPECT_TRUE(decoded);
	EXPECT_EQ(data.ack, expectedData.ack);
	EXPECT_EQ(static_cast<uint8_t>(data.action), static_cast<uint8_t>(expectedData.action));
	EXPECT_EQ(static_cast<uint8_t>(data.reason), static_cast<uint8_t>(expectedData.reason));
	EXPECT_EQ(data.packetNumber, expectedData.packetNumber);
}

TEST_F(TAppSyncFTWCPacketUnitTest,test_DecodeNullPacket_expectData) {

	TAppSyncFTWCPacket packet;
	TAppSyncFTWCPacket::FTWCData data;

	bool decoded = packet.Decode(data, nullptr, 0);

	EXPECT_FALSE(decoded);
}


