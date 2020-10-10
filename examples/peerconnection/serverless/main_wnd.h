/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_MAIN_WND_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_MAIN_WND_H_

#include <map>
#include <memory>
#include <string>

#include "api/media_stream_interface.h"
#include "api/video/video_frame.h"
#include "examples/peerconnection/serverless/peer_connection_client.h"
#include "media/base/media_channel.h"
#include "media/base/video_common.h"
#if defined(WEBRTC_WIN)
#include "rtc_base/win32.h"
#endif  // WEBRTC_WIN
#include "test/testsupport/frame_writer.h"
#include "test/testsupport/video_frame_writer.h"

class MainWndCallback {
 public:
  virtual void UIThreadCallback(int msg_id, void* data) = 0;
  virtual void Close() = 0;
  virtual void OnFrameCallback(const webrtc::VideoFrame& video_frame) = 0;

 protected:
  virtual ~MainWndCallback() {}
};

// Pure virtual interface for the main window.
class MainWindow {
 public:
  virtual ~MainWindow() {}

  enum UI {
    WAIT_FOR_CONNECTION,
    STREAMING,
  };

  virtual void RegisterObserver(MainWndCallback* callback) = 0;

  virtual bool IsWindow() = 0;
  virtual void MessageBox(const char* caption,
                          const char* text,
                          bool is_error) = 0;

  virtual UI current_ui() = 0;

  virtual void SwitchToConnectUI() = 0;
  virtual void SwitchToStreamingUI() = 0;

  virtual void StartLocalRenderer(webrtc::VideoTrackInterface* local_video) = 0;
  virtual void StopLocalRenderer() = 0;
  virtual void StartRemoteRenderer(
      webrtc::VideoTrackInterface* remote_video) = 0;
  virtual void StopRemoteRenderer() = 0;

  virtual void QueueUIThreadCallback(int msg_id, void* data) = 0;

  virtual void StartAutoCloseTimer(int interval_ms) = 0;
};

#ifdef WIN32

class MainWnd : public MainWindow {
 public:
  static const wchar_t kClassName[];
  // kAutoCloseTimerIDEvent is used for AutoClose Timer.
  static const int kAutoCloseTimerIDEvent = 1;

  enum WindowMessages {
    UI_THREAD_CALLBACK = WM_APP + 1,
  };

  MainWnd();
  ~MainWnd();

  bool Create();
  bool Destroy();
  bool PreTranslateMessage(MSG* msg);

  virtual void RegisterObserver(MainWndCallback* callback);
  virtual bool IsWindow();
  virtual void SwitchToConnectUI();
  virtual void SwitchToStreamingUI();
  virtual void MessageBox(const char* caption, const char* text, bool is_error);
  virtual UI current_ui() { return ui_; }

  virtual void StartLocalRenderer(webrtc::VideoTrackInterface* local_video);
  virtual void StopLocalRenderer();
  virtual void StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video);
  virtual void StopRemoteRenderer();

  virtual void QueueUIThreadCallback(int msg_id, void* data);

  virtual void StartAutoCloseTimer(int interval_ms);

  HWND handle() const { return wnd_; }

  class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
   public:
    VideoRenderer(HWND wnd,
                  int width,
                  int height,
                  bool remote,
                  MainWndCallback* callback_,
                  webrtc::VideoTrackInterface* track_to_render);
    virtual ~VideoRenderer();

    void Lock() { ::EnterCriticalSection(&buffer_lock_); }

    void Unlock() { ::LeaveCriticalSection(&buffer_lock_); }

    // VideoSinkInterface implementation
    void OnFrame(const webrtc::VideoFrame& frame) override;

    const BITMAPINFO& bmi() const { return bmi_; }
    const uint8_t* image() const { return image_.get(); }

   protected:
    void SetSize(int width, int height);

    enum {
      SET_SIZE,
      RENDER_FRAME,
    };

    HWND wnd_;
    BITMAPINFO bmi_;
    std::unique_ptr<uint8_t[]> image_;
    CRITICAL_SECTION buffer_lock_;
    rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
    std::unique_ptr<webrtc::test::VideoFrameWriter> frame_writer_;
    bool is_remote_;
    MainWndCallback* callback_;
  };

  // A little helper class to make sure we always to proper locking and
  // unlocking when working with VideoRenderer buffers.
  template <typename T>
  class AutoLock {
   public:
    explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
    ~AutoLock() { obj_->Unlock(); }

   protected:
    T* obj_;
  };

 protected:
  enum ChildWindowID {
    LABEL1_ID = 1,
  };

  void OnPaint();
  void OnDestroyed();

  bool OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT* result);

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
  static bool RegisterWindowClass();

  void CreateChildWindow(HWND* wnd,
                         ChildWindowID id,
                         const wchar_t* class_name,
                         DWORD control_style,
                         DWORD ex_style);
  void CreateChildWindows();

  void LayoutConnectUI(bool show);

 private:
  std::unique_ptr<VideoRenderer> local_renderer_;
  std::unique_ptr<VideoRenderer> remote_renderer_;
  UI ui_;
  HWND wnd_;
  DWORD ui_thread_id_;
  HWND label1_;
  bool destroyed_;
  void* nested_msg_;
  MainWndCallback* callback_;
  static ATOM wnd_class_;
};
#endif  // WIN32

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_MAIN_WND_H_
