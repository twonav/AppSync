#include "stdafx.h"
#include "gtest/gtest.h"
#include "AppSyncFTNCPacket.h"

using namespace Ble;

class TAppSyncFTNCPacketUnitTest : public testing::Test {

protected:

	virtual void SetUp() {}
	virtual void TearDown() {}

};

TEST_F(TAppSyncFTNCPacketUnitTest,test_EncodeFileRequest_enterData_expect_memcmp_ok) {
	TAppSyncFTNCPacket packet;
	std::vector<uint8_t> encodedData;

	TAppSyncFTNCPacket::FTNCData data;
	data.crc32 = 0xAABBCCDD;
	data.action = TAppSyncFTNCPacket::Action::request;
	data.fileName = "MyFile.trk";
	data.numPackets = 258;

	packet.EncodeFileRequest(encodedData, data);

	std::vector<uint8_t> expectedEncodedData {0x00, // msg type
											  0x02, 0x1, 0x00, 0x00, // packet number
											  0xDD, 0xCC, 0xBB, 0xAA, // crc32
											  0x0A, 0x00, // filename len
											  0x4d, 0x79, 0x46, 0x69, 0x6c, 0x65, 0x2e, 0x74, 0x72, 0x6b, //filename
											  0xe0}; //chksm

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTNCPacketUnitTest,test_EncodeFileRequest_enterDataWithUTF8Filename_expect_memcmp_ok) {
	TAppSyncFTNCPacket packet;
	std::vector<uint8_t> encodedData;

	TAppSyncFTNCPacket::FTNCData data;
	data.crc32 = 0xAABBCCDD;
	data.action = TAppSyncFTNCPacket::Action::request;
	data.fileName = "àà@$^^€ïöÜ.üü";
	data.numPackets = 258;

	packet.EncodeFileRequest(encodedData, data);

	std::vector<uint8_t> expectedEncodedData {0x00, // msg type
											  0x02, 0x1, 0x00, 0x00, // packet number
											  0xDD, 0xCC, 0xBB, 0xAA, // crc32
											  0x16, 0x00, // filename len
											  0xc3, 0xa0, 0xc3, 0xa0, 0x40, 0x24, 0x5e, 0x5e, 0xe2, 0x82, 0xac, 0xc3, 0xaf, 0xc3, 0xb6, 0xc3, 0x9c, 0x2e, 0xc3, 0xbc, 0xc3, 0xbc,
											  0x93}; //chksm

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTNCPacketUnitTest,test_EncodeFileRequest_enterDataNoFileName_expect_memcmp_ok) {
	TAppSyncFTNCPacket packet;
	std::vector<uint8_t> encodedData;

	TAppSyncFTNCPacket::FTNCData data;
	data.crc32 = 0xAABBCCDD;
	data.action = TAppSyncFTNCPacket::Action::request;
	data.fileName = "";
	data.numPackets = 258;

	packet.EncodeFileRequest(encodedData, data);

	std::vector<uint8_t> expectedEncodedData {0x00, // msg type
											  0x02, 0x1, 0x00, 0x00, // packet number
											  0xDD, 0xCC, 0xBB, 0xAA, // crc32
											  0x00, 0x00, // filename len
											  // no filename
											  0x11}; //chksm

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}

TEST_F(TAppSyncFTNCPacketUnitTest,test_EncodeFileCancel_expect_memcmp_ok) {
	TAppSyncFTNCPacket packet;
	std::vector<uint8_t> encodedData;

	packet.EncodeFileCancel(encodedData);
	std::vector<uint8_t> expectedEncodedData {0x01, // msg type
											  0x00, 0x00, 0x00, 0x00, // packet number
											  0x00, 0x00, 0x00, 0x00, // crc32
											  0x00, 0x00, // filename len
											  // no filename
											  0x01}; //chksm

	bool result = (memcmp(&expectedEncodedData[0], &encodedData[0], encodedData.size()) == 0);

	EXPECT_EQ(encodedData.size(), expectedEncodedData.size());
	EXPECT_TRUE(result);
}


TEST_F(TAppSyncFTNCPacketUnitTest,test_Decode_enterPacket_expectData) {
	std::vector<uint8_t> packetBytes {0x00, // msg type
									  0x02, 0x1, 0x00, 0x00, // packet number
									  0xDD, 0xCC, 0xBB, 0xAA, // crc32
									  0x0A, 0x00, // filename len
									  0x4d, 0x79, 0x46, 0x69, 0x6c, 0x65, 0x2e, 0x74, 0x72, 0x6b, //filename
									  0xe0}; //chksm

	TAppSyncFTNCPacket::FTNCData expectedData;
	expectedData.crc32 = 0xAABBCCDD;
	expectedData.action = TAppSyncFTNCPacket::Action::request;
	expectedData.fileName = "MyFile.trk";
	expectedData.numPackets = 258;

	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;
	bool decoded = packet.Decode(data, &packetBytes[0], packetBytes.size());

	EXPECT_TRUE(decoded);
	EXPECT_EQ(data.crc32, expectedData.crc32);
	EXPECT_EQ(static_cast<uint8_t>(data.action), static_cast<uint8_t>(expectedData.action));
	EXPECT_STREQ(data.fileName.c_str(), expectedData.fileName.c_str());
}

TEST_F(TAppSyncFTNCPacketUnitTest,test_Decode_enterPacketUTF8Filename_expectData) {
	std::vector<uint8_t> packetBytes {0x00, // msg type
									  0x02, 0x1, 0x00, 0x00, // packet number
									  0xDD, 0xCC, 0xBB, 0xAA, // crc32
									  0x16, 0x00, // filename len
									  0xc3, 0xa0, 0xc3, 0xa0, 0x40, 0x24, 0x5e, 0x5e, 0xe2, 0x82, 0xac, 0xc3, 0xaf, 0xc3, 0xb6, 0xc3, 0x9c, 0x2e, 0xc3, 0xbc, 0xc3, 0xbc,
									  0x93}; //chksm

	TAppSyncFTNCPacket::FTNCData expectedData;
	expectedData.crc32 = 0xAABBCCDD;
	expectedData.action = TAppSyncFTNCPacket::Action::request;
	expectedData.fileName = "àà@$^^€ïöÜ.üü";
	expectedData.numPackets = 258;

	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;
	bool decoded = packet.Decode(data, &packetBytes[0], packetBytes.size());

	EXPECT_TRUE(decoded);
	EXPECT_EQ(data.crc32, expectedData.crc32);
	EXPECT_EQ(static_cast<uint8_t>(data.action), static_cast<uint8_t>(expectedData.action));
	EXPECT_STREQ(data.fileName.c_str(), expectedData.fileName.c_str());
}

TEST_F(TAppSyncFTNCPacketUnitTest,test_Decode_enterPacketNoFilename_expectData) {
	std::vector<uint8_t> packetBytes {0x00, // msg type
									  0x02, 0x1, 0x00, 0x00, // packet number
									  0xDD, 0xCC, 0xBB, 0xAA, // crc32
									  0x00, 0x00, // filename len
									  // no filename
									  0x11}; //chksm


	TAppSyncFTNCPacket::FTNCData expectedData;
	expectedData.crc32 = 0xAABBCCDD;
	expectedData.action = TAppSyncFTNCPacket::Action::request;
	expectedData.fileName = "NoNamedFile";
	expectedData.numPackets = 258;

	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;
	bool decoded = packet.Decode(data, &packetBytes[0], packetBytes.size());

	EXPECT_TRUE(decoded);
	EXPECT_EQ(data.crc32, expectedData.crc32);
	EXPECT_EQ(static_cast<uint8_t>(data.action), static_cast<uint8_t>(expectedData.action));
	EXPECT_STREQ(data.fileName.c_str(), expectedData.fileName.c_str());
}

TEST_F(TAppSyncFTNCPacketUnitTest,test_Decode_enterPacketBadChecksum_expectDecodedFalse) {
	std::vector<uint8_t> packetBytes {0x00, // msg type
									  0x02, 0x1, 0x00, 0x00, // packet number
									  0xDD, 0xCC, 0xBB, 0xAA, // crc32
									  0x0A, 0x00, // filename len
									  0x4d, 0x79, 0x46, 0x69, 0x6c, 0x65, 0x2e, 0x74, 0x72, 0x6b, //filename
									  0x00}; //bad chksm

	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;
	bool decoded = packet.Decode(data, &packetBytes[0], packetBytes.size());

	EXPECT_FALSE(decoded);
}

TEST_F(TAppSyncFTNCPacketUnitTest,test_Decode_enterNullBytes_expectDecodedFalse) {
	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;
	bool decoded = packet.Decode(data, nullptr, 10);

	EXPECT_FALSE(decoded);
}

TEST_F(TAppSyncFTNCPacketUnitTest,test_Decode_enterBadLength_expectDecodedFalse) {
	std::vector<uint8_t> packetBytes {0x00, // msg type
									  0x02, 0x1, 0x00, 0x00, // packet number
									  0x00}; //bad chksm

	TAppSyncFTNCPacket::FTNCData expectedData;
	expectedData.crc32 = 0xAABBCCDD;
	expectedData.action = TAppSyncFTNCPacket::Action::request;
	expectedData.fileName = "MyFile.trk";
	expectedData.numPackets = 258;

	TAppSyncFTNCPacket packet;
	TAppSyncFTNCPacket::FTNCData data;
	bool decoded = packet.Decode(data, nullptr, packetBytes.size());

	EXPECT_FALSE(decoded);
}
