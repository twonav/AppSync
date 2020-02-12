#include "stdafx.h"
#include "gtest/gtest.h"
#include "AppSyncFTNDPacket.h"

using namespace Ble;

class TAppSyncFTNDPacketUnitTest : public testing::Test {

protected:

	virtual void SetUp() {}
	virtual void TearDown() {}

};

TEST_F(TAppSyncFTNDPacketUnitTest,test_Encode_enterData_expect_memcmp_ok) {
	TAppSyncFTNDPacket packet;
	std::vector<uint8_t> encodedData;
	std::vector<uint8_t> fileBytes {0x00, 0x01, 0x02, 0x03, 0x04};

	TAppSyncFTNDPacket::FTNDData data;
	data.fileBytes = &fileBytes[0];
	data.fileBytesLen = fileBytes.size();
	data.packetNumber = 2;

	std::vector<uint8_t> expectedEncodedData {0x02, 0x00, 0x01, 0x02, 0x03, 0x04, 0x0C};

	packet.Encode(encodedData, data);

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTNDPacketUnitTest,test_Encode_enterDataNullFileBytes_expect_memcmp_ok) {
	TAppSyncFTNDPacket packet;
	std::vector<uint8_t> encodedData;

	TAppSyncFTNDPacket::FTNDData data;
	data.fileBytes = nullptr;
	data.fileBytesLen = 0;
	data.packetNumber = 2;

	std::vector<uint8_t> expectedEncodedData {0x02, 0x02};

	packet.Encode(encodedData, data);

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTNDPacketUnitTest,test_Decode_enterBytes_expectData) {
	std::vector<uint8_t> packetBytes {0x02, 0x00, 0x01, 0x02, 0x03, 0x04, 0x0C};
	TAppSyncFTNDPacket packet;
	TAppSyncFTNDPacket::FTNDData data;

	std::vector<uint8_t> fileBytes {0x00, 0x01, 0x02, 0x03, 0x04};
	TAppSyncFTNDPacket::FTNDData expectedData;
	expectedData.fileBytes = &fileBytes[0];
	expectedData.fileBytesLen = fileBytes.size();
	expectedData.packetNumber = 2;

	bool decoded = packet.Decode(data, &packetBytes[0], packetBytes.size());

	EXPECT_TRUE(decoded);

	bool result = (memcmp(&data.fileBytes[0], &expectedData.fileBytes[0], fileBytes.size()) == 0);
	EXPECT_TRUE(result);
	EXPECT_EQ(data.fileBytesLen, expectedData.fileBytesLen);
	EXPECT_EQ(data.packetNumber, expectedData.packetNumber);
}

TEST_F(TAppSyncFTNDPacketUnitTest,test_Decode_enterNullBytes_expectDecodedFalse) {
	TAppSyncFTNDPacket packet;
	TAppSyncFTNDPacket::FTNDData data;

	bool decoded = packet.Decode(data, nullptr, 0);

	EXPECT_FALSE(decoded);
}
