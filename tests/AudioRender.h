#pragma once
#include <fstream>
// Offline harness for the actual plugin processor; native float32 mono I/O.
// --render-audio input.f32 output.f32 circuit sustain tone volume sample-rate tail-seconds
inline int renderAudio(int argc,char** argv) {
 if(argc!=10){std::cerr<<"Expected input output circuit sustain tone volume rate tail\n";return 30;}
 std::ifstream input(argv[2],std::ios::binary|std::ios::ate);
 if(!input)return 31;
 auto bytes=input.tellg();if(bytes<=0||bytes%4!=0)return 32;
 std::vector<float> samples(size_t(bytes)/4);input.seekg(0);
 input.read(reinterpret_cast<char*>(samples.data()),bytes);if(!input)return 33;
 for(float x:samples)if(!std::isfinite(x))return 34;
 int circuit=std::stoi(argv[4]);double rate=std::stod(argv[8]);
 // Last argument is the tail duration, in seconds.
 double tail=std::stod(argv[9]);
 if(circuit<1||circuit>14||rate<44100||rate>192000||tail<0||tail>5)return 35;
 MuffProcessor p;
 for(auto item:std::initializer_list<std::pair<const char*,float>>{{"circuit",float(circuit)},{"sustain",std::stof(argv[5])},{"tone",std::stof(argv[6])},{"volume",std::stof(argv[7])}}){
  if(std::string(item.first)!="circuit"&&(item.second<0||item.second>1))return 36;
  auto* parameter=p.state.getParameter(item.first);
  parameter->setValueNotifyingHost(parameter->convertTo0to1(item.second));
 }
 p.setNonRealtime(true);p.prepareToPlay(rate,256);if(!p.circuitsReady())return 37;
 const int pre=int(rate*.1),latency=p.getLatencySamples();
 const size_t length=samples.size()+size_t(rate*tail),total=size_t(pre+latency)+length;
 std::vector<float> output(length);juce::MidiBuffer midi;
 for(size_t offset=0;offset<total;offset+=256){
  int n=int(std::min(size_t(256),total-offset));juce::AudioBuffer<float> b(2,n);b.clear();
  for(int i=0;i<n;++i){auto index=int64_t(offset+i)-pre;if(index>=0&&size_t(index)<samples.size())for(int c=0;c<2;++c)b.setSample(c,i,samples[size_t(index)]);}
  p.processBlock(b,midi);
  for(int i=0;i<n;++i){float y=b.getSample(0,i);if(!std::isfinite(y)||y!=b.getSample(1,i))return 38;auto index=int64_t(offset+i)-pre-latency;if(index>=0&&size_t(index)<length)output[size_t(index)]=y;}
 }
 std::ofstream file(argv[3],std::ios::binary);file.write(reinterpret_cast<const char*>(output.data()),std::streamsize(output.size()*4));
 std::cout<<"latency_samples="<<latency<<" solver_failures="<<p.solverFailures()<<" samples="<<length<<'\n';
 return file&&p.solverFailures()==0?0:39;
}
