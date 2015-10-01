#pragma once

#include "calibration/Editor.h"
#include "base/interval.h"
#include "Dialogs.h"

#include <string>
#include <map>
#include <set>
#include <functional>
#include "Rtypes.h"
#include "analysis/plot/root_draw.h"
#include "calibration/gui/Indicator_traits.h"
#include "calibration/Editor.h"

#include "TCanvas.h"
#include "TPaveText.h"

#include "TRootEmbeddedCanvas.h"

class TH2D;


namespace ant {
namespace calibration {
namespace gui {

class EditorCanvas;
class EditorWindow;

class EmbeddedEditorCanvas : public TRootEmbeddedCanvas,
        public update_notify_traits
{
private:
    EditorCanvas* theCanvas;

public:
    EmbeddedEditorCanvas(EditorWindow* EditorWindow, const std::string& calID, const TGWindow *p = 0);

    /**
     * @brief SelectInvalid
     */
    virtual void SelectInvalid();
    virtual void SetCalID(const std::string& calID);
    virtual std::list<std::uint32_t> GetSelected() const;
    virtual void clearSelections();
    virtual void EditSelection();
    virtual void ExpandSelection();
    virtual bool InDataEditMode() const;

    virtual void UpdateMe() override;
};


class EditorCanvas:
        public TCanvas,
        public update_notify_traits
{
private:


    TH2D*                         calHist;
    TH1D*                         calDataHist;


    EditorWindow*       editorWindow;
    std::shared_ptr<ant::calibration::Editor> editor;

    std::string             currentCalID;

    std::set<std::uint32_t> indexMemory;
    interval<std::uint32_t> indexInterVal;

    bool                    flag_intervalStart_set;
    bool                    flag_data_editor;

    void markInterval(Int_t y);
    void markLine(Int_t y);

    void StartEditData(TCalibrationData& theData, std::uint32_t stepIndex);
    void applyDataChanges(TCalibrationData& theData);

    void updateCalHist();
    void HandleKeypress(const char key);

    void unFillLine(uint32_t lineNumber);
    void fillLine(uint32_t lineNumber);

public:
    EditorCanvas(EditorWindow* EditorWindow, const std::string& calID, int winID);
    void SetCalID(const std::string& calID);

    bool getDataEditorFlag() const;

    std::list<std::uint32_t> CreateSelectionList();

    virtual void UpdateMe() override;
    void ResetCalibration();
    void MarkInvalid();
    bool EditData();
    void ExpandSelection();
    void HandleInput(EEventType button, Int_t x, Int_t y) override;
};

}
}
}
