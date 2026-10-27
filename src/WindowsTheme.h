#pragma once
#ifdef _WIN32
#include <QApplication>
#include <QAbstractNativeEventFilter>
#include <QOperatingSystemVersion>
#include <QPlatformSurfaceEvent>
#include <QPointer>
#include <QStyleHints>
#include <QTimer>
#include <QWindow>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>

// Qt's Quick Controls style does not opt Win32 system/tray menus into dark mode.
// Windows exposes these menu hooks only as optional UxTheme ordinal exports.
// Resolve them from System32, with the Windows 10 1903 signature/version guard.
// Microsoft uses the same hooks in PowerToys/ZoomIt/Utility.cpp.
class WindowsTheme final : public QObject, public QAbstractNativeEventFilter {
    enum class AppMode { Default, AllowDark, ForceDark, ForceLight, Max };
    using SetAppMode = AppMode (WINAPI *)(AppMode);
    using AllowWindow = bool (WINAPI *)(HWND, bool);
    using RefreshPolicy = void (WINAPI *)();
    HMODULE themeLibrary = nullptr;
    SetAppMode setAppMode = nullptr;
    AllowWindow allowWindow = nullptr;
    RefreshPolicy refreshPolicy = nullptr, flushMenus = nullptr;
    bool active = false;
    bool queued = false;
    bool dark = false;
    QPointer<QWindow> literalTitleWindow;
    void apply(QWindow *window) {
        if (!window || !window->handle()) return;
        const auto hwnd = reinterpret_cast<HWND>(window->winId());
        if (allowWindow) allowWindow(hwnd, dark);
        const BOOL enabled = dark;
        // DWMWA_USE_IMMERSIVE_DARK_MODE; unsupported systems ignore the request.
        DwmSetWindowAttribute(hwnd, 20, &enabled, sizeof(enabled));
    }
    void refresh() {
        HIGHCONTRASTW contrast{};
        contrast.cbSize = sizeof(contrast);
        const bool highContrast = SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0)
                                  && (contrast.dwFlags & HCF_HIGHCONTRASTON);
        if (refreshPolicy) refreshPolicy();
        dark = !highContrast && qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
        // Match Qt's detected system scheme in this process, without changing Windows settings.
        if (setAppMode) setAppMode(highContrast ? AppMode::Default : dark ? AppMode::ForceDark : AppMode::ForceLight);
        if (flushMenus) flushMenus();
        for (auto window : QGuiApplication::allWindows()) apply(window);
    }
    void scheduleRefresh() {
        if (queued) return;
        queued = true;
        QTimer::singleShot(0, this, [this] { queued = false; refresh(); });
    }
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::PlatformSurface) {
            auto surface = static_cast<QPlatformSurfaceEvent *>(event);
            if (surface->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
                QPointer<QWindow> window = qobject_cast<QWindow *>(object);
                QTimer::singleShot(0, this, [this, window] { if (window) apply(window); });
            }
        } else if (object == qApp && event->type() == QEvent::ApplicationPaletteChange) {
            scheduleRefresh();
        }
        return false;
    }
    bool nativeEventFilter(const QByteArray &, void *message, qintptr *result) override {
        const auto msg = static_cast<MSG *>(message);
        if (msg->message == WM_SETTEXT && literalTitleWindow && literalTitleWindow->handle()
            && msg->hwnd == reinterpret_cast<HWND>(literalTitleWindow->winId())) {
            const auto title = literalTitleWindow->title().toStdWString();
            *result = DefWindowProcW(msg->hwnd, WM_SETTEXT, msg->wParam, reinterpret_cast<LPARAM>(title.c_str()));
            return true;
        }
        if (msg->message == WM_SETTINGCHANGE || msg->message == WM_THEMECHANGED) scheduleRefresh();
        return false;
    }
public:
    void preserveWindowTitle(QWindow *window) {
        if (!active || !window) return;
        literalTitleWindow = window;
        window->create();
        const auto title = window->title().toStdWString();
        SetWindowTextW(reinterpret_cast<HWND>(window->winId()), title.c_str());
    }
    WindowsTheme() {
        if (QGuiApplication::platformName() != "windows") return;
        active = true;
        if (QOperatingSystemVersion::current().microVersion() >= 18362) {
            themeLibrary = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (themeLibrary) {
                setAppMode = reinterpret_cast<SetAppMode>(GetProcAddress(themeLibrary, MAKEINTRESOURCEA(135)));
                allowWindow = reinterpret_cast<AllowWindow>(GetProcAddress(themeLibrary, MAKEINTRESOURCEA(133)));
                refreshPolicy = reinterpret_cast<RefreshPolicy>(GetProcAddress(themeLibrary, MAKEINTRESOURCEA(104)));
                flushMenus = reinterpret_cast<RefreshPolicy>(GetProcAddress(themeLibrary, MAKEINTRESOURCEA(136)));
            }
        }
        qApp->installEventFilter(this);
        qApp->installNativeEventFilter(this);
        connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this] { scheduleRefresh(); });
        refresh();
    }
    ~WindowsTheme() override {
        if (!active) return;
        qApp->removeNativeEventFilter(this);
        qApp->removeEventFilter(this);
        if (themeLibrary) FreeLibrary(themeLibrary);
    }
};
#endif
