#include "../Source/Plugin.cpp"
#include <iostream>
#include <thread>
#include <atomic>
int main(int argc,char** argv) {
 juce::ScopedJuceInitialiser_GUI initialiser;
 MuffProcessor p;
 p.state.getParameter("model")->setValueNotifyingHost(.8f);
 p.state.getParameter("tone")->setValueNotifyingHost(.23f);
 juce::MemoryBlock saved;p.getStateInformation(saved);
 MuffProcessor q;q.setStateInformation(saved.getData(),int(saved.getSize()));
 if(q.state.getRawParameterValue("model")->load()!=4 || std::abs(q.state.getRawParameterValue("tone")->load()-.23f)>1e-5f) return 1;
 q.setStateInformation("bad",3);
 if(q.state.getRawParameterValue("model")->load()!=4) return 2;
 {
 std::atomic<bool> running{true};
 std::thread restorer([&]{while(running.load()){q.setStateInformation(saved.getData(),int(saved.getSize()));std::this_thread::yield();}});
 struct Join {std::atomic<bool>& r;std::thread& t;~Join(){r=false;t.join();}} join{running,restorer};
 for(double rate:{44100.,48000.,96000.}) {
  q.prepareToPlay(rate,512);
  juce::AudioBuffer<float> b(2,512);juce::MidiBuffer midi;
  double energy=0;
  for(int k=0;k<150;++k) {
   q.state.getParameter("model")->setValueNotifyingHost(float(k%6)/5);
   q.state.getParameter("bypass")->setValueNotifyingHost(k>100?1.f:0.f);
   for(int i=0;i<512;++i) { b.setSample(0,i,.15f*std::sin(float(k*512+i)*.06f));b.setSample(1,i,0); }
   q.processBlock(b,midi);
   for(int i=0;i<512;++i) {
    // IIR arithmetic can leave a numerical floor far below float audio precision.
    float y=b.getSample(0,i);if(!std::isfinite(y)||std::abs(y)>4||std::abs(b.getSample(1,i))>1e-12f) { std::cerr<<"audio failure rate="<<rate<<" block="<<k<<" sample="<<i<<" left="<<y<<" right="<<b.getSample(1,i)<<"\n";return 3; }
    energy+=y*y;
   }
  }
  if(energy<1 || q.getLatencySamples()<0) return 4;
  q.releaseResources();
 }
 }
 for(int channels:{1,2}) {
  auto layout=q.getBusesLayout();
  layout.inputBuses.set(0,channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo());
  layout.outputBuses=layout.inputBuses;
  if(!q.setBusesLayout(layout)) return 6;
  q.prepareToPlay(48000,512);
  juce::AudioBuffer<float> large(channels,2049);large.clear();
  large.setSample(0,0,std::numeric_limits<float>::quiet_NaN());
  juce::MidiBuffer midi;q.processBlock(large,midi);
  for(int c=0;c<channels;++c)for(int i=0;i<large.getNumSamples();++i)if(!std::isfinite(large.getSample(c,i)))return 7;
  q.releaseResources();
 }
 if(argc>1) {
  q.state.getParameter("model")->setValueNotifyingHost(1.f);
  q.state.getParameter("tone")->setValueNotifyingHost(.5f);
  q.state.getParameter("bypass")->setValueNotifyingHost(0.f);
  std::unique_ptr<juce::AudioProcessorEditor> editor(q.createEditor());
  juce::Image image(juce::Image::ARGB,editor->getWidth(),editor->getHeight(),true);
  juce::Graphics graphics(image);editor->paintEntireComponent(graphics,true);
  juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
  juce::FileOutputStream stream(file);if(!stream.openedOk()||!juce::PNGImageFormat().writeImageToStream(image,stream))return 5;
 }
 std::cout<<"PASS state roundtrip, invalid state, full 4x processor, automation, bypass, channel isolation at 3 sample rates\n";
 return 0;
}
