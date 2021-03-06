#ifndef __GEEK_BOY_GCW_H__
#define __GEEK_BOY_GCW_H__

#include "../../../src/core/emu_interface.h"
#include "../../../src/sfx/sfx.h"

#include "../include/gambatte.h"

using namespace gcw;
using namespace gambatte;
using namespace std;

/* shifts as defined in inputgetter.h */
enum KeyShift
{
  KEY_A = 0,
  KEY_B = 1,
  KEY_SELECT = 2,
  KEY_START = 3,
  KEY_RIGHT = 4,
  KEY_LEFT = 5,
  KEY_UP = 6,
  KEY_DOWN = 7
};

constexpr size_t AUDIO_SAMPLES_PER_FRAME = 35112;
constexpr size_t AUDIO_SAMPLES_ADDITIONAL = 2064;

class GambatteCore : public CoreInterface, public InputGetter
{
private:
  GB gb;
  
  std::size_t bufsamples = 0;
	bool audioOutBufLow = false;
  
public:
  GambatteCore()
  {
    registerInformations({System::Type::GAME_BOY, System::Type::GAME_BOY_COLOR}, "gambatte", "Gambatte", "1.0");
    
    registerFeature(CoreFeature::CAN_SAVE_STATE);
    registerFeature(CoreFeature::CAN_SOFT_RESET);
    registerFeature(CoreFeature::CAN_SAVE_RAM);
    
    registerButton(ButtonSetting("A", GCW_KEY_A, KEY_A, true));
    registerButton(ButtonSetting("B", GCW_KEY_B, KEY_B, true));
    registerButton(ButtonSetting("Start", GCW_KEY_START, KEY_START, true));
    registerButton(ButtonSetting("Select", GCW_KEY_SELECT, KEY_SELECT, true));
    registerButton(ButtonSetting("Up", GCW_KEY_UP, KEY_UP, true));
    registerButton(ButtonSetting("Right", GCW_KEY_RIGHT, KEY_RIGHT, true));
    registerButton(ButtonSetting("Left", GCW_KEY_LEFT, KEY_LEFT, true));
    registerButton(ButtonSetting("Down", GCW_KEY_DOWN, KEY_DOWN, true));
    
    setGfxFormat(160, 144, FORMAT_XRGB888);
    setSfxFormat(SfxAudioSpec(4,(AUDIO_SAMPLES_PER_FRAME + AUDIO_SAMPLES_ADDITIONAL), 44100));
  }
  
  unsigned operator()() override { return buttonStatus; }
  
  void emulationFrame() override
  {
    ptrdiff_t vidFrameDoneSampleCnt = -1;
    
    while (vidFrameDoneSampleCnt < 0)
    {
      size_t runsamples = AUDIO_SAMPLES_PER_FRAME - bufsamples;
      vidFrameDoneSampleCnt = gb.runFor(reinterpret_cast<gambatte::uint_least32_t*>(gfxBuffer.data), gfxBuffer.width, reinterpret_cast<u32*>(audioBuffer) + bufsamples, runsamples);
      size_t const outsamples = vidFrameDoneSampleCnt >= 0 ? bufsamples + vidFrameDoneSampleCnt : bufsamples + runsamples;
      bufsamples += runsamples;
      bufsamples -= outsamples;
      
      AudioStatus const &astatus = writeAudioSamples(outsamples, bufsamples);
			audioOutBufLow = astatus.low;
    }
  }
  
  void loadRomByFileName(const string& name) override
  {
    gb.load(name);
  }
  
  void emulationSuspended() override { }
  void emulationResumed() override { }
  void emulationStopped() override { }
  void emulationStarted() override { }
  
  void stateSaveTo(const string& path) override
  {
    gb.saveState(nullptr, 0, path);
  }
  
  void stateLoadFrom(const string& path) override
  {
    gb.loadState(path);
  }
  
  void sramSetPath(const string& path) override { gb.setSaveDir(path); }
  void sramSaveTo(const std::string& path, const std::string& romName) override { gb.saveSavedata(); }
  void sramLoadFrom(const std::string& path, const std::string& romName) override { /* implicit on rom load */ }
  
  void initialize() {
    gb.setInputGetter(this);
  }
  
  void softReset() override
  {
    gb.reset();
  }

  void releaseResources() override
  {
    
  }

  void reset() override
  {
    gb.reset();
    audioOutBufLow = false;
    bufsamples = 0;
    buttonStatus = 0;
  }
  
  
};

static GambatteCore emulator;

__attribute__((constructor)) void onLoad()
{

}

__attribute__((destructor)) void onUnload()
{

}

extern "C" CoreInterface* retrieve()
{
  return &emulator;
}

#endif