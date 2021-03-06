#pragma once

#include "analysis/physics/Physics.h"
#include "base/interval.h"

class TH1D;
class TH2D;

namespace ant {
namespace analysis {
namespace physics {

class ProtonTagger : public Physics {
public:

    TTree* tree = nullptr;

    double    b_tagTime = 0.0;
    unsigned  b_tagCh   = 0;
    double    b_ggIM    = 0.0;
    double    b_MM      = 0.0;
    double    b_angle   = 0.0;
    double    b_Time    = 0.0;
    double    b_E       = 0.0;
    double    b_veto    = 0.0;
    unsigned  b_Size    = 0;
    double    b_cbtime  = 0.0;
    double    b_Eshort  = 0.0;
    double    b_EshortCentral = 0.0;


    ProtonTagger(const std::string& name, OptionsPtr opts);

    virtual void ProcessEvent(const TEvent& event, manager_t& manager) override;
    virtual void ShowResult() override;
};

}
}
}
