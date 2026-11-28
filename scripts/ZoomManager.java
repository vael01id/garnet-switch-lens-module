import android.animation.ValueAnimator;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.view.animation.LinearInterpolator;
import java.lang.ref.WeakReference;
import java.lang.reflect.Method;

/**
 * Klee-style per-frame exponential log-zoom smoother injected directly into
 * com.android.camera (no LSPosed/Vector required).
 *
 * Hooked from Lt5/f;.a0(FI)Z via a zero-resize trampoline using existing
 * DEX string/method IDs ("ZoomManager" / "apply").
 */
public final class ZoomManager {
    private static final String TAG = "GarnetZoomSmoother";
    private static final float TAU_MS = 115.0f;                 // responsive ease-out time constant
    private static final float ZOOM_OUT_ALPHA_SCALE = 0.703125f; // gentler log-zoom deceleration on zoom out (45/64)
    private static final float SNAP_EPSILON = 0.001f;
    private static final long MIN_VSYNC_DT_MS = 4L;
    private static final long MAX_ANIM_MS = 2000L;

    private static boolean inCallback = false;
    private static boolean isAnimating = false;
    private static float currentRatio = 0.0f;
    private static float targetRatio = 0.0f;
    private static int lastMode = 0;
    private static long lastFrameTimeMs = 0L;
    private static long animStartTimeMs = 0L;
    private static long lastRetargetTimeMs = 0L;
    private static int frameCount = 0;

    private static WeakReference<Object> activeZm = null;
    private static ValueAnimator ticker = null;

    private static Class<?> cachedClass = null;
    private static Method cachedSetZoom = null;
    private static Method cachedGetZoom = null;

    private ZoomManager() {}

    /**
     * Entrypoint called from Lt5/f;.a0(FI)Z.
     *
     * @return Boolean.TRUE if intercepted by the smoother (caller returns true),
     *         or null to fall through to the original Lt5/f;.a0 implementation.
     */
    public static Object apply(Object zm, Object ratioObj, Object modeObj) {
        if (inCallback || zm == null || ratioObj == null || modeObj == null) {
            return null;
        }
        try {
            if (Looper.myLooper() != Looper.getMainLooper()) {
                return null;
            }
            float target = ((Float) ratioObj).floatValue();
            int mode = ((Integer) modeObj).intValue();

            if (Float.isNaN(target) || Float.isInfinite(target) || target <= 0.0f) {
                stopTicker();
                return null;
            }

            // Let camera reset / mode-init / force-reapply actions (8, 17) pass straight through.
            if (mode == 8 || mode == 17) {
                stopTicker();
                currentRatio = target;
                targetRatio = target;
                return null;
            }

            if (!ensureMethods(zm)) {
                return null;
            }

            Object prevZm = (activeZm != null) ? activeZm.get() : null;
            boolean zmChanged = (prevZm != zm);
            if (zmChanged) {
                stopTicker();
            }

            long now = SystemClock.uptimeMillis();
            if (!isAnimating) {
                float cur = readCurrentZoom(zm);
                if (Float.isNaN(cur) || Float.isInfinite(cur) || cur <= 0.0f) {
                    return null;
                }
                if (Math.abs(target - cur) < SNAP_EPSILON) {
                    return null;
                }
                currentRatio = cur;
                targetRatio = target;
                lastMode = mode;
                activeZm = new WeakReference<Object>(zm);
                animStartTimeMs = now;
                lastRetargetTimeMs = now;
                lastFrameTimeMs = now;
                frameCount = 0;
                Log.i(TAG, "smooth " + cur + " -> " + target + " (mode=" + mode + ", tau=" + TAU_MS + "ms)");
                startTicker();
            } else {
                // Continuous retargeting mid-flight (rapid lens taps, slider drag, pinch)
                if (Math.abs(target - targetRatio) >= 0.0001f) {
                    targetRatio = target;
                    lastRetargetTimeMs = now;
                }
                lastMode = mode;
            }
            return Boolean.TRUE;
        } catch (Throwable t) {
            stopTicker();
            Log.e(TAG, "apply error, falling back", t);
            return null;
        }
    }

    private static boolean ensureMethods(Object zm) {
        Class<?> cls = zm.getClass();
        if (cls == cachedClass && cachedSetZoom != null && cachedGetZoom != null) {
            return true;
        }
        try {
            Method setM = cls.getMethod("a0", float.class, int.class);
            Method getM = cls.getMethod("h0");
            setM.setAccessible(true);
            getM.setAccessible(true);
            cachedClass = cls;
            cachedSetZoom = setM;
            cachedGetZoom = getM;
            return true;
        } catch (Throwable t) {
            Log.e(TAG, "method lookup failed for " + cls.getName(), t);
            return false;
        }
    }

    private static float readCurrentZoom(Object zm) {
        try {
            Object val = cachedGetZoom.invoke(zm);
            return (val instanceof Float) ? ((Float) val).floatValue() : 0.0f;
        } catch (Throwable t) {
            return 0.0f;
        }
    }

    private static void invokeRealA0(Object zm, float ratio, int mode) {
        inCallback = true;
        try {
            cachedSetZoom.invoke(zm, Float.valueOf(ratio), Integer.valueOf(mode));
        } catch (Throwable t) {
            Log.e(TAG, "invokeRealA0 error", t);
            stopTicker();
        } finally {
            inCallback = false;
        }
    }

    private static void startTicker() {
        stopTicker();
        ValueAnimator anim = ValueAnimator.ofFloat(0.0f, 1.0f);
        anim.setDuration(10000L);
        anim.setRepeatCount(ValueAnimator.INFINITE);
        anim.setInterpolator(new LinearInterpolator());
        anim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                onTick();
            }
        });
        ticker = anim;
        isAnimating = true;
        lastFrameTimeMs = SystemClock.uptimeMillis();
        anim.start();
    }

    private static void onTick() {
        if (!isAnimating || inCallback) {
            return;
        }
        Object zm = (activeZm != null) ? activeZm.get() : null;
        if (zm == null) {
            stopTicker();
            return;
        }

        long now = SystemClock.uptimeMillis();
        long rawDt = now - lastFrameTimeMs;
        // Enforce strict 1x per VSync (>= 4ms between ticks after frame 0)
        if (frameCount > 0 && rawDt < MIN_VSYNC_DT_MS) {
            return;
        }
        lastFrameTimeMs = now;

        float dt;
        if (frameCount == 0 && rawDt <= 0L) {
            dt = 16.0f;
        } else if (rawDt > 50L) {
            dt = 50.0f;
        } else {
            dt = (float) rawDt;
        }

        float target = targetRatio;
        float cur = currentRatio;

        float alpha = (float) (1.0 - Math.exp(-dt / TAU_MS));
        if (alpha < 0.01f) {
            alpha = 0.01f;
        } else if (alpha > 1.0f) {
            alpha = 1.0f;
        }

        float next;
        if (target <= cur) {
            // Zoom OUT: gentler log-zoom deceleration
            alpha *= ZOOM_OUT_ALPHA_SCALE;
            next = (float) (cur * Math.pow(target / cur, alpha));
        } else {
            // Zoom IN: responsive linear exponential ease-out
            next = cur + (target - cur) * alpha;
        }

        frameCount++;
        long totalElapsed = now - animStartTimeMs;
        long sinceRetarget = now - lastRetargetTimeMs;
        boolean done = Math.abs(target - next) < SNAP_EPSILON || sinceRetarget >= MAX_ANIM_MS;
        if (done) {
            next = target;
        }
        currentRatio = next;

        invokeRealA0(zm, next, lastMode);

        if (done) {
            Log.i(TAG, "done -> " + target + " (" + frameCount + "f, " + totalElapsed + "ms)");
            stopTicker();
        }
    }

    private static void stopTicker() {
        isAnimating = false;
        ValueAnimator anim = ticker;
        ticker = null;
        if (anim != null) {
            try {
                anim.removeAllUpdateListeners();
                anim.cancel();
            } catch (Throwable ignored) {
            }
        }
    }
}
