#pragma once

#include "CalibType.h"
#include <new>

class TF1;

namespace ant {
namespace calibration {

class DataManager;

namespace gui {
class PeakingFitFunction;
}

namespace gui {
class FitPhotonPeaks;
}

namespace photon{
struct GUI_Photon : GUI_CalibType {
    GUI_Photon(const std::string& basename,
            OptionsPtr options,
            CalibType& type,
            const std::shared_ptr<DataManager>& calmgr,
            const detector_ptr_t& detector,
            const interval<double>& projectionrange,
            const double proton_peak_mc_pos
            );

    virtual std::shared_ptr<TH1> GetHistogram(const WrapTFile& file) const override;
    virtual void InitGUI(gui::ManagerWindow_traits& window) override;

    virtual DoFitReturn_t DoFit(const TH1& hist, unsigned ch) override;
    virtual void DisplayFit() override;
    virtual void StoreFit(unsigned channel) override;
    virtual bool FinishSlice() override;

protected:
    std::shared_ptr<gui::PeakingFitFunction> func;

    gui::CalCanvas* canvas;
    TH1D* h_projection = nullptr;
    TH1D* h_peaks = nullptr;
    TH1D* h_relative = nullptr;

    interval<double> projection_range;
    const double proton_peak_mc;

    double AutoStopOnChi2 = 16;
    const std::string full_hist_name;
};
}
}
}
