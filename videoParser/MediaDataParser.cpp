#include "StdAfx.h"
#include "MediaDataParser.h"
#include <libavutil/pixfmt.h>

uint32_t gCrcTable[] = { 0xFF };

Descriptor::Descriptor() : mIsDolbyVision(false) {
    mDoVi = 0x44 << 24 | 0x4F << 16 | 0x56 << 8 | 0x49;
}

Descriptor::~Descriptor() {

}

int32_t Descriptor::parse(const uint8_t *data, int32_t dataLength) {
    if (data != NULL && dataLength <= 0) {
        return -1;
    }

    int32_t index = 0;

    while(index < dataLength) {
        uint8_t tag = data[index];
        index += 1;
        uint8_t descriptorLength = data[index];

        index += 1;
        if (tag == 5) { // registor descriptor
            uint32_t id = data[index] << 24 | data[index+1] << 16 | data[index+2] << 8 | data[index+3];
            mIsDolbyVision = id == mDoVi;
            break;
        } else {
	        index += descriptorLength;
        }
		
    }
    return 0;
}




//////////////////////
MediaDataParser::MediaDataParser() : mIsHevc(false), mCurrentDataSize(0), mVideoCodecType(-1) {
}


MediaDataParser::~MediaDataParser(){
}

int32_t MediaDataParser::filterData(uint8_t *data, int32_t dataLength, int32_t &dataOffset, const uint8_t *pmtData, uint8_t *outputBuffer, int32_t &outDataLength) {
    if (data == NULL || dataLength <=0 /*|| pmtData == NULL*/ || outputBuffer == NULL) {  
        return 0;
    }

    int32_t index = 0;
    /*if (mCurrentDataSize > 0) {
        if (mCurrentDataSize < TS_PACKET_SIZE) {
            int32_t len = TS_PACKET_SIZE - mCurrentDataSize > dataLength ? dataLength : TS_PACKET_SIZE - mCurrentDataSize;
            if (len > 0) {
                memcpy(mBuffer + mCurrentDataSize, data, len);
                mCurrentDataSize += len;
                index = len;
                dataOffset = len;
            }
        }
    }

    if (mCurrentDataSize == TS_PACKET_SIZE) {
        //int32_t pid = ((mBuffer[1] & 0x1F) << 8) | mBuffer[2];
        if (mBuffer[0] != 0x47) {
            printf("data error \n");
        }
        parsePacket(mBuffer, TS_PACKET_SIZE);
        mCurrentDataSize = 0;

        fwrite(mBuffer, 1, TS_PACKET_SIZE, mDumpFile);
    }*/

    int32_t packetCount = 0;
    while (index + TS_PACKET_SIZE <= dataLength) {
        uint8_t *dataIndex = data + index;

        if (mCurrentDataSize > 0) {
            if (mCurrentDataSize < TS_PACKET_SIZE) {
                int32_t len = TS_PACKET_SIZE - mCurrentDataSize > dataLength ? dataLength : TS_PACKET_SIZE - mCurrentDataSize;
                if (len > 0) {
                    memcpy(mBuffer + mCurrentDataSize, data, len);
                    mCurrentDataSize += len;
                    index += len;
                    dataOffset = len;
                }
            }
        } else {
            index += TS_PACKET_SIZE;
        }

        if (mCurrentDataSize == TS_PACKET_SIZE) {
            dataIndex = mBuffer;
            mCurrentDataSize = 0;
        }

        parsePacket(dataIndex, TS_PACKET_SIZE);

        packetCount++;
        //int32_t pid = ((dataIndex[1] & 0x1F) << 8) | dataIndex[2];
        if (dataIndex[0] != 0x47) {
            printf("data error 2  \n");
        }
        fwrite(dataIndex, 1, TS_PACKET_SIZE, mDumpFile);
    }

    if (index < dataLength) {
        if (dataLength - index <= TS_PACKET_SIZE) {
            memcpy(mBuffer + mCurrentDataSize, data+index, dataLength - index);
            mCurrentDataSize += dataLength - index;
        } else {
            //UNILOGE("filterData, maybe error");
        }
    }

    printf("filterData, totalDataSize:%d, mCurrentDataSize:%d, index:%d, dataOffset:%d \n", dataLength, mCurrentDataSize, index, dataOffset);
    return index - dataOffset;
}

int MediaDataParser::parseData(const uint8_t * data, int32_t dataLength) {
    if (mCurrentDataSize > 0) {
        if (mCurrentDataSize + dataLength < TS_PACKET_SIZE) {
            memcpy(mPacketData + mCurrentDataSize, data, dataLength);
            mCurrentDataSize += dataLength;
        } else {
            int32_t leftSize = TS_PACKET_SIZE - mCurrentDataSize > dataLength ? dataLength : TS_PACKET_SIZE - mCurrentDataSize; // TODO
            memcpy(mPacketData + mCurrentDataSize, data, leftSize);
            parsePacket(mPacketData, TS_PACKET_SIZE);
            dataLength -= leftSize;

            mCurrentDataSize = 0;

            int32_t offset = leftSize;
            while (!isParsed() && dataLength >= TS_PACKET_SIZE) {
                parsePacket(data + offset, TS_PACKET_SIZE);
                offset += TS_PACKET_SIZE;
                dataLength -= TS_PACKET_SIZE;
            }
            
            if (isParsed()) {
                mCurrentDataSize = 0;
                return 0;
            }

            if (dataLength > 0) {
                memcpy(mPacketData, data + offset , dataLength);
            }
            mCurrentDataSize = dataLength;
        }
    } else {
        int index = 0;
        while (dataLength >= TS_PACKET_SIZE) {
            parsePacket(data + index * TS_PACKET_SIZE, TS_PACKET_SIZE);
            dataLength -= TS_PACKET_SIZE;
            index++;

            if (isParsed()) {
                mCurrentDataSize = 0;
                return 0;
            }

        }

        if (dataLength > 0) {
            memcpy(mPacketData + mCurrentDataSize, data + index * TS_PACKET_SIZE, dataLength);
            mCurrentDataSize += dataLength;
        }
    }


    return 0;
}

int MediaDataParser::parsePacket(const uint8_t * data, int32_t dataLength) {
    if (data == NULL) {
        return -1;
    }

    int pid = ((data[1] & 0x1F) << 8) | data[2];



    if (pid == 0){
        mPmtPid = parsePat(&mPat, data + 5);
    } else if (pid == 0x11) {
        TsSdtTable sdt;
        parseSdt(&sdt, data + 5 + data[4]);
        uint8_t sdtData[188] = { 0 };
        memcpy(sdtData, data, 188);
        resetSdt(&sdt, sdtData + 5 + sdtData[4]);
    } else if (mPmtPid == pid){
        parserPmt(&mPmt, data + 5 + data[4]);
        /*if (mPmt.pmtStream.size() > 1 && mPmt.pmtStream[0].esInfoLenght > 0) {
            int32_t pmtLength = mPmt.pmtStream[0].vDataLength + 1 + 4 + data[4];
            uint8_t buffer[188] = { 0 };
            uint8_t *index = buffer;
            memcpy(buffer, data, pmtLength);
            printf("[parsePacket] mBuffer[5]=%d", buffer[5]);

            // audio descriptor
            uint8_t audioDescriptor[15] = { 0x87, 0xE1, 0x01, 0xF0, 0x0A, 0xCC, 0x08, 0xC0, 0xC4, 0x90, 0x75, 0x6E, 0x64, 0x01, 0x10 };
            //uint8_t audioDescriptor[5] = { 0x87, 0xE1, 0x01, 0xF0, 0x00 };
            memcpy(buffer + pmtLength, audioDescriptor, sizeof(audioDescriptor));

            int32_t audioDataLength = sizeof(audioDescriptor);
            int32_t dataLength = 0;

            dataLength += 2; // program_number
            dataLength += 1; // version
            dataLength += 1; //section_number
            dataLength += 1; //last_section_number
            dataLength += 2; // pcr_pid;
            dataLength += 2; // program_info_length

            dataLength += 1; // stream_type
            dataLength += 2; // element_pid
            dataLength += 2; // es_info_length
            dataLength += mPmt.pmtStream[0].esInfoLenght;

            dataLength += audioDataLength;
            dataLength += 4; // crc_length

            int32_t dataIndex = 3; // from 0
            dataIndex += 1; // adaption
            dataIndex += data[dataIndex]; //adaption_length;
            printf("[parsePacket] adaption length:%d", data[dataIndex]);

            dataIndex += 1; // table_id
            int32_t tableDataIndex = dataIndex;

            if (dataLength > 0x3FD) {
                printf("PMT info, section_length(%d) is too long", dataLength);
            }

            dataIndex += 1;
            uint32_t section_length = 0xB000 | dataLength;
            buffer[dataIndex] = section_length >> 8;
            dataIndex += 1;
            buffer[dataIndex] = section_length;

            if (gCrcTable[0] == 0xFF){
                makeCrcTable(gCrcTable);
            }

            int32_t allDataLen =  dataLength - 4 + 3; // from table_id to crc (exclude crc length)
            uint32_t crc = crc32Calculate(buffer + 5, allDataLen);
            //uint32_t crc = crc32Calculate(mBuffer + dataIndex, 12 + mPmt.pmtStream[0].esInfoLenght + 5 + sizeof(audioDescriptor));

            printf("[parsePacket] section_lenth:%0x/%d, pmtLength:%d, video data length:%d, audio data length:%d, allDataLength:%d", dataLength, dataLength, pmtLength, mPmt.pmtStream[0].esInfoLenght, sizeof(audioDescriptor), allDataLen);
            int crcIndex = pmtLength + sizeof(audioDescriptor);
            buffer[crcIndex] = (uint8_t)((crc >> 24) & 0xFF);
            buffer[crcIndex + 1] = (uint8_t)((crc >> 16) & 0xFF);
            buffer[crcIndex + 2] = (uint8_t)((crc >> 8) & 0xFF);
            buffer[crcIndex + 3] = (uint8_t)(crc & 0xFF);

            printf("");
        }
        */

    }else if (pid == mVideoPID) {
        int32_t extraDataLength = dataLength;
        const uint8_t *extraData = getExtraData(data, extraDataLength);
        if (extraData != NULL) {
            memcpy(mSPS.extraData, extraData, extraDataLength);
            mSPS.extraDataLength = extraDataLength;
        }

        parseVideo(data, TS_PACKET_SIZE);
    }

    return 0;
}

int MediaDataParser::parsePat(TsPatTable *patTable, const uint8_t *buffer){
    int pmtID = -1;
    patTable->tableId                   = buffer[0];
    patTable->sectionSyntaxIndicator    = buffer[1] >> 7;
    patTable->zero                      = buffer[1] >> 6 & 0x1;
    patTable->reserved_1                = buffer[1] >> 4 & 0x3;
    patTable->sectionLength             = (buffer[1] & 0x0F) << 8 | buffer[2]; 
    patTable->transportStreamId         = buffer[3] << 8 | buffer[4];
    patTable->reserved_2                = buffer[5] >> 6;
    patTable->versionNumber             = buffer[5] >> 1 &  0x1F;
    patTable->currentNextIndicator      = (buffer[5] << 7) >> 7;
    patTable->sectionNumber             = buffer[6];
    patTable->lastSectionNumber         = buffer[7];

    int len = 0;
    len = 3 + patTable->sectionLength;
    patTable->crc_32 = (buffer[len-4] & 0x000000FF) << 24
        | (buffer[len-3] & 0x000000FF) << 16
        | (buffer[len-2] & 0x000000FF) << 8 
        | (buffer[len-1] & 0x000000FF);

    int n = 0;
    for ( n = 0; n < patTable->sectionLength - 12; n += 4 ){
        unsigned  program_num = buffer[8 + n ] << 8 | buffer[9 + n ];  
        patTable->reserved_3                = buffer[10 + n ] >> 5; 

        patTable->networkPid = 0x00;
        if ( program_num == 0x00){  
            patTable->networkPid = (buffer[10 + n ] & 0x1F) << 8 | buffer[11 + n ];
            unsigned pid = patTable->networkPid;
        } else {
            TsPatProgram PAT_program;
            PAT_program.program_map_PID = (buffer[10 + n] & 0x1F) << 8 | buffer[11 + n];
            PAT_program.program_number = program_num;
            patTable->program.push_back( PAT_program );

            pmtID = PAT_program.program_map_PID;
        }         
    }

    return pmtID;
}

int MediaDataParser::parserPmt(TsPmtTable *pmtTable, const uint8_t *buffer) {
 
    pmtTable->tableId                           = buffer[0];
    pmtTable->sectionSyntaxIndicator            = buffer[1] >> 7;
    pmtTable->zero                              = buffer[1] >> 6 & 0x01;
    pmtTable->reserved_1                        = buffer[1] >> 4 & 0x03;
    pmtTable->sectionLength                     = (buffer[1] & 0x0F) << 8 | buffer[2];
    pmtTable->programNumber                     = buffer[3] << 8 | buffer[4];
    pmtTable->reserved_2                        = buffer[5] >> 6;
    pmtTable->versionNumber                     = buffer[5] >> 1 & 0x1F;
    pmtTable->currentNextIndicator              = (buffer[5] << 7) >> 7; 
    pmtTable->sectionNumber                     = buffer[6];
    pmtTable->lastSectionNumber                 = buffer[7];  
    pmtTable->reserved_3                        = buffer[8] >> 5;
    pmtTable->pcrPid                            = ((buffer[8] << 8) | buffer[9]) & 0x1FFF;
    pmtTable->reserved_4                        = buffer[10] >> 4;
    pmtTable->programInfoLength                 = (buffer[10] & 0x0F) << 8 | buffer[11];
    // Get CRC_32
    int len = 0;  
    len = pmtTable->sectionLength + 3;      
    pmtTable->crc_32 = (buffer[len-4] & 0x000000FF) << 24 | (buffer[len-3] & 0x000000FF) << 16 | (buffer[len-2] & 0x000000FF) << 8  | (buffer[len-1] & 0x000000FF);   

    int pos = 12;  
    // program info descriptor  
    if ( pmtTable->programInfoLength != 0 ){
        pos += pmtTable->programInfoLength;
    }
    // Get stream type and PID      
    for ( ; pos <= (pmtTable->sectionLength + 2 ) -  4; ) {  
        TsPmtStream pmt_stream;  
        pmt_stream.streamType    = buffer[pos];
        pmtTable->reserved_5     = buffer[pos+1] >> 5;
        pmt_stream.elementaryPid = ((buffer[pos+1] << 8) | buffer[pos+2]) & 0x1FFF;
        pmtTable->reserved_6     = buffer[pos+3] >> 4;
        pmt_stream.esInfoLenght  = (buffer[pos+3] & 0x0F) << 8 | buffer[pos+4];
        pmt_stream.vDataLength     = pmt_stream.esInfoLenght + pos + 5;
        //pmt_stream.descriptor     = 0x00;  
        if (pmt_stream.esInfoLenght != 0) {  
            //pmt_stream.descriptor = buffer[pos + 5];
			pmt_stream.AddDescriptor(buffer + pos + 5, pmt_stream.esInfoLenght);
            pos += pmt_stream.esInfoLenght;

			printf("isDolbyVision:%d", pmt_stream.isDolbyVision());
        }

        if (pmt_stream.streamType == 0x24) {
            mVideoCodecType = 1;
            mVideoPID = pmt_stream.elementaryPid;
        } else if (pmt_stream.streamType == 0x1B) {
            mVideoCodecType = 0;
            mVideoPID = pmt_stream.elementaryPid;
        } else if (pmt_stream.streamType == 0x06) {
            mVideoPID = pmt_stream.elementaryPid;
        } 
        else if (pmt_stream.streamType == 0x0F){
            // AAC Stream
        } else if (pmt_stream.streamType == 0x87){ 
            // EAC-3
        } else if (pmt_stream.streamType == 0xC1) {
            //AC-3
        }

        pos += 5;  
        pmtTable->pmtStream.push_back(pmt_stream);
       
    }  
    return 0;  
}

int MediaDataParser::parseSdt(TsSdtTable *stdTable, const uint8_t *buffer) {
    stdTable->tableId = buffer[0];
    stdTable->sectionSyntaxIndicatior = buffer[1] >> 7;
    stdTable->reservedFutureUse1 = buffer[1] >> 6 & 0x01;
    stdTable->reserved = buffer[1] >> 4 && 0x03;
    stdTable->sectionLength = (buffer[1] & 0x0F) << 8 | buffer[2];
    stdTable->transportStreamId = buffer[3] << 8 | buffer[4];
    stdTable->reserved2 = buffer[5] >> 6;
    stdTable->versionNumber = buffer[5] & 0x1F;
    stdTable->currentNextIndicator = buffer[5] & 0x01;
    stdTable->sectionNumber = buffer[6];
    stdTable->lastSectionNumber = buffer[7];
    stdTable->originalNetworkId = buffer[8] << 8 | buffer[9];
    stdTable->reservedFutureUse2 = buffer[10];

    SdtItem item;
    int32_t i = 11;
    while(i < stdTable->sectionLength - 1) {
        item.serviceId = buffer[i] << 8 | buffer[i+1];
        item.reservedFutureUse = buffer[i+2] >> 2;
        item.eitScheduleFlag = buffer[i+2] & 0x02;
        item.eitPresentFollowingFlag = buffer[i+2] & 0x01;
        item.runningStatus = buffer[i+3] >> 5;
        item.freeCAMode = buffer[i+3] >> 4  & 0x01;
        item.descriptorsLoopLength = buffer[i+3] & 0xFF << 8 | buffer[i+4];

        int32_t curPos = i+5;
        int32_t j = 0;
        while(j < item.descriptorsLoopLength) {
            SdtDescriptor  descriptor;
            descriptor.descriptorTag = buffer[curPos];
            descriptor.descriptorLength = buffer[curPos+1];
            descriptor.serviceType = buffer[curPos+2];
            descriptor.serviceProviderNameLength = buffer[curPos+3];
            curPos += 3;
            descriptor.serviceProvider = new uint8_t[descriptor.serviceProviderNameLength + 1];

            memset(descriptor.serviceProvider, 0, descriptor.serviceProviderNameLength + 1);
            memcpy(descriptor.serviceProvider, buffer+curPos+1, descriptor.serviceProviderNameLength);
            curPos += 1 + descriptor.serviceProviderNameLength;

            descriptor.serviceNameLength = buffer[curPos];
            descriptor.serviceName = new uint8_t[descriptor.serviceNameLength];
            memset(descriptor.serviceName, 0, descriptor.serviceNameLength+1);
            memcpy(descriptor.serviceName, buffer+curPos+1, descriptor.serviceNameLength);
            curPos += 1 + descriptor.serviceNameLength;
             
            j += 5+descriptor.serviceProviderNameLength+descriptor.serviceNameLength;

            item.descriptor.push_back(descriptor);
        };

        i += item.descriptorsLoopLength + 5;

        stdTable->stdItems.push_back(item);
    };

    return 0;
}

int MediaDataParser::resetSdt(TsSdtTable *stdTable, uint8_t *buffer) {
    if (buffer == NULL) {
        return -1;
    }

    const std::string serviceProviderName = "actinidia";
    const std::string serviceName = "MPEG_TS";
    int32_t dataIndex = 1; // table_id;
    dataIndex += 2; // section_length;
    dataIndex += 2; // transport_stream_id;
    dataIndex += 1;
    dataIndex += 1; // section_number;
    dataIndex += 1; // last_section_number;
    dataIndex += 2; // original_network_id;
    dataIndex += 1; // reserved;
    uint32_t dataLength = 8; // from section_number
    for (int32_t i = 0; i < stdTable->stdItems.size(); i++) {
        int32_t itemDataIndex = dataIndex + 2 + 1;
        int32_t descriptorLength = 0;
        dataLength += 2 + 1 + 2;  // service_id + reserved_future_use + descriptor_loop_lenght
        dataIndex += 2 + 1 + 2;
   
        SdtItem items = stdTable->stdItems[i];
        for (int32_t j=0; j < items.descriptor.size(); j++) {
            SdtDescriptor des = items.descriptor[j];
            dataLength += 5;

            dataIndex += 3;
            buffer[dataIndex] = serviceProviderName.length();
            dataIndex += 1;
            memcpy(buffer+dataIndex, serviceProviderName.c_str(), serviceProviderName.length());
            dataIndex += serviceProviderName.length();

            buffer[dataIndex] = serviceName.length();
            dataIndex += 1;
           
            memcpy(buffer+dataIndex, serviceName.c_str(), serviceName.length());
            dataLength += serviceProviderName.length() + serviceName.length();
            dataIndex += serviceName.length();

            descriptorLength = 5 + serviceName.length() + serviceProviderName.length();
        }

        uint32_t section_length = 0xB000 | descriptorLength;
        buffer[itemDataIndex] = (uint8_t)(section_length >> 8);
        buffer[itemDataIndex+1] = (uint8_t)(section_length);
    }


    uint32_t section_length = 0xB000 | dataLength + 4; // dataLength + crc
    buffer[1] = (uint8_t)(section_length >> 8);
    buffer[2] = (uint8_t)(section_length);

    uint32_t crc = crc32Calculate(buffer, dataLength + 3);
    buffer[dataIndex] = (uint8_t)((crc >> 24) & 0xFF);
    buffer[dataIndex + 1] = (uint8_t)((crc >> 16) & 0xFF);
    buffer[dataIndex + 2] = (uint8_t)((crc >> 8) & 0xFF);
    buffer[dataIndex + 3] = (uint8_t)(crc & 0xFF);


    // 
    //crc = crc32Calculate(buffer, dataLength + 3 + 4);

	return 0;
}

int MediaDataParser::parseVideo(const uint8_t * buffer, int32_t bufferLength) {
    if (buffer == NULL || bufferLength < 4) {
        return -1;
    }

    int startCodePos = findStartCode(buffer, bufferLength);
    if (startCodePos < 0) {
        return startCodePos;
    }

    int32_t i = startCodePos;
    while(i < bufferLength - 4) {
        if(buffer[i] == 0 && buffer[i+1] == 0 && buffer[i+2] == 1) {
            if (mVideoCodecType == -1) {
                int32_t codecType = parseVideoCodecType(buffer + i, bufferLength);
                if (codecType != -1) {
                    mVideoCodecType = codecType;
                }
            }
            
            if (mVideoCodecType == V_HEVC) {
                uint16_t header = buffer[i+3] << 8 | buffer[i+4];
                uint8_t nal_unit_type   = (header & 0x7e00) >> 9;
                if (nal_unit_type == 0x21) { //SPS
                    HevcParseSPS(buffer + i + 5, TS_PACKET_SIZE - i - 3);
                    break;
                }
            } else if(mVideoCodecType == V_AVC) {
                uint8_t nal_unit_type = buffer[i+3] & 0x1F;
                if (nal_unit_type == 0x7) {
                    H264ParseSPS(buffer + i + 4 , TS_PACKET_SIZE - i - 4);
                }
            }
        }
        i++;
    }

    return 0;
}



void MediaDataParser::parseVideo() {
    uint8_t buffer[512] = { 0 };
    mIsHevc = true;
    char *file = mIsHevc ? "dolby_vision_1500.ts" : "88289853e08e9627c716cdc2e4d65d32.ts";
    FILE *f = fopen(file, "rb");
    if (f == NULL) {
        return;
    }

    mDumpFile = fopen("dump.ts", "wb");
    if (mDumpFile == NULL) {
        return;
    }

    if (gCrcTable[0] == 0xFF){
        makeCrcTable(gCrcTable);
    }

    int32_t packetIndex = 1;
    SPS_INFO sps_info;
    while(!feof(f) /*&& !isParsed()*/) {
        size_t dataLength = fread(buffer, 1, 500, f);
        
        int32_t dataOffset = 0;
        uint8_t cachePacket[188];
        int32_t outputLength = 0;
        filterData(buffer, dataLength, dataOffset,  NULL,  cachePacket, outputLength);
    }

    if (mCurrentDataSize > 0) {
        fwrite(mBuffer, 1, mCurrentDataSize, mDumpFile);
    }

    if (mDumpFile != NULL) {
        fclose(mDumpFile);
    }
}

void MediaDataParser::testCrc() {

    //uint8_t data[] = {  0x02, 0xB0, 0x1D, 0x00, 0x01, 0xC1, 0x00, 0x00, 0xE1, 0x00, 0xF0, 0x00, 0x1B, 0xE1, 0x00, 0xF0, 0x00, 0x0F, 0xE1, 0x01, 0xF0, 0x06, 0x0A, 0x04, 0x75, 0x6E,  0x64,  0x00 };
    //uint8_t data[] = {0x02, 0xB0, 0x32, 0x00 , 0x01 , 0xC1 , 0x00 , 0x00 , 0xE1 , 0x00 , 0xF0 , 0x00 , 0x06 , 0xE1 , 0x00 , 0xF0 , 0x1B , 0x05 , 0x04 , 0x44 , 0x4F , 0x56 , 0x49 , 0x38 , 0x0D , 0x02 , 0x20 , 0x00 , 0x00 , 0x00 , 0x90 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x78 , 0xF0 , 0xB0 , 0x04 , 0x01 , 0x00 , 0x0A , 0x1D , 0x87 , 0xE1 , 0x01 , 0xF0 , 0x00 };

    // ÅË½ðÁ«
    uint8_t data[] = {0x02, 0xB0 , 0x3C , 0x00 , 0x01 , 0xC1 , 0x00 , 0x00 , 0xE1 , 0x00 , 0xF0 , 0x00 , 0x06 , 0xE1 , 0x00 , 0xF0 , 0x1B , 0x05 , 0x04 , 0x44 , 0x4F , 0x56 , 0x49 , 0x38 , 0x0D , 0x02 , 0x20 , 0x00 , 0x00 , 0x00 , 0x90 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x78 , 0xF0 , 0xB0 , 0x04 , 0x01 , 0x00 , 0x0A , 0x1D , 0x87 , 0xE1 , 0x01 , 0xF0 , 0x0A , 0xCC , 0x08 , 0xC0 , 0xC4 , 0x90 , 0x75 , 0x6E , 0x64 , 0x01 , 0x10, 0xB8, 0x49 , 0xF9 ,0xF2  };
    if (gCrcTable[0] == 0xFF){
        makeCrcTable(gCrcTable);
    }

    int length = sizeof(data);
    uint32_t crc = crc32Calculate(data, length);


    uint32_t value = 0xB000;
    value |= 60;

    uint8_t  section_len[2];
    section_len[0] = value >> 8;
    section_len[1] = value;
    uint8_t *index = section_len;

    printf("");
}

const uint8_t *MediaDataParser::getExtraData(const uint8_t *data, int &length) {
   /* if (data == NULL || length < 4) {
        return NULL;
    }

    if (data[0] != 0x47) {
        return NULL;
    }

    int32_t pid = (data[1] << 8 | data[2]) & 0x1FFF;
    if (pid != 256){
        return NULL;
    }

    int32_t dataBegin = 4;

    bool payloadStart = ((data[1] << 8 | data[2]) & 0x4000) != 0;
    if (!payloadStart) {
        return NULL;
    }


    bool hasAdaptation = (data[3] & 0x20) != 0;
    if (hasAdaptation) {
        int32_t adaptationLength = data[4];
        dataBegin = dataBegin + 1 + adaptationLength;
    }

    dataBegin += 3; // 001
    dataBegin += 5; // PES size

    int32_t pesPayLoadSize =  data[dataBegin];
    dataBegin += pesPayLoadSize + 1; // 0001
    */

    int32_t dataBegin = findStartCode(data, length);
    if (dataBegin < 0) {
        return NULL;
    }

    int32_t extraDataLength = 0;
    int32_t dataIndex = dataBegin;
    bool foundPPS = false;
    while(dataIndex < length - 4){
        if (data[dataIndex] == 0 && data[dataIndex+1] == 0 && data[dataIndex+2] == 1) {
            if (mIsHevc) {
                uint16_t header = data[dataIndex+3] << 8 | data[dataIndex+4];
                uint8_t nalUnitType   = (header & 0x7e00) >> 9;
                //if (nalUnitType == 0x20 || nalUnitType == 0x21 || nalUnitType == 0x22 || nalUnitType == 0x23) {
                if (nalUnitType == 0x22) {
                    foundPPS = true;
                    dataIndex++;
                    continue;
                } else {
                    //dataIndex++;
                }
            } else {
                uint8_t nalUnitType = data[dataIndex+3] & 0x1F;
                //if (nalUnitType == 0x9 || nalUnitType == 0x7 || nalUnitType == 0x8) {
                if (nalUnitType == 0x08) {
                    dataIndex++;
                    continue;
                } else {
                    //dataIndex++;
                }
            }
           
            if (foundPPS) {
                extraDataLength = dataIndex - dataBegin - 1;
                break;
            }

            dataIndex++;
        } else {
            dataIndex++;
        }
    }

    if (extraDataLength > 0) {
        length = extraDataLength;
        return data+dataBegin;
    } else {
        return NULL;
    }
}

int MediaDataParser::parseVideoCodecType(const uint8_t *data, int length) {
    if (data == NULL || length < 4) {
        return -1;
    }

    int32_t dataIndex = 0;
    while (dataIndex < length - 4) {
        if (data[dataIndex] == 0 && data[dataIndex+1] == 0 && data[dataIndex+2] == 1) {
            uint16_t header = data[dataIndex+3] << 8 | data[dataIndex+4];
            uint8_t nalUnitType  = (header & 0x7e00) >> 9;
            uint8_t nalUnitType2 = data[dataIndex+3] & 0x1F;
            if (nalUnitType == 0x21) {
                return 1;
            }

            if (nalUnitType2 == 0x07) {
                return 0;
            }
            dataIndex++;
        } else {

            dataIndex++;
        }

    }
     
    return -1;
}

SPS_INFO MediaDataParser::HevcParseSPS(const uint8_t *data, int length) {
    SPS_INFO &info = mSPS;
    Bitstream bs(data, length*8, true);
    unsigned int i;
    int sub_layer_profile_present_flag[8], sub_layer_level_present_flag[8];

    bs.skipBits(4); // sps_video_parameter_set_id

    unsigned int sps_max_sub_layers_minus1 = bs.readBits(3);
    bs.skipBits(1); // sps_temporal_id_nesting_flag

    // skip over profile_tier_level
    unsigned int general_profile_space = bs.readBits(2);
    unsigned int general_tier_flag = bs.readBits(1);
    unsigned int general_profile_idc = bs.readBits(5);
    bs.skipBits(32 + 4 + 43 + 1);
    unsigned int general_level_idc = bs.readBits(8);

    for (i=0; i<sps_max_sub_layers_minus1; i++)
    {
        sub_layer_profile_present_flag[i] = bs.readBits(1);
        sub_layer_level_present_flag[i] = bs.readBits(1);
    }
    if (sps_max_sub_layers_minus1 > 0)
    {
        for (i=sps_max_sub_layers_minus1; i<8; i++)
            bs.skipBits(2);
    }
    for (i=0; i<sps_max_sub_layers_minus1; i++)
    {
        if (sub_layer_profile_present_flag[i])
            bs.skipBits(8 + 32 + 4 + 43 + 1);
        if (sub_layer_level_present_flag[i])
            bs.skipBits(8);
    }
    // end skip over profile_tier_level

    bs.readGolombUE(); // sps_seq_parameter_set_id
    unsigned int chroma_format_idc = bs.readGolombUE();

    if (chroma_format_idc == 3)
        bs.skipBits(1); // separate_colour_plane_flag

    info.width  = bs.readGolombUE();
    info.height = bs.readGolombUE();

    int conformance_window_flag = bs.readBits1();
    if (conformance_window_flag) {
        bs.readGolombUE();
        bs.readGolombUE();
        bs.readGolombUE();
        bs.readGolombUE();
    }

    unsigned int bit_depth_luma_minus8 =  bs.readGolombUE();
    unsigned int bit_depth_chroma_minus8 = bs.readGolombUE();

    info.format = mapPixelFormatHEVC(bit_depth_chroma_minus8 + 8, chroma_format_idc);
    info.finished = true;

    return info;
}

SPS_INFO MediaDataParser::H264ParseSPS(const uint8_t *data, int length) {
    SPS_INFO &info = mSPS;

    Bitstream bs(data, length*8, true);
    bs.mOffset = 0;
    int profileIdc = bs.readBits(8);
    info.profileIdc = profileIdc;
    /* constraint_set0_flag = bs.readBits1();    */
    /* constraint_set1_flag = bs.readBits1();    */
    /* constraint_set2_flag = bs.readBits1();    */
    /* constraint_set3_flag = bs.readBits1();    */
    /* reserved             = bs.readBits(4);    */
    bs.skipBits(8);
    int levelIdc = bs.readBits(8);
    info.level = levelIdc;
    unsigned int seq_parameter_set_id = bs.readGolombUE(9);
    if( profileIdc == 100 || profileIdc == 110 ||
        profileIdc == 122 || profileIdc == 244 || profileIdc == 44 ||
        profileIdc == 83 || profileIdc == 86 || profileIdc == 118 ||
        profileIdc == 128 ) {
        int chromaFormatIdc = bs.readGolombUE(9); // chroma_format_idc
        if(chromaFormatIdc == 3) {
            bs.skipBits(1);           // residual_colour_transform_flag
        }

        unsigned int bit_depth_luma_minus8   = bs.readGolombUE(); // bit_depth_luma - 8 
        unsigned int bit_depth_chroma_minus8 = bs.readGolombUE(); // bit_depth_chroma - 8          
        info.format = mapPixelFormatAVC(bit_depth_chroma_minus8 + 8, chromaFormatIdc);

        bs.skipBits(1);    // transform_bypass
        if (bs.readBits1()) { // seq_scaling_matrix_present
            for (int i = 0; i < ((chromaFormatIdc != 3) ? 8 : 12); i++) {
                if (bs.readBits1()) { // seq_scaling_list_present
                    int last = 8, next = 8, size = (i<6) ? 16 : 64;
                    for (int j = 0; j < size; j++) {
                        if (next)
                            next = (last + bs.readGolombSE()) & 0xff;
                        last = !next ? last: next;
                    }
                }
            }
        }
    }

    int log2_max_frame_num_minus4 = bs.readGolombUE(); // log2_max_frame_num - 4
    int picOrderCntType = bs.readGolombUE(9);
    if (picOrderCntType == 0) {
        bs.readGolombUE(); // log2_max_poc_lsb - 4 
    } else if (picOrderCntType == 1) {
        bs.readBits1();
        bs.readGolombSE();         // offset_for_non_ref_pic       
        bs.readGolombSE();         // offset_for_top_to_bottom_field  
        unsigned int tmp = bs.readGolombUE();   // num_ref_frames_in_pic_order_cnt_cycle 
        for (unsigned int i = 0; i < tmp; i++) {
            bs.readGolombSE();       // offset_for_ref_frame[i]
        }
    } else if(picOrderCntType != 2) {
        return info;
    }

    bs.readGolombUE(9); // ref_frames
    bs.skipBits(1);     // gaps_in_frame_num_allowed
    int32_t width   = bs.readGolombUE() + 1;
    int32_t height  = bs.readGolombUE() + 1;
    unsigned int frameMbsOnly = bs.readBits1();

    info.width = width * 16;
    info.height= height * 16 * (2-frameMbsOnly);
    if (!frameMbsOnly){
        bs.readBits1();
    }
    bs.skipBits(1);     // direct_8x8_inference_flag
    if (bs.readBits1()) { // frame_cropping_flag
        uint32_t cropLeft   = bs.readGolombUE();
        uint32_t cropRight  = bs.readGolombUE();
        uint32_t cropTop    = bs.readGolombUE();
        uint32_t cropBottom = bs.readGolombUE();
   
        info.width -= 2*(cropLeft + cropRight);
        if (frameMbsOnly) {
            info.height -= 2*(cropTop + cropBottom);
        } else {
            info.height -= 4*(cropTop + cropBottom);
        }
    }

    info.finished = true;
    return info;
}

int32_t MediaDataParser::findStartCode(const uint8_t *data, int length) {
    if (data == NULL || length < 4) {
        return -1;
    }

    if (data[0] != 0x47) {
        return -1;
    }

    int32_t pid = (data[1] << 8 | data[2]) & 0x1FFF;
    if (pid != mVideoPID){
        return -1;
    }

    int32_t dataBegin = 4;

    bool payloadStart = ((data[1] << 8 | data[2]) & 0x4000) != 0;
    if (!payloadStart) {
        return -1;
    }


    bool hasAdaptation = (data[3] & 0x20) != 0;
    if (hasAdaptation) {
        int32_t adaptationLength = data[4];
        dataBegin = dataBegin + 1 + adaptationLength;
    }

    dataBegin += 3; // 001
    dataBegin += 5; // PES size

    int32_t pesPayLoadSize =  data[dataBegin];
    dataBegin += pesPayLoadSize + 1; // 0001

    return dataBegin;
}

int32_t MediaDataParser::mapPixelFormatHEVC(int32_t bitDepth, int32_t chromaFormatIdc) {
    int32_t fmt = -1;
    switch (bitDepth) {
    case 8:
        if (chromaFormatIdc == 0) fmt = AV_PIX_FMT_GRAY8;
        else if (chromaFormatIdc == 1) fmt = AV_PIX_FMT_YUV420P;
        else if (chromaFormatIdc == 2) fmt = AV_PIX_FMT_YUV422P;
        else if (chromaFormatIdc == 3) fmt = AV_PIX_FMT_YUV444P;
        break;
    case 9:
        if (chromaFormatIdc == 0) fmt = AV_PIX_FMT_GRAY16;
        else if (chromaFormatIdc == 1) fmt = AV_PIX_FMT_YUV420P9;
        else if (chromaFormatIdc == 2) fmt = AV_PIX_FMT_YUV422P9;
        else if (chromaFormatIdc == 3) fmt = AV_PIX_FMT_YUV444P9;
        break;
    case 10:
        if (chromaFormatIdc == 0) fmt = AV_PIX_FMT_GRAY10;
        else if (chromaFormatIdc == 1) fmt = AV_PIX_FMT_YUV420P10;
        else if (chromaFormatIdc == 2) fmt = AV_PIX_FMT_YUV422P10;
        else if (chromaFormatIdc == 3) fmt = AV_PIX_FMT_YUV444P10;
        break;
    case 12:
        if (chromaFormatIdc == 0) fmt = AV_PIX_FMT_GRAY12;
        else if (chromaFormatIdc == 1) fmt = AV_PIX_FMT_YUV420P12;
        else if (chromaFormatIdc == 2) fmt = AV_PIX_FMT_YUV422P12;
        else if (chromaFormatIdc == 3) fmt = AV_PIX_FMT_YUV444P12;
        break;
    default:
        fmt = AV_PIX_FMT_NONE;
    }

    return fmt;
}
int32_t MediaDataParser::mapPixelFormatAVC(int32_t bitDepth, int32_t chromaFormatIdc) {
    int32_t fmt = -1;
    
    switch (bitDepth) {
    case 9:
        if (chromaFormatIdc == 3)      fmt = AV_PIX_FMT_YUV444P9;
        else if (chromaFormatIdc == 2) fmt = AV_PIX_FMT_YUV422P9;
        else                           fmt = AV_PIX_FMT_YUV420P9;
        break;
    case 10:
        if (chromaFormatIdc == 3)      fmt = AV_PIX_FMT_YUV444P10;
        else if (chromaFormatIdc == 2) fmt = AV_PIX_FMT_YUV422P10;
        else                           fmt = AV_PIX_FMT_YUV420P10;
        break;
    case 8:
        if (chromaFormatIdc == 3)      fmt = AV_PIX_FMT_YUV444P;
        else if (chromaFormatIdc == 2) fmt = AV_PIX_FMT_YUV422P;
        else                           fmt = AV_PIX_FMT_YUV420P;
        break;
    default:
        fmt = AV_PIX_FMT_NONE;
    }

    return fmt;
}

int MediaDataParser::makeCrcTable(uint32_t *crc32Table) {
    for(uint32_t i = 0; i < 256; i++ ) {  
        uint32_t k = 0;  
        for(uint32_t j = (i << 24) | 0x800000; j != 0x80000000; j <<= 1 ) {  
            k = (k << 1) ^ (((k ^ j) & 0x80000000) ? 0x04c11db7 : 0);  
        }  
        crc32Table[i] = k;  
    } 
    return 0;
} 

uint32_t MediaDataParser::crc32Calculate(const uint8_t *buffer, uint32_t size){
    uint32_t crc32Reg = 0xFFFFFFFF;
    for (uint32_t i = 0; i < size; i++) {
        crc32Reg = (crc32Reg << 8 ) ^ gCrcTable[((crc32Reg >> 24) ^ *buffer++) & 0xFF];
    }
    return crc32Reg;
} 
//////////////////////////////////////////////////
void Bitstream::skipBits(unsigned int num)
{
    if (mDoEP3)
    {

        while (num)
        {
            unsigned int tmp = mOffset >> 3;
            if (!(mOffset & 7) && (mData[tmp--] == 3) && (mData[tmp--] == 0) && (mData[tmp] == 0))
                mOffset += 8;   // skip EP3 byte

            if (!(mOffset & 7) && (num >= 8)) // byte boundary, speed up things a little bit
            {
                mOffset += 8;
                num -= 8;
            }
            else if ((tmp = 8-(mOffset & 7)) <= num) // jump to byte boundary
            {
                mOffset += tmp;
                num -= tmp;
            }
            else
            {
                mOffset += num;
                num = 0;
            }

            //mOffset += num;
            //num = 0;

            if (mOffset >= mDataLength)
            {
                mError = true;
                break;
            }
        }

        return;
    }

    mOffset += num;
}

unsigned int Bitstream::readBits(int num)
{
    unsigned int r = 0;

    while(num > 0)
    {
        if (mDoEP3)
        {
            size_t tmp = mOffset >> 3;
            if (!(mOffset & 7) && (mData[tmp--] == 3) && (mData[tmp--] == 0) && (mData[tmp] == 0))
                mOffset += 8;   // skip EP3 byte
        }

        if(mOffset >= mDataLength)
        {
            mError = true;
            return 0;
        }

        num--;

        if(mData[mOffset / 8] & (1 << (7 - (mOffset & 7))))
            r |= 1 << num;

        mOffset++;
    }
    return r;
}

unsigned int Bitstream::showBits(int num)
{
    unsigned int r = 0;
    size_t offs = mOffset;

    while(num > 0)
    {
        if(offs >= mDataLength)
        {
            mError = true;
            return 0;
        }

        num--;

        if(mData[offs / 8] & (1 << (7 - (offs & 7))))
            r |= 1 << num;

        offs++;
    }
    return r;
}

unsigned int Bitstream::readGolombUE(int maxbits)
{
    int lzb = -1;
    int bits = 0;

    for(int b = 0; !b; lzb++, bits++)
    {
        if (bits > maxbits)
            return 0;
        b = readBits1();
    }

    //return (1 << lzb) - 1 + readBits(lzb);

    int ret = (1 << lzb) - 1 + readBits(lzb);
    if (ret != 0) {
        int v  = ret;
        int pos = (v & 1);
        v = (v + 1) >> 1;

        printf("v:%d", v);
    }

    return ret;
}

signed int Bitstream::readGolombSE()
{
    int v = 0, pos = 0;
    v = readGolombUE();
    if(v == 0)
        return 0;

    pos = (v & 1);
    v = (v + 1) >> 1;
    return pos ? v : -v;
}
