#include "../Source/MuffDSP.h"
#include <iostream>
#include <vector>
int main() {
 int failures=0;
 auto check=[&](bool ok,const char* msg){ if(!ok){std::cerr<<msg<<'\n';++failures;} };
 std::array<double,6> signatures{};
 for(double rate : {44100.,48000.,96000.}) for(int model=0;model<6;++model) {
  muff::Engine e; e.prepare(rate*4);
  double energy=0;
  for(int i=0;i<100000;++i) {
   float x=.2f*std::sin(float(i)*.037f);
   auto y=e.process(x,0,model,.6f,.5f);
   check(std::isfinite(y)&&std::abs(y)<4,"nonfinite or excessive output");
   check(e.process(0,1,model,.6f,.5f)==0,"channel crosstalk");
   energy+=y*y;
  }
  check(energy>1,"no processed audio");
  if(rate==48000) signatures[model]=energy;
  for(int i=0;i<200000;++i) e.process(0,0,model,.6f,.5f);
  check(std::abs(e.process(0,0,model,.6f,.5f))<1e-5,"DC or unstable tail");
  e.reset(); check(e.process(0,0,model,1,1)==0,"reset failed");
 }
 for(int i=0;i<6;++i) for(int j=i+1;j<6;++j) check(std::abs(signatures[i]-signatures[j])>.01,"identical models");
 std::cout<<"18 sample-rate/model cases; stereo isolation, stability, DC, reset, distinct voicings: "<<(failures?"FAIL":"PASS")<<'\n';
 return failures?1:0;
}
