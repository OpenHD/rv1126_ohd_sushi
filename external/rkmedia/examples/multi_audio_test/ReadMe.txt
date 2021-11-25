#Overview
The examples in this directory are used to demonstrate how to independently control a single device for recording/playing when multiple microphones and multiple speakers are used.

#Operating instructions
1. "asound.conf" should be copied to the /etc directory

2. "rkmedia_multi_ai_test" example description
   Output files: /tmp/ai_chn0.pcm and /tmp/ai_chn1.pcm.
   Output format: S16LE, Mono, 16K

   Note: The above example is equivalent to the following command:
   -> arecord -Dplug:cap_chn0 -c 1 -r 16000 -f S16_LE /tmp/ai_chn0.wav
   -> arecord -Dplug:cap_chn1 -c 1 -r 16000 -f S16_LE /tmp/ai_chn1.wav

3. "rkmedia_multi_ao_test" example description
   Input files: /tmp/ao_chn0.pcm and /tmp/ao_chn1.pcm.
   Input format: S16LE, Mono, 16K

   Note: The above example is equivalent to the following command:
   -> aplay -Dplug:play_chn0 -c 1 -r 16000 -f S16_LE /tmp/ao_chn0.wav
   -> aplay -Dplug:play_chn1 -c 1 -r 16000 -f S16_LE /tmp/ao_chn1.wav
