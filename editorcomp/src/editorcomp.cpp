#include "editorcomp.h"
#include "Editor.h"
#include "Editors.h"

using namespace std;

Editors *editors = nullptr;

SHAREDSYMBOL void WINPORT_DllStartup(const char *path) {
    // No operations.
}

const wchar_t *getMsg(PluginStartupInfo &info, int msgId) {
    return info.GetMsg(info.ModuleNumber, msgId);
}

const wchar_t *title[1];

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info) {
    editors = new Editors(*Info, *Info->FSF);
    title[0] = getMsg(editors->getInfo(), 0);
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
    memset(Info, 0, sizeof(*Info));
    Info->StructSize = sizeof(*Info);
    Info->Flags = /*PF_EDITOR |*/ PF_DISABLEPANELS;

    Info->PluginConfigStringsNumber = 1;
    Info->PluginConfigStrings = title;
    Info->PluginMenuStringsNumber = 1;
    Info->PluginMenuStrings = title;
};

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber) {
    PluginStartupInfo &info = editors->getInfo();

    int w = 47;
    int h = 10;

    struct FarDialogItem fdi[] = {
            {DI_DOUBLEBOX, 1,  1, w - 2, h - 2, 0,     0, 0, 0,    getMsg(info, 0)},
            {DI_TEXT,      3,  2, 0,     h - 1, FALSE, 0, 0, 0,    getMsg(info, 1)},
            {DI_CHECKBOX,  3,  4, 0,     0,     TRUE,  0, 0, 0,    getMsg(info, 2)},
            {DI_SINGLEBOX, 2,  6, w - 3, 6,     FALSE, 0, 0, 0,    L""},
            {DI_BUTTON,    11, 7, 0,     0,     FALSE, 0, 0, TRUE, getMsg(info, 3)},
            {DI_BUTTON,    26, 7, 0,     0,     FALSE, 0, 0, 0,    getMsg(info, 4)}
    };

    unsigned int size = sizeof(fdi) / sizeof(fdi[0]);
    fdi[2].Param.Selected = editors->getEnabled();
    HANDLE hDlg = info.DialogInit(info.ModuleNumber, -1, -1, w, h, L"config", fdi, size, 0, 0, nullptr, 0);

    int runResult = info.DialogRun(hDlg);
    if (runResult == int(size) - 2)
        editors->setEnabled(info.SendDlgMessage(hDlg, DM_GETCHECK, 2, 0) == BSTATE_CHECKED);

    info.DialogFree(hDlg);
    return 1;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item) {
    return INVALID_HANDLE_VALUE;
};

SHAREDSYMBOL int WINAPI ProcessEditorEventW(int Event, void *Param) {
    if (Event == EE_CLOSE) {
        Editor *editor = editors->getEditor(Param);
        editors->remove(editor);
    } else {
        Editor *editor = editors->getEditor();

        if (editors->getEnabled()) {
            if (Event == EE_READ) {
                //
            } else {
                if (Event == EE_REDRAW) {
                    editor->updateWords();
                    editor->processSuggestion();
                } else if (Event == EE_SAVE) {
                    //?
                }
            }
        }
    }

    return 0;
}

SHAREDSYMBOL int WINAPI ProcessEditorInputW(const INPUT_RECORD *ir) {
    if (!editors->getEnabled())
        return 0;

    if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.dwControlKeyState == 0
        && ir->Event.KeyEvent.wVirtualScanCode == 0
        && !ir->Event.KeyEvent.bKeyDown
        && ir->Event.KeyEvent.wVirtualKeyCode == 0)
        return 0;

    Editor *editor = editors->getEditor();

    // Is regular key event?
    if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.dwControlKeyState == 0
        && ir->Event.KeyEvent.wVirtualScanCode == 0 && ir->Event.KeyEvent.bKeyDown) {

        // Tab ?
        if (editor->getState() == DO_ACTION && ir->Event.KeyEvent.wVirtualKeyCode == 9) {
            editor->confirmSuggestion();
            return 1;
        }

        // Escape ?
        if (editor->getState() == DO_ACTION &&
            (ir->Event.KeyEvent.wVirtualKeyCode == 27 || ir->Event.KeyEvent.wVirtualKeyCode == 46)) {
            bool has_suggestion = editor->getSuggestionLength() > 0;
            editor->declineSuggestion();
            return has_suggestion;
        }

    }

    if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.bKeyDown
        && ir->Event.KeyEvent.wVirtualKeyCode != VK_CONTROL
        && ir->Event.KeyEvent.wVirtualKeyCode != VK_SHIFT
        && ir->Event.KeyEvent.wVirtualKeyCode != VK_MENU)
        editor->declineSuggestion();


    if (ir->EventType == MOUSE_EVENT && ir->Event.MouseEvent.dwButtonState)
        editor->declineSuggestion();

    // Is regular key event?
    if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.dwControlKeyState == 0
        && ir->Event.KeyEvent.wVirtualScanCode == 0 && !ir->Event.KeyEvent.bKeyDown
        && ir->Event.KeyEvent.wVirtualKeyCode != 9 && ir->Event.KeyEvent.wVirtualKeyCode != 8
        && ir->Event.KeyEvent.wVirtualKeyCode != 27 && ir->Event.KeyEvent.wVirtualKeyCode != 46)
        editors->getEditor()->on();

    return 0;
}

SHAREDSYMBOL void WINAPI ExitFARW() {
    if (editors != nullptr) {
        delete editors;
        editors = nullptr;
    }
}
