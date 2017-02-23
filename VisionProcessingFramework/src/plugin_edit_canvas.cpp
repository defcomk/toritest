/**
 * @file      plugin_edit_canvas.cpp
 * @brief     Source for PluginEditCanvas class
 * @author    Vision Processing Community
 * @copyright 2016 Vision Processing Community. All rights reserved.
 */

#include <string>
#include <vector>

#include "./main_wnd_define.h"
#include "./plugin_edit_canvas.h"
#include "./plugin_edit_canvas_define.h"
#include "./plugin_manager_wnd.h"

/* Custom event definition for wxWidgets */
#define EVT_CUSTOM_PATCHED(event, id, func)                            \
  DECLARE_EVENT_TABLE_ENTRY(                                           \
      event, id, wxID_ANY,                                             \
      (wxObjectEventFunction)(                                         \
          wxEventFunction) static_cast<wxCommandEventFunction>(&func), \
      (wxObject *)NULL)                                                \
  ,

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_LOCAL_EVENT_TYPE(UPDATE_CANVAS, wxNewEventType())
END_DECLARE_EVENT_TYPES()

DEFINE_LOCAL_EVENT_TYPE(UPDATE_CANVAS)

/* Declare the event for PluginEditCanvas */
BEGIN_EVENT_TABLE(PluginEditCanvas, wxScrolledWindow)
EVT_PAINT(PluginEditCanvas::OnPaint)
EVT_LEFT_DOWN(PluginEditCanvas::OnMouseDown)
EVT_RIGHT_DOWN(PluginEditCanvas::OnMouseDown)
EVT_LEFT_UP(PluginEditCanvas::OnMouseUp)
EVT_RIGHT_UP(PluginEditCanvas::OnMouseUp)
EVT_CUSTOM_PATCHED(UPDATE_CANVAS, wxID_ANY, PluginEditCanvas::OnUpdateCanvas)
END_EVENT_TABLE()

/**
 * @brief
 * Constructor.
 * @param parent [in] parent class pointer.
 * @param id [in] wxWidgets window ID.
 * @param thread_running_cycle_manager [in]
 *        ThreadRunningCycleManager class pointer.
 */
PluginEditCanvas::PluginEditCanvas(
    wxWindow *parent, const wxWindowID id,
    ThreadRunningCycleManager *thread_running_cycle_manager)
    : wxScrolledWindow(parent, id, wxPoint(350, 60),
                       wxSize(kEditCanvasScreenWidth, kEditCanvasScreenHeight),
                       wxHSCROLL | wxVSCROLL | wxFULL_REPAINT_ON_RESIZE) {
  parent_ = dynamic_cast<PluginManagerWnd *>(parent);

  // background color of this canvas
  SetBackgroundColour(wxColour(255, 200, 200));

  wxdlg_plus_menu_ = new WxDlgPlusMenu(this, wxID_ANY, wxT("Menu"));
  wxdlg_plugin_menu_ = new WxDlgPluginMenu(this, wxID_ANY, wxT("Menu"));
  wxdlg_cycle_menu_ = new WxDlgCycleMenu(this, wxID_ANY, wxT("Cycle setting"));

  thread_running_cycle_manager_ = thread_running_cycle_manager;

  scroled_x_ = 0;
  scroled_y_ = 0;

  DEBUG_PRINT("PluginEditCanvas::PluginEditCanvas \n");
}

/**
 * @brief
 * Destructor.
 */
PluginEditCanvas::~PluginEditCanvas() {
  // Do nothing
  DEBUG_PRINT("PluginEditCanvas::~PluginEditCanvas\n");
}

/**
 * @brief
 * Print debug logs all of plugin connection status(prev, current and next).
 */
void PluginEditCanvas::EditCanvasDebugLog(void) {
  DEBUG_PRINT("[Debug]y_size = %d \n", flow_plugin_list_.size());
  for (int i = 0; i < flow_plugin_list_.size(); i++) {
    DEBUG_PRINT("[Debug]y = %d , x_size = %d \n", i,
                flow_plugin_list_[i].size());
    for (int j = 0; j < flow_plugin_list_[i].size(); j++) {
      DEBUG_PRINT("[Debug]****** Check(%d, %d) start ****** \n", j, i);
      DEBUG_PRINT("[Debug] prev_item_info = %s \n",
                  flow_plugin_list_[i][j]->GetPrevPluginName().c_str());

      DEBUG_PRINT("[Debug] current_item_info = %s \n",
                  flow_plugin_list_[i][j]->plugin_name().c_str());

      DEBUG_PRINT("[Debug] next_item_size = %d \n",
                  flow_plugin_list_[i][j]->next_item_info_.size());
      for (int k = 0; k < flow_plugin_list_[i][j]->next_item_info_.size();
           k++) {
        DEBUG_PRINT("[Debug] next_item_info(%d) = %s \n", k,
                    flow_plugin_list_[i][j]->GetNextPluginName(k).c_str());
      }

      DEBUG_PRINT("[Debug]****** Check(%d, %d) end   ****** \n", j, i);
    }
  }
}

void PluginEditCanvas::OnPaint(wxPaintEvent &event) {
  UpdateCanvasPluginList();

  wxPaintDC dc_line(this);
  wxPaintDC dc_circle(this);
  wxPaintDC dc_Rectangle(this);
  wxBitmap bitmap;
  bitmap.LoadFile(wxT(kBitmapPlusPath), wxBITMAP_TYPE_BMP);

  wxBrush brush(wxColour(255, 0, 100));
  dc_line.Clear();
  dc_line.SetPen(wxPen(wxColour(0, 0, 0)));
  dc_line.SetBrush(brush);

  wxBrush brush_circle(wxColour(255, 0, 100));
  dc_circle.Clear();
  dc_circle.SetPen(wxPen(wxColour(0, 0, 0)));
  dc_circle.SetBrush(brush);

  dc_Rectangle.Clear();
  dc_Rectangle.SetBrush(brush);

  // Update unscrolled position
  CalcUnscrolledPosition(0, 0, &scroled_x_, &scroled_y_);
  for (int i = 0; i < flow_plugin_list_.size(); i++) {
    for (int j = 0; j < flow_plugin_list_[i].size(); j++) {
      flow_plugin_list_[i][j]->UpdateUnScrolledPosition(scroled_x_, scroled_y_);

      // Draw flow item
      flow_plugin_list_[i][j]->Draw(dc_line, dc_circle, dc_Rectangle);
    }
  }
  DEBUG_PRINT("PluginEditCanvas::OnPaint \n");
}

void PluginEditCanvas::OnMouseDown(wxMouseEvent &event) {
  DEBUG_PRINT("PluginEditCanvas::OnMouseDown\n");
  bool isTouchAddPlugin = false;
  bool isTouchCurrentPlugin = false;
  bool isTouchCyclePlugin = false;
  int i, j;
  int x = event.GetX();
  int y = event.GetY();
  PluginEditItemInfo *current_item_info;

  // Clear selected state.
  for (i = 0; i < flow_plugin_list_.size(); i++) {
    for (j = 0; j < flow_plugin_list_[i].size(); j++) {
      flow_plugin_list_[i][j]->set_selected(false);
    }
  }

  // Check touched item.
  for (i = 0; i < flow_plugin_list_.size(); i++) {
    for (j = 0; j < flow_plugin_list_[i].size(); j++) {
      if (flow_plugin_list_[i][j]->IsTouchAddPluginPane(x, y) == true) {
        isTouchAddPlugin = true;
        break;
      }
      if (flow_plugin_list_[i][j]->IsTouchCurrentPluginPane(x, y) == true) {
        isTouchCurrentPlugin = true;
        break;
      }
      if (flow_plugin_list_[i][j]->IsTouchCyclePane(x, y) == true) {
        isTouchCyclePlugin = true;
        break;
      }
    }
    if (isTouchAddPlugin == true || isTouchCurrentPlugin == true ||
        isTouchCyclePlugin == true) {
      break;
    }
  }

  if (isTouchAddPlugin == false && isTouchCurrentPlugin == false &&
      isTouchCyclePlugin == false) {
    // Do not touch valid item.
    Refresh();
    Update();
    PostUpdateCanvasEvent();
    return;
  }

  // Get touch pos
  touched_plugin_num_.x = j;
  touched_plugin_num_.y = i;
  current_item_info =
      flow_plugin_list_[touched_plugin_num_.y][touched_plugin_num_.x];

  if (event.LeftDown() == true) {
    DEBUG_PRINT("Left down \n");
    DEBUG_PRINT("down: x=%d,y=%d\n", x, y);

    // Add plugin
    if (isTouchAddPlugin == true) {
      if (parent_->image_proc_state() == kStop) {
        int plugin_add_enable_state;
        if (wxdlg_plus_menu_ == NULL) {
          wxdlg_plus_menu_ = new WxDlgPlusMenu(this, wxID_ANY, wxT("Menu"));
        }

        DEBUG_PRINT("Screen pos(%d,%d)\n", this->GetScreenPosition().x,
                    this->GetScreenPosition().y);

        // Update button enable/disable state
        plugin_add_enable_state = current_item_info->plugin_add_enable_state();

        if (flow_plugin_list_.size() >= kMaxColPluginNum) {
          plugin_add_enable_state =
              plugin_add_enable_state & !kEnableToAddSubNextPlugin;
        }

        wxdlg_plus_menu_->UpdateButtonState(current_item_info,
                                            plugin_add_enable_state);

        // Set position then show popup dialog
        //  about selecting how to add(or delete) plugin.
        wxdlg_plus_menu_->Move(this->GetScreenPosition().x + x,
                               this->GetScreenPosition().y + y);
        wxdlg_plus_menu_->ShowModal();
      }
    } else if (isTouchCurrentPlugin == true) {
      if (parent_->image_proc_state() == kStop) {
        if (isTouchCurrentPlugin == true) {
          PluginBase *plugin;
          std::string plugin_name = "";

          plugin_name = current_item_info->plugin_name();
          if (plugin_name != "") {
            if (wxdlg_plugin_menu_ == NULL) {
              wxdlg_plugin_menu_ =
                  new WxDlgPluginMenu(this, wxID_ANY, wxT("Menu"));
            }

            // Set position then show popup dialog about replace plugin.
            wxdlg_plugin_menu_->Move(this->GetScreenPosition().x + x,
                                     this->GetScreenPosition().y + y);
            wxdlg_plugin_menu_->ShowModal();
          }
        }
      }
    } else if (isTouchCyclePlugin == true) {
      if (parent_->image_proc_state() == kStop) {
        std::string target_name = "";
        std::string prev_name = "";
        int current_plugin_setting_state =
            current_item_info->current_plugin_setting_state();
        if (current_plugin_setting_state == kBranchLineDummyPlugin) {
          std::vector<std::string> next_plugin_names =
              GetValidNextPluginNames(current_item_info);
          target_name = next_plugin_names[0];
        } else {
          target_name = current_item_info->plugin_name();
        }
        prev_name = current_item_info->GetPrevPluginName();

        // Set position then show popup dialog about cycle setting.
        unsigned int cycle =
            thread_running_cycle_manager_->GetCycle(prev_name, target_name);
        wxdlg_cycle_menu_->set_cycle(cycle);
        wxdlg_cycle_menu_->ShowModal();
        cycle = wxdlg_cycle_menu_->cycle();
        thread_running_cycle_manager_->AddCycle(prev_name, target_name, cycle);
      }
    }
  } else if (event.RightDown() == true) {
    DEBUG_PRINT("Right down \n");
    if (isTouchCurrentPlugin == true) {
      PluginBase *plugin;
      std::string plugin_name = "";

      plugin_name = current_item_info->plugin_name();
      if (plugin_name != "") {
        plugin = plugin_manager_->GetPlugin(plugin_name);
        plugin->OpenSettingWindow(parent_->image_proc_state());
      }
    }
  }

  // Refresh Edit window
  Refresh();
}

void PluginEditCanvas::OnMouseUp(wxMouseEvent &event) {
  int x = event.GetX();
  int y = event.GetY();

  DEBUG_PRINT("up: x=%d,y=%d\n", x, y);
}

/**
 * @brief
 * Check action type(add, branch delete or replace ),
 * then update Plugin flow.
 */
void PluginEditCanvas::UpdateCanvasPluginList(void) {
  DEBUG_PRINT("PluginEditCanvas::UpdateCanvasPluginList\n");

  CalcUnscrolledPosition(0, 0, &scroled_x_, &scroled_y_);

  FlowActionType action_type = kFlowActionInvalid;
  std::string selected_plugin_name = "";
  if (wxdlg_plugin_menu_ != NULL) {
    PluginMenuEventType event_type;
    event_type = wxdlg_plugin_menu_->event_type();
    switch (event_type) {
      case kPluginMenuEventReplacePressed:
        action_type = kFlowActionReplace;
        selected_plugin_name = wxdlg_plugin_menu_->GetPluginName();
        break;
      case kPluginMenuEventNone:
      default:
        break;
    }
    wxdlg_plugin_menu_->Reset();
  }
  if (wxdlg_plus_menu_ != NULL) {
    PlusMenuEventType event_type;
    event_type = wxdlg_plus_menu_->event_type();
    switch (event_type) {
      case kPlusMenuEventAddPressed:
        action_type = kFlowActionAddNext;
        selected_plugin_name = wxdlg_plus_menu_->GetPluginName();
        break;
      case kPlusMenuEventBranchPressed:
        action_type = kFlowActionAddBranch;
        selected_plugin_name = wxdlg_plus_menu_->GetPluginName();
        break;
      case kPlusMenuEventDeletePressed:
        action_type = kFlowActionDelete;
        break;
      case kPlusMenuEventNone:
      default:
        break;
    }
    wxdlg_plus_menu_->Reset();
  }

  if (action_type == kFlowActionInvalid) {
    DEBUG_PRINT("Not update plugin flow");
    return;
  }

  DEBUG_PRINT("touched Pos=(%d,%d)\n", touched_plugin_num_.x,
              touched_plugin_num_.y);

  // check add plugin type (add or branch or delete)
  PluginEditItemInfo *current_item_info =
      flow_plugin_list_[touched_plugin_num_.y][touched_plugin_num_.x];
  std::string target_name = GetTargetPluginName(selected_plugin_name);

  if (action_type == kFlowActionAddNext) {
    if (current_item_info->current_plugin_setting_state() == kSetBlankPlugin) {
      if (false == AddNewPluginToFlow(target_name, current_item_info)) {
        DEBUG_PRINT("[UpdateCanvasPluginList] Failed to add %s\n",
                    target_name.c_str());
        return;
      }
    } else if ((current_item_info->current_plugin_setting_state() ==
                kSetValidPlugin) ||
               (current_item_info->current_plugin_setting_state() ==
                kBranchPointDummyPlugin)) {
      if (false == InsertNewPluginToFlow(target_name, current_item_info)) {
        DEBUG_PRINT("[UpdateCanvasPluginList] Falied to insert %s to %s \n",
                    target_name.c_str(),
                    current_item_info->plugin_name().c_str());
        return;
      }
    } else {
      DEBUG_PRINT(
          "[UpdateCanvasPluginList] Failed to add %s"
          " due to invalid state = %d \n",
          target_name.c_str(),
          current_item_info->current_plugin_setting_state());
      return;
    }
  } else if (action_type == kFlowActionAddBranch) {
    if (flow_plugin_list_.size() >= kMaxColPluginNum) {
      DEBUG_PRINT(
          "[UpdateCanvasPluginList] Failed to add branch"
          " due to max branch size reached\n");

      return;
    }
    if (false == InsertNewBlanchPluginToFlow(target_name, current_item_info)) {
      DEBUG_PRINT(
          "[UpdateCanvasPluginList] Falied to insert %s"
          " to %s as branch\n",
          target_name.c_str(), current_item_info->plugin_name().c_str());
      return;
    }
  } else if (action_type == kFlowActionReplace) {
    if (touched_plugin_num_.y == 0 && touched_plugin_num_.x == 0) {
      if (flow_plugin_list_[touched_plugin_num_.y].size() > 1) {
        RemovePluginFlowList(touched_plugin_num_);
        if (false ==
            ReplaceRootPlugin(selected_plugin_name, current_item_info)) {
          DEBUG_PRINT(
              "[UpdateCanvasPluginList] Falied to replace"
              "from %s to %s\n",
              current_item_info->plugin_name().c_str(), target_name.c_str());

          return;
        }
      }
    } else if (false == ReplacePlugin(target_name, current_item_info)) {
      DEBUG_PRINT(
          "[UpdateCanvasPluginList] Falied to replace"
          "from %s to %s\n",
          current_item_info->plugin_name().c_str(), target_name.c_str());

      return;
    }
  } else if (action_type == kFlowActionDelete) {
    DEBUG_PRINT("Delete Plugin \n");
    if (flow_plugin_list_[touched_plugin_num_.y].size() > 1) {
      RemovePluginFlowList(touched_plugin_num_);

    } else {
      DEBUG_PRINT(
          "[UpdateCanvasPluginList] Failed to delete"
          " due to flow list size = 0\n");
    }
    // for debug only.
    this->EditCanvasDebugLog();

  } else {
    // Invalid root
    DEBUG_PRINT(
        "[UpdateCanvasPluginList] Failed to delete"
        " due to invalid actio type = %d\n",
        action_type);
  }

  // Update scrollbar state
  UpdateScrollbarState();
}

/**
 * @brief
 * The handler function for custom event(UPDATE_CANVAS).
 * Refresh of plugin edit canvas screen.
 * @param event [in] wxCommandEvent.
 */
void PluginEditCanvas::OnUpdateCanvas(wxCommandEvent &event) {
  DEBUG_PRINT("PluginEditCanvas::OnUpdateCanvas\n");
  Refresh();
}

/**
 * @brief
 * Posts custom event(UPDATE_CANVAS) to PluginEditCanvas.
 * Call this function if fource refresh is required.
 */
void PluginEditCanvas::PostUpdateCanvasEvent(void) {
  DEBUG_PRINT("PluginEditCanvas::PostUpdateCanvasEvent\n");

  wxCommandEvent event(UPDATE_CANVAS);

  event.SetString(wxT("This is the data"));

  wxPostEvent(this, event);
}

/**
 * @brief
 * Set image processing state.
 * @param state [in] Image processing state.
 */
void PluginEditCanvas::set_image_proc_state(
    ImageProcessingState image_proc_state) {
  // Set state flag
  for (int i = 0; i < flow_plugin_list_.size(); i++) {
    for (int j = 0; j < flow_plugin_list_[i].size(); j++) {
      flow_plugin_list_[i][j]->set_state(image_proc_state);
    }
  }
}

/**
 * @brief
 * Get PluginManager class pointer to management plugin flow.
 * And set root plugin on plugin edit canvas.
 * @param plugin_manager [in] PluginManager class pointer.
 */
void PluginEditCanvas::SetPluginManagerInfo(PluginManager *plugin_manager) {
  DEBUG_PRINT("PluginEditCanvas::SetPluginManagerInfo \n");
  plugin_manager_ = plugin_manager;

  // delete flow plugin list and plugin list.
  flow_plugin_list_.clear();
  parent_->RemoveAllPluginNameFromPluginList();

  // Set root plugin plugin edit canvas.
  PluginBase *root_plugin = plugin_manager_->root_plugin();
  if (root_plugin == NULL) {
    DEBUG_PRINT("[SetPluginManagerInfo]root_plugin == NULL\n");
  } else {
    SetRootFlow();
  }

  // for debug only.
  this->EditCanvasDebugLog();
  if (wxdlg_plus_menu_ == NULL) {
    DEBUG_PRINT("WxDlgPlusMenu instance is not create yet.");
  }
}

/**
 * @brief
 * Set root plugin on plugin edit canvas,
 * when root plugin is set to plugin_manager.
 */
void PluginEditCanvas::SetRootFlow(void) {
  PluginBase *root_plugin = plugin_manager_->root_plugin();

  PluginEditItemInfo *root_item_info = new PluginEditItemInfo();
  PluginEditItemInfo *empty_info = new PluginEditItemInfo();
  std::vector<PluginEditItemInfo *> main_flow_list;

  // root plugin info.
  root_item_info->set_position(wxPoint(0, 0));
  root_item_info->UpdateUnScrolledPosition(scroled_x_, scroled_y_);
  root_item_info->set_plugin_name(root_plugin->plugin_name());
  root_item_info->set_plugin_add_enable_state(kDisableToAddPrevPlugin);
  root_item_info->set_current_plugin_setting_state(kSetValidPlugin);
  root_item_info->next_item_info_.push_back(empty_info);

  // empty plugin info to be connected to the root plugin
  empty_info->set_position(wxPoint(1, 0));
  empty_info->set_current_plugin_setting_state(kSetBlankPlugin);
  empty_info->UpdateUnScrolledPosition(scroled_x_, scroled_y_);
  empty_info->prev_item_info_ = root_item_info;

  DEBUG_PRINT("[SetRootFlow] root plugin name =  %s\n",
              root_item_info->plugin_name().c_str());
  DEBUG_PRINT("[SetRootFlow] empty plugin name = %s\n",
              empty_info->GetPrevPluginName().c_str());

  // Set root plugin on plugin edit canvas.
  main_flow_list.push_back(root_item_info);
  main_flow_list.push_back(empty_info);
  flow_plugin_list_.push_back(main_flow_list);

  // Update plugin name list box
  parent_->AddPluginNameToPluginList(root_plugin->plugin_name());
}

/**
 * @brief
 * Clear the selected state of all of the plugin flow items.
 */
void PluginEditCanvas::Reset() {
  // Clear selected state
  for (int i = 0; i < flow_plugin_list_.size(); i++) {
    for (int j = 0; j < flow_plugin_list_[i].size(); j++) {
      flow_plugin_list_[i][j]->set_selected(false);
    }
  }

  // move scroll bar to default position
  Scroll(0, 0);
  UpdateScrollbarState();
}

/**
 * @brief
 * Remove plugin flow after del_pos.
 * @param del_pos [in] delete position.
 */
void PluginEditCanvas::RemovePluginFlowList(wxPoint del_pos) {
  wxPoint current_pos = flow_plugin_list_[del_pos.y][del_pos.x]->position();
  std::string plugin_name = "";

  // Delete all items (= Reload).
  if (del_pos.x == 0 && del_pos.y == 0) {
    IPlugin *current_iplugin;
    plugin_name = flow_plugin_list_[del_pos.y][del_pos.x]->plugin_name();
    if (plugin_name != "") {
      // Get root plugin
      current_iplugin = plugin_manager_->GetPlugin(plugin_name);

      // Remove all next plugin
      current_iplugin->RemoveNextPlugin();
    }

    flow_plugin_list_.clear();
    thread_running_cycle_manager_->DeleteAllCycle();
    parent_->RemoveAllPluginNameFromPluginList();

    // move scroll bar to default position
    Scroll(0, 0);
    UpdateScrollbarState();
    return;
  }

  /************* �폜�Ώۏ����􂢏o�� start *************/
  // �O�i�v���O�C���̐ڑ���Ԃ𓥂܂��A�폜�Ώۂ�PluginEditItemInfo���i�[����B
  PluginEditItemInfo *delete_point_info;

  // ����_��Dummy�v���O�C���폜�𔻒肷��t���O(���}�A���ʕ����̍폜����)
  // ���|�w�{�|�x�{���|
  //   �_  �{�|  �{���|
  bool need_to_delete_base_dummy = false;
  // �x�[�X���C�����폜�𔻒肷��t���O(���}�A���ʕ����̍폜����)
  // ���|�{�|�w�{���|�x
  //   �_�{�|  �{���|
  bool need_to_delete_all_next = false;
  // �폜��A�����ɋ�v���O�C���}���𔻒肷��t���O
  bool need_to_add_blank_item = false;

  if (flow_plugin_list_[del_pos.y][del_pos.x]->plugin_name().find("Dummy") !=
      wxNOT_FOUND) {
    // 1. ����_��ō폜������s���ꍇ
    // ����_���A�����邱�Ƃ͂Ȃ����߁APrev�͏�Ƀv���O�C���ł���(���}�A���ʕ����̍폜����)
    // ���|�w�{�|�x�{�|���|
    //          �_ �{�|���|
    // 1��O��delete point�ɐݒ肷��B
    delete_point_info =
        flow_plugin_list_[del_pos.y][del_pos.x]->prev_item_info_;

    // �����܂ߑS�č폜����
    need_to_delete_all_next = true;

    // ��i�Ƀv���O�C���ڑ��\�Ƃ��邽�߁A��v���O�C���𖖔��ɒǉ�����B
    need_to_add_blank_item = true;

    DEBUG_PRINT("[RemovePluginFlowList] Remove on dummy Plugin(1) \n");
  } else if (flow_plugin_list_[del_pos.y][del_pos.x]->plugin_name().find(
                 "Dummy") == wxNOT_FOUND) {
    // 2. �L���ȃv���O�C����ō폜������s���ꍇ
    //    ��O�ɐڑ����Ă���v���O�C��(����_/�ΐ�/�L���ȃv���O�C��)�ɂ��폜�Ώۂ��قȂ邽�߁A
    //    Prev��State���ɏ�����������B
    if (flow_plugin_list_[del_pos.y][del_pos.x]
            ->prev_item_info_->plugin_name()
            .find("Dummy") == wxNOT_FOUND) {
      // 2-1.
      // Prev�ɗL���ȃv���O�C�����ݒ肳��Ă���ꍇ(���}�A���ʕ����̍폜����)
      // ���|�w�{���x�{���|
      // 1��O��delete point�ɐݒ肷��B
      delete_point_info =
          flow_plugin_list_[del_pos.y][del_pos.x]->prev_item_info_;

      // ��i�Ƀv���O�C���ڑ��\�Ƃ��邽�߁A��v���O�C���𖖔��ɒǉ�����B
      need_to_add_blank_item = true;

      DEBUG_PRINT("[RemovePluginFlowList] Remove on dummy Plugin(2-1) \n");
    } else {
      // 2-2. Prev��Dummy�v���O�C�����ݒ肳��Ă���ꍇ
      //      Prev�̍폜�Ώۂ���肷�邽�߂ɁAPrev�̃v���O�C������
      //      Prev->Prev�̃x�[�X�v���O�C�����ƈ�v���邩�`�F�b�N����
      if (flow_plugin_list_[del_pos.y][del_pos.x]
              ->prev_item_info_->plugin_name() ==
          flow_plugin_list_[del_pos.y][del_pos.x]
              ->prev_item_info_->prev_item_info_->GetNextPluginName(0)) {
        if (flow_plugin_list_[del_pos.y][del_pos.x]
                ->prev_item_info_->prev_item_info_->next_item_info_.size() ==
            1) {
          // 2-2-1.
          // ���O�̃v���O�C���FDummy�v���O�C�����A�x�[�X���C���̃A�C�e�����폜
          //       ���̑���𕪊򂪂Ȃ���ԂŎ��{�B
          //       ->���̃P�[�X�͑��݂��Ȃ��B�w�ُ�n�x�B
          DEBUG_PRINT(
              "[RemovePluginFlowList] Remove on dummy Plugin(2-2-1) \n");
          return;  // invalid ���[�g�Ȃ̂ő�return
        } else {
          // 2-2-2. Prev��Dummy�v���O�C�����ݒ肳��Ă����ԂŁA
          //       �x�[�X���C���̗L���ȃv���O�C�����폜(���}�A���ʕ����̍폜����)
          // ���|�{�|�w�{���|�x
          //      �_�{�|�{���|
          // 1��O��delete point�ɐݒ肷��B
          delete_point_info =
              flow_plugin_list_[del_pos.y][del_pos.x]->prev_item_info_;
          // ��i�Ƀv���O�C���ڑ��\�Ƃ��邽�߁A��v���O�C���𖖔��ɒǉ�����B
          need_to_add_blank_item = true;
          DEBUG_PRINT(
              "[RemovePluginFlowList] Remove on dummy Plugin(2-2-2) \n");
        }
      } else {
        if (flow_plugin_list_[del_pos.y][del_pos.x]
                ->prev_item_info_->prev_item_info_->next_item_info_.size() ==
            2) {
          // 2-2-3. Prev��Dummy�v���O�C�����ݒ肳��Ă����ԂŁA
          //       �x�[�X���C���ȊO�̗L���ȃv���O�C�����폜�B
          //       ����ɁA�폜���邱�Ƃŕ��򂪖����Ȃ�ꍇ�B(���}�A���ʕ����̍폜����)
          // ���|�{�|�{���|
          //      �_�{�|�w �{���|�x
          // Prev��Dummy�v���O�C���܂ߍ폜����K�v�����邽�߁A2��O��delete
          // point�ɐݒ肷��B
          delete_point_info = flow_plugin_list_[del_pos.y][del_pos.x]
                                  ->prev_item_info_->prev_item_info_;

          // ���򂪖����Ȃ邽�߁A����_���폜����
          need_to_delete_base_dummy = true;
          DEBUG_PRINT(
              "[RemovePluginFlowList] Remove on dummy Plugin(2-2-3) \n");
        } else {
          // 2-2-3. Prev��Dummy�v���O�C�����ݒ肳��Ă����ԂŁA
          //       �x�[�X���C���ȊO�̗L���ȃv���O�C�����폜�B
          //       ����ɁA�폜���邱�Ƃŕ��򂪂܂��c��ꍇ�B(���}�A���ʕ����̍폜����)
          // ���|�{�|�{���|
          //      �_�{�|�w �{���|�x
          //      �_�{�|�{���|
          // Prev��Dummy�v���O�C���܂ߍ폜����K�v�����邽�߁A2��O��delete
          // point�ɐݒ肷��B
          delete_point_info = flow_plugin_list_[del_pos.y][del_pos.x]
                                  ->prev_item_info_->prev_item_info_;

          DEBUG_PRINT("[RemovePluginFlowList] Remove on dummy Plugin(6) \n");
        }
      }
    }
  }
  /************* �폜�Ώۏ����􂢏o�� end *************/

  // �폜�����O�ɁA�폜�Ώۂ���A�Ȃ�v���O�C����column�����擾����
  int col_cnt = delete_point_info->next_item_info_[0]->GetBranchHeight();

  // Prev�v���O�C�����̎擾
  // delete point��Dummy�̏ꍇ��valid�Ȗ��O�������B
  // ����ȊO�́Adelete point�̃v���O�C������Prev�ɐݒ�
  std::string prev_plugin_name = "";
  PluginEditItemInfo *prev_info;
  if (delete_point_info->plugin_name().find("Dummy") == wxNOT_FOUND) {
    prev_plugin_name = delete_point_info->plugin_name();
    prev_info = delete_point_info;
  } else {
    prev_plugin_name = GetValidPrevPluginName(delete_point_info);
    prev_info = GetValidPrevItemInfo(delete_point_info);
  }

  if (need_to_delete_all_next == true) {
    // delete point�ɘA�Ȃ�v���O�C����S�č폜
    DEBUG_PRINT("[RemovePluginFlowList] delete all \n");
    // PluginManager�փv���O�C���ؒf��ʒm
    plugin_manager_->DisconnectAllPlugin(prev_plugin_name);
  } else {
    // delete point�ɘA�Ȃ�v���O�C���̂����A����̃A�C�e���݂̂��폜
    DEBUG_PRINT("[RemovePluginFlowList] delete specified \n");

    if ((flow_plugin_list_[del_pos.y][del_pos.x]
             ->current_plugin_setting_state() == kBranchPointDummyPlugin) ||
        (flow_plugin_list_[del_pos.y][del_pos.x]
             ->current_plugin_setting_state() == kBranchLineDummyPlugin)) {
      // Dummy�v���O�C����ō폜���s���ꍇ�A�폜�Ώۈȍ~�̗L���ȃv���O�C�����̂�
      // PluginManager�փv���O�C���ؒf��ʒm����B
      std::vector<std::string> next_plugin_names =
          GetValidNextPluginNames(delete_point_info);
      for (int i = 0; i < next_plugin_names.size(); i++) {
        // Prev�v���O�C��������Cycle�����폜
        thread_running_cycle_manager_->DeleteCycle(prev_plugin_name);

        if (IsIncludePlugin(next_plugin_names[i], prev_info) == true) {
          DEBUG_PRINT("[RemovePluginFlowList] delete specified : %s \n",
                      next_plugin_names[i].c_str());

          // PluginManager�փv���O�C���ؒf��ʒm
          plugin_manager_->DisconnectPlugin(prev_plugin_name,
                                            next_plugin_names[i]);
        }
      }
    } else {
      // Prev�v���O�C��������Cycle�����폜
      thread_running_cycle_manager_->DeleteCycle(
          prev_plugin_name,
          flow_plugin_list_[del_pos.y][del_pos.x]->plugin_name());

      DEBUG_PRINT(
          "[RemovePluginFlowList] delete specified : %s \n",
          flow_plugin_list_[del_pos.y][del_pos.x]->plugin_name().c_str());
      plugin_manager_->DisconnectPlugin(
          prev_plugin_name,
          flow_plugin_list_[del_pos.y][del_pos.x]->plugin_name());
    }
  }

  // �t���[/�t���[���X�g/next���폜����
  for (int num_next = 0; num_next < delete_point_info->next_item_info_.size();
       num_next++) {
    if (need_to_delete_all_next == true) {
      // next��S�č폜����ꍇ
      DEBUG_PRINT(
          "[RemovePluginFlowList] Before erase size: next_item_info.size() = "
          "%d \n",
          delete_point_info->next_item_info_.size());
      DEBUG_PRINT(
          "[RemovePluginFlowList] Remove name = %s\n",
          delete_point_info->next_item_info_[num_next]->plugin_name().c_str());
      RemoveFlowAndFlowList(delete_point_info->next_item_info_[num_next]);
      delete_point_info->next_item_info_.erase(
          delete_point_info->next_item_info_.begin() + num_next);
      DEBUG_PRINT(
          "[RemovePluginFlowList] After erase size: next_item_info.size() = %d "
          "\n",
          delete_point_info->next_item_info_.size());
      num_next--;
    } else {
      // �����next�݂̂��폜����ꍇ
      if (IsIncludePlugin(
              flow_plugin_list_[del_pos.y][del_pos.x]->plugin_name(),
              delete_point_info->next_item_info_[num_next]) == true) {
        DEBUG_PRINT(
            "[RemovePluginFlowList] Before erase size: next_item_info.size() = "
            "%d \n",
            delete_point_info->next_item_info_.size());
        DEBUG_PRINT("[RemovePluginFlowList] Remove name = %s\n",
                    delete_point_info->next_item_info_[num_next]
                        ->plugin_name()
                        .c_str());
        RemoveFlowAndFlowList(delete_point_info->next_item_info_[num_next]);
        delete_point_info->next_item_info_.erase(
            delete_point_info->next_item_info_.begin() + num_next);
        DEBUG_PRINT(
            "[RemovePluginFlowList] After erase size: next_item_info.size() = "
            "%d \n",
            delete_point_info->next_item_info_.size());
        break;
      }
    }
  }

  /************* Shift���� start *************/
  wxPoint target_pos;
  // X�����AShift����
  if (need_to_delete_base_dummy == true) {
    // ����_�폜�̔���������ꍇ�̂݁A�ȍ~�̏������s���B
    DEBUG_PRINT("[RemovePluginFlowList]  Remove Branch point\n");

    std::vector<PluginEditItemInfo *> temp_next_item_info;
    // 1.����_��next�����ꎞ�̈�ɑޔ�����
    temp_next_item_info = flow_plugin_list_[delete_point_info->position().y]
                                           [delete_point_info->position().x + 1]
                                               ->next_item_info_;

    // 2.����_�폜�ɔ����A����_�̏��𕪊�_�Ɍq����v���O�C�����ōX�V����B
    flow_plugin_list_[delete_point_info->position().y]
                     [delete_point_info->position().x + 1] =
                         flow_plugin_list_[delete_point_info->position().y]
                                          [delete_point_info->position().x + 2];

    // 3.Prev/Next���Đݒ肷��
    flow_plugin_list_[delete_point_info->position().y]
                     [delete_point_info->position().x + 1]
                         ->prev_item_info_ = delete_point_info;
    delete_point_info->next_item_info_ = temp_next_item_info;
    for (int num_next = 1; num_next < delete_point_info->next_item_info_.size();
         num_next++) {
      delete_point_info->next_item_info_[num_next]->prev_item_info_ =
          delete_point_info;
    }

    // 4. flow_plugin_list���番��_���폜����
    flow_plugin_list_[delete_point_info->position().y].erase(
        flow_plugin_list_[delete_point_info->position().y].begin() +
        delete_point_info->position().x + 2);

    // 5. flow_plugin_list�폜�ɔ����A�ȍ~�̃v���O�C���\���ʒu��X������-1����B
    for (int num_next = 0; num_next < delete_point_info->next_item_info_.size();
         num_next++) {
      delete_point_info->next_item_info_[num_next]->ShiftPositionAllNext(-1, 0);
    }

    // 6.
    // flow_plugin_list�폜�ɔ����AY�����̓��ʒu�ɐݒ肵�Ă����v���O�C�����폜����B
    DEBUG_PRINT("[RemovePluginFlowList]col_cnt = %d \n ", col_cnt);
    DEBUG_PRINT("[RemovePluginFlowList]delpos_y = %d \n ",
                delete_point_info->position().y);
    // 7. value�������Ax��erase����B
    for (int y_cnt = delete_point_info->position().y + 1; y_cnt <= col_cnt;
         y_cnt++) {
      DEBUG_PRINT("Shift -1 x position [%d] \n", y_cnt);
      flow_plugin_list_[y_cnt].erase(flow_plugin_list_[y_cnt].begin() +
                                     delete_point_info->position().x + 1);
    }
  }

  // Y�����A Shift��������1
  for (int i = 0; i < flow_plugin_list_.size(); i++) {
    bool find_active_item = false;
    for (int j = 0; j < flow_plugin_list_[i].size(); j++) {
      if (flow_plugin_list_[i][j]->plugin_name() != "") {
        find_active_item = true;
      }
    }
    if (find_active_item == false) {
      DEBUG_PRINT("Shift -1 y line \n");
      flow_plugin_list_.erase(flow_plugin_list_.begin() + i);

      for (int k = i; k < flow_plugin_list_.size(); k++) {
        for (int l = 0; l < flow_plugin_list_[k].size(); l++) {
          flow_plugin_list_[k][l]->ShiftPosition(0, -1);
        }
      }
      i--;
    }
  }

  DEBUG_PRINT("[RemovePluginFlowList] End Shift\n");
  /************* Shift���� end   *************/
  if (need_to_add_blank_item == true) {
    // Add blank EditItemInfo to end of the plugin flow.
    PluginEditItemInfo *next_item_info = new PluginEditItemInfo();
    wxPoint position;
    position.x = del_pos.x;
    position.y = del_pos.y;
    next_item_info->set_position(position);
    next_item_info->UpdateUnScrolledPosition(scroled_x_, scroled_y_);
    next_item_info->set_current_plugin_setting_state(kSetBlankPlugin);
    next_item_info->prev_item_info_ = delete_point_info;
    flow_plugin_list_[position.y].push_back(next_item_info);

    // �O�i�v���O�C����next����update
    delete_point_info->next_item_info_.push_back(next_item_info);
  }
}

/**
 * @brief
 * Get display strings according to
 *  the action type(add, branch delete or replace).
 * @param type [in] action type.
 * @return diplay strings.
 */
std::vector<std::string> PluginEditCanvas::GetDispString(FlowActionType type) {
  std::vector<std::string> disp_strings;
  std::string prev_plugin_name = "";
  std::string next_plugin_name = "";
  std::vector<std::string> next_plugin_names;
  PluginEditItemInfo *target_item_info;

  target_item_info =
      flow_plugin_list_[touched_plugin_num_.y][touched_plugin_num_.x];

  prev_plugin_name = GetValidPrevPluginName(target_item_info);
  DEBUG_PRINT("[GetDispString] Prev name = %s\n", prev_plugin_name.c_str());

  // check action type
  if (type == kFlowActionAddNext) {
    next_plugin_name = target_item_info->plugin_name();
    if (next_plugin_name.find("Dummy") == wxNOT_FOUND) {
      next_plugin_names.push_back(next_plugin_name);
      DEBUG_PRINT("[GetDispString] Next name = %s\n", next_plugin_name.c_str());
    } else {
      next_plugin_names =
          GetValidNextPluginNames(target_item_info->prev_item_info_);
    }
  } else if (type == kFlowActionAddBranch) {
    next_plugin_names.push_back(next_plugin_name);
  } else if (type == kFlowActionReplace) {
    if (touched_plugin_num_.y == 0 && touched_plugin_num_.x == 0) {
      disp_strings = plugin_manager_->GetPluginNames(kInputPlugin);
      return disp_strings;
    } else {
      next_plugin_name = GetValidBaseNextPluginName(target_item_info);
      next_plugin_names = GetValidNextPluginNames(target_item_info);
    }
  } else {
    DEBUG_PRINT("GetDispString invalid type = %d, return blank strings\n",
                type);
    return disp_strings;
  }

  // Get connectable plugin names.
  if (next_plugin_names.size() <= 1) {
    DEBUG_PRINT("[PluginEditCanvas] %d one next plugins\n",
                next_plugin_names.size());
    disp_strings = plugin_manager_->GetConnectablePluginNames(prev_plugin_name,
                                                              next_plugin_name);

  } else {
    disp_strings = plugin_manager_->GetConnectablePluginNames(
        prev_plugin_name, next_plugin_names);
  }

  if (type == kFlowActionReplace) {
    // Get rid of the same name of old name.
    PluginBase *plugin;
    plugin = plugin_manager_->GetPlugin(target_item_info->plugin_name());

    std::string old_target_plugin_name = plugin->plugin_name();

    if (plugin->is_cloned() == true) {
      std::string suffix = ".";
      int size = old_target_plugin_name.rfind(suffix);
      old_target_plugin_name.resize(size);
    }

    for (int i = 0; i < disp_strings.size(); i++) {
      if (disp_strings[i] == old_target_plugin_name) {
        disp_strings.erase(disp_strings.begin() + i);
        DEBUG_PRINT("[GetDispString] Replase remove = %s from list\n",
                    old_target_plugin_name.c_str());
        break;
      }
    }
  }

  return disp_strings;
}

/**
 * @brief
 * Update scroll bar state.
 *  - Enable horizontal scrollbars
 *      when x-length of plugin flow over the screen width.
 *  - Enable vertical scrollbars
 *      when y-length of plugin flow over the screen height.
 */
void PluginEditCanvas::UpdateScrollbarState(void) {
  int max_plugin_width = 1;
  int max_plugin_height = 1;
  bool enable_x_scrolling;
  bool enable_y_scrolling;
  int no_units_x;
  int no_units_y;

  if (flow_plugin_list_.size() == 0) {
    enable_x_scrolling = false;
    no_units_x = 0;
    enable_y_scrolling = false;
    no_units_y = 0;
  } else {
    for (int i = 0; i < flow_plugin_list_.size(); i++) {
      if (max_plugin_width < flow_plugin_list_[i].size()) {
        max_plugin_width = flow_plugin_list_[i].size();
      }
    }

    max_plugin_width = max_plugin_width * kOnePluginPaneWidth + 50;
    max_plugin_height = flow_plugin_list_.size() * kOnePluginPaneHeight;

    if (max_plugin_width > kEditCanvasScreenWidth) {
      enable_x_scrolling = true;
      no_units_x = max_plugin_width;
    } else {
      enable_x_scrolling = false;
      no_units_x = 0;
    }
    if (max_plugin_height > kEditCanvasScreenHeight) {
      enable_y_scrolling = true;
      no_units_y = max_plugin_height;
    } else {
      enable_y_scrolling = false;
      no_units_y = 0;
    }
  }

  SetScrollbars(1, 1, no_units_x, no_units_y, 0, 0);
  EnableScrolling(enable_x_scrolling, enable_y_scrolling);
}

/**
 * @brief
 * Get target plugin name.
 * if the same name of the plugin is already added,
 * create a clone plugin then return a different name.
 * This function should be used only for the purpose
 * to get the plugin name to be added(insert or replace).
 * @param src_target_name [in] plugin name.
 * @return name of target plugin name.
 */
std::string PluginEditCanvas::GetTargetPluginName(std::string src_target_name) {
  DEBUG_PRINT("[GetTargetPluginName]target_name = %s \n",
              src_target_name.c_str());
  std::string target_name = src_target_name;
  if (plugin_manager_) {
    IPlugin *target_plugin = plugin_manager_->GetPlugin(target_name);
    // Check if clone needed
    if (plugin_manager_->IsExistPluginForFlow(
            reinterpret_cast<PluginBase *>(target_plugin))) {
      if (target_plugin->plugin_type() != kInputPlugin) {
        target_plugin = plugin_manager_->ClonePlugin(target_plugin);
        if (target_plugin == NULL) {
          wxString plugin_name(target_name.c_str(), wxConvUTF8);
          wxString message;
          message = message.Format(
              wxT("Failed to clone plugin %s.\nPlease see the log window."),
              wxString::FromUTF8(target_name.c_str()).c_str());
          wxMessageDialog dialog(NULL, message, wxT("Message"),
                                 wxOK | wxSTAY_ON_TOP, wxPoint(100, 100));
          dialog.ShowModal();
          return "";
        }
        target_name = target_plugin->plugin_name();
      }
    }
  }
  return target_name;
}

/**
 * @brief
 * Get the name of a valid previous plugin
 * that are connected to the flow on the bases of the target_Item.
 * @param target_Item [in] target PluginEditItemInfo.
 * @return the name of a valid previous plugin.
 */
std::string PluginEditCanvas::GetValidPrevPluginName(
    PluginEditItemInfo *target_Item) {
  std::string temp_plugin_name = "";

  temp_plugin_name = target_Item->GetPrevPluginName();
  if (temp_plugin_name.find("Dummy") == wxNOT_FOUND) {
    DEBUG_PRINT("[GetValidPrevPluginName] Prev name = %s\n",
                temp_plugin_name.c_str());
    return temp_plugin_name;
  } else if (target_Item->prev_item_info_ == NULL) {
    return temp_plugin_name;
  } else {
    return GetValidPrevPluginName(target_Item->prev_item_info_);
  }
}

/**
 * @brief
 * Get the names of a valid next plugin
 * that are connected to the flow on the bases of the target_Item.
 * @param target_Item [in] target PluginEditItemInfo.
 * @return the names of a next previous plugin.
 */
std::vector<std::string> PluginEditCanvas::GetValidNextPluginNames(
    PluginEditItemInfo *target_Item) {
  std::vector<std::string> next_plugin_names;
  std::vector<std::string> temp_plugin_names;
  std::string temp_plugin_name = "";

  DEBUG_PRINT("[GetValidNextPluginNames] Next size = %d\n",
              target_Item->next_item_info_.size());

  for (unsigned int i = 0; i < target_Item->next_item_info_.size(); i++) {
    temp_plugin_name = target_Item->GetNextPluginName(i);

    if ((temp_plugin_name.find("Dummy") == wxNOT_FOUND) &&
        (temp_plugin_name != "")) {
      next_plugin_names.push_back(temp_plugin_name);
      DEBUG_PRINT("[PluginEditCanvas] Next name(%d) = %s\n", i,
                  temp_plugin_name.c_str());
      DEBUG_PRINT("[PluginEditCanvas] Not Dummy\n");
    } else {
      DEBUG_PRINT("[PluginEditCanvas] Dummy\n");
      temp_plugin_names =
          GetValidNextPluginNames(target_Item->next_item_info_[i]);
      for (int j = 0; j < temp_plugin_names.size(); j++) {
        next_plugin_names.push_back(temp_plugin_names[j]);
      }
    }
  }
  return next_plugin_names;
}

/**
 * @brief
 * Get the name of a valid base next plugin
 * that are connected to the flow on the bases of the target_Item.
 * @param target_Item [in] target PluginEditItemInfo.
 * @return the name of a valid base next plugin.
 */
std::string PluginEditCanvas::GetValidBaseNextPluginName(
    PluginEditItemInfo *target_Item) {
  std::string base_next_plugin_name = "";

  if (target_Item->next_item_info_.size() == 0) {
    return base_next_plugin_name;
  }
  DEBUG_PRINT("[GetValidBaseNextPluginName] Next size = %d\n",
              target_Item->next_item_info_.size());

  base_next_plugin_name = target_Item->GetNextPluginName(0);
  if (base_next_plugin_name.find("Dummy") != wxNOT_FOUND) {
    base_next_plugin_name =
        GetValidBaseNextPluginName(target_Item->next_item_info_[0]);
  }

  return base_next_plugin_name;
}

/**
 * @brief
 * Get the PluginEditItemInfo of a valid previous plugin
 * that are connected to the flow on the bases of the target_Item.
 * @param target_Item [in] target PluginEditItemInfo.
 * @return the PluginEditItemInfo of a valid previous plugin.
 */
PluginEditItemInfo *PluginEditCanvas::GetValidPrevItemInfo(
    PluginEditItemInfo *target_Item) {
  if (target_Item->GetPrevPluginName().find("Dummy") == wxNOT_FOUND) {
    return target_Item->prev_item_info_;
  } else {
    return GetValidPrevItemInfo(target_Item->prev_item_info_);
  }
}

/**
 * @brief
 * Remove Flow and Flow list
 * that are connected to the target_Item and target_Item itself.
 * @param target_Item [in] target PluginEditItemInfo.
 */
void PluginEditCanvas::RemoveFlowAndFlowList(PluginEditItemInfo *target_Item) {
  std::string plugin_name = "";

  plugin_name = target_Item->plugin_name();
  // Release clone plugin and plugin name list box
  if ((plugin_name != "") || (plugin_name.find("Dummy") == wxNOT_FOUND)) {
    PluginBase *plugin = plugin_manager_->GetPlugin(plugin_name);
    if (plugin && plugin->is_cloned()) {
      plugin_manager_->ReleasePlugin(plugin);
    }
    parent_->RemovePluginNameFromPluginList(plugin_name);
    DEBUG_PRINT("[RemoveFlowAndFlowList] Remove FlowList = %s\n",
                plugin_name.c_str());
  }

  // if next size is not zero, clear next items.
  for (unsigned int i = 0; i < target_Item->next_item_info_.size(); i++) {
    RemoveFlowAndFlowList(target_Item->next_item_info_[i]);
  }

  // remove itself from flow plugin list
  DEBUG_PRINT("[RemoveFlowAndFlowList] remove (%d, %d) \n",
              target_Item->position().x, target_Item->position().y);
  flow_plugin_list_[target_Item->position().y].erase(
      flow_plugin_list_[target_Item->position().y].begin() +
      target_Item->position().x);
}

/**
 * @brief
 * Check the target_Item and next_item of target_Item include plugin named
 * plugin_name.
 * @param target_Item [in] target PluginEditItemInfo.
 * @return true  = target_Item or next_item of target_Item
 *                 include plugin named plugin_name.
 *         false = target_Item or next_item of target_Item
 *                 does not include plugin named plugin_name.
 */
bool PluginEditCanvas::IsIncludePlugin(std::string plugin_name,
                                       PluginEditItemInfo *target_Item) {
  bool ret = false;

  if (target_Item->plugin_name() == plugin_name) {
    return true;
  }

  if (target_Item->next_item_info_.size() > 0) {
    for (int i = 0; i < target_Item->next_item_info_.size(); i++) {
      ret = IsIncludePlugin(plugin_name, target_Item->next_item_info_[i]);
      if (ret == true) {
        break;
      }
    }
  }

  return ret;
}

/**
 * @brief
 * Save Plugin flow and settings to ".flow" file.
 * @param file_path [in] file path.
 * @return true  = succeeded to save file.
 *         false = failed to save file.
 */
bool PluginEditCanvas::SavePluginFlowToFile(wxString file_path) {
  bool ret = false;
  wxTextFile text_file;
  std::vector<wxString> all_plugin_settings;
  std::vector<wxString> all_cycle_settings;
  all_plugin_settings.push_back(wxT(kFlowSettingString));
  all_cycle_settings.push_back(wxT(kFlowCycleString));

  DEBUG_PRINT("PluginEditCanvas::SavePluginFlowToFile\n");

  ret = text_file.Open(file_path);
  if (ret == false) {
    ret = text_file.Create(file_path);
    if (ret == false) {
      DEBUG_PRINT("Fail to create file = %s \n",
                  (const char *)file_path.mb_str());
      return false;
    }
  }

  text_file.Clear();

  // Save plugin relation and plugin's settings and cycle.
  for (int i = 0; i < flow_plugin_list_.size(); i++) {
    if (i == 0) {
      text_file.AddLine(wxT(kMainFlowString));
    } else {
      text_file.AddLine(wxT(kSubFlowString));
    }

    bool is_set_prev_info = false;
    for (int j = 0; j < flow_plugin_list_[i].size(); j++) {
      PluginEditItemInfo *current_plugin_info = flow_plugin_list_[i][j];
      std::string current_plugin_name = flow_plugin_list_[i][j]->plugin_name();
      if (current_plugin_name == "") {
        // Do nothing
      } else if (current_plugin_name.find("Dummy") == wxNOT_FOUND) {
        // for sub flow
        if (i > 0 && is_set_prev_info == false) {
          // The first item of the subflow,
          // set following data
          // [branch source plugin, clone flag, branch plugin, clone flag]
          std::string prev_plugin_name =
              GetValidPrevPluginName(current_plugin_info);
          PluginBase *plugin;
          int is_prev_cloned, is_current_cloned;

          plugin = plugin_manager_->GetPlugin(prev_plugin_name);
          if (plugin->is_cloned() == true) {
            is_prev_cloned = 1;
          } else {
            is_prev_cloned = 0;
          }

          plugin = plugin_manager_->GetPlugin(current_plugin_name);
          if (plugin->is_cloned() == true) {
            is_current_cloned = 1;
          } else {
            is_current_cloned = 0;
          }

          wxString wx_prev_plugin_name(prev_plugin_name.c_str(), wxConvUTF8);
          wxString wx_current_plugin_name(current_plugin_name.c_str(),
                                          wxConvUTF8);

          text_file.AddLine(wx_prev_plugin_name +
                            wxString::Format(wxT(",%d,"), is_prev_cloned) +
                            wx_current_plugin_name +
                            wxString::Format(wxT(",%d,"), is_current_cloned));
          is_set_prev_info = true;
        }

        // set following data
        // [current plugin, clone flag, next plugin, clone flag]
        PluginBase *current_plugin, *next_plugin;
        int is_current_cloned, is_next_cloned;

        current_plugin = plugin_manager_->GetPlugin(current_plugin_name);
        if (current_plugin->is_cloned() == true) {
          is_current_cloned = 1;
        } else {
          is_current_cloned = 0;
        }

        std::string next_plugin_name =
            GetValidBaseNextPluginName(current_plugin_info);
        if ((next_plugin_name != "") &&
            (next_plugin_name.find("Dummy") == wxNOT_FOUND)) {
          next_plugin = plugin_manager_->GetPlugin(next_plugin_name);
          if (next_plugin->is_cloned() == true) {
            is_next_cloned = 1;
          } else {
            is_next_cloned = 0;
          }
        } else {
          is_next_cloned = 0;
        }

        wxString wx_current_plugin_name(current_plugin_name.c_str(),
                                        wxConvUTF8);
        wxString wx_next_plugin_name(next_plugin_name.c_str(), wxConvUTF8);
        text_file.AddLine(wx_current_plugin_name +
                          wxString::Format(wxT(",%d,"), is_current_cloned) +
                          wx_next_plugin_name +
                          wxString::Format(wxT(",%d,"), is_next_cloned));

        // Save plugin settings.
        if (current_plugin->GetPluginSettings().size() > 0) {
          all_plugin_settings.push_back(wxT("name=") + wx_current_plugin_name);
          std::vector<wxString> temp_settings;
          temp_settings = current_plugin->GetPluginSettings();
          for (int param_count = 0; param_count < temp_settings.size();
               param_count++) {
            all_plugin_settings.push_back(temp_settings[param_count]);
          }
        }
      } else {
        // Save cycle settings.
        if (current_plugin_info->current_plugin_setting_state() ==
            kBranchLineDummyPlugin) {
          if (thread_running_cycle_manager_) {
            int cycle_val;
            wxString temp_string;
            std::string prev_plugin_name =
                GetValidPrevPluginName(current_plugin_info);

            std::string next_plugin_name =
                GetValidBaseNextPluginName(current_plugin_info);

            cycle_val = thread_running_cycle_manager_->GetCycle(
                prev_plugin_name, next_plugin_name);

            wxString wx_src_plugin_name(prev_plugin_name.c_str(), wxConvUTF8);
            wxString wx_dst_plugin_name(next_plugin_name.c_str(), wxConvUTF8);

            temp_string = wx_src_plugin_name + wxT(",") + wx_dst_plugin_name +
                          wxString::Format(wxT(",%d,"), cycle_val);

            all_cycle_settings.push_back(temp_string);
          }
        }
      }
    }
    is_set_prev_info = false;
  }

  // Add a all plugin settings.
  for (int param_count = 0; param_count < all_plugin_settings.size();
       param_count++) {
    text_file.AddLine(all_plugin_settings[param_count]);
  }

  // Add a all cycle settings.
  for (int param_count = 0; param_count < all_cycle_settings.size();
       param_count++) {
    text_file.AddLine(all_cycle_settings[param_count]);
  }

  text_file.Write();
  text_file.Close();

  return true;
}

/**
 * @brief
 * Load Plugin flow and settings from ".flow" file.
 * @param file_path [in] file path.
 * @return true  = succeeded to save file.
 *         false = failed to save file.
 */
bool PluginEditCanvas::LoadPluginFlowFromFile(wxString file_path) {
  bool ret = false;
  wxTextFile text_file;
  std::vector<LoadPluginFlowInfo> load_item_list;
  std::string suffix = ".";

  // Before loading, delete all of the plugin info.
  IPlugin *root_iplugin;
  root_iplugin = plugin_manager_->root_plugin();
  root_iplugin->RemoveNextPlugin();
  plugin_manager_->ReleaseAllClonePlugin();
  flow_plugin_list_.clear();
  if (thread_running_cycle_manager_ != NULL) {
    thread_running_cycle_manager_->DeleteAllCycle();
  }
  parent_->RemoveAllPluginNameFromPluginList();

  DEBUG_PRINT("PluginEditCanvas::LoadPluginFlowFromFile\n");

  ret = text_file.Open(file_path);
  if (ret == false) {
    DEBUG_PRINT("Could not open file =%s\n", (const char *)file_path.mb_str());
    return false;
  }

  ret = text_file.Eof();
  if (ret == true) {
    DEBUG_PRINT(
        "[LoadPluginFlowFromFile]Failed to load flow"
        " due to .flow file is empty \n");
    text_file.Close();
    return false;
  }

  wxString line_str, token_str;
  wxStringTokenizer tokenizer;

  // Search main flow
  for (line_str = text_file.GetFirstLine(); !text_file.Eof();
       line_str = text_file.GetNextLine()) {
    if (line_str.find(wxT(kMainFlowString)) != wxNOT_FOUND) {
      DEBUG_PRINT("Find [main_flow] \n");
      break;
    }
  }

  if (text_file.Eof() == true) {
    text_file.Close();
    DEBUG_PRINT(
        "[LoadPluginFlowFromFile]Failed to load flow"
        " due to invalid format(does not include [main_flow])\n");
    return false;
  }

  // Get main flow info and create flow.
  int flow_count = 0;
  for (line_str = text_file.GetNextLine(); !text_file.Eof();
       line_str = text_file.GetNextLine(), flow_count++) {
    std::string current_plugin_name, next_plugin_name;
    unsigned long current_clone_flag, next_clone_flag; /* NOLINT */

    tokenizer.SetString(line_str, wxT(","), wxTOKEN_DEFAULT);
    if (tokenizer.CountTokens() != 4) {
      if (flow_count == 0) {
        DEBUG_PRINT(
            "[LoadPluginFlowFromFile]Failed to load flow"
            " due to invalid format(does not include main flow)\n");
        text_file.Close();
        return false;
      } else {
        // End of reading main flow.
        // Start reading sub flow.
        DEBUG_PRINT(
            "[LoadPluginFlowFromFile]Success to load main flow"
            " cound = %d\n",
            flow_count);
        break;
      }
    }

    // Get current plugin name and clone flag.
    token_str = tokenizer.GetNextToken();
    current_plugin_name = std::string(token_str.mb_str());
    token_str = tokenizer.GetNextToken();
    token_str.ToULong(&current_clone_flag);

    // Save plugin name in the file.
    LoadPluginFlowInfo item;
    item.file_plugin_name = current_plugin_name;
    load_item_list.push_back(item);

    if (current_clone_flag == 1) {
      int size = current_plugin_name.rfind(suffix);
      current_plugin_name.resize(size);
    }
    DEBUG_PRINT("current_plugin_name = %s, clone = %d \n",
                current_plugin_name.c_str(), current_clone_flag);

    // Get next plugin name and clone flag.
    token_str = tokenizer.GetNextToken();
    next_plugin_name = std::string(token_str.mb_str());
    token_str = tokenizer.GetNextToken();
    token_str.ToULong(&next_clone_flag);

    if (next_clone_flag == 1) {
      int size = next_plugin_name.rfind(suffix);
      next_plugin_name.resize(size);
    }
    DEBUG_PRINT("next_plugin_name = %s, clone = %d \n",
                next_plugin_name.c_str(), next_clone_flag);

    // Create plugin flow.
    if (flow_count == 0) {
      plugin_manager_->set_root_plugin(current_plugin_name);
      SetRootFlow();
    } else {
      std::string target_name = GetTargetPluginName(current_plugin_name);
      AddNewPluginToFlow(target_name, flow_plugin_list_[0][flow_count]);
    }

    // Save PluginEditCanvas class pointer.
    load_item_list[load_item_list.size() - 1].item_info =
        flow_plugin_list_[0][flow_count];
  }

  // Get sub flow info and create flow.
  bool is_end_sub_flow = false;
  do {
    for (; !text_file.Eof(); line_str = text_file.GetNextLine()) {
      if (line_str.find(wxT(kSubFlowString)) != wxNOT_FOUND) {
        DEBUG_PRINT("Find [sub_flow] \n");
        break;
      }
      if (line_str.find(wxT(kFlowSettingString)) != wxNOT_FOUND) {
        DEBUG_PRINT("Find [settings] \n");
        is_end_sub_flow = true;
        break;
      }
    }

    if (is_end_sub_flow == true) {
      DEBUG_PRINT("Exit while(1) \n");
      break;
    }

    if (text_file.Eof() == true) {
      DEBUG_PRINT("[LoadPluginFlowFromFile]Load only [main_flow])\n");
      text_file.Close();
      UpdateScrollbarState();
      PostUpdateCanvasEvent();
      return true;
    }

    // sub flow���`�F�b�N&�v���O�C���\�z
    // sub flow���X�g�擪���́A�v���O�C���ڑ��ɒ��ڎg�p���Ȃ����߁A
    // �J�E���^�����l��-1�ɂ��Ă����B
    flow_count = -1;
    PluginEditItemInfo *base_item = NULL;
    int branch_x, branch_y;
    for (line_str = text_file.GetNextLine(); !text_file.Eof();
         line_str = text_file.GetNextLine(), flow_count++) {
      std::string file_plugin_name, current_plugin_name, next_plugin_name;
      unsigned long current_clone_flag, next_clone_flag; /* NOLINT */

      tokenizer.SetString(line_str, wxT(","), wxTOKEN_DEFAULT);
      if (tokenizer.CountTokens() != 4) {
        if (flow_count == -1) {
          DEBUG_PRINT("[LoadPluginFlowFromFile]Load only [main_flow])\n");
          text_file.Close();
          UpdateScrollbarState();
          PostUpdateCanvasEvent();
          return true;
        } else {
          // 1�Z�b�g�̃T�u�t���[�`�F�b�N�����B
          DEBUG_PRINT("load sub flow count = %d\n", flow_count);
          break;
        }
      }

      // Get current plugin name and clone flag.
      token_str = tokenizer.GetNextToken();
      current_plugin_name = std::string(token_str.mb_str());
      token_str = tokenizer.GetNextToken();
      token_str.ToULong(&current_clone_flag);

      file_plugin_name = current_plugin_name;
      if (flow_count != -1) {
        // Save plugin name in the file.
        LoadPluginFlowInfo item;
        item.file_plugin_name = file_plugin_name;
        load_item_list.push_back(item);
      }

      if (current_clone_flag == 1) {
        int size = current_plugin_name.rfind(suffix);
        current_plugin_name.resize(size);
      }
      DEBUG_PRINT("current_plugin_name = %s, clone = %d \n",
                  current_plugin_name.c_str(), current_clone_flag);

      // Get next plugin name and clone flag.
      token_str = tokenizer.GetNextToken();
      next_plugin_name = std::string(token_str.mb_str());
      token_str = tokenizer.GetNextToken();
      token_str.ToULong(&next_clone_flag);

      if (next_clone_flag == 1) {
        int size = next_plugin_name.rfind(suffix);
        next_plugin_name.resize(size);
      }
      DEBUG_PRINT("next_plugin_name = %s, clone = %d \n",
                  next_plugin_name.c_str(), next_clone_flag);

      if (flow_count == -1) {
        // sub flow 1�ڂ̏��́A���򌳃v���O�C�����擾���g�p����B
        // suffix�����ύX�O�̃��X�g��H��A���򌳃v���O�C���̑}�������擾����B
        for (int list_count = 0; list_count < load_item_list.size();
             list_count++) {
          if (load_item_list[list_count].file_plugin_name == file_plugin_name) {
            base_item =
                load_item_list[list_count].item_info->next_item_info_[0];
            break;
          }
        }
        if (base_item == NULL) {
          // ���򌳃v���O�C�����擾���s�B
          // �A���Amain flow�ǂݍ��݂ɐ������Ă��邽�߁Atrue��ԋp�B
          DEBUG_PRINT(
              "[LoadPluginFlowFromFile]Failed to load sub flow,"
              "Load only [main_flow])\n");
          text_file.Close();
          UpdateScrollbarState();
          PostUpdateCanvasEvent();
          return true;
        }

      } else if (flow_count == 0) {
        std::string target_name = GetTargetPluginName(current_plugin_name);

        // sub flow 2�ڂ̃A�C�e���͕��򃊃X�g���\�z����B
        InsertNewBlanchPluginToFlow(target_name, base_item);

        // �\�z�������X�g�}���ʒu�����擾����B
        for (int i = 0; i < flow_plugin_list_.size(); i++) {
          for (int j = 0; j < flow_plugin_list_[i].size(); j++) {
            if (flow_plugin_list_[i][j]->plugin_name() == target_name) {
              branch_y = i;
              branch_x = j;
              break;
            }
          }
        }

        load_item_list[load_item_list.size() - 1].item_info =
            flow_plugin_list_[branch_y][branch_x];

      } else {
        std::string target_name = GetTargetPluginName(current_plugin_name);

        // sub flow3�ڈȍ~�̃A�C�e���́A���򃊃X�g�̌�i�ɐڑ�����B
        AddNewPluginToFlow(target_name,
                           flow_plugin_list_[branch_y][branch_x + flow_count]);

        load_item_list[load_item_list.size() - 1].item_info =
            flow_plugin_list_[branch_y][branch_x + flow_count];
      }
    }
  } while (!text_file.Eof());

  // Get setting info and set for each plugin.
  bool is_end_settings = false;
  for (; !text_file.Eof(); line_str = text_file.GetNextLine()) {
    if (line_str.find(wxT(kFlowSettingString)) != wxNOT_FOUND) {
      DEBUG_PRINT("Find [settings] \n");
      break;
    }
  }

  if (text_file.Eof() == true) {
    // �ݒ�ۑ��ɑΉ������v���O�C���A�C�e������
    // flow�ǂݍ��݊������Ă��邽�߁Atrue��ԋp
    DEBUG_PRINT("[LoadPluginFlowFromFile]Load complete\n");
    text_file.Close();
    UpdateScrollbarState();
    PostUpdateCanvasEvent();
    return true;
  }
  line_str = text_file.GetNextLine();

  do {
    for (; !text_file.Eof(); line_str = text_file.GetNextLine()) {
      if (line_str.find(wxT("name=")) != wxNOT_FOUND) {
        DEBUG_PRINT("Find [name=] \n");
        break;
      } else if (line_str.find(wxT(kFlowCycleString)) != wxNOT_FOUND) {
        DEBUG_PRINT("Find [cycle] \n");
        is_end_settings = true;
        break;
      }
    }

    if (is_end_settings == true) {
      break;
    }

    wxString target_name;
    std::string std_target_name;
    PluginEditItemInfo *base_item = NULL;
    PluginBase *target_plugin;
    target_name = line_str.AfterFirst(wxT('='));
    std_target_name = std::string(target_name.mb_str());
    DEBUG_PRINT("[Load setting] target_name = %s \n", target_name.c_str());

    // �v���O�C�������L�[�l�Ƃ��āAload_item_list�̃��X�g���������Ă����B
    for (int list_count = 0; list_count < load_item_list.size(); list_count++) {
      if (load_item_list[list_count].file_plugin_name == std_target_name) {
        base_item = load_item_list[list_count].item_info;
        break;
      }
    }
    if (base_item == NULL) {
      // �ݒ���擾���s�B
      // flow�ǂݍ��݂ɐ������Ă��邽�߁Atrue��ԋp�B
      DEBUG_PRINT("[LoadPluginFlowFromFile]Failed to load settings\n");
      text_file.Close();
      UpdateScrollbarState();
      PostUpdateCanvasEvent();
      return true;
    }

    // ����line���������A"name="���܂�ł��Ȃ����temporary��string
    // vector�ɒl���l�߂�B
    // ����line���������A"name="���܂ނ������́AEOF�o������ꍇ�͑{���I����
    // load_item_list[���o�ԍ�].item_info��SetPluginSettings()��
    // read�����ݒ�l��ʒm����B
    std::vector<wxString> plugin_params;
    for (line_str = text_file.GetNextLine(); !text_file.Eof();
         line_str = text_file.GetNextLine()) {
      if (line_str.find(wxT("name=")) != wxNOT_FOUND) {
        break;
      }
      if (line_str.find(wxT(kFlowCycleString)) != wxNOT_FOUND) {
        printf("Find [cycle] \n");
        is_end_settings = true;
        break;
      }
      plugin_params.push_back(line_str);
    }

    DEBUG_PRINT("plugin_params.size() = %d \n", plugin_params.size());
    if (plugin_params.size() > 0) {
      target_plugin = plugin_manager_->GetPlugin(base_item->plugin_name());
      target_plugin->SetPluginSettings(plugin_params);
      DEBUG_PRINT("SetPluginSettings \n");
    }
  } while (!text_file.Eof() || is_end_settings == false);

  // Get cycle value and set for each cycle.
  if (text_file.Eof() == true) {
    // Cycle���Ȃ�
    // flow�ǂݍ��݊������Ă��邽�߁Atrue��ԋp
    text_file.Close();
    UpdateScrollbarState();
    PostUpdateCanvasEvent();
    DEBUG_PRINT("[LoadPluginFlowFromFile]Load complete\n");
    return true;
  }

  if (thread_running_cycle_manager_ != NULL) {
  line_str = text_file.GetNextLine();
  do {
    // 1�s�ǂݍ���
    tokenizer.SetString(line_str, wxT(","), wxTOKEN_DEFAULT);
    if (tokenizer.CountTokens() != 3) {
      text_file.Close();
      UpdateScrollbarState();
      PostUpdateCanvasEvent();
      DEBUG_PRINT("[LoadPluginFlowFromFile]cycle does not exist \n");
      return false;
    }

    PluginEditItemInfo *src_item = NULL;
    PluginEditItemInfo *dst_item = NULL;
    std::string file_plugin_name;
    unsigned long cycle; /* NOLINT */

    // Get src plugin name.
    token_str = tokenizer.GetNextToken();
    file_plugin_name = std::string(token_str.mb_str());
    for (int list_count = 0; list_count < load_item_list.size(); list_count++) {
      if (load_item_list[list_count].file_plugin_name == file_plugin_name) {
        src_item = load_item_list[list_count].item_info;
        break;
      }
    }

    // Get dst plugin name.
    token_str = tokenizer.GetNextToken();
    file_plugin_name = std::string(token_str.mb_str());
    for (int list_count = 0; list_count < load_item_list.size(); list_count++) {
      if (load_item_list[list_count].file_plugin_name == file_plugin_name) {
        dst_item = load_item_list[list_count].item_info;
        break;
      }
    }

    // Get cycle value.
    token_str = tokenizer.GetNextToken();
    token_str.ToULong(&cycle);

    // Set cycle value
      thread_running_cycle_manager_->AddCycle(
          src_item->plugin_name(), dst_item->plugin_name(), (unsigned int)cycle);
    line_str = text_file.GetNextLine();
  } while (!text_file.Eof());
  }
  text_file.Close();

  UpdateScrollbarState();
  PostUpdateCanvasEvent();

  return true;
}

/**
 * @brief
 * Add plugin to plugin flow.
 * @param target_name [in] plugin name to be added.
 * @param current_item_info
 *   [in] current item on the flow to add target_name plugin.
 * @return true  = succeeded to add plugin.
 *         false = failed to add plugin.
 */
bool PluginEditCanvas::AddNewPluginToFlow(
    std::string target_name, PluginEditItemInfo *current_item_info) {
  std::string prev_name = "";
  std::vector<std::string> next_plugin_names;

  DEBUG_PRINT("Add new plugin\n");

  prev_name = GetValidPrevPluginName(current_item_info);

  DEBUG_PRINT("[AddNewPluginToFlow] prev_name : %s \n", prev_name.c_str());
  DEBUG_PRINT("[AddNewPluginToFlow] target_name : %s \n", target_name.c_str());
  DEBUG_PRINT("[AddNewPluginToFlow] next_plugin_names : size = 0 \n");

  if (plugin_manager_->ConnectPlugin(prev_name, target_name,
                                     next_plugin_names) == false) {
    DEBUG_PRINT("[AddNewPluginToFlow] Fail to connect plugin \n");
    return false;
  }

  IPlugin *target_plugin;
  target_plugin = plugin_manager_->GetPlugin(target_name);

  wxPoint target_pos = current_item_info->position();

  current_item_info->set_plugin_name(target_name);
  current_item_info->set_plugin_add_enable_state(kEnableToAddAllPlugin);
  current_item_info->set_current_plugin_setting_state(kSetValidPlugin);

  // Add blank EditItemInfo only if target has output port candidate spec.
  if (target_plugin->output_port_candidate_specs().size() == 0) {
    current_item_info->set_plugin_add_enable_state(kEnableToAddPrevPlugin |
                                                   kEnableToAddSubNextPlugin);
  } else {
    PluginEditItemInfo *new_item_info = new PluginEditItemInfo();
    new_item_info->set_position(wxPoint(target_pos.x + 1, target_pos.y));

    new_item_info->set_current_plugin_setting_state(kSetBlankPlugin);
    new_item_info->UpdateUnScrolledPosition(scroled_x_, scroled_y_);

    // Update next and prev pointer
    current_item_info->next_item_info_.push_back(new_item_info);
    new_item_info->prev_item_info_ = current_item_info;

    flow_plugin_list_[target_pos.y].push_back(new_item_info);
  }

  // Update plugin name list box
  parent_->AddPluginNameToPluginList(target_name);

  // for debug only.
  this->EditCanvasDebugLog();

  return true;
}

/**
 * @brief
 * Insert plugin to plugin flow.
 * @param target_name [in] plugin name to be inserted.
 * @param current_item_info
 *   [in] current item on the flow to insert target_name plugin.
 * @return true  = succeeded to insert plugin.
 *         false = failed to insert plugin.
 */
bool PluginEditCanvas::InsertNewPluginToFlow(
    std::string target_name, PluginEditItemInfo *current_item_info) {
  std::string prev_name = "";
  std::string next_name = "";
  std::vector<std::string> next_plugin_names;

  DEBUG_PRINT("Insert new plugin\n");

  prev_name = GetValidPrevPluginName(current_item_info);

  bool need_to_update_cycle_src_name = false;
  bool need_to_update_cycle_dst_name = false;
  if (current_item_info->current_plugin_setting_state() == kSetValidPlugin) {
    next_name = current_item_info->plugin_name();
    next_plugin_names.push_back(next_name);
    need_to_update_cycle_dst_name = true;
  } else if (current_item_info->current_plugin_setting_state() ==
             kBranchPointDummyPlugin) {
    next_plugin_names =
        GetValidNextPluginNames(current_item_info->prev_item_info_);
    need_to_update_cycle_src_name = true;
  } else {
    DEBUG_PRINT("[CanvasDebug]Invalid state =%d \n",
                current_item_info->current_plugin_setting_state());
    return false;
  }

  DEBUG_PRINT("[InsertNewPluginToFlow] prev_name : %s \n", prev_name.c_str());
  DEBUG_PRINT("[InsertNewPluginToFlow] target_name : %s \n",
              target_name.c_str());
  DEBUG_PRINT("[InsertNewPluginToFlow] next_plugin_names : size = d \n",
              next_plugin_names.size());
  for (int i = 0; i < next_plugin_names.size(); i++) {
    DEBUG_PRINT("[InsertNewPluginToFlow] next_plugin_names[%d] : %s \n", i,
                next_plugin_names[i].c_str());
  }

  if (plugin_manager_->ConnectPlugin(prev_name, target_name,
                                     next_plugin_names) == false) {
    DEBUG_PRINT("[InsertNewPluginToFlow] Fail to connect plugin \n");
    return false;
  }

  wxPoint target_pos = current_item_info->position();

  // Create new EditItemInfo.
  PluginEditItemInfo *new_item_info = new PluginEditItemInfo();

  // Add blank EditItemInfo to end of the plugin flow.
  new_item_info->set_plugin_name(target_name);
  new_item_info->set_plugin_add_enable_state(kEnableToAddAllPlugin);
  new_item_info->set_current_plugin_setting_state(kSetValidPlugin);
  new_item_info->set_position(target_pos);
  new_item_info->UpdateUnScrolledPosition(scroled_x_, scroled_y_);
  new_item_info->next_item_info_ =
      current_item_info->prev_item_info_->next_item_info_;

  current_item_info->prev_item_info_->next_item_info_.clear();
  current_item_info->prev_item_info_->next_item_info_.push_back(new_item_info);
  new_item_info->prev_item_info_ = current_item_info->prev_item_info_;

  for (int i = 0; i < new_item_info->next_item_info_.size(); i++) {
    new_item_info->next_item_info_[i]->prev_item_info_ = new_item_info;
  }

  flow_plugin_list_[target_pos.y].insert(
      flow_plugin_list_[target_pos.y].begin() + target_pos.x, new_item_info);

  for (int i = 0; i < new_item_info->next_item_info_.size(); i++) {
    new_item_info->next_item_info_[i]->ShiftPositionAllNext(1, 0);

    if (new_item_info->GetNextPluginName(i).find("Dummy") == wxNOT_FOUND) {
      int temp = new_item_info->next_item_info_[i]->plugin_add_enable_state();
      new_item_info->next_item_info_[i]->set_plugin_add_enable_state(
          temp | kEnableToAddSubNextPlugin);
    }
  }

  /* �z��ƍ��W�̕s��v���Ȃ������߁A����_�`�T�u�t���[�����Ԃɋ�v���O�C����}������B*/
  int height =
      new_item_info->next_item_info_[new_item_info->next_item_info_.size() - 1]
          ->GetBranchHeight();
  for (int i = target_pos.y + 1; i <= height; i++) {
    DEBUG_PRINT("Shift x position [%d] \n", i);
    PluginEditItemInfo *dummy_plugin = new PluginEditItemInfo();
    dummy_plugin = new PluginEditItemInfo();
    flow_plugin_list_[i].insert(flow_plugin_list_[i].begin() + target_pos.x,
                                dummy_plugin);
  }

  // update cycle info
  if (need_to_update_cycle_src_name) {
    thread_running_cycle_manager_->UpdateSrcName(prev_name, target_name);
  }
  if (need_to_update_cycle_dst_name) {
    for (int i = 0; i < next_plugin_names.size(); i++) {
      thread_running_cycle_manager_->UpdateDstName(
          prev_name, next_plugin_names[i], target_name);
    }
  }

  // Update plugin name list box
  parent_->AddPluginNameToPluginList(target_name);

  // for debug only.
  this->EditCanvasDebugLog();

  return true;
}

/**
 * @brief
 * Create blanch and insert plugin to plugin flow.
 * @param target_name [in] plugin name to be inserted.
 * @param current_item_info
 *   [in] current item on the flow to insert target_name plugin.
 * @return true  = succeeded to insert plugin.
 *         false = failed to insert plugin.
 */
bool PluginEditCanvas::InsertNewBlanchPluginToFlow(
    std::string target_name, PluginEditItemInfo *current_item_info) {
  std::string prev_name = "";
  std::string next_name = "";
  std::vector<std::string> next_plugin_names;

  DEBUG_PRINT("Insert new branch and plugin\n");

  prev_name = GetValidPrevPluginName(current_item_info);

  DEBUG_PRINT("[InsertNewBlanchPluginToFlow] prev_name : %s \n",
              prev_name.c_str());
  DEBUG_PRINT("[InsertNewBlanchPluginToFlow] target_name : %s \n",
              target_name.c_str());
  DEBUG_PRINT("[InsertNewBlanchPluginToFlow] next_plugin_names : size = 0 \n");

  if (plugin_manager_->ConnectPlugin(prev_name, target_name,
                                     next_plugin_names) == false) {
    DEBUG_PRINT("[InsertNewBlanchPluginToFlow] Fail to connect plugin \n");
    return false;
  }
  if (thread_running_cycle_manager_) {
    thread_running_cycle_manager_->AddCycle(prev_name, target_name, 1);
  }

  wxPoint target_pos = current_item_info->position();

  // Shift plugin's position start.
  // Y������shift�ʂ������l�B�Œ�1shift����B
  int shift_y = 1;
  // Y�����̖����ɒǉ�����ꍇ�������Ashift�ʂ��`�F�b�N����B
  if ((target_pos.y + 1) < flow_plugin_list_.size()) {
    // �ǉ��Ώۂ��A����̃t���[�̓`�F�b�N����K�v�Ȃ��B
    int i = target_pos.y;

    std::vector<std::string> shift_checked_names;
    std::vector<std::string> temp_shift_checked_names;
    bool checked_name = false;
    std::string shift_check_name = "";
    shift_check_name = current_item_info->GetPrevPluginName();

    for (; i < flow_plugin_list_.size(); i++) {
      int j = 0;
      // �`�F�b�N�Ώۂ̃v���O�C���ݒ�ʒu���`�F�b�N����B
      // Dummy���܂ޑS�v���O�C�����ŏ��Ɍ����x���W���擾�B
      for (; j < flow_plugin_list_[i].size(); j++) {
        if (flow_plugin_list_[i][j]->plugin_name() != "") {
          DEBUG_PRINT("[InsertNewBlanchPluginToFlow]  break = (%d, %d) \n", j,
                      i);
          break;
        }
      }
      if (j < target_pos.x) {
        // �w�`�F�b�N�Ώۂ�x���W < �ǉ��Ώۂ�x���W�x
        // �t���[�̏Փ˂�����邽�߂ɁA�`�F�b�N�ΏۂɘA�Ȃ�S�ẴA�C�e����Y���W��1shift����B

        DEBUG_PRINT("[InsertNewBlanchPluginToFlow] shift_check_name = %s\n",
                    shift_check_name.c_str());
        // �ǉ��Ώۂ̍��W�ɐݒ�ς݂̃v���O�C������Next���Ɋ܂ޏꍇ�́Ashift�ΏۊO�Ƃ���B
        if (IsIncludePlugin(shift_check_name, flow_plugin_list_[i][j]) ==
            false) {
          // ���ɁAshift�ς݂ł��邱�Ƃ����o�����ꍇ�́A2�d��shift���s��Ȃ��B
          for (int k = 0; k < shift_checked_names.size(); k++) {
            if (shift_checked_names[k] ==
                flow_plugin_list_[i][j]->plugin_name()) {
              checked_name = true;
              break;
            }
          }

          if (checked_name == false) {
            temp_shift_checked_names =
                flow_plugin_list_[i][j]->ShiftPositionAllNext(0, 1);
            for (int k = 0; k < temp_shift_checked_names.size(); k++) {
              shift_checked_names.push_back(temp_shift_checked_names[k]);
            }
            DEBUG_PRINT("[InsertNewBlanchPluginToFlow]  Shift ALL += 1 \n");
          } else {
            DEBUG_PRINT(
                "[InsertNewBlanchPluginToFlow]  Shift ALL += 0(i = %d) 1st\n",
                i);
          }
        } else {
          DEBUG_PRINT(
              "[InsertNewBlanchPluginToFlow]  Shift ALL += 0(i = %d) 2nd\n", i);
        }
      } else {
        // �w�`�F�b�N�Ώۂ�x���W >= �ǉ��Ώۂ�x���W�x
        // �t���[�̏Փ˂�����邽�߂ɁA�ǉ��ΏۃA�C�e���̑}���ʒu��1shift����B

        // if���[�g�ŁAshift�ς݂̃A�C�e�����܂ޏꍇ�́Ashift���Ȃ��B
        //  (���򂩂�X�ɕ�����J��Ԃ����ꍇ�̍l��)
        for (int k = 0; k < shift_checked_names.size(); k++) {
          if (shift_checked_names[k] ==
              flow_plugin_list_[i][j]->plugin_name()) {
            checked_name = true;
            break;
          }
        }
        if (checked_name == false) {
          shift_y += 1;
          DEBUG_PRINT("[InsertNewBlanchPluginToFlow]  shift_y += 1 \n");
        } else {
          DEBUG_PRINT("[InsertNewBlanchPluginToFlow]  shift_y += 0 \n");
        }
      }
      checked_name = false;
    }
  }
  DEBUG_PRINT("[InsertNewBlanchPluginToFlow] shift_y = %d \n", shift_y);
  // Shift plugin's position end.

  // ���򌳃v���O�C���̃T�C�Y + 2�̃��X�g��p�ӂ���B
  std::vector<PluginEditItemInfo *> new_branch_list(target_pos.x + 2);
  for (int i = 0; i < new_branch_list.size(); i++) {
    new_branch_list[i] = new PluginEditItemInfo();
  }
  DEBUG_PRINT("[InsertNewBlanchPluginToFlow]new branch list size = %d \n",
              new_branch_list.size());

  // 1.����撼��Ɏΐ��\���p��Dummy�v���O�C����}������B
  std::string dummy_name;
  new_branch_list[target_pos.x]->set_current_plugin_setting_state(
      kBranchLineDummyPlugin);
  new_branch_list[target_pos.x]->set_plugin_add_enable_state(kDisableAddPlugin);
  new_branch_list[target_pos.x]->set_position(
      wxPoint(target_pos.x, target_pos.y + shift_y));
  new_branch_list[target_pos.x]->UpdateUnScrolledPosition(scroled_x_,
                                                          scroled_y_);
  dummy_name =
      std::string(wxString::Format(wxT("Dummy[%d,%d]"),
                                   new_branch_list[target_pos.x]->position().x,
                                   new_branch_list[target_pos.x]->position().y)
                      .mb_str());
  new_branch_list[target_pos.x]->set_plugin_name(dummy_name);

  DEBUG_PRINT("[InsertNewBlanchPluginToFlow] Add new row (%d, %d)\n",
              target_pos.x, target_pos.y + shift_y);

  // ���򌳂�next��Dummy�v���O�C������ǉ�����B
  flow_plugin_list_[target_pos.y][target_pos.x]
      ->prev_item_info_->next_item_info_.push_back(
          new_branch_list[target_pos.x]);

  // prev : ���򌳂�ݒ肷��B
  new_branch_list[target_pos.x]->prev_item_info_ =
      flow_plugin_list_[target_pos.y][target_pos.x]->prev_item_info_;
  // next : �����+1��ݒ肷��B
  new_branch_list[target_pos.x]->next_item_info_.push_back(
      new_branch_list[target_pos.x + 1]);

  // 2.�����+1�ɒǉ��Ώۂ̃v���O�C����ݒ肷��B
  new_branch_list[target_pos.x + 1]->set_plugin_name(target_name);
  new_branch_list[target_pos.x + 1]->set_plugin_add_enable_state(
      kEnableToAddAllPlugin);
  new_branch_list[target_pos.x + 1]->set_position(
      wxPoint(target_pos.x + 1, target_pos.y + shift_y));
  new_branch_list[target_pos.x + 1]->set_current_plugin_setting_state(
      kSetValidPlugin);
  new_branch_list[target_pos.x + 1]->UpdateUnScrolledPosition(scroled_x_,
                                                              scroled_y_);

  // prev : ������ݒ肷��
  // next : �ȍ~�̃`�F�b�N���ʎ���Őݒ�L�������܂邽�߂����ł͐ݒ肵�Ȃ��B
  new_branch_list[target_pos.x + 1]->prev_item_info_ =
      new_branch_list[target_pos.x];
  // ��v���O�C���ǉ����邩���f����B
  IPlugin *target_plugin;
  // �V�K�ǉ��v���O�C���̃|�C���^��ǉ�����B
  target_plugin = plugin_manager_->GetPlugin(target_name);
  if (target_plugin->output_port_candidate_specs().size() == 0) {
    new_branch_list[target_pos.x + 1]->set_plugin_add_enable_state(
        kDisableToAddNextPlugin);
  } else {
    // �Ȃ���+1�̏���ݒ肷��B
    PluginEditItemInfo *branch_next_item_info;
    branch_next_item_info = new PluginEditItemInfo();
    branch_next_item_info->set_position(
        wxPoint(target_pos.x + 2, target_pos.y + shift_y));
    branch_next_item_info->set_current_plugin_setting_state(kSetBlankPlugin);
    branch_next_item_info->UpdateUnScrolledPosition(scroled_x_, scroled_y_);

    // �V���ȗ�����X�g�ǉ�����B
    new_branch_list.push_back(branch_next_item_info);

    // next : �����ɑ}�������v���O�C����next�ւ��̌�̋�v���O�C����ݒ肷��B
    new_branch_list[target_pos.x + 1]->next_item_info_.push_back(
        new_branch_list[target_pos.x + 2]);

    // prev : ��v���O�C����prev�ɁA�}�������v���O�C����ݒ肷��B
    new_branch_list[target_pos.x + 2]->prev_item_info_ =
        new_branch_list[target_pos.x + 1];
  }

  DEBUG_PRINT("[InsertNewBlanchPluginToFlow] size = %d , add col = %d\n",
              flow_plugin_list_.size(), (target_pos.y + shift_y));
  // �\�z�ς݃t���[�̓r���ɑ}������ꍇ��insert�A
  // �Ō�ɑ}������ꍇ��push_back�ŐV���ȃt���[��ǉ�����B
  if ((target_pos.y + shift_y) <= flow_plugin_list_.size()) {
    DEBUG_PRINT("[InsertNewBlanchPluginToFlow] insert new list\n");
    flow_plugin_list_.insert(flow_plugin_list_.begin() + target_pos.y + shift_y,
                             new_branch_list);
  } else {
    DEBUG_PRINT("[InsertNewBlanchPluginToFlow] push_back new list\n");
    flow_plugin_list_.push_back(new_branch_list);
  }

  // ���򌳂ɋ��Dummy�v���O�C����}�����邩���肷��B
  // ����|�C���g�ŘA�����ĕ��򂵂��ꍇ�ȂǁA
  // ����Dummy�v���O�C���}���ς݂ł���ꍇ�́A�ǉ��s�v�B
  std::string base_name =
      flow_plugin_list_[target_pos.y][target_pos.x]->plugin_name();

  if (base_name.find("Dummy") == wxNOT_FOUND) {
    //   prev/next(+�����v���O�C��)�͑O��̏���ݒ肷��B
    PluginEditItemInfo *base_dummy_plugin = new PluginEditItemInfo();
    base_dummy_plugin->set_current_plugin_setting_state(
        kBranchPointDummyPlugin);
    base_dummy_plugin->set_position(target_pos);
    base_dummy_plugin->UpdateUnScrolledPosition(scroled_x_, scroled_y_);
    dummy_name = std::string(wxString::Format(wxT("Dummy[%d,%d]"),
                                              base_dummy_plugin->position().x,
                                              base_dummy_plugin->position().y)
                                 .mb_str());
    base_dummy_plugin->set_plugin_name(dummy_name);

    // prev : ���򌳂̑O�i�v���O�C����ݒ肷��B
    base_dummy_plugin->prev_item_info_ =
        flow_plugin_list_[target_pos.y][target_pos.x]->prev_item_info_;

    // next : �O�i�v���O�C����next����0�Ԗڂ̗v�f��ݒ�B
    //        �O�i�v���O�C����0�Ԗڂ̗v�f�́A���̌�Dummy�v���O�C���ŏ㏑������B
    base_dummy_plugin->next_item_info_.push_back(
        flow_plugin_list_[target_pos.y][target_pos.x]
            ->prev_item_info_->next_item_info_[0]);

    flow_plugin_list_[target_pos.y].insert(
        flow_plugin_list_[target_pos.y].begin() + target_pos.x,
        base_dummy_plugin);

    // next :
    // �O�i�v���O�C����next����0�Ԗڂ̗v�f��Dummy�v���O�C���ŏ㏑������B
    flow_plugin_list_[target_pos.y][target_pos.x - 1]->next_item_info_[0] =
        flow_plugin_list_[target_pos.y][target_pos.x];

    // ��i�v���O�C����prev�����ADummy�v���O�C���ɕt���ւ���B
    for (int i = 0; i <
         flow_plugin_list_[target_pos.y][target_pos.x]->next_item_info_.size();
         i++) {
      flow_plugin_list_[target_pos.y][target_pos.x]
          ->next_item_info_[i]
          ->prev_item_info_ = flow_plugin_list_[target_pos.y][target_pos.x];
    }

    // Dummy�v���O�C����prev�ɁA���򌳂̑O�i�v���O�C����ݒ肷��B
    flow_plugin_list_[target_pos.y][target_pos.x]->prev_item_info_ =
        flow_plugin_list_[target_pos.y][target_pos.x - 1];

    /* �z��ƍ��W�̕s��v���Ȃ������߁A����_�`shift_y�̊Ԃ�
     * �̊Ԃɋ�v���O�C����}������B*/
    for (int i = target_pos.y + 1; i <= target_pos.y + shift_y - 1; i++) {
      DEBUG_PRINT("Shift x position [%d] \n", i);
      PluginEditItemInfo *dummy_plugin = new PluginEditItemInfo();
      dummy_plugin = new PluginEditItemInfo();
      flow_plugin_list_[i].insert(flow_plugin_list_[i].begin() + target_pos.x,
                                  dummy_plugin);
    }

    // Dummy�v���O�C���̒ǉ��ɔ����A�ǉ��ӏ��ɘA�Ȃ�A�C�e����S��x������1
    // shift����B
    flow_plugin_list_[target_pos.y][target_pos.x]
        ->next_item_info_[0]
        ->ShiftPositionAllNext(1, 0);
  } else {
    // Do nothing
    DEBUG_PRINT("[InsertNewBlanchPluginToFlow] branch Dummy found \n");
  }

  // List box�X�V����
  parent_->AddPluginNameToPluginList(target_name);

  // for debug only.
  this->EditCanvasDebugLog();

  return true;
}

/**
 * @brief
 * Replace plugin on plugin flow.
 * @param target_name [in] plugin name to be replaced.
 * @param current_item_info
 *   [in] current item on the flow to replace target_name plugin.
 * @return true  = succeeded to replace plugin.
 *         false = failed to replace plugin.
 */
bool PluginEditCanvas::ReplacePlugin(std::string target_name,
                                     PluginEditItemInfo *current_item_info) {
  std::string old_target_name = current_item_info->plugin_name();
  std::string new_target_name = target_name;
  std::string prev_name = "";
  std::vector<std::string> next_plugin_names;

  DEBUG_PRINT("Replace plugin\n");

  prev_name = GetValidPrevPluginName(current_item_info);
  next_plugin_names = GetValidNextPluginNames(current_item_info);

  if (plugin_manager_->ReplacePlugin(prev_name, old_target_name,
                                     new_target_name,
                                     next_plugin_names) == false) {
    return false;
  }

  PluginBase *old_target_plugin;
  PluginBase *new_target_plugin;

  wxPoint target_pos = current_item_info->position();

  // new�̃v���O�C�����ōX�V����
  current_item_info->set_plugin_name(new_target_name);

  // �V�K�ǉ��v���O�C���̃|�C���^���擾����B
  old_target_plugin = plugin_manager_->GetPlugin(old_target_name);
  new_target_plugin = plugin_manager_->GetPlugin(new_target_name);

  // ���ɐڑ�����(���Ă���)��v���O�C���̏�Ԑ���
  if ((old_target_plugin->output_port_candidate_specs().size() == 0) &&
      (new_target_plugin->output_port_candidate_specs().size() == 0)) {
    // If OutputPlugin added, update state as DisableToAddNextPlugin.
    current_item_info->set_plugin_add_enable_state(kEnableToAddPrevPlugin |
                                                   kEnableToAddSubNextPlugin);
  } else if ((old_target_plugin->output_port_candidate_specs().size() == 0) &&
             (new_target_plugin->output_port_candidate_specs().size() != 0)) {
    // ��v���O�C����}��
    // Without OutputPlugin, connect blank EditItemInfo to end of the plugin
    // flow.
    PluginEditItemInfo *new_item_info = new PluginEditItemInfo();
    new_item_info->set_position(wxPoint(target_pos.x + 1, target_pos.y));

    new_item_info->set_current_plugin_setting_state(kSetBlankPlugin);
    new_item_info->UpdateUnScrolledPosition(scroled_x_, scroled_y_);

    // �S�����v���O�C���ǉ��\�ŏ�ԍX�V
    current_item_info->set_plugin_add_enable_state(kEnableToAddAllPlugin);
    // �v���O�C���ݒ����Next�����X�V
    current_item_info->next_item_info_.push_back(new_item_info);
    // ���̌�ɐڑ�������v���O�C����Prev�����X�V
    new_item_info->prev_item_info_ = current_item_info;

    flow_plugin_list_[target_pos.y].push_back(new_item_info);
  } else if ((old_target_plugin->output_port_candidate_specs().size() != 0) &&
             (new_target_plugin->output_port_candidate_specs().size() == 0)) {
    // ��v���O�C�����폜
    current_item_info->next_item_info_.clear();
    flow_plugin_list_[target_pos.y].erase(
        flow_plugin_list_[target_pos.y].begin() +
        flow_plugin_list_[target_pos.y].size() - 1);

    // If OutputPlugin added, update state as DisableToAddNextPlugin.
    current_item_info->set_plugin_add_enable_state(kEnableToAddPrevPlugin |
                                                   kEnableToAddSubNextPlugin);
  }

  if (old_target_plugin && old_target_plugin->is_cloned()) {
    plugin_manager_->ReleasePlugin(old_target_plugin);
  }

  // Update plugin name list box
  parent_->RemovePluginNameFromPluginList(old_target_name);
  parent_->AddPluginNameToPluginList(new_target_name);

  // Update cycle info
  thread_running_cycle_manager_->UpdateSrcName(old_target_name,
                                               new_target_name);
  thread_running_cycle_manager_->UpdateDstName(old_target_name,
                                               new_target_name);

  // for debug only.
  this->EditCanvasDebugLog();

  return true;
}

/**
 * @brief
 * Replace root plugin.
 * @param target_name [in] plugin name to be replaced.
 * @param current_item_info [in] current item of root plugin.
 * @return true  = succeeded to replace plugin.
 *         false = failed to replace plugin.
 */
bool PluginEditCanvas::ReplaceRootPlugin(
    std::string target_name, PluginEditItemInfo *current_item_info) {
  DEBUG_PRINT("Replace root plugin\n");
  std::string old_target_name = current_item_info->plugin_name();
  std::string new_target_name = target_name;
  if (old_target_name != new_target_name) {
    if (!plugin_manager_->set_root_plugin(new_target_name)) {
      return false;
    }
    SetPluginManagerInfo(plugin_manager_);
    PostUpdateCanvasEvent();
  }
  return true;
}

/* Declare the event for WxDlgPlusMenu */
BEGIN_EVENT_TABLE(WxDlgPlusMenu, wxDialog)
EVT_BUTTON(kButtonAddId, WxDlgPlusMenu::OnAddButton)
EVT_BUTTON(kButtonBranchId, WxDlgPlusMenu::OnBranchButton)
EVT_BUTTON(kButtonDeleteId, WxDlgPlusMenu::OnDeleteButton)
EVT_SHOW(WxDlgPlusMenu::OnShow)
END_EVENT_TABLE()

/**
 * @brief
 * Constructor.
 * @param parent [in] wxWindow class pointer.
 * @param id [in] wxWidgets window ID.
 * @param str [in] window title strings.
 */
WxDlgPlusMenu::WxDlgPlusMenu(wxWindow *parent, const wxWindowID id,
                             const wxString &str)
    : wxDialog(parent, -1, str, wxPoint(200, 200), wxSize(100, 90),
               wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP) {
  parent_ = dynamic_cast<PluginEditCanvas *>(parent);
  event_type_ = kPlusMenuEventNone;

  button_add_ = new wxButton(this, kButtonAddId, wxT("add"), wxPoint(0, 0),
                             wxSize(100, 30));

  button_branch_ = new wxButton(this, kButtonBranchId, wxT("branch"),
                                wxPoint(0, 30), wxSize(100, 30));

  button_delete_ = new wxButton(this, kButtonDeleteId, wxT("delete"),
                                wxPoint(0, 60), wxSize(100, 30));

  wxdlg_select_plugin_ =
      new WxDlgSelectPlugin(this, wxID_ANY, wxT("Select Plugin"));
}

/**
 * @brief
 * Destructor.
 */
WxDlgPlusMenu::~WxDlgPlusMenu() {
  DEBUG_PRINT("WxDlgPlusMenu::~WxDlgPlusMenu\n");
}

void WxDlgPlusMenu::OnAddButton(wxCommandEvent &event) {
  DEBUG_PRINT("WxDlgPlusMenu::OnAddButton\n");
  if (wxdlg_select_plugin_ == NULL) {
    wxdlg_select_plugin_ =
        new WxDlgSelectPlugin(this, wxID_ANY, wxT("Select Plugin"));
  }

  wxdlg_select_plugin_->SetDispString(
      parent_->GetDispString(kFlowActionAddNext));
  wxdlg_select_plugin_->ShowModal();
  if (wxdlg_select_plugin_->is_update_plugin_list()) {
    event_type_ = kPlusMenuEventAddPressed;
  }
  Close();
}
void WxDlgPlusMenu::OnBranchButton(wxCommandEvent &event) {
  DEBUG_PRINT("WxDlgPlusMenu::OnBranchButton\n");
  if (wxdlg_select_plugin_ == NULL) {
    wxdlg_select_plugin_ =
        new WxDlgSelectPlugin(this, wxID_ANY, wxT("Select Plugin"));
  }

  wxdlg_select_plugin_->SetDispString(
      parent_->GetDispString(kFlowActionAddBranch));
  wxdlg_select_plugin_->ShowModal();
  if (wxdlg_select_plugin_->is_update_plugin_list()) {
    event_type_ = kPlusMenuEventBranchPressed;
  }
  Close();
}

void WxDlgPlusMenu::OnDeleteButton(wxCommandEvent &event) {
  DEBUG_PRINT("WxDlgPlusMenu::OnDelete \n");
  event_type_ = kPlusMenuEventDeletePressed;
  Close();
}

void WxDlgPlusMenu::OnShow(wxShowEvent &event) {
  DEBUG_PRINT("WxDlgPlusMenu::OnShow\n");

  DEBUG_PRINT("Refresh\n");
  wxRect rect = parent_->GetRect();
  parent_->Refresh(true, &rect);
  parent_->Update();
  parent_->PostUpdateCanvasEvent();

  DEBUG_PRINT("WxDlgPlusMenu::OnShow exit\n");
}

/**
 * @brief
 * Clear the selected state of all of the plugin flow items.
 */
void WxDlgPlusMenu::Reset() {
  event_type_ = kPlusMenuEventNone;
  if (wxdlg_select_plugin_) {
    wxdlg_select_plugin_->Reset();
  }
}

/**
 * @brief
 * Get plugin name which selected on the popup window.
 */
std::string WxDlgPlusMenu::GetPluginName(void) {
  DEBUG_PRINT("WxDlgPlusMenu::GetPluginName\n");

  if (wxdlg_select_plugin_ == NULL) {
    return NULL;
  }
  return wxdlg_select_plugin_->GetPluginName();
}

/**
 * @brief
 * Update button enable/disable state of WxDlgPlusMenu.
 */
void WxDlgPlusMenu::UpdateButtonState(PluginEditItemInfo *target_item_info,
                                      int plugin_add_enable_state) {
  bool is_connect_dummy = false;

  if (target_item_info->GetPrevPluginName().find("Dummy") != wxNOT_FOUND) {
    is_connect_dummy = true;
  }

  button_add_->Enable();
  if ((plugin_add_enable_state & kEnableToAddSubNextPlugin) &&
      (is_connect_dummy == false)) {
    button_branch_->Enable();
  } else {
    button_branch_->Disable();
  }

  if (target_item_info->current_plugin_setting_state() == kNotSetPlugin) {
    button_delete_->Disable();
  } else {
    button_delete_->Enable();
  }
}

/* Declare the event for WxDlgSelectPlugin */
BEGIN_EVENT_TABLE(WxDlgSelectPlugin, wxDialog)
EVT_BUTTON(kButtonApplyId, WxDlgSelectPlugin::OnApply)
END_EVENT_TABLE()

/**
 * @brief
 * Constructor.
 * @param parent [in] wxWindow class pointer.
 * @param id [in] wxWidgets window ID.
 * @param str [in] window title strings.
 */
WxDlgSelectPlugin::WxDlgSelectPlugin(wxWindow *parent, const wxWindowID id,
                                     const wxString &str)
    : wxDialog(parent, -1, str, wxPoint(200, 200), wxSize(220, 240),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) {
  wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
  // Creating a PluginList Input object
  list_box_plugin_ =
      new wxListBox(this, kListBoxPluginId, wxPoint(10, 10), wxSize(200, 190));

  button_apply_ = new wxButton(this, kButtonApplyId, wxT("Apply"),
                               wxPoint(100, 205), wxSize(80, 30));

  top_sizer->Add(list_box_plugin_, 1, wxEXPAND | wxALL, 10);
  button_sizer->Add(button_apply_, 0, wxALL, 10);
  top_sizer->Add(button_sizer, 0, wxALIGN_CENTER);
  SetSizerAndFit(top_sizer);

  // Initialize parameter
  is_update_plugin_list_ = false;
}

/**
 * @brief
 * Destructor.
 */
WxDlgSelectPlugin::~WxDlgSelectPlugin() {
  DEBUG_PRINT("WxDlgSelectPlugin::~WxDlgSelectPlugin\n");
}

void WxDlgSelectPlugin::OnApply(wxCommandEvent &event) {
  DEBUG_PRINT("WxDlgSelectPlugin::OnApply\n");
  int select_num;

  select_num = list_box_plugin_->GetSelection();
  if (select_num != wxNOT_FOUND) {
    is_update_plugin_list_ = true;
  }
  Close();
}

/**
 * @brief
 * Clear the selected state of WxDlgSelectPlugin.
 */
void WxDlgSelectPlugin::Reset(void) {
  DEBUG_PRINT("WxDlgSelectPlugin::Reset\n");

  is_update_plugin_list_ = false;
}

/**
 * @brief
 * Get a plugin name which selected on WxDlgSelectPlugin.
 * return plugin name.
 */
std::string WxDlgSelectPlugin::GetPluginName(void) {
  DEBUG_PRINT("WxDlgSelectPlugin::GetPluginName\n");

  int select_num;
  std::string string = "";

  select_num = list_box_plugin_->GetSelection();
  if (select_num == wxNOT_FOUND) {
    return string;
  }

  string = std::string(list_box_plugin_->GetString(select_num).mb_str());
  return string;
}

/**
 * @brief
 * There is a need to set the appropriate names
 *  depending on the operation(add, branch or replace).
 * param disp_strings [in] name list which display on WxDlgSelectPlugin.
 */
void WxDlgSelectPlugin::SetDispString(std::vector<std::string> disp_strings) {
  list_box_plugin_->Clear();

  std::vector<std::string>::iterator itr;
  wxArrayString wx_array_string;

  for (itr = disp_strings.begin(); itr != disp_strings.end(); itr++) {
    wxString wx_string(itr->c_str(), wxConvUTF8);
    wx_array_string.Add(wx_string);
  }

  list_box_plugin_->InsertItems(wx_array_string, 0);
}

/* Declare the event for WxDlgPluginMenu */
BEGIN_EVENT_TABLE(WxDlgPluginMenu, wxDialog)
EVT_BUTTON(kButtonReplaceId, WxDlgPluginMenu::OnReplaceButton)
EVT_SHOW(WxDlgPluginMenu::OnShow)
END_EVENT_TABLE()

/**
 * @brief
 * Constructor.
 * @param parent [in] wxWindow class pointer.
 * @param id [in] wxWidgets window ID.
 * @param str [in] window title strings.
 */
WxDlgPluginMenu::WxDlgPluginMenu(wxWindow *parent, const wxWindowID id,
                                 const wxString &str)
    : wxDialog(parent, -1, str, wxPoint(200, 200), wxSize(100, 30),
               wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP) {
  parent_ = dynamic_cast<PluginEditCanvas *>(parent);
  wxdlg_select_plugin_ = NULL;
  event_type_ = kPluginMenuEventNone;

  button_replace_ = new wxButton(this, kButtonReplaceId, wxT("replace"),
                                 wxPoint(0, 0), wxSize(100, 30));

  wxdlg_select_plugin_ =
      new WxDlgSelectPlugin(this, wxID_ANY, wxT("Select Plugin"));
}

/**
 * @brief
 * Destructor.
 */
WxDlgPluginMenu::~WxDlgPluginMenu() {
  DEBUG_PRINT("WxDlgPluginMenu::~WxDlgPluginMenu\n");
}

void WxDlgPluginMenu::OnReplaceButton(wxCommandEvent &event) {
  DEBUG_PRINT("WxDlgPluginMenu::OnReplaceButton\n");
  if (wxdlg_select_plugin_ == NULL) {
    wxdlg_select_plugin_ =
        new WxDlgSelectPlugin(this, wxID_ANY, wxT("Select Plugin"));
  }

  wxdlg_select_plugin_->SetDispString(
      parent_->GetDispString(kFlowActionReplace));
  wxdlg_select_plugin_->ShowModal();
  if (wxdlg_select_plugin_->is_update_plugin_list()) {
    event_type_ = kPluginMenuEventReplacePressed;
  }
  Close();
}

/**
 * @brief
 * Clear the selected state of all of the plugin flow items.
 */
void WxDlgPluginMenu::Reset() {
  event_type_ = kPluginMenuEventNone;
  if (wxdlg_select_plugin_) {
    wxdlg_select_plugin_->Reset();
  }
}

void WxDlgPluginMenu::OnShow(wxShowEvent &event) {
  DEBUG_PRINT("WxDlgPluginMenu::OnShow\n");

  DEBUG_PRINT("Refresh\n");
  wxRect rect = parent_->GetRect();
  parent_->Refresh(true, &rect);
  parent_->Update();
  parent_->PostUpdateCanvasEvent();

  DEBUG_PRINT("WxDlgPluginMenu::OnShow exit\n");
}

/**
 * @brief
 * Get plugin name which selected on the popup window.
 */
std::string WxDlgPluginMenu::GetPluginName(void) {
  DEBUG_PRINT("WxDlgPluginMenu::GetPluginName\n");

  if (wxdlg_select_plugin_ == NULL) {
    return NULL;
  }
  return wxdlg_select_plugin_->GetPluginName();
}

/* Declare the event for wxDialog */
BEGIN_EVENT_TABLE(WxDlgCycleMenu, wxDialog)
EVT_CLOSE(WxDlgCycleMenu::OnClose)
EVT_BUTTON(kButtonApplyId, WxDlgCycleMenu::OnApplyButton)
END_EVENT_TABLE()

/**
 * @brief
 * Constructor.
 * @param parent [in] wxWindow class pointer.
 * @param id [in] wxWidgets window ID.
 * @param str [in] window title strings.
 */
WxDlgCycleMenu::WxDlgCycleMenu(wxWindow *parent, const wxWindowID id,
                               const wxString &str)
    : wxDialog(parent, -1, str, wxPoint(200, 200), wxSize(200, 100),
               wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP) {
  DEBUG_PRINT("WxDlgCycleMenu::WxDlgCycleMenu \n");

  parent_ = dynamic_cast<PluginEditCanvas *>(parent);

  static_text_info_ = new wxStaticText(
      this, wxID_ANY, wxT("Processing interval cycle\n setting of subflow"),
      wxPoint(10, 10), wxSize(180, 40));

  text_cycle_value_ =
      new wxTextCtrl(this, wxID_ANY, wxT(""), wxPoint(30, 60), wxSize(50, 25));

  button_apply_ = new wxButton(this, kButtonApplyId, wxT("Apply"),
                               wxPoint(100, 60), wxSize(70, 30));
}

/**
 * @brief
 * Destructor.
 */
WxDlgCycleMenu::~WxDlgCycleMenu() {
  DEBUG_PRINT("WxDlgCycleMenu::~WxDlgCycleMenu \n");
}

void WxDlgCycleMenu::OnClose(wxCloseEvent &event) {
  DEBUG_PRINT("WxDlgCycleMenu::OnClose \n");
  Show(false);
  parent_->Enable();
}

void WxDlgCycleMenu::OnApplyButton(wxCommandEvent &event) {
  DEBUG_PRINT("WxDlgCycleMenu::OnApplyButton \n");
  Show(false);

  long unsigned int cycle;  // NOLINT
  text_cycle_value_->GetValue().ToULong(&cycle);
  cycle_ = static_cast<unsigned int>(cycle);
  if (cycle_ == 0) {
    cycle_ = 1;
  }
  parent_->Enable();
}

/**
 * @brief
 * Set cycle value to display on WxDlgCycleMenu dialog.
 * @param cycle [in] cycle value to display.
 */
void WxDlgCycleMenu::set_cycle(unsigned int cycle) {
  if (cycle == 0) {
    cycle = 1;
  }
  text_cycle_value_->SetValue(wxString::Format(wxT("%d"), cycle));
  cycle_ = cycle;
}
