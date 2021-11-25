// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mp3_server_media_subsession.hh"
#include "MPEG4GenericRTPSink.hh"

#include "utils.h"

namespace easymedia {
static unsigned const samplingFrequencyTable[16] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,  7350,  0,     0,     0};

MP3ServerMediaSubsession *MP3ServerMediaSubsession::createNew(
    UsageEnvironment &env, Live555MediaInput &wisInput,
    unsigned samplingFrequency, unsigned numChannels, unsigned char profile) {
  return new MP3ServerMediaSubsession(env, wisInput, samplingFrequency,
                                      numChannels, profile);
}

MP3ServerMediaSubsession::MP3ServerMediaSubsession(
    UsageEnvironment &env, Live555MediaInput &mediaInput,
    unsigned samplingFrequency, unsigned numChannels, unsigned char profile)
    : OnDemandServerMediaSubsession(env, True /*reuse the first source*/),
      fMediaInput(mediaInput), fSamplingFrequency(samplingFrequency),
      fNumChannels(numChannels) {
  unsigned char audioSpecificConfig[2];
  unsigned char const audioObjectType = profile + 1;
  unsigned char samplingFrequencyIndex = 0;
  for (unsigned char i = 0; i < 16; i++) {
    if (samplingFrequencyTable[i] == fSamplingFrequency) {
      samplingFrequencyIndex = i;
      break;
    }
  }
  audioSpecificConfig[0] =
      (audioObjectType << 3) | (samplingFrequencyIndex >> 1);
  // audioSpecificConfig[1] = (samplingFrequencyIndex<<7) |
  // (channelConfiguration<<3);
  audioSpecificConfig[1] = (samplingFrequencyIndex << 7) | (fNumChannels << 3);
  sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0],
          audioSpecificConfig[1]);
}

MP3ServerMediaSubsession::~MP3ServerMediaSubsession() { LOG_FILE_FUNC_LINE(); }

FramedSource *
MP3ServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/,
                                                unsigned &estBitrate) {
  LOG_FILE_FUNC_LINE();
  estBitrate = 96; // kbps, estimate
  return fMediaInput.audioSource();
}

RTPSink *MP3ServerMediaSubsession::createNewRTPSink(
    Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
    FramedSource *inputSource) {
  if (!inputSource) {
    RKMEDIA_LOGI("inputSource is not ready, can not create new rtp sink\n");
    return NULL;
  }

  // MP3AudioFileSource* mp3Source = (MP3AudioFileSource*)inputSource;
  setAudioRTPSinkBufferSize();
  RTPSink *rtpsink = MPEG4GenericRTPSink::createNew(
      envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, fSamplingFrequency,
      "audio", "MP3-hbr", fConfigStr, fNumChannels);
  setVideoRTPSinkBufferSize();
  return rtpsink;
}

// std::mutex MP3ServerMediaSubsession::kMutex;
void MP3ServerMediaSubsession::startStream(
    unsigned clientSessionId, void *streamToken, TaskFunc *rtcpRRHandler,
    void *rtcpRRHandlerClientData, unsigned short &rtpSeqNum,
    unsigned &rtpTimestamp,
    ServerRequestAlternativeByteHandler *serverRequestAlternativeByteHandler,
    void *serverRequestAlternativeByteHandlerClientData) {
  OnDemandServerMediaSubsession::startStream(
      clientSessionId, streamToken, rtcpRRHandler, rtcpRRHandlerClientData,
      rtpSeqNum, rtpTimestamp, serverRequestAlternativeByteHandler,
      serverRequestAlternativeByteHandlerClientData);
  // kMutex.lock();
  if (kSessionIdList.empty())
    fMediaInput.Start(envir());
  if (fMediaInput.GetStartAudioStreamCallback() != NULL) {
    fMediaInput.GetStartAudioStreamCallback()();
  }
  RKMEDIA_LOGI("%s:%s:%p - clientSessionId: 0x%08x\n", __FILE__, __func__, this,
               clientSessionId);
  kSessionIdList.push_back(clientSessionId);
  // kMutex.unlock();
}
void MP3ServerMediaSubsession::deleteStream(unsigned clientSessionId,
                                            void *&streamToken) {
  // kMutex.lock();
  RKMEDIA_LOGI("%s - clientSessionId: 0x%08x\n", __func__, clientSessionId);
  kSessionIdList.remove(clientSessionId);
  if (kSessionIdList.empty())
    fMediaInput.Stop(envir());
  // kMutex.unlock();
  OnDemandServerMediaSubsession::deleteStream(clientSessionId, streamToken);
}

} // namespace easymedia
