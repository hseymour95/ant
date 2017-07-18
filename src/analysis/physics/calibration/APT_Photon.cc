#include "APT_Photon.h"

#include "expconfig/ExpConfig.h"

#include "base/std_ext/container.h"

#include "base/Logger.h"

#include <numeric>  // std::accumulate

using namespace std;
using namespace ant;
using namespace ant::analysis::physics;
APT_Photon::APT_Photon(const string& name, OptionsPtr opts) :
    Physics(name, opts)
{
    const auto detector = ExpConfig::Setup::GetDetector(Detector_t::Type_t::APT);
    const auto nChannels = detector->GetNChannels();

    const BinSettings apt_channels(nChannels);
    const BinSettings apt_rawvalues(300);
    const BinSettings energybins(500, 0, 10);
    //const BinSettings cb_energy(600, 0, 1200);
    const BinSettings apt_energy(100, 0, 10);

    h_pedestals = HistFac.makeTH2D(
                      "PID Pedestals",
                      "Raw ADC value",
                      "#",
                      apt_rawvalues,
                      apt_channels,
                      "Pedestals");
}
