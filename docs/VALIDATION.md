# Validation status — 0.2.0

Experimental schematic-based circuit release. This is not a hardware-calibrated replica.

Verified in the cloud environment on 2026-10-19:

| Check | Outcome |
|---|---|
| Linux Release VST3 | Built; full pluginval 1.0.4 strictness 5 passed, including editor/automation |
| Linux CTest | 3/3 passed: legacy DSP, processor/state/migration, circuit numerics |
| Windows x64 VST3 | Cross-compiled with MinGW and JUCE 7.0.12 |
| Windows CTest under Wine 10 | 3/3 passed |
| Circuit solver vs ngspice 44.2 | 41/41 transient cases passed; worst waveform NRMSE approximately 2.112% |
| Circuit numerical suite | 42 model/internal-rate cases; no convergence failures in tested DC/silence/automation/audio cases |
| Windows pluginval audio-only under Wine | Passed strictness 5 with GUI tests explicitly skipped |
| Windows editor off-screen render | Passed; this does not establish VST3 editor embedding in native Windows |
| Full Windows pluginval GUI | Timed out at Editor after 60 seconds under Wine; cause unresolved |
| Native Windows CI / FL Studio | Unconfirmed; GitHub API access previously returned Forbidden |
| Physical pedal A/B and transistor/pot characterization | Not performed |

Windows audio-only validation and off-screen rendering do not turn the failed Editor check into a pass. The native Windows workflow retains full GUI validation.

The SPICE test uses the same exported component graph and documented nominal transistor/diode assumptions. It independently validates the numerical solve, not transcription accuracy or similarity to a real pedal. The metrics use the complete waveform, including onset, with no time/gain/DC fitting. See [ANALOG_MODEL.md](ANALOG_MODEL.md), the [case metrics](validation/spice-summary-0.2.0.json) and reproducible tools.

Processor tests cover old 0.1.0 state migration, invalid state handling, legacy automation range, every new circuit through the oversampler/mixer, finite output, mono/stereo, large blocks and NaN input. The boost topology is tested to be independent of Sustain. Not all possible signals, hosts or component tolerances have been validated.
