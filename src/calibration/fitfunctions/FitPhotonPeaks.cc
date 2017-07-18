#include "FitPhotonPeaks.h"

#include "base/interval.h"
#include "base/Logger.h"
#include "BaseFunctions.h"

#include "TF1.h"
#include "TH1.h"

using namespace ant;
using namespace ant::calibration;
using namespace ant::calibration::gui;
FitPhotonPeaks::FitPhotonPeaks()
{
    func=functions::photonpeaks::getTF1();

    func->SetNpx(1000);
    func->SetParName(0,"#sigma_{o}");
    func->SetParName(1,"#sigma_{sp}");
    func->SetParName(2,"x_{o}");
    func->SetParName(3,"g_{sp}");
    func->SetParName(4,"n");

    AddKnob<KnobsTF1::ParameterKnob>(func->GetParName(0), func, 0, IndicatorProperties::Type_t::slider_vertical);
    AddKnob<KnobsTF1::ParameterKnob>(func->GetParName(1), func, 1, IndicatorProperties::Type_t::slider_vertical);
    AddKnob<KnobsTF1::ParameterKnob>(func->GetParName(2), func, 2, IndicatorProperties::Type_t::slider_vertical);
    AddKnob<KnobsTF1::ParameterKnob>(func->GetParName(1), func, 3, IndicatorProperties::Type_t::slider_vertical);
    AddKnob<KnobsTF1::ParameterKnob>(func->GetParName(1), func, 4, IndicatorProperties::Type_t::slider_vertical);
    AddKnob<KnobsTF1::RangeKnob>("Min", func, KnobsTF1::RangeKnob::RangeEndType::lower);
    AddKnob<KnobsTF1::RangeKnob>("Max", func, KnobsTF1::RangeKnob::RangeEndType::upper);

}
FitPhotonPeaks::~FitPhotonPeaks()
{
}
void FitPhotonPeaks::Draw()
{
    func->Draw("same");
}

void FitPhotonPeaks::Fit(TH1 *hist)
{
    FitFunction::doFit(hist);
}

void FitPhotonPeaks::SetDefaults(TH1 *hist)
{
    if(hist) {
        func->SetParameter(0, 8);
        func->SetParameter(1,8);
        const double max_pos = hist->GetXaxis()->GetBinCenter(hist->GetMaximumBin());
        func->SetParameter(2,max_pos);
        func->SetParameter(3,20);
        func->SetParameter(4,100);
    } else {
        SetRange({0,500});
        func->SetParameter(0, 8);
        func->SetParameter(1,8);
        func->SetParameter(2,100);
        func->SetParameter(3,20);
        func->SetParameter(4,100);
    }
}

void FitPhotonPeaks::SetRange(ant::interval<double> i)
{
    setRange(func, i);
}

ant::interval<double> FitPhotonPeaks::GetRange() const
{
    return getRange(func);
}

FitFunction::SavedState_t FitPhotonPeaks::Save() const
{
    std::vector<double> params;

    saveTF1(func,params);

    return params;
}

void FitPhotonPeaks::Load(const SavedState_t &data)
{
    if(data.size() != std::size_t(2+func->GetNpar())) {
        LOG(WARNING) << "Can't load parameters";
        return;
    }
    SavedState_t::const_iterator pos = data.begin();
    loadTF1(pos, func);

    Sync();

}
double FitPhotonPeaks::GetPeakPosition() const
{
    return func->GetParameter(2);
}
double FitPhotonPeaks::GetPeakWidth() const
{
    return sqrt(pow(func->GetParameter(0),2)+func->GetParameter(4)*pow(func->GetParameter(1),2));
}
