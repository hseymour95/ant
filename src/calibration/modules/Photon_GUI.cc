#include "Photon_GUI.h"

#include "calibration/DataManager.h"
#include "calibration/gui/CalCanvas.h"
#include "calibration/fitfunctions/FitPhotonPeaks.h"

#include "tree/TCalibrationData.h"

#include "base/math_functions/Linear.h"
#include "base/std_ext/math.h"
#include "base/std_ext/string.h"
#include "base/TH_ext.h"
#include "base/Logger.h"

#include "TH2.h"
#include "TH3.h"
#include "TF1.h"

#include "TGNumberEntry.h"

using namespace std;
using namespace ant;
using namespace ant::calibration;
using namespace ant::calibration::photon;

GUI_Photon::GUI_Photon(const string& basename,
                         OptionsPtr options,
                         CalibType& type,
                         const std::shared_ptr<DataManager>& calmgr,
                         const detector_ptr_t& detector,
                         const interval<double>& projectionrange,
                         const double proton_peak_mc_pos
                         ) :
    GUI_CalibType(basename, options, type, calmgr, detector),
    func(make_shared<gui::FitPhotonPeaks>()),
    projection_range(projectionrange),
    proton_peak_mc(proton_peak_mc_pos),
    full_hist_name(
            options->Get<string>("HistogramPath", CalibModule_traits::GetName())
            + "/"
            + options->Get<string>("HistogramName", "projections_hep"))
{

}

std::shared_ptr<TH1> GUI_Photon::GetHistogram(const WrapTFile& file) const
{
    return file.GetSharedHist<TH1>(full_hist_name);
}

void GUI_Photon::InitGUI(gui::ManagerWindow_traits& window)
{
    GUI_CalibType::InitGUI(window);
    window.AddNumberEntry("Chi2/NDF limit for autostop", AutoStopOnChi2);

    canvas = window.AddCalCanvas();

    h_peaks = new TH1D("h_peaks","Peak positions",GetNumberOfChannels(),0,GetNumberOfChannels());
    h_peaks->SetXTitle("Channel Number");
    h_peaks->SetYTitle("High Energy Proton Peak / MeV"); // TO DO change titles of plots
    h_relative = new TH1D("h_relative","Relative change from previous gains",GetNumberOfChannels(),0,GetNumberOfChannels());
    h_relative->SetXTitle("Channel Number");
    h_relative->SetYTitle("Relative change / %");

    LOG(INFO) << "Use high energy protons for PID gain calibration";
}

gui::CalibModule_traits::DoFitReturn_t GUI_Photon::DoFit(const TH1& hist, unsigned ch)
{
    if(detector->IsIgnored(ch))
        return DoFitReturn_t::Skip;

    auto& hist2 = dynamic_cast<const TH2&>(hist);
    h_projection = hist2.ProjectionX("h_projection",ch+1,ch+1);

    // stop at empty histograms
    if(h_projection->GetEntries()==0)
        return DoFitReturn_t::Display;

    //auto range = interval<double>(1,9);
    auto range = interval<double>(.8,9.5);

    func->SetDefaults(h_projection);
    func->SetRange(range);
    const auto it_fit_param = fitParameters.find(ch);
    if(it_fit_param != fitParameters.end() && !IgnorePreviousFitParameters && false) {
        VLOG(5) << "Loading previous fit parameters for channel " << ch;
        func->Load(it_fit_param->second);
    }
    else {
        func->FitSignal(h_projection);
    }

    auto fit_loop = [this] (size_t retries) {
        do {
            func->Fit(h_projection);
            VLOG(5) << "Chi2/dof = " << func->Chi2NDF();
            if(func->Chi2NDF() < AutoStopOnChi2) {
                return true;
            }
            retries--;
        }
        while(retries>0);
        return false;
    };

    if(fit_loop(5))
        return DoFitReturn_t::Next;

    // try with defaults ...
    func->SetDefaults(h_projection);
    func->Fit(h_projection);

    if(fit_loop(5))
        return DoFitReturn_t::Next;

    // ... and with defaults and first a signal only fit ...
    func->SetDefaults(h_projection);
    func->FitSignal(h_projection);

    if(fit_loop(5))
        return DoFitReturn_t::Next;

    // ... and as a last resort background, signal and a few last fit tries
    func->SetDefaults(h_projection);
    func->FitBackground(h_projection);
    func->Fit(h_projection);
    func->FitSignal(h_projection);

    if(fit_loop(5))
        return DoFitReturn_t::Next;

    // reached maximum retries without good chi2
    LOG(INFO) << "Chi2/dof = " << func->Chi2NDF();
    return DoFitReturn_t::Display;
}

void GUI_Photon::DisplayFit()
{
    canvas->Show(h_projection, func.get());
}

void GUI_Photon::StoreFit(unsigned channel)
{
    const double oldValue = previousValues[channel];
    const double peak = func->GetPeakPosition();
    const double newValue = oldValue * proton_peak_mc / peak;

    calibType.Values[channel] = newValue;

    const double relative_change = 100*(newValue/oldValue-1);

    LOG(INFO) << "Stored Ch=" << channel << ": PeakPosition " << peak
              << " MeV,  gain changed " << oldValue << " -> " << newValue //TO DO: NOT MEV
              << " (" << relative_change << " %)";


    // don't forget the fit parameters
    fitParameters[channel] = func->Save();

    h_peaks->SetBinContent(channel+1, peak);
    h_relative->SetBinContent(channel+1, relative_change);

}

bool GUI_Photon::FinishSlice()
{
    canvas->Clear();
    canvas->Divide(1,2);

    canvas->cd(1);
    h_peaks->SetStats(false);
    h_peaks->Draw("P");
    canvas->cd(2);
    h_relative->SetStats(false);
    h_relative->Draw("P");

    return true;
}
