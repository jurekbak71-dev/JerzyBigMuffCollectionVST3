#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "MuffDSP.h"
class MuffProcessor;
class PedalLook : public juce::LookAndFeel_V4 {
public:
 void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) override {
  auto r=b.getLocalBounds().toFloat().reduced(5);auto c=r.getCentre();float radius=juce::jmin(r.getWidth(),r.getHeight())*.5f;
  g.setColour(juce::Colours::black.withAlpha(.35f));g.fillEllipse(c.x-radius+2,c.y-radius+4,radius*2,radius*2);
  juce::Path nut;for(int i=0;i<6;++i){float a=float(i)*juce::MathConstants<float>::twoPi/6;auto x=c.x+std::cos(a)*radius;auto y=c.y+std::sin(a)*radius;if(i==0)nut.startNewSubPath(x,y);else nut.lineTo(x,y);}nut.closeSubPath();
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xffeeeeeb),c.x,c.y-radius,juce::Colour(0xff62686c),c.x,c.y+radius,false));g.fillPath(nut);
  float k=radius*.68f;g.setColour(juce::Colour(0xff44494c));g.fillEllipse(c.x-k-2,c.y-k-2,k*2+4,k*2+4);
  g.setGradientFill(juce::ColourGradient(down?juce::Colour(0xff989e9f):juce::Colour(0xfffafaf5),c.x-k,c.y-k,juce::Colour(0xff767e82),c.x+k,c.y+k,false));g.fillEllipse(c.x-k,c.y-k,k*2,k*2);
  g.setColour(juce::Colours::white.withAlpha(over?.8f:.4f));g.drawEllipse(c.x-k+3,c.y-k+3,k*2-6,k*2-6,1);
 }
 void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
  auto r=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(10);
  auto c=r.getCentre(); float radius=juce::jmin(r.getWidth(),r.getHeight())*.5f;
  g.setColour(juce::Colours::black.withAlpha(.25f)); g.fillEllipse(c.x-radius+3,c.y-radius+5,radius*2,radius*2);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff454545),c.x,c.y-radius,juce::Colour(0xff101010),c.x,c.y+radius,false));
  g.fillEllipse(c.x-radius,c.y-radius,radius*2,radius*2);
  g.setColour(juce::Colour(0xff777777)); g.drawEllipse(c.x-radius,c.y-radius,radius*2,radius*2,1.5f);
  for(int i=0;i<24;++i) { float a=float(i)*juce::MathConstants<float>::twoPi/24; g.setColour(juce::Colours::black.withAlpha(.35f)); g.drawLine(c.x+std::sin(a)*radius*.82f,c.y-std::cos(a)*radius*.82f,c.x+std::sin(a)*radius*.97f,c.y-std::cos(a)*radius*.97f,2); }
  float a=start+pos*(end-start); g.setColour(juce::Colours::ivory); g.drawLine(c.x+std::sin(a)*radius*.45f,c.y-std::cos(a)*radius*.45f,c.x+std::sin(a)*radius*.86f,c.y-std::cos(a)*radius*.86f,3.5f);
 }
};
class MuffEditor : public juce::AudioProcessorEditor, private juce::Timer {
 MuffProcessor& proc; PedalLook look;
 juce::Slider volume,tone,sustain;
 juce::ComboBox model;
 juce::TextButton foot {"BYPASS"};
 using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
 std::unique_ptr<SliderAttachment> va,ta,sa;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ma;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
 void timerCallback() override { repaint(); }
public:
 explicit MuffEditor(MuffProcessor&);
 ~MuffEditor() override { setLookAndFeel(nullptr); }
 void paint(juce::Graphics&) override;
 void resized() override;
};
class MuffProcessor : public juce::AudioProcessor {
 std::array<muff::Engine,6> engines;
 std::array<juce::SmoothedValue<float>,6> modelWeights;
 juce::dsp::Oversampling<float> oversampling {2,2,juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,true};
 juce::SmoothedValue<float> gain, toneSmooth, sustainSmooth, wet;
 std::vector<float> gains,tones,sustains,mixes;
 std::array<std::vector<float>,6> weights;
 std::atomic<float> *volumeParam,*toneParam,*sustainParam,*modelParam,*bypassParam;
public:
 juce::AudioProcessorValueTreeState state;
 static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout p;
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"volume",1},"Volume",0.f,1.f,.5f));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"tone",1},"Tone",0.f,1.f,.5f));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sustain",1},"Sustain",0.f,1.f,.5f));
  p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"model",1},"Model",juce::StringArray{"Triangle","Ram's Head","Op-Amp","Civil War","Green Russian","NYC"},0));
  p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false)); return p;
 }
 MuffProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"MuffState",layout()) {
  volumeParam=state.getRawParameterValue("volume"); toneParam=state.getRawParameterValue("tone"); sustainParam=state.getRawParameterValue("sustain"); modelParam=state.getRawParameterValue("model"); bypassParam=state.getRawParameterValue("bypass");
 }
 const juce::String getName() const override { return "Jerzy Muff Collection"; }
 bool acceptsMidi() const override{return false;} bool producesMidi() const override{return false;} bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return .3;}
 int getNumPrograms() override{return 1;} int getCurrentProgram() override{return 0;} void setCurrentProgram(int) override{}
 const juce::String getProgramName(int) override{return "Default";} void changeProgramName(int,const juce::String&) override{}
 bool isBusesLayoutSupported(const BusesLayout& l) const override { auto c=l.getMainOutputChannelSet(); return (c==juce::AudioChannelSet::mono()||c==juce::AudioChannelSet::stereo())&&c==l.getMainInputChannelSet(); }
 void prepareToPlay(double rate,int size) override {
  oversampling.initProcessing(static_cast<size_t>(size)); oversampling.reset(); for(auto& e:engines)e.prepare(rate*4);
  for(size_t m=0;m<6;++m){modelWeights[m].reset(rate*4,.02);modelWeights[m].setCurrentAndTargetValue(int(m)==int(modelParam->load())?1.f:0.f);weights[m].resize(static_cast<size_t>(size)*4);}
  setLatencySamples(int(oversampling.getLatencyInSamples()));
  gain.reset(rate*4,.02); toneSmooth.reset(rate*4,.02); sustainSmooth.reset(rate*4,.02); wet.reset(rate*4,.01);
  gain.setCurrentAndTargetValue(volumeParam->load()*2); toneSmooth.setCurrentAndTargetValue(toneParam->load()); sustainSmooth.setCurrentAndTargetValue(sustainParam->load()); wet.setCurrentAndTargetValue(bypassParam->load()>.5f?0.f:1.f);
  gains.resize(static_cast<size_t>(size)*4); tones.resize(gains.size()); sustains.resize(gains.size()); mixes.resize(gains.size());
 }
 void releaseResources() override {oversampling.reset();for(auto& e:engines)e.reset();}
 void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override {
  juce::ScopedNoDenormals noDenormals;
  auto count=buffer.getNumSamples(); if(count==0) return;
  if(gains.empty()) {buffer.clear();return;}
  for(int c=0;c<buffer.getNumChannels();++c)for(int i=0;i<count;++i)if(!std::isfinite(buffer.getSample(c,i)))buffer.setSample(c,i,0);
  juce::dsp::AudioBlock<float> fullBlock(buffer);
  for(size_t offset=0;offset<static_cast<size_t>(count);) {
  auto length=std::min(gains.size()/4,static_cast<size_t>(count)-offset);
  auto block=fullBlock.getSubBlock(offset,length);auto up=oversampling.processSamplesUp(block);
  gain.setTargetValue(volumeParam->load()*2); toneSmooth.setTargetValue(toneParam->load()); sustainSmooth.setTargetValue(sustainParam->load()); wet.setTargetValue(bypassParam->load()>.5f?0.f:1.f);
  auto selected=juce::jlimit(0,5,int(modelParam->load()));
  for(size_t m=0;m<6;++m) modelWeights[m].setTargetValue(int(m)==selected?1.f:0.f);
  auto n=up.getNumSamples();
  for(size_t i=0;i<n;++i) { gains[i]=gain.getNextValue();tones[i]=toneSmooth.getNextValue();sustains[i]=sustainSmooth.getNextValue();mixes[i]=wet.getNextValue(); for(size_t m=0;m<6;++m)weights[m][i]=modelWeights[m].getNextValue(); }
  for(size_t c=0;c<up.getNumChannels();++c) { auto* data=up.getChannelPointer(c); for(size_t i=0;i<n;++i) { float x=data[i];float y=0;for(size_t m=0;m<6;++m)y+=engines[m].process(x,int(c),int(m),sustains[i],tones[i])*weights[m][i];y*=gains[i];data[i]=x+(y-x)*mixes[i]; } }
  oversampling.processSamplesDown(block);offset+=length;
  }
 }
 juce::AudioProcessorParameter* getBypassParameter() const override {return state.getParameter("bypass");}
 bool hasEditor() const override{return true;} juce::AudioProcessorEditor* createEditor() override {return new MuffEditor(*this);}
 void getStateInformation(juce::MemoryBlock& b) override {auto xml=state.copyState().createXml(); copyXmlToBinary(*xml,b);}
 void setStateInformation(const void* d,int n) override {auto xml=getXmlFromBinary(d,n); if(xml&&xml->hasTagName("MuffState")) state.replaceState(juce::ValueTree::fromXml(*xml));}
};
MuffEditor::MuffEditor(MuffProcessor& p):AudioProcessorEditor(p),proc(p) {
 setLookAndFeel(&look);
 for(auto* s:{&volume,&tone,&sustain}) {s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0); s->setDoubleClickReturnValue(true,.5); addAndMakeVisible(s);}
 volume.setName("Volume");tone.setName("Tone");sustain.setName("Sustain");
 va=std::make_unique<SliderAttachment>(p.state,"volume",volume);ta=std::make_unique<SliderAttachment>(p.state,"tone",tone);sa=std::make_unique<SliderAttachment>(p.state,"sustain",sustain);
 model.addItemList({"Triangle","Ram's Head","Op-Amp","Civil War","Green Russian","NYC"},1); addAndMakeVisible(model); ma=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"model",model);
 foot.setButtonText("");foot.setName("Bypass");foot.setClickingTogglesState(true);foot.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff8d9193));foot.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff55595b));addAndMakeVisible(foot); ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",foot);
 setSize(520,650);startTimerHz(15);
}
void MuffEditor::resized() {volume.setBounds(52,100,120,120);tone.setBounds(200,100,120,120);sustain.setBounds(348,100,120,120);model.setBounds(140,405,240,32);foot.setBounds(221,493,78,70);}
void MuffEditor::paint(juce::Graphics& g) {
 auto selected=juce::jlimit(0,5,int(proc.state.getRawParameterValue("model")->load()));
 const juce::Colour inks[]={juce::Colour(0xff9d171b),juce::Colour(0xff6a2459),juce::Colour(0xffdd6520),juce::Colour(0xff264b61),juce::Colour(0xffd9cf99),juce::Colour(0xffbd2225)};
 auto ink=inks[selected];auto body=selected==4?juce::Colour(0xff354535):juce::Colour(0xffd2d2c9);
 g.fillAll(juce::Colour(0xff18191b));g.setGradientFill(juce::ColourGradient(body.brighter(.25f),25,10,body.darker(.22f),490,640,false));g.fillRoundedRectangle(18,12,484,626,22);
 g.setColour(juce::Colours::black.withAlpha(.12f));for(int y=22;y<630;y+=3)g.drawHorizontalLine(y,28,492);
 for(auto pt:{juce::Point<float>(40,36),{480,36},{40,614},{480,614}}){g.setColour(juce::Colour(0xff969996));g.fillEllipse(pt.x-6,pt.y-6,12,12);g.setColour(juce::Colour(0xff424542));g.drawLine(pt.x-3,pt.y+3,pt.x+3,pt.y-3,1.5f);}
 g.setColour(ink);g.setFont(juce::Font(18,juce::Font::bold));g.drawText("VOLUME",45,73,134,25,juce::Justification::centred);g.drawText("TONE",193,73,134,25,juce::Justification::centred);g.drawText("SUSTAIN",341,73,134,25,juce::Justification::centred);
 g.setFont(juce::Font(56,juce::Font::bold));g.drawText("MUFF",40,248,440,65,juce::Justification::centred);g.setFont(juce::Font(31,juce::Font::bold));g.drawText("COLLECTION",40,311,440,42,juce::Justification::centred);
 g.setFont(juce::Font(14));g.drawText("J E R Z Y   /   F U Z Z   M A C H I N E",40,364,440,25,juce::Justification::centred);
 bool enabled=proc.state.getRawParameterValue("bypass")->load()<.5f;g.setColour(enabled?juce::Colour(0xffff3930):juce::Colour(0xff522523));g.fillEllipse(253,460,14,14);
 g.setFont(juce::Font(12));g.setColour(ink);g.drawText("JERZY CUSTOM EFFECTS",45,581,430,20,juce::Justification::centred);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MuffProcessor;}
