#pragma once
#include <stdint.h>
#include <memory.h>
#include <vector>
#include "Descriptor.h"
#define TS_PACKET_SIZE 188

/*class Descriptor {
public:
    Descriptor();
    ~Descriptor();

    int32_t parse(const uint8_t *data, int32_t dataLength);
    bool isDolbyVision() { return mIsDolbyVision; }
private:
    bool mIsDolbyVision;
    uint32_t mDoVi;
};
*/

struct SPS_INFO {
    SPS_INFO() : width(0), height(0),   profileIdc(0), level(0), extraDataLength(0), finished(false), format(-1) {
        memset(extraData, 0, sizeof(extraData));
    }
    int32_t width;
    int32_t height;
    int32_t profileIdc;
    int32_t level;
    int32_t extraDataLength;
    int32_t format;
    uint8_t extraData[TS_PACKET_SIZE];

    bool finished;
};

typedef struct TsPatProgram{
    unsigned program_number    :16; //节目号
    unsigned program_map_PID   :13;   //节目映射表的PID，节目号大于0时对应的PID，每个节目对应一个
}TsPatProgram;

//PAT表结构体
typedef struct TsPatTable{
    uint8_t tableId                        : 8; //固定为0x00 ，标志是该表是PAT
    uint8_t sectionSyntaxIndicator        : 1; //段语法标志位，固定为1
    uint8_t zero                            : 1; //0
    uint8_t reserved_1                        : 2; // 保留位
    unsigned sectionLength                    : 12; //表示这个字节后面有用的字节数，包括CRC32
    unsigned transportStreamId            : 16; //该传输流的ID，区别于一个网络中其它多路复用的流
    uint8_t reserved_2                        : 2;// 保留位
    uint8_t versionNumber                    : 5; //范围0-31，表示PAT的版本号
    uint8_t currentNextIndicator            : 1; //发送的PAT是当前有效还是下一个PAT有效
    uint8_t sectionNumber                    : 8; //分段的号码。PAT可能分为多段传输，第一段为00，以后每个分段加1，最多可能有256个分段
    uint8_t lastSectionNumber            : 8;  //最后一个分段的号码

    std::vector<TsPatProgram> program;
    uint8_t reserved_3                        : 3; // 保留位
    unsigned networkPid                    : 13; //网络信息表（NIT）的PID,节目号为0时对应的PID为network_PID

    unsigned crc_32                            : 32;  //CRC32校验码
} TsPatTable;

typedef struct TsPmtStream    
{    
    unsigned streamType                       : 8; //指示特定PID的节目元素包的类型。该处PID由elementary PID指定    
    unsigned elementaryPid                    : 13; //该域指示TS包的PID值。这些TS包含有相关的节目元素    
    unsigned esInfoLenght                    : 12; //前两位bit为00。该域指示跟随其后的描述相关节目元素的byte数
    uint16_t vDataLength;

	void AddDescriptor(const uint8_t *data, int32_t dataLength) {
		desManager.parse(data, dataLength);
	}

	bool isDolbyVision() { return desManager.isDolbyVision(); }

    DescriptorManager desManager;
}TsPmtStream; 

//PMT 表结构体  
typedef struct TsPmtTable  
{  
    unsigned tableId                        : 8; //固定为0x02, 表示PMT表  
    unsigned sectionSyntaxIndicator        : 1; //固定为0x01  
    unsigned zero                            : 1; //0x01  
    unsigned reserved_1                      : 2; //0x03  
    unsigned sectionLength                  : 12;//首先两位bit置为00，它指示段的byte数，由段长度域开始，包含CRC。  
    unsigned programNumber                    : 16;// 指出该节目对应于可应用的Program map PID  
    unsigned reserved_2                        : 2; //0x03  
    unsigned versionNumber                    : 5; //指出TS流中Program map section的版本号  
    unsigned currentNextIndicator            : 1; //当该位置1时，当前传送的Program map section可用；  
    //当该位置0时，指示当前传送的Program map section不可用，下一个TS流的Program map section有效。  
    unsigned sectionNumber                    : 8; //固定为0x00  
    unsigned lastSectionNumber            : 8; //固定为0x00  
    unsigned reserved_3                        : 3; //0x07  
    unsigned pcrPid                        : 13; //指明TS包的PID值，该TS包含有PCR域，  
    //该PCR值对应于由节目号指定的对应节目。  
    //如果对于私有数据流的节目定义与PCR无关，这个域的值将为0x1FFF。  
    unsigned reserved_4                        : 4; //预留为0x0F  
    unsigned programInfoLength            : 12; //前两位bit为00。该域指出跟随其后对节目信息的描述的byte数。  

    std::vector<TsPmtStream> pmtStream;  //每个元素包含8位, 指示特定PID的节目元素包的类型。该处PID由elementary PID指定  
    unsigned reserved_5                        : 3; //0x07  
    unsigned reserved_6                        : 4; //0x0F  
    unsigned crc_32                            : 32;   
} TsPmtTable;

typedef struct TsSdtTable {
    unsigned tableId : 8;
    unsigned sectionSyntaxIndicatior:1;
    unsigned reservedFutureUse1:1;
    unsigned reserved:2;
    unsigned sectionLength:12;
    unsigned transportStreamId:16;
    unsigned reserved2:2;
    unsigned versionNumber:5;
    unsigned currentNextIndicator:1;
    unsigned sectionNumber:8;
    unsigned lastSectionNumber:8;
    unsigned originalNetworkId:16;
    unsigned reservedFutureUse2:8;

    std::vector<SdtItem> stdItems;
} TsSdtTable;

enum MediaType { Media_Unknow = -1, Media_Video, Media_Audio, Media_Subtitle };

enum VideoCodecType { V_UnknewCodec = -1, V_AVC, V_HEVC };

class MediaDataParser
{
public:
    MediaDataParser();
    ~MediaDataParser();

    int32_t MediaDataParser::filterData(uint8_t *data, int32_t dataLength, int32_t &dataOffset, const uint8_t *pmtData, uint8_t *outputBuffer, int32_t &outDataLength);
    int parseData(const uint8_t * data, int32_t dataLength);
    int parsePacket(const uint8_t * data, int32_t dataLength);
    void parseVideo();
    void testCrc();

    int parsePat(TsPatTable *patTable, const uint8_t *buffer);
    int parserPmt(TsPmtTable *pmtTable, const uint8_t *buffer);
    int parseSdt(TsSdtTable *stdTable, const uint8_t *buffer);
    int resetSdt(TsSdtTable *stdTable, uint8_t *buffer);
    int resetDTSDescriptor(TsPmtTable *pmtTable, uint8_t *buffer);

    int parseVideo(const uint8_t * buffer, int32_t bufferLength);

    const uint8_t *getExtraData(const uint8_t *data, int &length);
    int parseVideoCodecType(const uint8_t *data, int length);
    SPS_INFO HevcParseSPS(const uint8_t *data, int length);
    SPS_INFO H264ParseSPS(const uint8_t *data, int length);
    bool isParsed() { return mSPS.finished; }

private:
    int32_t findStartCode(const uint8_t *data, int length);
    int32_t mapPixelFormatHEVC(int32_t bitDepth, int32_t chromaFormatIdc);
    int32_t mapPixelFormatAVC(int32_t bitDepth, int32_t chromaFormatIdc);

    int makeCrcTable(uint32_t *crc32Table);
    uint32_t crc32Calculate(const uint8_t *buffer, uint32_t size);
private:
    bool mIsHevc;

    FILE *mDumpFile;
    uint8_t mPacketData[188];
    int32_t mCurrentDataSize;

    uint8_t mPmtData[TS_PACKET_SIZE];
    uint8_t mBuffer[188];
    int32_t mPmtId;

    TsPatTable mPat;
    TsPmtTable mPmt;
    SPS_INFO mSPS;

    int32_t mVideoCodecType; // 0--h264; 1--h265
    int32_t mVideoPID;
    int32_t mPmtPid;
};


class Bitstream {
public:
    const uint8_t       *mData;
    size_t         mOffset;
    const size_t   mDataLength;
    bool           mError;
    const bool     mDoEP3;

public:
    Bitstream(const uint8_t *data, size_t bits)
        : mData(data)
        , mOffset(0)
        , mDataLength(bits)
        , mError(false)
        , mDoEP3(false)
    {}
    // this is a bitstream that has embedded emulation_prevention_three_byte
    // sequences that need to be removed as used in HECV.
    // Data must start at byte 2
    Bitstream(const uint8_t *data, size_t bits, bool doEP3)
        : mData(data)
        , mOffset(0) // skip header and use as sentinel for EP3 detection
        , mDataLength(bits)
        , mError(false)
        , mDoEP3(doEP3)
    {}

    void         skipBits(unsigned int num);
    unsigned int readBits(int num);
    unsigned int showBits(int num);
    unsigned int readBits1() { return readBits(1); }
    unsigned int readGolombUE(int maxbits = 32);
    signed int   readGolombSE();
    size_t       length() { return mDataLength; }
    bool         isError() { return mError; }
};