#include "stdafx.h"
#include "Descriptor.h"


Descriptor::Descriptor(uint8_t tag, const uint8_t *data, int32_t dataLength) : mDescriptorTag(tag), mData(data), mDataLength(dataLength) {

}

Descriptor::~Descriptor() {

}

int32_t Descriptor::parse() {
    return 0;
}

/*
* RegistorRescriptor
*/
RegistorDescriptor::RegistorDescriptor(uint8_t tag, const uint8_t *data, int32_t dataLength):Descriptor(tag, data, dataLength), mIsDolbyVision(false){
    mDoVi = 0x44 << 24 | 0x4F << 16 | 0x56 << 8 | 0x49;
}

RegistorDescriptor::~RegistorDescriptor() {

}

int32_t RegistorDescriptor::parse(){
    if (mData != NULL && mDataLength <= 0) {
        return -1;
    }

    if (descritorTag() != 5) {
        return -1;
    }

    int32_t index = 0;

    uint32_t id = mData[index] << 24 | mData[index+1] << 16 | mData[index+2] << 8 | mData[index+3];
    mIsDolbyVision = mDoVi == id;

    return 0;
}

DTSAudioDescriptor::DTSAudioDescriptor(uint8_t tag, const uint8_t *data, int32_t dataLength) : Descriptor(tag, data, dataLength) {
    memset(mNewData, 0, sizeof(mNewData));
}

DTSAudioDescriptor::DTSAudioDescriptor(DTSParameters dtsParam):Descriptor(0, NULL, 0), mDTSParam(dtsParam) {
    memset(mNewData, 0, sizeof(mNewData));
}

DTSAudioDescriptor::~DTSAudioDescriptor() {

}

int32_t DTSAudioDescriptor::parse() {
    if (mData == NULL || mDataLength <= 1) {
        //generateData();
        return 0;
    }


    return 0;
}

uint8_t *DTSAudioDescriptor::generateData(int32_t &outLength) {
    int32_t index = 0;
    mNewData[index++] = 0x7B; // descriptor_tag
    mNewData[index++] = 0x13; // descriptor_length
    mNewData[index++] = 0xC0; // has substream_core, has substream_0


    uint8_t *data = mNewData;
    // subStream_core  info
    index += subStreamCore(mNewData + index);
    index += subStreamCore(mNewData + index);

    outLength = index;
    return mNewData;
}

int32_t  DTSAudioDescriptor::sampleFrequeceToIndex(int32_t sampleFrequece){
    switch(sampleFrequece) {
    case 24000:
        return 11;
    case 48000:
        return 12;
    case 96000:
        return 13;
    default:
        return 0;// TODO
    }
}

 int32_t DTSAudioDescriptor::subStreamCore(uint8_t *data) {
     int32_t index = 0;

     data[index++] = 0; // subStream data length
     data[index++] =   mDTSParam.channelCount & 0x1F ; // asset number

     uint8_t temp = (mDTSParam.lfeFlag ? 1 : 0) << 7;
     temp |= (sampleFrequeceToIndex(mDTSParam.sampleFrequece) & 0x0F) << 3;
     temp |= (mDTSParam.sampleBits == 16 ? 0 : 1) << 2 ;
     temp &= 0xFC;
     data[index++] = temp;

     for (int32_t i = 0; i <= mDTSParam.assetCount; i++) {
         //asset info
         temp = (mDTSParam.assetConstruction & 0x1F) << 3;
         temp |= ((mDTSParam.vbrFlag ? 1 : 0 )<< 2);
         temp |= (mDTSParam.postEncodeBrScalingFlag ? 1 : 0) << 1;
         temp |= mDTSParam.componentTypeFlag ? 1 : 0;

         data[index++] = temp;

         temp = 0;
         temp = (mDTSParam.languageCodeFlag ? 1 : 0) << 7;
         temp |= ((mDTSParam.bitRate & 0x1FC0) >>6);

         data[index++] = temp;

         temp = 0;
         temp = (mDTSParam.bitRate &0x3F) << 2;
         temp &= 0xFC;
         data[index++] = temp;
         if (mDTSParam.componentTypeFlag) {
             data[index++] = 0; 
         }

         if (mDTSParam.languageCodeFlag) {
             // default  english
             data[index++] = 0x65;
             data[index++] = 0x6E;
             data[index++] = 0x67;
         }
     }

     data[0] = index - 1; 
     return index;
 }

/*
* DescriptorManager
*/
DescriptorManager::DescriptorManager(){

}
DescriptorManager::~DescriptorManager() {

}

int32_t DescriptorManager::parse(const uint8_t *data, int32_t dataLength) {
    if (data != NULL && dataLength <= 0) {
        return -1;
    }

    int32_t index = 0;

    while(index < dataLength) {
        uint8_t tag = data[index];

        index += 1;
        uint8_t descriptorLength = data[index];

        index += 1;
        
        Descriptor *des = NULL;
        if (tag == 5) {
            des = new RegistorDescriptor(tag, data+index, descriptorLength);
        } else if (tag == 0x7B) {
            des = new DTSAudioDescriptor(tag, data+index, descriptorLength);
        } else {
            des = new Descriptor(tag, data+index, descriptorLength);
        }

        des->parse();
        mDescriptors[tag] = des;
        
        index += descriptorLength;
    }
    return 0;
}

bool DescriptorManager::isDolbyVision() {
    std::map<int32_t, Descriptor*>::iterator it = mDescriptors.find(5);
    if (it != mDescriptors.end()) {
        RegistorDescriptor *des = (RegistorDescriptor*)it->second;
        return des->isDolbyVision();
    } else {
        return false;
    }
}