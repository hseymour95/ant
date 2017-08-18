#include "PID_Energy.h"

#include "Energy_GUI.h"

#include "gui/CalCanvas.h"

#include "calibration/fitfunctions/FitGaus.h"

#include "expconfig/detectors/PID.h"

#include "base/Logger.h"

#include "tree/TEventData.h"

#include <list>


using namespace std;
using namespace ant;
using namespace ant::calibration;

PID_Energy::PID_Energy(
        const detector_ptr_t& pid,
        const std::shared_ptr<DataManager>& calmgr,
        const Calibration::Converter::ptr_t& converter,
        defaults_t defaultPedestals,
        defaults_t defaultGains,
        defaults_t defaultThresholds_Raw,
        defaults_t defaultThresholds_MeV,
        defaults_t defaultRelativeGains
        ) :
    Energy(pid,
           calmgr,
           converter,
           defaultPedestals,
           {0.0}, // PID does not calibrate scintillation photons
           defaultGains,
           defaultThresholds_Raw,
           defaultThresholds_MeV,
           defaultRelativeGains),
    pid_detector(pid)
{

}


PID_Energy::~PID_Energy()
{

}

void PID_Energy::GetGUIs(std::list<std::unique_ptr<gui::CalibModule_traits> >& guis, OptionsPtr options)
{
    if(options->HasOption("HistogramPath")) {
        LOG(INFO) << "Overwriting histogram path to " << options->Get<string>("HistogramPath");
    }

    guis.emplace_back(std_ext::make_unique<energy::GUI_Pedestals>(
                          GetName(),
                          options,
                          Pedestals,
                          calibrationManager,
                          pid_detector,
                          make_shared<gui::FitGaus>()
                          ));

    if(options->HasOption("UseHEP")) {
        guis.emplace_back(std_ext::make_unique<energy::GUI_HEP>(
                              GetName(),
                              options,
                              RelativeGains,
                              calibrationManager,
                              pid_detector,
                              3.5 // MeV from MC cocktail
                              ));
    }
    else if(options->HasOption("UseBanana")) {
        guis.emplace_back(std_ext::make_unique<energy::GUI_Banana>(
                    GetName(),
                    options,
                    RelativeGains,
                    calibrationManager,
                    pid_detector,
                    interval<double>(0,1000.0),
                    1.0 // MeV, from 2pi0 MC cocktail, -> same as PID, banana is quite flat there
                    ));
    }
    else {
        guis.emplace_back(std_ext::make_unique<energy::GUI_BananaSlices>(
                              GetName(),
                              options,
                              RelativeGains,
                              calibrationManager,
                              pid_detector,
                              interval<double>(100.0, 800.0)
                              ));
    }


}

void ant::calibration::PID_Energy::ApplyTo(TEventData& reconstructed)
{
    // search for PID/CB candidates and correct PID energy by CB theta angle
    for(TCandidate& cand : reconstructed.Candidates) {
        const bool cb_and_pid = cand.Detector & Detector_t::Type_t::CB &&
                                cand.Detector & Detector_t::Type_t::PID;
        if(!cb_and_pid)
            continue;
        cand.VetoEnergy *= sin(cand.Theta);
    }

    //reconstructed.Target.ActivePhotons = 5;
}
