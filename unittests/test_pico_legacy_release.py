"""Exercise the comparison stop worker with fake, fresh robot feedback."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class LegacyReleaseTest(unittest.TestCase):
    def test_fixed_targets_and_bounded_transmission(self):
        header = Path(__file__).resolve().parents[1] / 'teleop/legacy_release_stop.hpp'
        harness = r'''
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <future>
#include <string>
#include <thread>
namespace rcore { namespace sdk {
using JointPositions=std::array<double,6>;
using JointVelocities=std::array<double,6>;
struct Pose {std::array<double,3> p{},r{}; auto& tvec(){return p;} auto& rvec(){return r;}};
struct State {
 double timestamp=1; bool cartesian_valid=true;
 JointPositions positions{}; JointVelocities velocities{};
 std::array<double,3> cartesian_position{.1,.2,.3},cartesian_orientation{.4,.5,.6};
 struct Errors {int count=0; int GetLatchedErrorCount()const{return count;}} errors;
};
struct Robot {
 int p=0,j=0; bool stale=false,fault=false,invalid=false;
 auto& Motion(){return *this;}
 State GetState(){State s; s.positions.fill(.25); s.errors.count=fault;
 s.cartesian_valid=!invalid;
 if(!stale)s.timestamp=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
 return s;}
 void ServoP(const Pose& t){assert(t.p[0]==.1 && t.r[2]==.6);++p;}
 void JointPassthrough(const JointPositions& q,const JointVelocities& v){for(int i=0;i<6;++i){assert(q[i]==.25);assert(v[i]==0);}++j;}
};
}}
class PicoServoStopper {};
''' + '#include "' + str(header) + '"\n' + r'''
std::string Run(rcore::sdk::Robot& r,bool joint){
 PicoLegacyReleaseStopper s; s.Request(r,joint);s.Request(r,joint);
 s.Wait();std::string error;assert(s.Poll(error));assert(!s.Pending());return error;
}
int main(){
 rcore::sdk::Robot p; assert(Run(p,false).empty());assert(p.p==100 && p.j==0);
 rcore::sdk::Robot j; assert(Run(j,true).empty());assert(j.j==100 && j.p==0);
 rcore::sdk::Robot stale;stale.stale=true;assert(!Run(stale,false).empty());
 rcore::sdk::Robot fault;fault.fault=true;assert(!Run(fault,false).empty());assert(fault.p==0);
 rcore::sdk::Robot invalid;invalid.invalid=true;assert(!Run(invalid,false).empty());assert(invalid.p==0);
}
'''
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / 'test.cpp'
            cpp.write_text(harness)
            exe = Path(directory) / 'test'
            subprocess.run(['g++', '-std=c++17', '-pthread', str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=10)


if __name__ == '__main__':
    unittest.main()
