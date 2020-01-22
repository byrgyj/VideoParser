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
    unsigned program_number    :16; //��Ŀ��
    unsigned program_map_PID   :13;   //��Ŀӳ����PID����Ŀ�Ŵ���0ʱ��Ӧ��PID��ÿ����Ŀ��Ӧһ��
}TsPatProgram;

//PAT��ṹ��
typedef struct TsPatTable{
    uint8_t tableId                        : 8; //�̶�Ϊ0x00 ����־�Ǹñ���PAT
    uint8_t sectionSyntaxIndicator        : 1; //���﷨��־λ���̶�Ϊ1
    uint8_t zero                            : 1; //0
    uint8_t reserved_1                        : 2; // ����λ
    unsigned sectionLength                    : 12; //��ʾ����ֽں������õ��ֽ���������CRC32
    unsigned transportStreamId            : 16; //�ô�������ID��������һ��������������·���õ���
    uint8_t reserved_2                        : 2;// ����λ
    uint8_t versionNumber                    : 5; //��Χ0-31����ʾPAT�İ汾��
    uint8_t currentNextIndicator            : 1; //���͵�PAT�ǵ�ǰ��Ч������һ��PAT��Ч
    uint8_t sectionNumber                    : 8; //�ֶεĺ��롣PAT���ܷ�Ϊ��δ��䣬��һ��Ϊ00���Ժ�ÿ���ֶμ�1����������256���ֶ�
    uint8_t lastSectionNumber            : 8;  //���һ���ֶεĺ���

    std::vector<TsPatProgram> program;
    uint8_t reserved_3                        : 3; // ����λ
    unsigned networkPid                    : 13; //������Ϣ��NIT����PID,��Ŀ��Ϊ0ʱ��Ӧ��PIDΪnetwork_PID

    unsigned crc_32                            : 32;  //CRC32У����
} TsPatTable;

typedef struct TsPmtStream    
{    
    unsigned streamType                       : 8; //ָʾ�ض�PID�Ľ�ĿԪ�ذ������͡��ô�PID��elementary PIDָ��    
    unsigned elementaryPid                    : 13; //����ָʾTS����PIDֵ����ЩTS��������صĽ�ĿԪ��    
    unsigned esInfoLenght                    : 12; //ǰ��λbitΪ00������ָʾ��������������ؽ�ĿԪ�ص�byte��
    uint16_t vDataLength;

	void AddDescriptor(const uint8_t *data, int32_t dataLength) {
		desManager.parse(data, dataLength);
	}

	bool isDolbyVision() { return desManager.isDolbyVision(); }

    DescriptorManager desManager;
}TsPmtStream; 

//PMT ��ṹ��  
typedef struct TsPmtTable  
{  
    unsigned tableId                        : 8; //�̶�Ϊ0x02, ��ʾPMT��  
    unsigned sectionSyntaxIndicator        : 1; //�̶�Ϊ0x01  
    unsigned zero                            : 1; //0x01  
    unsigned reserved_1                      : 2; //0x03  
    unsigned sectionLength                  : 12;//������λbit��Ϊ00����ָʾ�ε�byte�����ɶγ�����ʼ������CRC��  
    unsigned programNumber                    : 16;// ָ���ý�Ŀ��Ӧ�ڿ�Ӧ�õ�Program map PID  
    unsigned reserved_2                        : 2; //0x03  
    unsigned versionNumber                    : 5; //ָ��TS����Program map section�İ汾��  
    unsigned currentNextIndicator            : 1; //����λ��1ʱ����ǰ���͵�Program map section���ã�  
    //����λ��0ʱ��ָʾ��ǰ���͵�Program map section�����ã���һ��TS����Program map section��Ч��  
    unsigned sectionNumber                    : 8; //�̶�Ϊ0x00  
    unsigned lastSectionNumber            : 8; //�̶�Ϊ0x00  
    unsigned reserved_3                        : 3; //0x07  
    unsigned pcrPid                        : 13; //ָ��TS����PIDֵ����TS������PCR��  
    //��PCRֵ��Ӧ���ɽ�Ŀ��ָ���Ķ�Ӧ��Ŀ��  
    //�������˽���������Ľ�Ŀ������PCR�޹أ�������ֵ��Ϊ0x1FFF��  
    unsigned reserved_4                        : 4; //Ԥ��Ϊ0x0F  
    unsigned programInfoLength            : 12; //ǰ��λbitΪ00������ָ���������Խ�Ŀ��Ϣ��������byte����  

    std::vector<TsPmtStream> pmtStream;  //ÿ��Ԫ�ذ���8λ, ָʾ�ض�PID�Ľ�ĿԪ�ذ������͡��ô�PID��elementary PIDָ��  
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