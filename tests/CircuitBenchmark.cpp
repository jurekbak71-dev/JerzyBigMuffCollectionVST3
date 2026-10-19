#include "../Source/Circuit/AnalogCircuit.h"
#include <chrono>
#include <iostream>
int main(int argc,char** argv){
 const size_t model=argc>1?size_t(std::atoi(argv[1])):13;
 muff::circuit::AnalogCircuit left,right;
 if(!left.prepare(192000,model,1,.5,.7)||!right.prepare(192000,model,1,.5,.7))return 1;
 double energy=0;constexpr int samples=48000; // 250 ms stereo at the internal rate
 auto start=std::chrono::steady_clock::now();
 for(int i=0;i<samples;++i){float x=float(.2*std::sin(i*.0143989663));energy+=left.process(x,1,.5f,.7f);energy+=right.process(x,1,.5f,.7f);}
 double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 auto a=left.statistics(),b=right.statistics();
 std::cout<<"stereo_seconds="<<seconds<<" realtime_ratio="<<seconds/.25<<" checksum="<<energy<<" failures="<<a.failures+b.failures<<" mean_iterations="<<double(a.iterations+b.iterations)/(2*samples)<<"\n";
 return a.failures||b.failures?2:0;
}
