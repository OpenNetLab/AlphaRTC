/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/serverless/main_wnd.h"

#include <math.h>

#include "api/video/i420_buffer.h"
#include "examples/peerconnection/serverless/defaults.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"

ATOM MainWnd::wnd_class_ = 0;
const wchar_t MainWnd::kClassName[] = L"WebRTC_MainWnd";

namespace {

const char kConnecting[] = "Connecting... ";
const char kNoVideoStreams[] = "(no video streams either way)";
const char kNoIncomingStream[] = "(no incoming video)";

void CalculateWindowSizeForText(HWND wnd,
                                const wchar_t* text,
                                size_t* width,
                                size_t* height) {
  HDC dc = ::GetDC(wnd);
  RECT text_rc = {0};
  ::DrawTextW(dc, text, -1, &text_rc, DT_CALCRECT | DT_SINGLELINE);
  ::ReleaseDC(wnd, dc);
  RECT client, window;
  ::GetClientRect(wnd, &client);
  ::GetWindowRect(wnd, &window);

  *width = text_rc.right - text_rc.left;
  *width += (window.right - window.left) - (client.right - client.left);
  *height = text_rc.bottom - text_rc.top;
  *height += (window.bottom - window.top) - (client.bottom - client.top);
}

HFONT GetDefaultFont() {
  static HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
  return font;
}

}  // namespace

MainWnd::MainWnd()
    : ui_(WAIT_FOR_CONNECTION),
      wnd_(NULL),
      label1_(NULL),
      destroyed_(false),
      nested_msg_(NULL),
      callback_(NULL) {}

MainWnd::~MainWnd() {
  RTC_DCHECK(!IsWindow());
}

bool MainWnd::Create() {
  RTC_DCHECK(wnd_ == NULL);
  if (!RegisterWindowClass())
    return false;

  ui_thread_id_ = ::GetCurrentThreadId();
  wnd_ =
      ::CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, kClassName, L"WebRTC",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), this);

  ::SendMessage(wnd_, WM_SETFONT, reinterpret_cast<WPARAM>(GetDefaultFont()),
                TRUE);

  CreateChildWindows();
  SwitchToConnectUI();

  return wnd_ != NULL;
}

bool MainWnd::Destroy() {
  BOOL ret = FALSE;
  if (IsWindow()) {
    ret = ::DestroyWindow(wnd_);
  }

  return ret != FALSE;
}

void MainWnd::RegisterObserver(MainWndCallback* callback) {
  callback_ = callback;
}

bool MainWnd::IsWindow() {
  return wnd_ && ::IsWindow(wnd_) != FALSE;
}

bool MainWnd::PreTranslateMessage(MSG* msg) {
  bool ret = false;
  if (msg->hwnd == NULL && msg->message == UI_THREAD_CALLBACK) {
    callback_->UIThreadCallback(static_cast<int>(msg->wParam),
                                reinterpret_cast<void*>(msg->lParam));
    ret = true;
  }
  return ret;
}

void MainWnd::SwitchToConnectUI() {
  RTC_DCHECK(IsWindow());
  ui_ = WAIT_FOR_CONNECTION;
  LayoutConnectUI(true);
}

void MainWnd::SwitchToStreamingUI() {
  LayoutConnectUI(false);
  ui_ = STREAMING;
}

void MainWnd::MessageBox(const char* caption, const char* text, bool is_error) {
  DWORD flags = MB_OK;
  if (is_error)
    flags |= MB_ICONERROR;

  ::MessageBoxA(handle(), text, caption, flags);
}

void MainWnd::StartLocalRenderer(webrtc::VideoTrackInterface* local_video) {
  local_renderer_.reset(
      new VideoRenderer(handle(), 1, 1, false, callback_, local_video));
}

void MainWnd::StopLocalRenderer() {
  local_renderer_.reset();
}

void MainWnd::StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video) {
  remote_renderer_.reset(
      new VideoRenderer(handle(), 1, 1, true, callback_, remote_video));
}

void MainWnd::StopRemoteRenderer() {
  remote_renderer_.reset();
}

void MainWnd::QueueUIThreadCallback(int msg_id, void* data) {
  ::PostThreadMessage(ui_thread_id_, UI_THREAD_CALLBACK,
                      static_cast<WPARAM>(msg_id),
                      reinterpret_cast<LPARAM>(data));
}

void MainWnd::StartAutoCloseTimer(int interval_ms) {
  ::SetTimer(wnd_, kAutoCloseTimerIDEvent, interval_ms, (TIMERPROC)NULL);
}

void MainWnd::OnPaint() {
  PAINTSTRUCT ps;
  ::BeginPaint(handle(), &ps);

  RECT rc;
  ::GetClientRect(handle(), &rc);

  VideoRenderer* local_renderer = local_renderer_.get();
  VideoRenderer* remote_renderer = remote_renderer_.get();
  if (ui_ == STREAMING && remote_renderer && local_renderer) {
    AutoLock<VideoRenderer> local_lock(local_renderer);
    AutoLock<VideoRenderer> remote_lock(remote_renderer);

    const BITMAPINFO& bmi = remote_renderer->bmi();
    int height = abs(bmi.bmiHeader.biHeight);
    int width = bmi.bmiHeader.biWidth;

    const uint8_t* image = remote_renderer->image();
    if (image != NULL) {
      HDC dc_mem = ::CreateCompatibleDC(ps.hdc);
      ::SetStretchBltMode(dc_mem, HALFTONE);

      // Set the map mode so that the ratio will be maintained for us.
      HDC all_dc[] = {ps.hdc, dc_mem};
      for (size_t i = 0; i < arraysize(all_dc); ++i) {
        SetMapMode(all_dc[i], MM_ISOTROPIC);
        SetWindowExtEx(all_dc[i], width, height, NULL);
        SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
      }

      HBITMAP bmp_mem = ::CreateCompatibleBitmap(ps.hdc, rc.right, rc.bottom);
      HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

      POINT logical_area = {rc.right, rc.bottom};
      DPtoLP(ps.hdc, &logical_area, 1);

      HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
      RECT logical_rect = {0, 0, logical_area.x, logical_area.y};
      ::FillRect(dc_mem, &logical_rect, brush);
      ::DeleteObject(brush);

      int x = (logical_area.x / 2) - (width / 2);
      int y = (logical_area.y / 2) - (height / 2);

      StretchDIBits(dc_mem, x, y, width, height, 0, 0, width, height, image,
                    &bmi, DIB_RGB_COLORS, SRCCOPY);

      if ((rc.right - rc.left) > 200 && (rc.bottom - rc.top) > 200) {
        const BITMAPINFO& bmi = local_renderer->bmi();
        image = local_renderer->image();
        int thumb_width = bmi.bmiHeader.biWidth / 4;
        int thumb_height = abs(bmi.bmiHeader.biHeight) / 4;
        StretchDIBits(dc_mem, logical_area.x - thumb_width - 10,
                      logical_area.y - thumb_height - 10, thumb_width,
                      thumb_height, 0, 0, bmi.bmiHeader.biWidth,
                      -bmi.bmiHeader.biHeight, image, &bmi, DIB_RGB_COLORS,
                      SRCCOPY);
      }

      BitBlt(ps.hdc, 0, 0, logical_area.x, logical_area.y, dc_mem, 0, 0,
             SRCCOPY);

      // Cleanup.
      ::SelectObject(dc_mem, bmp_old);
      ::DeleteObject(bmp_mem);
      ::DeleteDC(dc_mem);
    } else {
      // We're still waiting for the video stream to be initialized.
      HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
      ::FillRect(ps.hdc, &rc, brush);
      ::DeleteObject(brush);

      HGDIOBJ old_font = ::SelectObject(ps.hdc, GetDefaultFont());
      ::SetTextColor(ps.hdc, RGB(0xff, 0xff, 0xff));
      ::SetBkMode(ps.hdc, TRANSPARENT);

      std::string text(kConnecting);
      if (!local_renderer->image()) {
        text += kNoVideoStreams;
      } else {
        text += kNoIncomingStream;
      }
      ::DrawTextA(ps.hdc, text.c_str(), -1, &rc,
                  DT_SINGLELINE | DT_CENTER | DT_VCENTER);
      ::SelectObject(ps.hdc, old_font);
    }
  } else {
    HBRUSH brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
    ::FillRect(ps.hdc, &rc, brush);
    ::DeleteObject(brush);
  }

  ::EndPaint(handle(), &ps);
}

void MainWnd::OnDestroyed() {
  PostQuitMessage(0);
}

bool MainWnd::OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT* result) {
  switch (msg) {
    case WM_ERASEBKGND:
      *result = TRUE;
      return true;

    case WM_PAINT:
      OnPaint();
      return true;

    case WM_SIZE:
      if (ui_ == WAIT_FOR_CONNECTION) {
        LayoutConnectUI(true);
      }
      break;

    case WM_CTLCOLORSTATIC:
      *result = reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
      return true;

    case WM_TIMER:
      if (wp == kAutoCloseTimerIDEvent) {
        ::PostMessage(wnd_, WM_CLOSE, 0, 0);
        return true;
      }
      break;

    case WM_CLOSE:
      if (callback_)
        callback_->Close();
      break;
  }
  return false;
}

// static
LRESULT CALLBACK MainWnd::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  MainWnd* me =
      reinterpret_cast<MainWnd*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (!me && WM_CREATE == msg) {
    CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lp);
    me = reinterpret_cast<MainWnd*>(cs->lpCreateParams);
    me->wnd_ = hwnd;
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me));
  }

  LRESULT result = 0;
  if (me) {
    void* prev_nested_msg = me->nested_msg_;
    me->nested_msg_ = &msg;

    bool handled = me->OnMessage(msg, wp, lp, &result);
    if (WM_NCDESTROY == msg) {
      me->destroyed_ = true;
    } else if (!handled) {
      result = ::DefWindowProc(hwnd, msg, wp, lp);
    }

    if (me->destroyed_ && prev_nested_msg == NULL) {
      me->OnDestroyed();
      me->wnd_ = NULL;
      me->destroyed_ = false;
    }

    me->nested_msg_ = prev_nested_msg;
  } else {
    result = ::DefWindowProc(hwnd, msg, wp, lp);
  }

  return result;
}

// static
bool MainWnd::RegisterWindowClass() {
  if (wnd_class_)
    return true;

  WNDCLASSEXW wcex = {sizeof(WNDCLASSEX)};
  wcex.style = CS_DBLCLKS;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
  wcex.lpfnWndProc = &WndProc;
  wcex.lpszClassName = kClassName;
  wnd_class_ = ::RegisterClassExW(&wcex);
  RTC_DCHECK(wnd_class_ != 0);
  return wnd_class_ != 0;
}

void MainWnd::CreateChildWindow(HWND* wnd,
                                MainWnd::ChildWindowID id,
                                const wchar_t* class_name,
                                DWORD control_style,
                                DWORD ex_style) {
  if (::IsWindow(*wnd))
    return;

  // Child windows are invisible at first, and shown after being resized.
  DWORD style = WS_CHILD | control_style;
  *wnd = ::CreateWindowExW(ex_style, class_name, L"", style, 100, 100, 100, 100,
                           wnd_, reinterpret_cast<HMENU>(id),
                           GetModuleHandle(NULL), NULL);
  RTC_DCHECK(::IsWindow(*wnd) != FALSE);
  ::SendMessage(*wnd, WM_SETFONT, reinterpret_cast<WPARAM>(GetDefaultFont()),
                TRUE);
}

void MainWnd::CreateChildWindows() {
  // Create the child windows in tab order.
  CreateChildWindow(&label1_, LABEL1_ID, L"Static", ES_CENTER | ES_READONLY, 0);
}

void MainWnd::LayoutConnectUI(bool show) {
  struct Windows {
    HWND wnd;
    const wchar_t* text;
    size_t width;
    size_t height;
  } windows[] = {
      {label1_, L"Waiting for peer to connect..."},
  };

  if (show) {
    const size_t kSeparator = 5;
    size_t total_width = (ARRAYSIZE(windows) - 1) * kSeparator;

    for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
      CalculateWindowSizeForText(windows[i].wnd, windows[i].text,
                                 &windows[i].width, &windows[i].height);
      total_width += windows[i].width;
    }

    RECT rc;
    ::GetClientRect(wnd_, &rc);
    size_t x = (rc.right / 2) - (total_width / 2);
    size_t y = rc.bottom / 2;
    for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
      size_t top = y - (windows[i].height / 2);
      ::MoveWindow(windows[i].wnd, static_cast<int>(x), static_cast<int>(top),
                   static_cast<int>(windows[i].width),
                   static_cast<int>(windows[i].height), TRUE);
      x += kSeparator + windows[i].width;
      if (windows[i].text[0] != 'X')
        ::SetWindowTextW(windows[i].wnd, windows[i].text);
      ::ShowWindow(windows[i].wnd, SW_SHOWNA);
    }
  } else {
    for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
      ::ShowWindow(windows[i].wnd, SW_HIDE);
    }
  }
}

//
// MainWnd::VideoRenderer
//

MainWnd::VideoRenderer::VideoRenderer(
    HWND wnd,
    int width,
    int height,
    bool remote,
    MainWndCallback* callback,
    webrtc::VideoTrackInterface* track_to_render)
    : wnd_(wnd), rendered_track_(track_to_render), callback_(callback) {
  ::InitializeCriticalSection(&buffer_lock_);
  ZeroMemory(&bmi_, sizeof(bmi_));
  bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi_.bmiHeader.biPlanes = 1;
  bmi_.bmiHeader.biBitCount = 32;
  bmi_.bmiHeader.biCompression = BI_RGB;
  bmi_.bmiHeader.biWidth = width;
  bmi_.bmiHeader.biHeight = -height;
  bmi_.bmiHeader.biSizeImage =
      width * height * (bmi_.bmiHeader.biBitCount >> 3);
  rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
  is_remote_ = remote;
}

MainWnd::VideoRenderer::~VideoRenderer() {
  rendered_track_->RemoveSink(this);
  ::DeleteCriticalSection(&buffer_lock_);
}

void MainWnd::VideoRenderer::SetSize(int width, int height) {
  AutoLock<VideoRenderer> lock(this);

  if (width == bmi_.bmiHeader.biWidth && height == bmi_.bmiHeader.biHeight) {
    return;
  }

  bmi_.bmiHeader.biWidth = width;
  bmi_.bmiHeader.biHeight = -height;
  bmi_.bmiHeader.biSizeImage =
      width * height * (bmi_.bmiHeader.biBitCount >> 3);
  image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
}

void MainWnd::VideoRenderer::OnFrame(const webrtc::VideoFrame& video_frame) {
  AutoLock<VideoRenderer> lock(this);

  if (is_remote_ == true) {
    callback_->OnFrameCallback(video_frame);
  }

  rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
      video_frame.video_frame_buffer()->ToI420());
  if (video_frame.rotation() != webrtc::kVideoRotation_0) {
    buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
  }
  SetSize(buffer->width(), buffer->height());
  RTC_DCHECK(image_.get() != NULL);
  libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
                     buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
                     image_.get(),
                     bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biBitCount / 8,
                     buffer->width(), buffer->height());
  InvalidateRect(wnd_, NULL, TRUE);
}
