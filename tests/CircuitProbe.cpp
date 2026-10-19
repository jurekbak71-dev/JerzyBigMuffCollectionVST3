#include "../Source/Circuit/AnalogCircuit.h"
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
int main(int argc,char** argv) {
 const int model=argc>1?std::atoi(argv[1]):13;
 const double fs=argc>2?std::atof(argv[2]):192000;
 const double frequency=argc>3?std::atof(argv[3]):440;
 const double amplitude=argc>4?std::atof(argv[4]):.1;
 const double duration=argc>5?std::atof(argv[5]):.03;
 const double s=argc>6?std::atof(argv[6]):.5,t=argc>7?std::atof(argv[7]):.5,v=argc>8?std::atof(argv[8]):.5;
 muff::circuit::AnalogCircuit c;
 std::cout<<std::setprecision(14);
 if(!c.prepare(fs,static_cast<size_t>(model),s,t,v)){std::cerr<<"DC failed model "<<model<<'\n';return 1;}
 if(argc>9&&std::string(argv[9])=="--spice"){c.writeSpice(std::cout,frequency,amplitude,duration);return 0;}
 auto start=std::chrono::steady_clock::now();
 for(int i=0;i<int(fs*duration);++i){double input=amplitude*std::sin(6.283185307179586*frequency*(i+1)/fs);float y=c.process(float(input),float(s),float(t),float(v));std::cout<<(i+1)/fs<<","<<input<<","<<y<<"\n";}
 auto stats=c.statistics();double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::cerr<<"nodes="<<c.nodeCount()<<" samples="<<stats.samples<<" failures="<<stats.failures<<" meanIterations="<<double(stats.iterations)/double(stats.samples)<<" worst="<<stats.worstIterations<<" elapsed="<<elapsed<<"\n";
 return stats.failures?2:0;
}
