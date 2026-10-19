#include "../Source/Circuit/AnalogCircuit.h"
#include <cmath>
#include <iostream>
#include <limits>
int main(){
 int failures=0;
 auto check=[&](bool ok,const char* what){if(!ok){std::cerr<<what<<'\n';++failures;}};
 double expError=0;
 for(int i=0;i<=120000;++i){double x=-80+i*.001;double ref=std::exp(x);expError=std::max(expError,std::abs(muff::circuit::AnalogCircuit::exponential(x)/ref-1));}
 check(expError<4e-10,"junction exponential accuracy");
 int cases=0;
 for(size_t m=0;m<muff::circuit::revisions.size();++m)for(double fs:{176400.,192000.,384000.}){
  muff::circuit::AnalogCircuit circuit;
  if(!circuit.prepare(fs,m)){std::cerr<<"DC failed "<<m<<'\n';++failures;continue;}
  double quiet=0;
  for(int i=0;i<1000;++i)quiet=std::max(quiet,std::abs(double(circuit.process(0,.5f,.5f,.5f))));
  check(quiet<1e-7,"DC initialisation causes audible silence output");
  double energy=0;
  for(int i=0;i<6000;++i){float s=float(.5+.49*std::sin(i*.0007)),t=float(.5+.49*std::sin(i*.001)),v=float(.5+.49*std::sin(i*.0009));float y=circuit.process(float(.2*std::sin(i*.02)),s,t,v);check(std::isfinite(y)&&std::abs(y)<15,"invalid circuit output");energy+=y*y;}
  check(energy>1e-8,"silent circuit");
  check(std::isfinite(circuit.process(std::numeric_limits<float>::quiet_NaN(),.5f,.5f,.5f)),"NaN input handling");
  if(circuit.statistics().failures){std::cerr<<"convergence failed model="<<m<<" fs="<<fs<<" count="<<circuit.statistics().failures<<'\n';++failures;}
  ++cases;
 }
 // The boost route bypasses the Sustain network completely.
 muff::circuit::AnalogCircuit a,b;
 check(a.prepare(192000,7,0,.5,.5)&&b.prepare(192000,7,1,.5,.5),"boost DC");
 for(int i=0;i<2000;++i){float x=float(.03*std::sin(i*.04));check(std::abs(a.process(x,0,.5f,.5f)-b.process(x,1,.5f,.5f))<1e-8,"boost incorrectly uses sustain");}
 std::cout<<cases<<" model/rate cases; DC, automation, finite audio, convergence, boost topology; exp relative error="<<expError<<" failures="<<failures<<'\n';
 return failures?1:0;
}
