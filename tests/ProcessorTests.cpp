#include "../Source/Plugin.cpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
int benchmarkPlugin(){
 MuffProcessor p;juce::MidiBuffer midi;
 for(bool identical:{true,false}){
  p.prepareToPlay(48000,256);if(!p.circuitsReady())return 20;
  juce::AudioBuffer<float> b(2,256);double peak=0;
  auto start=std::chrono::steady_clock::now();
  for(int block=0;block<94;++block){for(int i=0;i<256;++i){float x=.1f*std::sin(float(i+block*256)*.0575958653f);b.setSample(0,i,x);b.setSample(1,i,identical?x:.15f*std::sin(float(i+block*256)*.11f));}p.processBlock(b,midi);for(int i=0;i<256;++i)peak=std::max(peak,std::abs(double(b.getSample(0,i))));}
  double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  std::cout<<"Supa VST processor "<<(identical?"dual-mono":"independent stereo")<<" seconds="<<elapsed<<" realtime_ratio="<<elapsed/(94.*256/48000)<<" peak="<<peak<<" failures="<<p.solverFailures()<<'\n';
  if(p.solverFailures())return 21;
 }
 return 0;
}
int main(int argc,char** argv) {
 juce::ScopedJuceInitialiser_GUI initialiser;
 if(argc>1&&std::string(argv[1])=="--benchmark")return benchmarkPlugin();
 MuffProcessor p;
 if(p.selectedModel()!=19)return 22;
 p.state.getParameter("circuit")->setValueNotifyingHost(0);
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
  if(!q.circuitsReady())return 8;
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
  if(q.solverFailures()!=0)return 9;
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
  if(q.solverFailures()!=0)return 9;
  q.releaseResources();
 }
 // Loading a 0.1.0 state must select the six-voice legacy engine.
 auto old=p.state.copyState();for(int i=old.getNumChildren()-1;i>=0;--i)if(old.getChild(i).getProperty("id").toString()=="circuit")old.removeChild(i,nullptr);
 auto xml=old.createXml();juce::MemoryBlock oldBinary;juce::AudioProcessor::copyXmlToBinary(*xml,oldBinary);
 q.state.getParameter("circuit")->setValueNotifyingHost(1.f);
 q.setStateInformation(oldBinary.getData(),int(oldBinary.getSize()));
 if(q.state.getRawParameterValue("circuit")->load()!=0)return 10;
 // Exercise every new circuit through the plugin's mixer and oversampler.
 q.prepareToPlay(48000,256);
 juce::AudioBuffer<float> cb(2,256);juce::MidiBuffer cm;
 for(size_t revision=0;revision<muff::circuit::revisions.size();++revision){
  q.state.getParameter("circuit")->setValueNotifyingHost(float(revision+1)/float(muff::circuit::revisions.size()));
  for(int block=0;block<6;++block){for(int i=0;i<256;++i){cb.setSample(0,i,.03f*std::sin(float(i+block*256)*.1f));cb.setSample(1,i,0);}q.processBlock(cb,cm);for(int i=0;i<256;++i)if(!std::isfinite(cb.getSample(0,i))||std::abs(cb.getSample(0,i))>15||std::abs(cb.getSample(1,i))>1e-7)return 11;}
 }
 if(q.solverFailures())return 12;
 juce::MemoryBlock circuitState;q.getStateInformation(circuitState);
 auto restored=std::make_unique<MuffProcessor>();restored->setStateInformation(circuitState.getData(),int(circuitState.getSize()));
 if(restored->selectedModel()!=19)return 23;
 q.releaseResources();
 if(argc>1) {
  q.state.getParameter("circuit")->setValueNotifyingHost(1);
  q.state.getParameter("model")->setValueNotifyingHost(1.f);
  q.state.getParameter("tone")->setValueNotifyingHost(.5f);
  q.state.getParameter("bypass")->setValueNotifyingHost(0.f);
  std::unique_ptr<juce::AudioProcessorEditor> editor(q.createEditor());
  juce::Image image(juce::Image::ARGB,editor->getWidth(),editor->getHeight(),true);
  juce::Graphics graphics(image);editor->paintEntireComponent(graphics,true);
  juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
  juce::FileOutputStream stream(file);if(!stream.openedOk()||!juce::PNGImageFormat().writeImageToStream(image,stream))return 5;
 }
 std::cout<<"PASS state roundtrip, invalid state, oversampled processor, automation, bypass, channel isolation, 14 circuits and legacy state migration\n";
 return 0;
}
