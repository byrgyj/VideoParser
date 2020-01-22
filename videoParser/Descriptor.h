#pragma once
#include <stdint.h>
//#include <memory.h>
#include <vector>
#include <map>

//date:2020-1-22
//author: byrgyj@126.com

typedef struct SdtDescriptor{
    uint8_t descriptorTag:8;
    uint8_t descriptorLength:8;
    uint8_t serviceType:8;
    uint8_t serviceProviderNameLength:8;
    uint8_t *serviceProvider;
    uint8_t serviceNameLength:8;
    uint8_t *serviceName;
} SdtDescriptor;

typedef struct SdtItem {
    unsigned serviceId:16;
    unsigned reservedFutureUse:6;
    unsigned eitScheduleFlag:1;
    unsigned eitPresentFollowingFlag:1;
    unsigned runningStatus:3;
    unsigned freeCAMode:1;
    unsigned descriptorsLoopLength:12;

    std::vector<SdtDescriptor> descriptor;
} SdtItem;

typedef struct DTSParameters {
    DTSParameters() : sampleFrequece(48000),sampleBits(16), channelCount(6), assetConstruction(11), bitRate(768), assetCount(0), lfeFlag(true), vbrFlag(true), postEncodeBrScalingFlag(false), componentTypeFlag(false), languageCodeFlag(true) {}
    int32_t sampleFrequece;
    int32_t sampleBits;
    int32_t channelCount;
    int32_t assetConstruction;
    int32_t bitRate;
    int32_t assetCount;

    bool lfeFlag;
    bool vbrFlag;
    bool postEncodeBrScalingFlag;
    bool componentTypeFlag;
    bool languageCodeFlag;

} DTSParameters;

enum DescriptorTag { Dsc_None, Dsc_Register};

class Descriptor {
public:
    Descriptor(uint8_t tag, const uint8_t *data, int32_t dataLength);
    virtual ~Descriptor();
    virtual int32_t parse();

    uint8_t descritorTag() const { return mDescriptorTag; }
protected:
  
    uint8_t mDescriptorTag;
    const uint8_t *mData;
    int32_t mDataLength;
};

// RegistorDescriptor
class RegistorDescriptor : public Descriptor {
public:
    RegistorDescriptor(uint8_t tag, const uint8_t *data, int32_t dataLength);
    ~RegistorDescriptor();

    virtual int32_t parse();
    bool isDolbyVision() { return mIsDolbyVision; }
private:
    bool mIsDolbyVision;
    uint32_t mDoVi;
};

// DTSAudioDescriptor 
class DTSAudioDescriptor : public Descriptor {
public:
    DTSAudioDescriptor(uint8_t tag, const uint8_t *data, int32_t dataLength);
    DTSAudioDescriptor(DTSParameters dtsParam);
    ~DTSAudioDescriptor();

    virtual int32_t parse();
    uint8_t *generateData(int32_t &outLength);

private:
    int32_t sampleFrequeceToIndex(int32_t sampleFrequece);
    int32_t subStreamCore(uint8_t *data);

private:
    uint8_t mNewData[188];
    DTSParameters mDTSParam;
};

// DescriptorManager
class DescriptorManager {
public:
    DescriptorManager();
    ~DescriptorManager();

    int32_t parse(const uint8_t *data, int32_t dataLength);
    bool isDolbyVision();
private:
    std::map<int32_t, Descriptor*> mDescriptors;
};
