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
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
 void timerCallback() override;
 int lastSelection=-1; bool lastBypass=false;
public:
 explicit MuffEditor(MuffProcessor&);
 ~MuffEditor() override { setLookAndFeel(nullptr); }
 void paint(juce::Graphics&) override;
 void resized() override;
};
#include "MuffProcessor.h"
juce::AudioProcessorEditor* MuffProcessor::createEditor(){return new MuffEditor(*this);}
MuffEditor::MuffEditor(MuffProcessor& p):AudioProcessorEditor(p),proc(p) {
 setLookAndFeel(&look);
 for(auto* s:{&volume,&tone,&sustain}) {s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0); s->setDoubleClickReturnValue(true,.5); addAndMakeVisible(s);}
 volume.setName("Volume");tone.setName("Tone");sustain.setName("Sustain");
 va=std::make_unique<SliderAttachment>(p.state,"volume",volume);ta=std::make_unique<SliderAttachment>(p.state,"tone",tone);sa=std::make_unique<SliderAttachment>(p.state,"sustain",sustain);
 model.addItemList({"Legacy: Triangle","Legacy: Ram's Head","Legacy: Op-Amp","Legacy: Civil War","Legacy: Green Russian","Legacy: NYC"},1);
 for(size_t i=0;i<muff::circuit::revisions.size();++i)model.addItem(muff::circuit::revisions[i].name,int(i)+7);
 addAndMakeVisible(model);model.setSelectedId(p.selectedModel()+1,juce::dontSendNotification);
 model.onChange=[this]{proc.selectModelFromEditor(model.getSelectedId()-1);};
 model.setTooltip("Schematic-based circuit revisions; Legacy entries retain the earlier approximate voicings.");
 foot.setButtonText("");foot.setName("Bypass");foot.setClickingTogglesState(true);foot.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff8d9193));foot.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff55595b));addAndMakeVisible(foot); ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",foot);
 setSize(520,650);startTimerHz(15);
}
void MuffEditor::timerCallback() {
 int current=proc.selectedModel();bool bypass=proc.state.getRawParameterValue("bypass")->load()>.5f;
 if(current!=lastSelection||bypass!=lastBypass){lastSelection=current;lastBypass=bypass;model.setSelectedId(current+1,juce::dontSendNotification);sustain.setEnabled(current!=13);repaint();}
}
void MuffEditor::resized() {volume.setBounds(52,100,120,120);tone.setBounds(200,100,120,120);sustain.setBounds(348,100,120,120);model.setBounds(80,405,360,32);foot.setBounds(221,493,78,70);}
void MuffEditor::paint(juce::Graphics& g) {
 auto selected=proc.selectedModel();
 bool supa=selected==19;
 const juce::Colour inks[]={juce::Colour(0xff9d171b),juce::Colour(0xff6a2459),juce::Colour(0xffdd6520),juce::Colour(0xff264b61),juce::Colour(0xffd9cf99),juce::Colour(0xffbd2225)};
 auto ink=supa?juce::Colour(0xff60227b):inks[selected%6];auto body=supa?juce::Colour(0xffdd8a25):((selected==4||selected==15||selected==16)?juce::Colour(0xff354535):juce::Colour(0xffd2d2c9));
 g.fillAll(juce::Colour(0xff18191b));g.setGradientFill(juce::ColourGradient(body.brighter(.25f),25,10,body.darker(.22f),490,640,false));g.fillRoundedRectangle(18,12,484,626,22);
 g.setColour(juce::Colours::black.withAlpha(.12f));for(int y=22;y<630;y+=3)g.drawHorizontalLine(y,28,492);
 for(auto pt:{juce::Point<float>(40,36),{480,36},{40,614},{480,614}}){g.setColour(juce::Colour(0xff969996));g.fillEllipse(pt.x-6,pt.y-6,12,12);g.setColour(juce::Colour(0xff424542));g.drawLine(pt.x-3,pt.y+3,pt.x+3,pt.y-3,1.5f);}
 g.setColour(ink);g.setFont(juce::Font(18,juce::Font::bold));g.drawText("VOLUME",45,73,134,25,juce::Justification::centred);g.drawText("TONE",193,73,134,25,juce::Justification::centred);g.drawText("SUSTAIN",341,73,134,25,juce::Justification::centred);
 g.setFont(juce::Font(56,juce::Font::bold));g.drawText(supa?"SUPA":"MUFF",40,248,440,65,juce::Justification::centred);g.setFont(juce::Font(31,juce::Font::bold));g.drawText(supa?"TONE BENDER":"COLLECTION",40,311,440,42,juce::Justification::centred);
 g.setFont(juce::Font(14));g.drawText("J E R Z Y   /   F U Z Z   M A C H I N E",40,364,440,25,juce::Justification::centred);
 bool enabled=proc.state.getRawParameterValue("bypass")->load()<.5f;g.setColour(enabled?juce::Colour(0xffff3930):juce::Colour(0xff522523));g.fillEllipse(253,460,14,14);
 g.setFont(juce::Font(12));g.setColour(ink);g.drawText("JERZY CUSTOM EFFECTS",45,581,430,20,juce::Justification::centred);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MuffProcessor;}
