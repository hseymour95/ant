#include "APT_Photon.h"

#include "Photon_GUI.h"

#include "gui/CalCanvas.h"

#include "calibration/fitfunctions/FitGaus.h"

#include "expconfig/detectors/APT.h"

#include "base/Logger.h"

#include "tree/TEventData.h"

#include <list>


using namespace std;
using namespace ant;
using namespace ant::calibration;

APT_Photon::APT_Photon(
        const detector_ptr_t& apt,
        const std::shared_ptr<DataManager>& calmgr,
        const Calibration::Converter::ptr_t& converter,
        //defaults_t defaultPedestals,
        //defaults_t defaultGains,
        //defaults_t defaultThresholds_Raw,
        //defaults_t defaultThresholds_MeV,
        //defaults_t defaultRelativeGains
        defaults_t defaultPhotonPeak
        ) :
    Photon(apt,
           calmgr,
           converter,
           //defaultPedestals,
           //defaultGains,
           //defaultThresholds_Raw,
           //defaultThresholds_MeV,
           //defaultRelativeGains),
           defaultPhotonPeak),
    apt_detector(apt)
{

}


APT_Photon::~APT_Photon()
{

}

void APT_Photon::GetGUIs(std::list<std::unique_ptr<gui::CalibModule_traits> >& guis, OptionsPtr options)
{
    if(options->HasOption("HistogramPath")) {
        LOG(INFO) << "Overwriting histogram path to " << options->Get<string>("HistogramPath");
    }

    //guis.emplace_back(std_ext::make_unique<energy::GUI_Pedestals>(
                          //GetName(),
                          //options,
                          //Pedestals,
                          //calibrationManager,
                          //apt_detector,
                          //make_shared<gui::FitGaus>()
                          //));
    guis.emplace_back(std_ext::make_unique<photon::GUI_Photon>(
                          GetName(),
                          options,
                          PhotonPeak,
                          calibrationManager,
                          apt_detector,
                          interval<double>(1.0, 1000.0)
                          //make_shared<gui::FitPhotonPeaks>
                          ));

    /***if(options->HasOption("UseHEP")) {
        guis.emplace_back(std_ext::make_unique<energy::GUI_HEP>(
                              GetName(),
                              options,
                              RelativeGains,
                              calibrationManager,
                              apt_detector,
                              3.5 // MeV from MC cocktail
                              ));***/
   /***}
    else if(options->HasOption("UseBanana")) {
        guis.emplace_back(std_ext::make_unique<energy::GUI_Banana>(
                    GetName(),
                    options,
                    RelativeGains,
                    calibrationManager,
                    apt_detector,
                    interval<double>(0,1000.0),
                    1.0 // MeV, from 2pi0 MC cocktail, -> same as APT, banana is quite flat there
                    ));**/
    /***}
    else {
        guis.emplace_back(std_ext::make_unique<energy::GUI_BananaSlices>(
                              GetName(),
                              options,
                              RelativeGains,
                              calibrationManager,
                              apt_detector,
                              interval<double>(100.0, 800.0)
                              ));
    }***/


}

void ant::calibration::APT_Photon::ApplyTo(TEventData& reconstructed)
{
    // search for APT/CB candidates and correct APT energy by CB theta angle
    for(TCandidate& cand : reconstructed.Candidates) {
        const bool cb_and_apt = cand.Detector & Detector_t::Type_t::CB &&
                                cand.Detector & Detector_t::Type_t::APT;
        if(!cb_and_apt)
            continue;
        cand.VetoEnergy *= sin(cand.Theta);
    }
}
