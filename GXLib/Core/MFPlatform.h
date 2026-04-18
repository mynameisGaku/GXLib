#pragma once
/// @file Core/MFPlatform.h
/// @brief Process-wide Media Foundation platform refcount wrapper.
///
/// Windows Media Foundation's `MFStartup`/`MFShutdown` are refcounted at the
/// process level: each `MFStartup` increments an internal counter; only the
/// matching final `MFShutdown` actually tears the platform down. Per-instance
/// `MFStartup`/`MFShutdown` calls therefore cause premature teardown when two
/// subsystems (MoviePlayer + VideoRecorder) coexist and one is destroyed
/// before the other.
///
/// `MFPlatform` centralises the refcount behind `Acquire`/`Release`. The
/// first `Acquire` calls `MFStartup`; the matching last `Release` calls
/// `MFShutdown`. Intermediate acquire/release pairs are O(1) refcount nudges.
///
/// Added 2026-04-18 to close the architecture-review E4 finding against
/// ADR-0020 §6 `mf_global_init` forbidden pattern.
///
/// ## Usage (MoviePlayer / VideoRecorder init path)
/// @code
/// if (!gx::MFPlatform::Acquire()) return false; // platform now initialised
/// // ... create IMFSourceReader / IMFSinkWriter / etc.
/// @endcode
///
/// ## Usage (MoviePlayer / VideoRecorder teardown path)
/// @code
/// // ... release MF COM objects first
/// gx::MFPlatform::Release(); // platform may shut down here if refcount hits 0
/// @endcode
///
/// Thread-safe: callers may Acquire/Release from any thread. Internal mutex
/// serialises the 0↔1 transitions.

#include "pch_common.h"

namespace gx
{
/// @addtogroup grp_core
/// @{

/// @brief Process-wide Media Foundation refcount wrapper.
class MFPlatform
{
public:
    /// @brief Increment platform refcount; initialise MF on the 0→1 transition.
    /// @return true if the platform is initialised after this call.
    ///         Returns false only if the first `MFStartup` failed (in which
    ///         case the refcount is unchanged).
    static bool Acquire();

    /// @brief Decrement platform refcount; shut down MF on the 1→0 transition.
    /// @note  Unbalanced Release (refcount already 0) logs a GX_LOG_ERROR and
    ///        is a no-op.
    static void Release();

    /// @brief True iff the platform is currently initialised (refcount > 0).
    static bool IsInitialized();

    /// @brief Current refcount (diagnostic use only).
    static int GetRefCount();
};

/// @}
} // namespace gx
