/**
 * @file      StatCollect.h
 * @brief     The header file of StatsCollectModule. StatsCollectModule is provided for RTC application to update their states to local file for states collection and training.
 * @repo      AlphaRTC
 * @author    Kangjie Xu <xkjrst@outlook.com>
 * @author    Dan Yang <v-danya@microsoft.com>
 * @author    Peng Cheng <pengc@microsoft.com>
 * @version   0.2
 * @copyright Copyright (c) Microsoft Corporation. All rights reserved.
 * @license   Licensed under the MIT License.
 **/

#ifndef STATES_COLLECTION_H_
#define STATES_COLLECTION_H_

#include <memory>
#include <queue>
#include <vector>
#include <mutex>
#include <float.h>
#include <climits>



namespace StatCollect {
    /**
     **========================================================
     ** StatsCollectInterface External Stats Definition
     **========================================================
    */
    /**
     ** Result type list
     ** Todo: wait the whole library finsihed, add this part comments
    **/
    enum SCResult {
        SC_SUCCESS = 0x00000000,
        SC_NEW_MEMORY_FAIL = 0x10000010,
        SC_COLLECT_TYPE_ERROR = 0x10000100,
        SC_CONNECT_ERROR = 0x10010000,
        SC_CONNECT_EXIST_ERROR = 0x10010001,
        SC_SESSION_ERROR = 0x10020000,
        SC_SAVE_ERROR = 0x10030000,
    };

    /**
     ** The type of queue in StatCollect (collectQueue_)
     ** SC_TYPE_STRUCT: collectQueue_ will use sturture to store states
     ** SC_TYPE_JSON: collectQueue_ will use json to store states
    **/
    enum SCType {
        SC_TYPE_STRUCT = 0,
        SC_TYPE_JSON = 1,
    };

    /**
     ** Empty List for each params in stast collection
     **
    **/
#define SC_PAYLOAD_TYPE_EMPTY                           '#'
#define SC_SEQUENCE_NUMBER_EMPTY                     USHRT_MAX
#define SC_SEND_TIMESTAMP_EMPTY                      UINT_MAX   
#define SC_SSRC_EMPTY                                UINT_MAX
#define SC_PADDING_LENGTH_EMPTY                      ULONG_MAX
#define SC_HEADER_LENGTH_EMPTY                       ULONG_MAX
#define SC_ARRIVAL_TIME_MS_EMPTY                     LLONG_MAX
#define SC_PAYLOAD_SIZE_EMPTY                        ULONG_MAX
#define SC_LOSS_RATE_EMPTY                           FLT_MAX
#define SC_FRAME_CAPTURED_EMPTY                      ULONG_MAX
#define SC_FRAME_SENT_EMPTY                          ULONG_MAX
#define SC_HUGE_FRAME_SENT_EMPTY                     ULONG_MAX
#define SC_KEY_FRAME_SENT_EMPTY                      ULONG_MAX
#define SC_JITTER_BUFFER_DELAY_EMPTY                 DBL_MAX
#define SC_JITTER_BUFFER_EMITTED_COUNT_EMPTY         ULONG_MAX            
#define SC_FRAME_RECEIVED_EMPTY                      ULONG_MAX 
#define SC_KEY_FRAME_RECEIVED_EMPTY                  ULONG_MAX 
#define SC_FRAMES_DECODED_EMPTY                      ULONG_MAX 
#define SC_FRAMES_DROPPED_EMPTY                      ULONG_MAX 
#define SC_PARTIAL_FRAME_LOST_EMPTY                  ULONG_MAX 
#define SC_FULL_FRAME_LOST_EMPTY                     ULONG_MAX 
#define SC_ECHO_RETURN_LOSS_EMPTY                    DBL_MAX 
#define SC_ECHO_RETURN_LOSS_ENHANCEMENT_EMPTY        DBL_MAX 
#define SC_TOTAL_SAMPLES_SENT_EMPTY                  ULLONG_MAX 
#define SC_ESTIAMTED_PLAYOUT_TIMESTAMP_EMPTY         LLONG_MAX 
#define SC_TOTAL_SAMPLE_RECEIVED_EMPTY               ULONG_MAX 
#define SC_CONCEALED_SAMPLES_EMPTY                   ULONG_MAX 
#define SC_CONCEALED_EVENTS_EMPTY                    ULONG_MAX
#define SC_PACER_PACING_RATE_EMPTY                   DBL_MAX
#define SC_PACER_PADDING_RATE_EMPTY                  DBL_MAX
    /**
     ** The structure stores the RTP packet header stats collection information.
     ** @param  unsigned char  payloadType,   the RTP header Payload Type.
     ** @param  unsigned short sequenceNumber,the RTP header Sequence Number
     ** @param  unsigned int   sendTimestamp, the RTP header send timestamp
     ** @param  unsigned int   ssrc,          the RTP header ssrc
     ** @param  size_t         paddingLength, the RTP header Padding size
     ** @param  size_t         headerLength,  the RTP header Header size
    */
    struct RTPHeader {
        unsigned char  payloadType;
        unsigned short sequenceNumber;
        unsigned int   sendTimestamp;
        unsigned int   ssrc;
        unsigned long  paddingLength;
        unsigned long  headerLength;
    };
    /**
     ** The structure stores the RTP packet stats collection information.
     ** @param  RTPHeader  hearder,       the RTP header info.
     ** @param  long long  arrivalTimeMs, the RTP packet receive timestamp.
     ** @param  size_t     payloadSize,   the RTP pakcet data size.
     ** @param  float      lossRate,      the loss rate of pakcet size.
    */
    struct PacketInfo {
        RTPHeader     header;
        long long     arrivalTimeMs;
        unsigned long payloadSize;
        float         lossRate;
    };

    /**
     ** The structure stores the stats video stats collection information.
     ** @param  unsigned long framesCaptured,               the video frames between captured and encoders
     ** @param  unsigned long framesSent,                   the video frames of sender sent.
     ** @param  unsigned long hugeFramesSent,               the video huge frames of sender sent.
     ** @param  unsigned long keyFramesSent,                the video key frames of sender sent.
     ** @param  double        jitterBufferDelay,            the video receiver jitterBuffer Delay.
     ** @param  unsigned long long jitterBufferEmittedCount,the video receiver jitterBuffer Emitted Count.
     ** @param  unsigned long framesReceived,               the video frames of receiver received.
     ** @param  unsigned long keyFramesReceived,            the video key frames of receiver received.
     ** @param  unsigned long framesDecoded,                the video frames of receiver decoder decoded.
     ** @param  unsigned long framesDropped,                the video frames of receiver decoder droped.
     ** @param  unsigned long partialFramesLost,            the video frames of receiver multi frame partial lost.
     ** @param  unsigned long fullFramesLost,               the video frames of receiver multi frame fully lost.
    */
    struct VideoInfo {
        //Sender stats
        unsigned long framesCaptured;
        unsigned long framesSent;
        unsigned long hugeFramesSent;
        unsigned long keyFramesSent;
        //Receiver stats
        double jitterBufferDelay;
        unsigned long long jitterBufferEmittedCount;
        unsigned long framesReceived;
        unsigned long keyFramesReceived;
        unsigned long framesDecoded;
        unsigned long framesDropped;
        unsigned long partialFramesLost;
        unsigned long fullFramesLost;
    };
    /**
     ** The structure stores the stats audio stats collection information.
     ** @param  double        echoReturnLoss,                the audio loss for echo cancellation
     ** @param  double        echoReturnLossEnhancement,     the enhancement for echo  audio cancellation
     ** @param  unsigned long long totalSamplesSent,         the count of audio frames sender sent
     ** @param  long long     estimatedPlayoutTimestamp,     the timestamp of estimate audio playout buffer
     ** @param  double        jitterBufferDelay,             the audio receiver jitterBuffer Delay.
     ** @param  unsigned long long jitterBufferEmittedCount, the audio receiver jitterBuffer Emitted Count.
     ** @param  unsigned long long totalSamplesReceived,     the audio frames of receiver received
     ** @param  unsigned long long concealedSamples,         the audio samples of receiver received
     ** @param  unsigned long long concealmentEvents,        the audio samples event of receiver received
    */
    struct AudioInfo {
        //Sender stats
        double echoReturnLoss;
        double echoReturnLossEnhancement;
        unsigned long long totalSamplesSent;
        //Receiver stats
        long long estimatedPlayoutTimestamp;
        double jitterBufferDelay;
        unsigned long long jitterBufferEmittedCount;
        unsigned long long totalSamplesReceived;
        unsigned long long concealedSamples;
        unsigned long long concealmentEvents;
    };
    /**
     ** The structure stores media stats collection information.
     ** @param   VideoInfo  videoInfo,     the stats collection of video.
     ** @param   AudioInfo  audioInfo,     the stats collection of audio.
    */
    struct MediaInfo {
        VideoInfo videoInfo;
        AudioInfo audioInfo;
    };
    /**
     ** The structure stores whole stats collection information.
     ** @param   PacketInfo perPacket,    the stats collection of packet.
     ** @param   MediaInfo  Media,        the stats collection of media.
    */
    struct CollectInfo {
        PacketInfo packetInfo;
        MediaInfo mediaInfo;
        bool      hastransportSequenceNumber;
        double    pacerPacingRate;
        double    pacerPaddingRate;
    };

    class StatsCollectModule {
    public:
        /**
         **========================================================
         ** StatsCollectInterface External Function StatsCollectModule
         **========================================================
        */
        /**
        ** The constructor function of StatsCollectModule class
        ** @param  SCType       collectType,  The collect type
        */
        StatsCollectModule(SCType collectType);
        
        /**
        **========================================================
        ** StatsCollectInterface External Function !StatsCollectModule
        **========================================================
        */
        /**
        ** The destructor function of StatsCollectModule class
        */
        ~StatsCollectModule();

        /**
         **========================================================
         ** StatsCollectInterface External Function StatsCollect
         **========================================================
        */
        /**
         ** Collect the pakcet info ,video info, audio info from RTC Program
         ** Just Put the collect info in a buffer queue.
         ** @param   double        pacerPacingRate,   the pacer pacing rate
         ** @param   double        pacerPaddingRate,  the pacer padding rate
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
         ** return: SC_SUCCESS             if successfully created
                    SC_COLLECT_TYPE_ERROR  if the collect type error.
                    SC_NEW_MEMORY_FAIL     if the new memory fail
        */
        SCResult StatsCollect(
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
            unsigned long long concealmentEvents);
        
        /**
         **========================================================
         ** StatsCollectInterface External Function StatsCollect
         **========================================================
        */
        /**
         ** Collect the pakcet info ,video info, audio info from RTC Program
         ** Just Put the collect info in a buffer queue.
         ** @param  int            collectType,       the collect type, 0 means struct, 1 means json.
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
         **
         ** return: SC_SUCCESS             if successfully created
                    SC_COLLECT_TYPE_ERROR  if the collect type error.
                    SC_NEW_MEMORY_FAIL     if the new memory fail
        */
        SCResult StatsCollect(
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
        );

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
        struct CollectInfo* StatsCollectByStruct(
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
            unsigned long long concealmentEvents);

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
        std::string StatsCollectByJSON(
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
            unsigned long long concealmentEvents);

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
        std::string ConvertStructToJSON(CollectInfo* CollectInfoPtr);

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
        SCResult SetStatsConfig(SCType collectType);

        /**
         **========================================================
         ** StatsCollectInterface External Function DumpData
         **========================================================
         */
        /**
         ** Dump packet data
         ** return: json string format if successfully
        */
        std::string DumpData();

    private:
        std::queue<void*>  collectQueue_;
        SCType collectType_;
        std::mutex* queueMutex_;
    };
}  // namespace StatCollect
#endif 