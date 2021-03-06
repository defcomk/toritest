/**
 * @file      output_disp_opencv.cpp
 * @brief     Display plugin by using Opencv.
 * @author    Vision Processing Community
 * @copyright 2016 Vision Processing Community. All rights reserved.
 */

#include "./output_disp_opencv.h"
#include <vector>

/**
 * @brief
 * Constructor.
 */
OutputDispOpencv::OutputDispOpencv() : PluginBase() {
  DEBUG_PRINT("OutputDispOpencv::OutputDispOpencv()\n");

  // Initialize base class(plugin_base.h)
  set_plugin_name("OutputDispOpencv");

  AddInputPortCandidateSpec(kGRAY8);  /* Raw8*/
  AddInputPortCandidateSpec(kGRAY16); /* Raw16*/
  AddInputPortCandidateSpec(kBGR888); /* BGR888 */
  AddInputPortCandidateSpec(kBGR48);  /* BGR48 */

  set_is_use_dest_buffer(false);

  // Initialize
  wnd_ = new OutputDispOpencvWnd(this);
  wxString wx_string(plugin_name().c_str(), wxConvUTF8);
  wnd_->InitDialog();
  current_image_size = cvSize(0, 0);
}

/**
 * @brief
 * Destructor.
 */
OutputDispOpencv::~OutputDispOpencv() { delete wnd_; }

/**
 * @brief
 * Initialize routine of the OutputDispOpencv plugin.
 * @param common [in] commom parameters.
 * @return If true, successful initialization.
 */
bool OutputDispOpencv::InitProcess(CommonParam* common) {
  DEBUG_PRINT("OutputDispOpencv::InitProcess \n");
  common_ = common;
  wnd_->SetWindowName(plugin_name());
  DEBUG_PRINT("PostCaptureInit!!!!!!!!!!\n");
  wnd_->PostCaptureInit();
  return true;
}

/**
 * @brief
 * Finalize routine of the OutputDispOpencv plugin.
 * This function is empty implementation.
 */
void OutputDispOpencv::EndProcess() {
  DEBUG_PRINT("OutputDispOpencv::EndProcess \n");
  DEBUG_PRINT("PostCaptureEnd!!!!!!!!!!\n");
  current_image_size = cvSize(0, 0);
  wnd_->PostCaptureEnd();
}

/**
 * @brief
 * Post-processing routine of the OutputDispOpencv plugin.
 * This function is empty implementation.
 */
void OutputDispOpencv::DoPostProcess(void) {
  //  DEBUG_PRINT("OutputDispOpencv::DoPostProcess) \n");
}

/**
 * @brief
 * Main routine of the OutputDispOpencv plugin.
 * @param src_ipl [in] src image data.
 * @param dst_ipl [out] dst image data.
 * @return If true, success in the main processing.
 */
bool OutputDispOpencv::DoProcess(cv::Mat* src_image, cv::Mat* dst_image) {
  cv::Mat* temp_image;

  if (src_image == NULL) {
    DEBUG_PRINT("[OutputDispOpencv]src_image == NULL\n");
    return false;
  }

  if (wnd_ == NULL) {
    DEBUG_PRINT("[OutputDispOpencv]wnd_ == NULL\n");
    return false;
  }

  if (src_image->depth() == CV_16U) {
    temp_image = UtilGetCvConvertScale(src_image, UTIL_CONVERT_10U_TO_8U, 0);
    if (temp_image) {
      if (wnd_->Enqueue(temp_image) == false) {
        DEBUG_PRINT("Enqueue failed");
        delete temp_image;
        return false;
      } else {
        delete temp_image;
      }
    }
  } else {
    if (wnd_->Enqueue(src_image) == false) {
      DEBUG_PRINT("Enqueue failed");
      return false;
    }
  }

  current_image_size = src_image->size();

  //  DEBUG_PRINT("PostCapture!!!!!!!!!!\n");
  wnd_->PostCapture();
  return true;
}

/**
 * @brief
 * Set onepush rectangle for common param..
 * @param start [in] start coordinate.
 * @param end [in] end coordinate.
 */
void OutputDispOpencv::SetOnepushRectangle(cv::Point start, cv::Point end) {
  int start_x;
  int start_y;
  int end_x;
  int end_y;

  if (current_image_size.width <= 0 || current_image_size.height <= 0) {
    DEBUG_PRINT("OutputDispOpencv::SetOnepushRectangle No image\n");
    return;
  }

  // If the mouse is clicked(or released) on the outside
  // of the screen(top or left side), correct x, y coordinate by 0.
  if (start.x < 0) {
    start.x = 0;
  }
  if (start.y < 0) {
    start.y = 0;
  }
  if (end.x < 0) {
    end.x = 0;
  }
  if (end.y < 0) {
    end.y = 0;
  }
  // If the mouse is clicked(or released) on the outside
  // of the screen(bottom or right side),
  // correct x, y coordinate by screen width(or height).
  if (current_image_size.width < start.x) {
    start.x = current_image_size.width;
  }
  if (current_image_size.width < end.x) {
    end.x = current_image_size.width;
  }
  if (current_image_size.height < start.y) {
    start.y = current_image_size.height;
  }
  if (current_image_size.height < end.y) {
    end.y = current_image_size.height;
  }

  // If start is grater than end, swap start and end position.
  if (start.x < end.x) {
    start_x = start.x;
    end_x = end.x;
  } else {
    start_x = end.x;
    end_x = start.x;
  }
  if (start.y < end.y) {
    start_y = start.y;
    end_y = end.y;
  } else {
    start_y = end.y;
    end_y = start.y;
  }
  common_->SetOnepushRectabgle(start_x, start_y, end_x, end_y);
}

/**
 * @brief
 * Convert the bit depth of image.
 * @param src_image [in] Pointer to the src image
 * @param cvt_mode [in] enum UtilConvertMode
 * @param shift [in] Value added to the scaled source array elements
 * @return Pointer to the converted image.
 */
cv::Mat* OutputDispOpencv::UtilGetCvConvertScale(cv::Mat* src_image,
                                                 int cvt_mode, double shift) {
  cv::Mat* dst_image;
  int src_channels = src_image->channels();
  int max_width = src_image->size().width * src_channels;
  double scale;

  if (cvt_mode == UTIL_CONVERT_10U_TO_8U) {
    switch (src_channels) {
      case 1:
        dst_image = new cv::Mat(
            cvSize(src_image->size().width, src_image->size().height), CV_8UC1);
        break;
      case 3:
        dst_image = new cv::Mat(
            cvSize(src_image->size().width, src_image->size().height), CV_8UC3);
        break;
      default:
        return NULL;
    }
    scale = 1.0 / 4.0;  // change scale 10bit -> 8bit
  } else if (cvt_mode == UTIL_CONVERT_10U_TO_16U) {
    switch (src_channels) {
      case 1:
        dst_image = new cv::Mat(
            cvSize(src_image->size().width, src_image->size().height),
            CV_16UC1);
        break;
      case 3:
        dst_image = new cv::Mat(
            cvSize(src_image->size().width, src_image->size().height),
            CV_16UC3);
        break;
      default:
        return NULL;
    }
    scale = 64.0;  // change scale 10bit -> 16bit
  } else {
    DEBUG_PRINT("Invalid convert mode =%d return NULL", cvt_mode);
    return NULL;
  }

  if (cvt_mode == UTIL_CONVERT_10U_TO_8U) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < dst_image->size().height; i++) {
      for (int j = 0; j < max_width; j += src_channels) {
        ((unsigned char*)(dst_image->data + dst_image->step * i))[j] =
            (reinterpret_cast<INT16*>(src_image->data +
                                      src_image->step * i))[j] *
                scale +
            shift;
        if (src_channels == 3) {
          ((unsigned char*)(dst_image->data + dst_image->step * i))[j + 1] =
              (reinterpret_cast<INT16*>(src_image->data +
                                        src_image->step * i))[j + 1] *
                  scale +
              shift;
          ((unsigned char*)(dst_image->data + dst_image->step * i))[j + 2] =
              (reinterpret_cast<INT16*>(src_image->data +
                                        src_image->step * i))[j + 2] *
                  scale +
              shift;
        }
      }
    }
  } else if (cvt_mode == UTIL_CONVERT_10U_TO_16U) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < dst_image->size().height; i++) {
      for (int j = 0; j < max_width; j += src_channels) {
        // 16bit code
        (reinterpret_cast<INT16*>(dst_image->data + dst_image->step * i))[j] =
            (reinterpret_cast<INT16*>(src_image->data +
                                      src_image->step * i))[j] *
                scale +
            shift;
        if (src_channels == 3) {
          (reinterpret_cast<INT16*>(dst_image->data +
                                    dst_image->step * i))[j + 1] =
              (reinterpret_cast<INT16*>(src_image->data +
                                        src_image->step * i))[j + 1] *
                  scale +
              shift;
          (reinterpret_cast<INT16*>(dst_image->data +
                                    dst_image->step * i))[j + 2] =
              (reinterpret_cast<INT16*>(src_image->data +
                                        src_image->step * i))[j + 2] *
                  scale +
              shift;
        }
      }
    }
  } else {
    DEBUG_PRINT("Invalid convert mode =%d return NULL", cvt_mode);
    return NULL;
  }

  return dst_image;
}

extern "C" PluginBase* Create(void) {
  DEBUG_PRINT("Create Template plugins\n");
  OutputDispOpencv* plugin = new OutputDispOpencv();
  return plugin;
}

extern "C" void Destroy(PluginBase* plugin) { delete plugin; }
