/**
 * @file      StatCollect.cc
 * @brief     The c++ file of StatsCollectModule. StatsCollectModule is provided for RTC application to update their states to local file for states collection and training.
 * @repo      AlphaRTC
 * @author    Dan Yang <v-danya@microsoft.com>
 * @author    Peng Cheng <pengc@microsoft.com>
 * @version   0.1
 * @copyright Copyright (c) Microsoft Corporation. All rights reserved.
 * @license   Licensed under the MIT License.
 **/

#include "StatCollect.h"
#include "json.hpp"

namespace StatCollect {
    
    /**
     **========================================================
     ** StatsCollectInterface External Function StatsCollectModule
     **========================================================
    */
    /**
    ** The constructor function of StatsCollectModule class
    ** @param  SCType       collectType,  The collect type
    */
    StatsCollectModule::StatsCollectModule(SCType collectType)
        : collectType_(collectType),
          queueMutex_(new std::mutex) {}

    /**
     **========================================================
     ** StatsCollectInterface External Function !StatsCollectModule
     **========================================================
    */
    /**
    ** The destructor function of StatsCollectModule class
    */
    StatsCollectModule::~StatsCollectModule() {
        while (!collectQueue_.empty()) {
            void* tempPtr = collectQueue_.front();
            free(tempPtr);
            collectQueue_.pop();
        }
        free(queueMutex_);
    }

    /**
     **========================================================
     ** StatsCollectInterface External Function StatsCollect
     **========================================================
    */
    /**
     ** Collect the pakcet info ,video info, audio info from RTC Program
     ** Just Put the collect info in a buffer queue.
     ** @param  double         pacerPacingRate,   the pacer pacing rate
     ** @param  double         pacerPaddingRate,  the pacer padding rate
     ** //Pakcet Info
     ** @param  unsigned char  payloadType,       the RTP header Payload Type
     ** @param  unsigned short sequenceNumber,    the RTP header Sequence Number
     ** @param  unsigned int   sendTimestamp,     the RTP header send timestamp
     ** @param  unsigned int   ssrc,              the RTP header ssrc
     ** @param  unsigned long  paddingLength,     the RTP header Padding size
     ** @param  unsigned long  headerLength,      the RTP header Header size
     ** @param  long long      arrivalTimeMs,     the RTP packet receive timestamp
     ** @param  unsigned long  payloadSize,       the RTP pakcet data size
     ** @param  float          lossRate,          the loss rate of pakcet size
        //Video Info
     ** @param  unsigned long framesCaptured,                     the video frames between captured and encoders
     ** @param  unsigned long framesSent,                         the video frames of sender sent.
     ** @param  unsigned long hugeFramesSent,                     the video huge frames of sender sent.
     ** @param  unsigned long keyFramesSent,                      the video key frames of sender sent.
     ** @param  double        videoJitterBufferDelay,             the video receiver jitterBuffer Delay.
     ** @param  unsigned long long videoJitterBufferEmittedCount, the video receiver jitterBuffer Emitted Count.
     ** @param  unsigned long framesReceived,                     the video frames of receiver received.
     ** @param  unsigned long keyFramesReceived,                  the video key frames of receiver received.
     ** @param  unsigned long framesDecoded,                      the video frames of receiver decoder decoded.
     ** @param  unsigned long framesDropped,                      the video frames of receiver decoder droped.
     ** @param  unsigned long partialFramesLost,                  the video frames of receiver multi frame partial lost.
     ** @param  unsigned long fullFramesLost,                     the video frames of receiver multi frame fully lost.
        //Audio Info
     ** @param  double             echoReturnLoss,                the audio loss for echo cancellation
     ** @param  double             echoReturnLossEnhancement,     the enhancement for echo  audio cancellation
     ** @param  unsigned long      long totalSamplesSent,         the count of audio frames sender sent
     ** @param  unsigned long long estimatedPlayoutTimestamp,     the timestamp of estimate audio playout buffer
     ** @param  double             audioJitterBufferDelay,        the audio receiver jitterBuffer Delay.
     ** @param  unsigned long long audioJitterBufferEmittedCount, the audio receiver jitterBuffer Emitted Count.
     ** @param  unsigned long long totalSamplesReceived,          the audio frames of receiver received
     ** @param  unsigned long long concealedSamples,              the audio samples of receiver received
     ** @param  unsigned long long concealmentEvents,             the audio samples event of receiver received

     ** return: SC_SUCCESS             if successfully created
                SC_COLLECT_TYPE_ERROR  if the collect type error.
                SC_NEW_MEMORY_FAIL     if the new memory fail
    */ 
    SCResult StatsCollectModule::StatsCollect(
        bool               hastransportSequenceNumber,
        double             pacerPacingRate,
        double             pacerPaddingRate,
        //Packet Info
        unsigned char      payloadType,
        unsigned short     sequenceNumber,
        unsigned int       sendTimestamp,
        unsigned int       ssrc,
        unsigned long      paddingLength,
        unsigned long      headerLength,
        unsigned long long arrivalTimeMs,
        unsigned long      payloadSize,
        float              lossRate,
        // Video Info
        unsigned long      framesCaptured,
        unsigned long      framesSent,
        unsigned long      hugeFramesSent,
        unsigned long      keyFramesSent,
        double             videoJitterBufferDelay,
        unsigned long long videoJitterBufferEmittedCount,
        unsigned long      framesReceived,
        unsigned long      keyFramesReceived,
        unsigned long      framesDecoded,
        unsigned long      framesDropped,
        unsigned long      partialFramesLost,
        unsigned long      fullFramesLost,
        // Audio Info
        double             echoReturnLoss,
        double             echoReturnLossEnhancement,
        unsigned long long totalSamplesSent,
        long long          estimatedPlayoutTimestamp,
        double             audioJitterBufferDelay,
        unsigned long long audioJitterBufferEmittedCount,
        unsigned long long totalSamplesReceived,
        unsigned long long concealedSamples,
        unsigned long long concealmentEvents) {

        void* resultPtr;
        if (collectType_ == SC_TYPE_STRUCT) {
            struct CollectInfo* collectInfoPtr = StatsCollectByStruct(
                hastransportSequenceNumber,
                pacerPacingRate,
                pacerPaddingRate,
                payloadType,
                sequenceNumber,
                sendTimestamp,
                ssrc,
                paddingLength,
                headerLength,
                arrivalTimeMs,
                payloadSize,
                lossRate,
                framesCaptured,
                framesSent,
                hugeFramesSent,
                keyFramesSent,
                videoJitterBufferDelay,
                videoJitterBufferEmittedCount,
                framesReceived,
                keyFramesReceived,
                framesDecoded,
                framesDropped,
                partialFramesLost,
                fullFramesLost,
                echoReturnLoss,
                echoReturnLossEnhancement,
                totalSamplesSent,
                estimatedPlayoutTimestamp,
                audioJitterBufferDelay,
                audioJitterBufferEmittedCount,
                totalSamplesReceived,
                concealedSamples,
                concealmentEvents);
            
            if (collectInfoPtr == NULL ) {
                return SC_NEW_MEMORY_FAIL;
	        }
            else {
                resultPtr = (void*)collectInfoPtr;
            }
            
        }
        else if (collectType_ == SC_TYPE_JSON) {
            std::string collectInfoJson = StatsCollectByJSON(
                hastransportSequenceNumber,
                pacerPacingRate,
                pacerPaddingRate,
                payloadType,
                sequenceNumber,
                sendTimestamp,
                ssrc,
                paddingLength,
                headerLength,
                arrivalTimeMs,
                payloadSize,
                lossRate,
                framesCaptured,
                framesSent,
                hugeFramesSent,
                keyFramesSent,
                videoJitterBufferDelay,
                videoJitterBufferEmittedCount,
                framesReceived,
                keyFramesReceived,
                framesDecoded,
                framesDropped,
                partialFramesLost,
                fullFramesLost,
                echoReturnLoss,
                echoReturnLossEnhancement,
                totalSamplesSent,
                estimatedPlayoutTimestamp,
                audioJitterBufferDelay,
                audioJitterBufferEmittedCount,
                totalSamplesReceived,
                concealedSamples,
                concealmentEvents);
            std::string* tempStrPrt = new std::string(collectInfoJson);
            resultPtr = static_cast<void*>(tempStrPrt);
        }
        else {
            return SC_COLLECT_TYPE_ERROR;
        }

        queueMutex_->lock();
        collectQueue_.push(resultPtr);
        queueMutex_->unlock();
        return SC_SUCCESS;
    }

    /**
     **========================================================
     ** StatsCollectInterface External Function StatsCollect
     **========================================================
    */
    /**
     ** Collect the pakcet info ,video info, audio info from RTC Program
     ** Just Put the collect info in a buffer queue.
     ** @param  double         pacerPacingRate,   the pacer pacing rate
     ** @param  double         pacerPaddingRate,  the pacer padding rate
     ** //Pakcet Info
     ** @param  unsigned char  payloadType,       the RTP header Payload Type
     ** @param  unsigned short sequenceNumber,    the RTP header Sequence Number
     ** @param  unsigned int   sendTimestamp,     the RTP header send timestamp
     ** @param  unsigned int   ssrc,              the RTP header ssrc
     ** @param  unsigned long  paddingLength,     the RTP header Padding size
     ** @param  unsigned long  headerLength,      the RTP header Header size
     ** @param  long long      arrivalTimeMs,     the RTP packet receive timestamp
     ** @param  unsigned long  payloadSize,       the RTP pakcet data size
     ** @param  float          lossRate,          the loss rate of pakcet size

    ** return: SC_SUCCESS             if successfully created
               SC_COLLECT_TYPE_ERROR  if the collect type error.
               SC_NEW_MEMORY_FAIL     if the new memory fail
    */
    SCResult StatsCollectModule::StatsCollect(
        bool               hastransportSequenceNumber,
        double             pacerPacingRate,
        double             pacerPaddingRate,
        //Packet Info
        unsigned char      payloadType,
        unsigned short     sequenceNumber,
        unsigned int       sendTimestamp,
        unsigned int       ssrc,
        unsigned long      paddingLength,
        unsigned long      headerLength,
        unsigned long long arrivalTimeMs,
        unsigned long      payloadSize,
        float              lossRate
        ) {

        void* resultPtr;
        if (collectType_ == SC_TYPE_STRUCT) {
            struct CollectInfo* collectInfoPtr = StatsCollectByStruct(
                hastransportSequenceNumber,
                pacerPacingRate,
                pacerPaddingRate,
                payloadType,
                sequenceNumber,
                sendTimestamp,
                ssrc,
                paddingLength,
                headerLength,
                arrivalTimeMs,
                payloadSize,
                lossRate,
                SC_FRAME_CAPTURED_EMPTY,
                SC_FRAME_SENT_EMPTY,
                SC_HUGE_FRAME_SENT_EMPTY,
                SC_KEY_FRAME_SENT_EMPTY,
                SC_JITTER_BUFFER_DELAY_EMPTY,
                SC_JITTER_BUFFER_EMITTED_COUNT_EMPTY,
                SC_FRAME_RECEIVED_EMPTY,
                SC_KEY_FRAME_RECEIVED_EMPTY,
                SC_FRAMES_DECODED_EMPTY,
                SC_FRAMES_DROPPED_EMPTY,
                SC_PARTIAL_FRAME_LOST_EMPTY,
                SC_FULL_FRAME_LOST_EMPTY,
                SC_ECHO_RETURN_LOSS_EMPTY,
                SC_ECHO_RETURN_LOSS_ENHANCEMENT_EMPTY,
                SC_TOTAL_SAMPLES_SENT_EMPTY,
                SC_ESTIAMTED_PLAYOUT_TIMESTAMP_EMPTY,
                SC_JITTER_BUFFER_DELAY_EMPTY,
                SC_JITTER_BUFFER_EMITTED_COUNT_EMPTY,
                SC_TOTAL_SAMPLE_RECEIVED_EMPTY,
                SC_CONCEALED_SAMPLES_EMPTY,
                SC_CONCEALED_EVENTS_EMPTY
                );
            
            if (collectInfoPtr == NULL) {
                return SC_NEW_MEMORY_FAIL;
	    }
            else {
                resultPtr = (void*)collectInfoPtr;
            }
        }
        else if (collectType_ == SC_TYPE_JSON) {
            std::string collectInfoJson = StatsCollectByJSON(
                hastransportSequenceNumber,
                pacerPacingRate,
                pacerPaddingRate,
                payloadType,
                sequenceNumber,
                sendTimestamp,
                ssrc,
                paddingLength,
                headerLength,
                arrivalTimeMs,
                payloadSize,
                lossRate,
                SC_FRAME_CAPTURED_EMPTY,
                SC_FRAME_SENT_EMPTY,
                SC_HUGE_FRAME_SENT_EMPTY,
                SC_KEY_FRAME_SENT_EMPTY,
                SC_JITTER_BUFFER_DELAY_EMPTY,
                SC_JITTER_BUFFER_EMITTED_COUNT_EMPTY,
                SC_FRAME_RECEIVED_EMPTY,
                SC_KEY_FRAME_RECEIVED_EMPTY,
                SC_FRAMES_DECODED_EMPTY,
                SC_FRAMES_DROPPED_EMPTY,
                SC_PARTIAL_FRAME_LOST_EMPTY,
                SC_FULL_FRAME_LOST_EMPTY,
                SC_ECHO_RETURN_LOSS_EMPTY,
                SC_ECHO_RETURN_LOSS_ENHANCEMENT_EMPTY,
                SC_TOTAL_SAMPLES_SENT_EMPTY,
                SC_ESTIAMTED_PLAYOUT_TIMESTAMP_EMPTY,
                SC_JITTER_BUFFER_DELAY_EMPTY,
                SC_JITTER_BUFFER_EMITTED_COUNT_EMPTY,
                SC_TOTAL_SAMPLE_RECEIVED_EMPTY,
                SC_CONCEALED_SAMPLES_EMPTY,
                SC_CONCEALED_EVENTS_EMPTY);
            std::string* tempStrPrt = &collectInfoJson;
            resultPtr = static_cast<void*>(tempStrPrt);
        }
        else {
            return SC_COLLECT_TYPE_ERROR;
        }
        queueMutex_->lock();
        collectQueue_.push(resultPtr);
        queueMutex_->unlock();
        return SC_SUCCESS;
    }

    /**
     **========================================================
     ** StatsCollectInterface External Function StatsCollectByStruct
     **========================================================
    */
    /**
     ** Collect the pakcet info ,video info, audio info from RTC Program
     ** Just Put the collect info in a buffer queue.
     ** @param  double         pacerPacingRate,   the pacer pacing rate
     ** @param  double         pacerPaddingRate,  the pacer padding rate
     ** //Pakcet Info
     ** @param  unsigned char  payloadType,       the RTP header Payload Type
     ** @param  unsigned short sequenceNumber,    the RTP header Sequence Number
     ** @param  unsigned int   sendTimestamp,     the RTP header send timestamp
     ** @param  unsigned int   ssrc,              the RTP header ssrc
     ** @param  unsigned long  paddingLength,     the RTP header Padding size
     ** @param  unsigned long  headerLength,      the RTP header Header size
     ** @param  long long      arrivalTimeMs,     the RTP packet receive timestamp
     ** @param  unsigned long  payloadSize,       the RTP pakcet data size
     ** @param  float          lossRate,          the loss rate of pakcet size
        //Video Info
     ** @param  unsigned long framesCaptured,                     the video frames between captured and encoders
     ** @param  unsigned long framesSent,                         the video frames of sender sent.
     ** @param  unsigned long hugeFramesSent,                     the video huge frames of sender sent.
     ** @param  unsigned long keyFramesSent,                      the video key frames of sender sent.
     ** @param  double        videoJitterBufferDelay,             the video receiver jitterBuffer Delay.
     ** @param  unsigned long long videoJitterBufferEmittedCount, the video receiver jitterBuffer Emitted Count.
     ** @param  unsigned long framesReceived,                     the video frames of receiver received.
     ** @param  unsigned long keyFramesReceived,                  the video key frames of receiver received.
     ** @param  unsigned long framesDecoded,                      the video frames of receiver decoder decoded.
     ** @param  unsigned long framesDropped,                      the video frames of receiver decoder droped.
     ** @param  unsigned long partialFramesLost,                  the video frames of receiver multi frame partial lost.
     ** @param  unsigned long fullFramesLost,                     the video frames of receiver multi frame fully lost.
        //Audio Info
     ** @param  double             echoReturnLoss,                the audio loss for echo cancellation
     ** @param  double             echoReturnLossEnhancement,     the enhancement for echo  audio cancellation
     ** @param  unsigned long      long totalSamplesSent,         the count of audio frames sender sent
     ** @param  unsigned long long estimatedPlayoutTimestamp,     the timestamp of estimate audio playout buffer
     ** @param  double             audioJitterBufferDelay,        the audio receiver jitterBuffer Delay.
     ** @param  unsigned long long audioJitterBufferEmittedCount, the audio receiver jitterBuffer Emitted Count.
     ** @param  unsigned long long totalSamplesReceived,          the audio frames of receiver received
     ** @param  unsigned long long concealedSamples,              the audio samples of receiver received
     ** @param  unsigned long long concealmentEvents,             the audio samples event of receiver received
     **
     ** return: struct Collect info pointer  if successfully created
                NULL                         if failed.
    */

    struct CollectInfo* StatsCollectModule::StatsCollectByStruct(
        bool               hastransportSequenceNumber,
        double             pacerPacingRate,
        double             pacerPaddingRate,
        //Packet Info
        unsigned char      payloadType,
        unsigned short     sequenceNumber,
        unsigned int       sendTimestamp,
        unsigned int       ssrc,
        unsigned long      paddingLength,
        unsigned long      headerLength,
        unsigned long long arrivalTimeMs,
        unsigned long      payloadSize,
        float              lossRate,
        // Video Info
        unsigned long      framesCaptured,
        unsigned long      framesSent,
        unsigned long      hugeFramesSent,
        unsigned long      keyFramesSent,
        double             videoJitterBufferDelay,
        unsigned long long videoJitterBufferEmittedCount,
        unsigned long      framesReceived,
        unsigned long      keyFramesReceived,
        unsigned long      framesDecoded,
        unsigned long      framesDropped,
        unsigned long      partialFramesLost,
        unsigned long      fullFramesLost,
        // Audio Info
        double             echoReturnLoss,
        double             echoReturnLossEnhancement,
        unsigned long long totalSamplesSent,
        long long          estimatedPlayoutTimestamp,
        double             audioJitterBufferDelay,
        unsigned long long audioJitterBufferEmittedCount,
        unsigned long long totalSamplesReceived,
        unsigned long long concealedSamples,
        unsigned long long concealmentEvents) {

        //Todo: How to use the PacerPacingRate pacerPaddingRate.
        struct CollectInfo* CollectInfoPtr = new CollectInfo;
            
        if (CollectInfoPtr == NULL) {
            return NULL;
        }
        CollectInfoPtr->hastransportSequenceNumber = hastransportSequenceNumber;
        CollectInfoPtr->pacerPacingRate = pacerPacingRate;
        CollectInfoPtr->pacerPaddingRate = pacerPaddingRate;
        CollectInfoPtr->packetInfo.header.payloadType = payloadType;
        CollectInfoPtr->packetInfo.header.sequenceNumber = sequenceNumber;
        CollectInfoPtr->packetInfo.header.sendTimestamp = sendTimestamp;
        CollectInfoPtr->packetInfo.header.ssrc = ssrc;
        CollectInfoPtr->packetInfo.header.paddingLength = paddingLength;
        CollectInfoPtr->packetInfo.header.headerLength = headerLength;
        CollectInfoPtr->packetInfo.arrivalTimeMs = arrivalTimeMs;
        CollectInfoPtr->packetInfo.payloadSize = payloadSize;
        CollectInfoPtr->packetInfo.lossRate = lossRate;
        CollectInfoPtr->mediaInfo.videoInfo.framesCaptured = framesCaptured;
        CollectInfoPtr->mediaInfo.videoInfo.framesSent = framesSent;
        CollectInfoPtr->mediaInfo.videoInfo.hugeFramesSent = hugeFramesSent;
        CollectInfoPtr->mediaInfo.videoInfo.keyFramesSent = keyFramesSent;
        CollectInfoPtr->mediaInfo.videoInfo.jitterBufferDelay = videoJitterBufferDelay;
        CollectInfoPtr->mediaInfo.videoInfo.jitterBufferEmittedCount = videoJitterBufferEmittedCount;
        CollectInfoPtr->mediaInfo.videoInfo.framesReceived = framesReceived;
        CollectInfoPtr->mediaInfo.videoInfo.keyFramesReceived = keyFramesReceived;
        CollectInfoPtr->mediaInfo.videoInfo.framesDecoded = framesDecoded;
        CollectInfoPtr->mediaInfo.videoInfo.framesDropped = framesDropped;
        CollectInfoPtr->mediaInfo.videoInfo.partialFramesLost = partialFramesLost;
        CollectInfoPtr->mediaInfo.videoInfo.fullFramesLost = fullFramesLost;
        CollectInfoPtr->mediaInfo.audioInfo.echoReturnLoss = echoReturnLoss;
        CollectInfoPtr->mediaInfo.audioInfo.echoReturnLossEnhancement = echoReturnLossEnhancement;
        CollectInfoPtr->mediaInfo.audioInfo.totalSamplesSent = totalSamplesSent;
        CollectInfoPtr->mediaInfo.audioInfo.estimatedPlayoutTimestamp = estimatedPlayoutTimestamp;
        CollectInfoPtr->mediaInfo.audioInfo.jitterBufferDelay = audioJitterBufferDelay;
        CollectInfoPtr->mediaInfo.audioInfo.jitterBufferEmittedCount = audioJitterBufferEmittedCount;
        CollectInfoPtr->mediaInfo.audioInfo.totalSamplesReceived = totalSamplesReceived;
        CollectInfoPtr->mediaInfo.audioInfo.concealedSamples = concealedSamples;
        CollectInfoPtr->mediaInfo.audioInfo.concealmentEvents = concealmentEvents;
        
        return CollectInfoPtr;

    }

    /*
     **========================================================
     ** StatsCollectInterface External Function StatsCollectByJSON
     **========================================================
     */
    /**
     ** Collect the pakcet info ,video info, audio info from RTC Program
     ** Just Put the collect info in a buffer queue.
     ** @param  double         pacerPacingRate,   the pacer pacing rate
     ** @param  double         pacerPaddingRate,  the pacer padding rate
     ** //Pakcet Info
     ** @param  unsigned char  payloadType,       the RTP header Payload Type
     ** @param  unsigned short sequenceNumber,    the RTP header Sequence Number
     ** @param  unsigned int   sendTimestamp,     the RTP header send timestamp
     ** @param  unsigned int   ssrc,              the RTP header ssrc
     ** @param  unsigned long  paddingLength,     the RTP header Padding size
     ** @param  unsigned long  headerLength,      the RTP header Header size
     ** @param  long long      arrivalTimeMs,     the RTP packet receive timestamp
     ** @param  unsigned long  payloadSize,       the RTP pakcet data size
     ** @param  float          lossRate,          the loss rate of pakcet size
        //Video Info
     ** @param  unsigned long framesCaptured,                     the video frames between captured and encoders
     ** @param  unsigned long framesSent,                         the video frames of sender sent.
     ** @param  unsigned long hugeFramesSent,                     the video huge frames of sender sent.
     ** @param  unsigned long keyFramesSent,                      the video key frames of sender sent.
     ** @param  double        videoJitterBufferDelay,             the video receiver jitterBuffer Delay.
     ** @param  unsigned long long videoJitterBufferEmittedCount, the video receiver jitterBuffer Emitted Count.
     ** @param  unsigned long framesReceived,                     the video frames of receiver received.
     ** @param  unsigned long keyFramesReceived,                  the video key frames of receiver received.
     ** @param  unsigned long framesDecoded,                      the video frames of receiver decoder decoded.
     ** @param  unsigned long framesDropped,                      the video frames of receiver decoder droped.
     ** @param  unsigned long partialFramesLost,                  the video frames of receiver multi frame partial lost.
     ** @param  unsigned long fullFramesLost,                     the video frames of receiver multi frame fully lost.
        //Audio Info
     ** @param  double             echoReturnLoss,                the audio loss for echo cancellation
     ** @param  double             echoReturnLossEnhancement,     the enhancement for echo  audio cancellation
     ** @param  unsigned long      long totalSamplesSent,         the count of audio frames sender sent
     ** @param  unsigned long long estimatedPlayoutTimestamp,     the timestamp of estimate audio playout buffer
     ** @param  double             audioJitterBufferDelay,        the audio receiver jitterBuffer Delay.
     ** @param  unsigned long long audioJitterBufferEmittedCount, the audio receiver jitterBuffer Emitted Count.
     ** @param  unsigned long long totalSamplesReceived,          the audio frames of receiver received
     ** @param  unsigned long long concealedSamples,              the audio samples of receiver received
     ** @param  unsigned long long concealmentEvents,             the audio samples event of receiver received

     ** return: json string format   if successfully
     */
    std::string StatsCollectModule::StatsCollectByJSON(
        bool               hastransportSequenceNumber,
        double             pacerPacingRate,
        double             pacerPaddingRate,
        //Packet Info
        unsigned char      payloadType,
        unsigned short     sequenceNumber,
        unsigned int       sendTimestamp,
        unsigned int       ssrc,
        unsigned long      paddingLength,
        unsigned long      headerLength,
        unsigned long long arrivalTimeMs,
        unsigned long      payloadSize,
        float              lossRate,
        // Video Info
        unsigned long      framesCaptured,
        unsigned long      framesSent,
        unsigned long      hugeFramesSent,
        unsigned long      keyFramesSent,
        double             videoJitterBufferDelay,
        unsigned long long videoJitterBufferEmittedCount,
        unsigned long      framesReceived,
        unsigned long      keyFramesReceived,
        unsigned long      framesDecoded,
        unsigned long      framesDropped,
        unsigned long      partialFramesLost,
        unsigned long      fullFramesLost,
        // Audio Info
        double             echoReturnLoss,
        double             echoReturnLossEnhancement,
        unsigned long long totalSamplesSent,
        long long          estimatedPlayoutTimestamp,
        double             audioJitterBufferDelay,
        unsigned long long audioJitterBufferEmittedCount,
        unsigned long long totalSamplesReceived,
        unsigned long long concealedSamples,
        unsigned long long concealmentEvents
        ) {
        
        using json = nlohmann::json;
        json CollectInfoJson;
        CollectInfoJson["hastransportSequenceNumber"]                              = hastransportSequenceNumber;
        CollectInfoJson["pacerPacingRate"]                                         = pacerPacingRate;
        CollectInfoJson["pacerPaddingRate"]                                        = pacerPaddingRate;
        CollectInfoJson["packetInfo"]["header"]["payloadType"]                     = payloadType;
        CollectInfoJson["packetInfo"]["header"]["sequenceNumber"]                  = sequenceNumber;
        CollectInfoJson["packetInfo"]["header"]["sendTimestamp"]                   = sendTimestamp;
        CollectInfoJson["packetInfo"]["header"]["ssrc"]                            = ssrc;
        CollectInfoJson["packetInfo"]["header"]["paddingLength"]                   = paddingLength;
        CollectInfoJson["packetInfo"]["header"]["headerLength"]                    = headerLength;
        CollectInfoJson["packetInfo"]["arrivalTimeMs"]                             = arrivalTimeMs;
        CollectInfoJson["packetInfo"]["payloadSize"]                               = payloadSize;
        CollectInfoJson["packetInfo"]["lossRates"]                                 = lossRate;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesCaptured"]                = framesCaptured;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesSent"]                    = framesSent;
        CollectInfoJson["mediaInfo"]["videoInfo"]["hugeFreameSent"]                = hugeFramesSent;
        CollectInfoJson["mediaInfo"]["videoInfo"]["keyFramesSent"]                 = keyFramesSent;
        CollectInfoJson["mediaInfo"]["videoInfo"]["videoJitterBufferDelay"]        = videoJitterBufferDelay;
        CollectInfoJson["mediaInfo"]["videoInfo"]["videoJitterBufferEmittedCount"] = videoJitterBufferEmittedCount;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesReceived"]                = framesReceived;
        CollectInfoJson["mediaInfo"]["videoInfo"]["keyFramesReceived"]             = keyFramesReceived;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesDecoded"]                 = framesDecoded;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesDroped"]                  = framesDropped;
        CollectInfoJson["mediaInfo"]["videoInfo"]["partialFramesLost"]             = partialFramesLost;
        CollectInfoJson["mediaInfo"]["videoInfo"]["fullFramesLost"]                = fullFramesLost;
        CollectInfoJson["mediaInfo"]["audioInfo"]["echoReturnLoss"]                = echoReturnLoss;
        CollectInfoJson["mediaInfo"]["audioInfo"]["echoReturnLossEnhancement"]     = echoReturnLossEnhancement;
        CollectInfoJson["mediaInfo"]["audioInfo"]["totalSamplesSent"]              = totalSamplesSent;
        CollectInfoJson["mediaInfo"]["audioInfo"]["estimatedPlayoutTimestamp"]     = estimatedPlayoutTimestamp;
        CollectInfoJson["mediaInfo"]["audioInfo"]["audioJitterBufferDelay"]        = audioJitterBufferDelay;
        CollectInfoJson["mediaInfo"]["audioInfo"]["audioJitterBufferEmittedCount"] = audioJitterBufferEmittedCount;
        CollectInfoJson["mediaInfo"]["audioInfo"]["totalSamplesReceived"]          = totalSamplesReceived;
        CollectInfoJson["mediaInfo"]["audioInfo"]["concealedSamples"]              = concealedSamples;
        CollectInfoJson["mediaInfo"]["audioInfo"]["concealmentEvents"]             = concealmentEvents;
    
        return CollectInfoJson.dump();
    }

    /*
     **========================================================
     ** StatsCollectInterface External Function ConvertStructToJSON
     **========================================================
     */
    /**
     ** Convert the struct to json
     ** @param  collectInfo* CollectInfoPtr,  the ptr of the struct
     ** return: json string format   if successfully
     */
    std::string StatsCollectModule::ConvertStructToJSON(CollectInfo* CollectInfoPtr) {

        using json = nlohmann::json;
        json CollectInfoJson;
        CollectInfoJson["hastransportSequenceNumber"]                              = CollectInfoPtr->hastransportSequenceNumber;
        CollectInfoJson["pacerPacingRate"]                                         = CollectInfoPtr->pacerPacingRate;
        CollectInfoJson["pacerPaddingRate"]                                        = CollectInfoPtr->pacerPaddingRate;
        CollectInfoJson["packetInfo"]["header"]["payloadType"]                     = CollectInfoPtr->packetInfo.header.payloadType;
        CollectInfoJson["packetInfo"]["header"]["sequenceNumber"]                  = CollectInfoPtr->packetInfo.header.sequenceNumber;
        CollectInfoJson["packetInfo"]["header"]["sendTimestamp"]                   = CollectInfoPtr->packetInfo.header.sendTimestamp;
        CollectInfoJson["packetInfo"]["header"]["ssrc"]                            = CollectInfoPtr->packetInfo.header.ssrc;
        CollectInfoJson["packetInfo"]["header"]["paddingLength"]                   = CollectInfoPtr->packetInfo.header.paddingLength;
        CollectInfoJson["packetInfo"]["header"]["headerLength"]                    = CollectInfoPtr->packetInfo.header.headerLength;
        CollectInfoJson["packetInfo"]["arrivalTimeMs"]                             = CollectInfoPtr->packetInfo.arrivalTimeMs;
        CollectInfoJson["packetInfo"]["payloadSize"]                               = CollectInfoPtr->packetInfo.payloadSize;
        CollectInfoJson["packetInfo"]["lossRates"]                                 = CollectInfoPtr->packetInfo.lossRate;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesCaptured"]                = CollectInfoPtr->mediaInfo.videoInfo.framesCaptured;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesSent"]                    = CollectInfoPtr->mediaInfo.videoInfo.framesSent;
        CollectInfoJson["mediaInfo"]["videoInfo"]["hugeFreameSent"]                = CollectInfoPtr->mediaInfo.videoInfo.hugeFramesSent;
        CollectInfoJson["mediaInfo"]["videoInfo"]["keyFramesSent"]                 = CollectInfoPtr->mediaInfo.videoInfo.keyFramesSent;
        CollectInfoJson["mediaInfo"]["videoInfo"]["videoJitterBufferDelay"]        = CollectInfoPtr->mediaInfo.videoInfo.jitterBufferDelay;
        CollectInfoJson["mediaInfo"]["videoInfo"]["videoJitterBufferEmittedCount"] = CollectInfoPtr->mediaInfo.videoInfo.jitterBufferEmittedCount;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesReceived"]                = CollectInfoPtr->mediaInfo.videoInfo.framesReceived;
        CollectInfoJson["mediaInfo"]["videoInfo"]["keyFramesReceived"]             = CollectInfoPtr->mediaInfo.videoInfo.keyFramesReceived;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesDecoded"]                 = CollectInfoPtr->mediaInfo.videoInfo.framesDecoded;
        CollectInfoJson["mediaInfo"]["videoInfo"]["framesDroped"]                  = CollectInfoPtr->mediaInfo.videoInfo.framesDropped;
        CollectInfoJson["mediaInfo"]["videoInfo"]["partialFramesLost"]             = CollectInfoPtr->mediaInfo.videoInfo.partialFramesLost;
        CollectInfoJson["mediaInfo"]["videoInfo"]["fullFramesLost"]                = CollectInfoPtr->mediaInfo.videoInfo.fullFramesLost;
        CollectInfoJson["mediaInfo"]["audioInfo"]["echoReturnLoss"]                = CollectInfoPtr->mediaInfo.audioInfo.echoReturnLoss;
        CollectInfoJson["mediaInfo"]["audioInfo"]["echoReturnLossEnhancement"]     = CollectInfoPtr->mediaInfo.audioInfo.echoReturnLossEnhancement;
        CollectInfoJson["mediaInfo"]["audioInfo"]["totalSamplesSent"]              = CollectInfoPtr->mediaInfo.audioInfo.totalSamplesSent;
        CollectInfoJson["mediaInfo"]["audioInfo"]["estimatedPlayoutTimestamp"]     = CollectInfoPtr->mediaInfo.audioInfo.estimatedPlayoutTimestamp;
        CollectInfoJson["mediaInfo"]["audioInfo"]["audioJitterBufferDelay"]        = CollectInfoPtr->mediaInfo.audioInfo.jitterBufferDelay;
        CollectInfoJson["mediaInfo"]["audioInfo"]["audioJitterBufferEmittedCount"] = CollectInfoPtr->mediaInfo.audioInfo.jitterBufferEmittedCount;
        CollectInfoJson["mediaInfo"]["audioInfo"]["totalSamplesReceived"]          = CollectInfoPtr->mediaInfo.audioInfo.totalSamplesReceived;
        CollectInfoJson["mediaInfo"]["audioInfo"]["concealedSamples"]              = CollectInfoPtr->mediaInfo.audioInfo.concealedSamples;
        CollectInfoJson["mediaInfo"]["audioInfo"]["concealmentEvents"]             = CollectInfoPtr->mediaInfo.audioInfo.concealmentEvents;
	    return CollectInfoJson.dump();
    }

    /**
    **========================================================
    ** StatsCollectInterface External Function DumpData
    **========================================================
    */
    /**
     ** Dump packet data
     ** return: json string format if successfully
    */
    std::string StatsCollectModule::DumpData() {
      std::string result;
        if (collectType_ != SC_TYPE_STRUCT && collectType_ != SC_TYPE_JSON) {
            return result;
        }
        /* Using Json collet type to save into DB */
        queueMutex_->lock();
        while (!collectQueue_.empty()) {
            CollectInfo* CollectInfoPtr = NULL;
            std::string* dataJsonPtr = NULL;
            if (collectType_ == SC_TYPE_STRUCT) {
                CollectInfoPtr = (CollectInfo*)collectQueue_.front();
              result = ConvertStructToJSON(CollectInfoPtr);            
            }
            else if (collectType_ == SC_TYPE_JSON) {
                std::string* dataJsonPtr = static_cast<std::string*>(collectQueue_.front());
              result = *dataJsonPtr;
            }                     
             collectQueue_.pop();
            if (!result.empty()) {
              if (collectType_ == SC_TYPE_STRUCT) {
                delete CollectInfoPtr;
              } else if (collectType_ == SC_TYPE_JSON) {
                delete dataJsonPtr;
              }
            }           
        }
        
        queueMutex_->unlock();  
        
        return result;
    }
  
    /**
     **========================================================
     ** StatsCollectInterface External Function SetStatsConfig
     **========================================================
     */
    /**
     ** Set the Stats Collect config
     ** @param  SCType         collectType,  the collectType
     ** return:
     **         SC_COLLECT_TYPE_ERROR,if the collect type error.
     **         SC_SUCCESS,           if the save operate succeed.
     */
    SCResult StatsCollectModule::SetStatsConfig(SCType collectType) {
      if (collectType != 0 && collectType != 1) {
        return SC_COLLECT_TYPE_ERROR;
      }
      collectType_ = collectType;
      return SC_SUCCESS;
    }
}  // namespace StatCollect