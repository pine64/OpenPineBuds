/*
 * Suggested ANC gain configurations for PineBuds Pro (BES2300-YP)
 *
 * These are starting-point total_gain values for ANC calibration.
 * The total_gain field controls overall filter amplification in the
 * ANC path. Higher values = more aggressive cancellation, but also
 * more risk of instability/feedback oscillation.
 *
 * Start conservative and increase in steps of ~50-100 while testing
 * for stability. If you hear ringing or whistling, back off.
 *
 * Values are for the aud_item.total_gain field (int32).
 */

#ifndef __SUGGESTED_ANC_GAINS_H__
#define __SUGGESTED_ANC_GAINS_H__

/*
 * Feedforward (FF) path — external mic → speaker
 * This is the primary ANC path. Higher gain = more cancellation
 * of external noise. Start at 300, max practical ~700.
 */
#define ANC_FF_GAIN_CONSERVATIVE    300
#define ANC_FF_GAIN_MODERATE        500
#define ANC_FF_GAIN_AGGRESSIVE      700

/*
 * Feedback (FB) path — error mic → speaker
 * Compensates for residual noise after FF cancellation.
 * Keep lower than FF to avoid instability. Start at 200, max ~500.
 */
#define ANC_FB_GAIN_CONSERVATIVE    200
#define ANC_FB_GAIN_MODERATE        350
#define ANC_FB_GAIN_AGGRESSIVE      500

/*
 * Suggested calibration procedure:
 *
 * 1. Load the extracted EF606 IIR coefficients as a baseline
 *    (see ef606_average_coefficients.h)
 *
 * 2. Set FF total_gain = ANC_FF_GAIN_CONSERVATIVE (300)
 *    Set FB total_gain = ANC_FB_GAIN_CONSERVATIVE (200)
 *    Set iir_bypass_flag = 0 (filters active)
 *
 * 3. Boot, test with pink noise. Listen for:
 *    - Low frequency attenuation (good)
 *    - Ringing or whistling (bad — reduce gain)
 *    - No audible difference (increase gain by 100)
 *
 * 4. Increase FF gain in steps of 100 up to 700.
 *    Increase FB gain in steps of 50 up to 500.
 *    Always test stability at each step.
 *
 * 5. The sweet spot for PineBuds Pro with stock ear tips
 *    is typically FF=400-600, FB=250-400 depending on
 *    ear canal seal quality.
 *
 * Note: Open-ear designs (like 1MORE Fit SE) cannot achieve
 * meaningful ANC because the acoustic seal is absent.
 * These values are for in-ear buds with silicone tips only.
 */

#endif /* __SUGGESTED_ANC_GAINS_H__ */
