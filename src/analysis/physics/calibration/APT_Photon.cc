#include "APT_Photon.h"

#include "expconfig/ExpConfig.h"

#include "base/std_ext/container.h"

#include "base/Logger.h"

#include <numeric>  // std::accumulate

using namespace std;
using namespace ant;
using namespace ant::analysis::physics;

Photon::Photon(const Detector_t::Type_t& detectorType, const string& name, OptionsPtr opts) :
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
                      "APT Pedestals",
                      "Raw ADC value",
                      "#",
                      apt_rawvalues,
                      apt_channels,
                      "Pedestals");
}

void APT_Photon::ProcessEvent(const TEvent& event, manager_t&)
{
    for(const TDetectorReadHit& dethit : event.Reconstructed().DetectorReadHits) {
        if(dethit.DetectorType != Detector->Type)
            continue;
        if(dethit.ChannelType != Channel_t::Type_t::Integral)
            continue;
        hTimeMultiplicity->Fill(dethit.Values.size(), dethit.Channel);
    }
}
void Photon::ShowResult()
{
    canvas(GetName())
            <<drawoption("colz")
            <<h_pedestals
            <<endc;
}
namespace ant {
namespace analysis {
namespace physics {

struct APT_Photon : Photon{
    APT_Photon(const std::string& name, OptionsPtr opts) :
        Photon(Detector_t::Type_t::APT, name, opts)
    {}
};
AUTO_REGISTER_PHYSICS(APT_Photon)
}}}
